#include "MFRC522.h"

int MFRC522_clrRegMask(struct spi_device *spi, unsigned char reg, unsigned char mask)
{
	int tmp;
	tmp = MFRC522_read1byte(spi,reg);
	if (tmp < 0) {
		printk("%s: read failed\n", __func__);
		return -1;
	}

	tmp = MFRC522_write1byte(spi, reg, ((unsigned char)tmp &  (~mask)) );
	if (tmp < 0) {
		printk("%s: write failed\n", __func__);
		return tmp;
	}

	return tmp;
}

int MFRC522_setRegMask(struct spi_device *spi, unsigned char reg, unsigned char mask) 
{
	int tmp;
	tmp = MFRC522_read1byte(spi, reg);
	if (tmp < 0) {
		printk("%s: read failed\n", __func__);
		return -1;
	}

	tmp = MFRC522_write1byte(spi, reg, ( (unsigned char)tmp | mask ) );
	if (tmp < 0) {
		printk("%s: write failed\n", __func__);
		return tmp;
	}

	return tmp;
}
int MFRC522_write1byte(struct spi_device *spi, unsigned char reg, unsigned char data) 
{
	int ret = 0; 
	unsigned char buf[2]; 
	buf[0] = (reg << 1) & 0x7e; // 0x7e for makes MSB & LSB zero
	buf[1] = data;

	ret = spi_write(spi,buf, 2);
       	if (ret < 0) {
		printk("%s: %02x, %02x FAILED\n", __func__, reg, data);
		return ret;
	}	
	return ret; 
}

unsigned char MFRC522_writenbytes(struct spi_device *spi, unsigned char reg, unsigned char* buf, unsigned char buflen)
{
	unsigned char xfer_buf[buflen+1]; 
	xfer_buf[0] = (reg << 1) & 0x7e; 
	memcpy(&xfer_buf[1], buf, buflen); 

	struct spi_transfer xfer = {
		.tx_buf = xfer_buf, 
		.len = buflen+1, 	
	}; 
	return spi_sync_transfer(spi, &xfer, 1); 
}

int  MFRC522_read1byte(struct spi_device *spi, unsigned char reg)
{
	int ret = 0;
	unsigned char addr[2];
       	addr[0]	= (0x80 | ((reg << 1) & 0x7e));
	
	ret = spi_w8r8(spi, addr[0]);
	if (ret < 0) {
		printk("%s: read1byte failed, ret=%02x\n", __func__, ret);
	}
	return ret;
}

unsigned char MFRC522_readnbytes(struct spi_device *spi, unsigned char reg, unsigned char *recv_buf, unsigned char n) 
{
	unsigned char xfer_buf[n];
	unsigned char addr = (0x80 | ((reg << 1) & 0x7e));
	for(int i = 0; i < n; i++) {
		xfer_buf[i] = addr;
	}
	// Not DMA-Safe, 0 for success, copy occur so performance-sensitive or bulk buffer should use spi_sync, async calls with DMA-safe buffer. 
	return spi_write_then_read(spi, xfer_buf, n, recv_buf, n);
}

void MFRC522_Init(struct spi_device *spi) 
{
	int ret; 
	/* 1. Soft Reset */ 
	ret = MFRC522_write1byte(spi, CommandReg, PCD_CMD_SOFTRESET);
	mdelay(50);

	/* 2. Init rest registers */
	ret = MFRC522_write1byte(spi, TxASKReg, 0x40);
	ret = MFRC522_write1byte(spi, ModeReg, 0x3D);
}

void MFRC522_AntennaOn(struct spi_device *spi) 
{
	int ret; 
	/* 1. Turn on Antenna */ 
	ret = MFRC522_read1byte(spi, TxControlReg);
	if (ret < 0) {
		printk("read TxControlReg Failed\n"); 
	}
	/* 0x03 = Enable Tx1, Tx2 output signal */ 
	if ((ret & 0x03) != 0x03) {	
		ret = MFRC522_write1byte(spi, TxControlReg, (ret | 0x03));
	}

	ret = MFRC522_read1byte(spi, TxControlReg);
	printk("%s: TxControlReg:0x%02x 0x03 should be set\n", __func__, ret);
}

int MFRC522_Transceive(struct spi_device *spi, unsigned char *buffer, unsigned char bufferlen, unsigned char *responsebuf, unsigned char responsebuflen, unsigned char bitframing) 
{
	int cnt; int ret;

	/* clear the bits that received after collision */ 
	MFRC522_clrRegMask(spi, CollReg, 0x80);

	/* make PCD_CMD IDLE */ 
	MFRC522_write1byte(spi, CommandReg, 0x00);

	/* reset ComIrqReg to notice the data is receviced from PICC */ 
	MFRC522_write1byte(spi, ComIrqReg, 0x7F); 

	/* flush FIFO buffer */ 
	MFRC522_setRegMask(spi, FIFOLevelReg, 0x80);

	/* send PICC_CMD to FIFO */ 
	/*
	for(int i = 0; i < bufferlen; i++) {
		MFRC522_write1byte(spi, FIFODataReg, buffer[i]);
	}
	*/ 
	MFRC522_writenbytes(spi, FIFODataReg, buffer, bufferlen); 

	/* setting proper bitframing */ 
	MFRC522_write1byte(spi, BitFramingReg, bitframing);

	/* send PCD_Transceive command */ 
	MFRC522_write1byte(spi, CommandReg, PCD_CMD_TRANSCEIVE);

	/* should be set for PCD_Transceive command */ 	
	MFRC522_setRegMask(spi, BitFramingReg, 0x80); 

	/* wait for the response of PICC */ 
	cnt = 2000; 
	while(1) {
		ret = MFRC522_read1byte(spi,ComIrqReg); 
		if (ret & 0x30) {
			// success ; 
			break;
		}
		if (ret & 0x01) {
			printk("%s: 0x01 TIMEOUT\n", __func__);
			return -1;
		}
		if (cnt-- < 0) {
			printk("%s: ERROR TIMEOUT, ret:%02x\n", __func__, ret);
			return -1; 
		}
	}

	/* get the size of the response data (byte) */
	ret = MFRC522_read1byte(spi, FIFOLevelReg);
	printk("%s: Size of response:%02x\n", __func__, ret);

	/* read the data from FIFO */ 

	for(int i = 0; i < ret; i++) {
		if (i >= responsebuflen) {
			printk("%s: FIFOLevel is bigger than response buffer lenght\n", __func__);
			printk("%s: FIFOLevel=0x%02x, responsebufferlen=0x%02x\n", __func__, ret, responsebuflen); 
			break;
		}
		responsebuf[i] = MFRC522_read1byte(spi, FIFODataReg);
		printk("%s: response=%02x\n", __func__, responsebuf[i]);
	}
	
	/*
	MFRC522_readnbytes(spi, FIFODataReg, responsebuf, ret);
	*responsebuf << 8; 
	*/
	return 0; 

}

/* Store CRC_A 2byte on responsebuf buffer */ 
int MFRC522_CalCRC(struct spi_device *spi, unsigned char *buffer, unsigned char bufferlen, unsigned char *responsebuf) 
{
	int cnt; int ret; 

	buffer[0] = PICC_CMD_SEL_CL1;
	buffer[1] = 0x70;

	MFRC522_write1byte(spi, CommandReg, 0x00);
	MFRC522_write1byte(spi, DivIrqReg, 0x04);
	MFRC522_setRegMask(spi, FIFOLevelReg, 0x80);

	for(int i = 0; i < bufferlen; i++) {
		MFRC522_write1byte(spi, FIFODataReg, buffer[i]);
	}
	
	MFRC522_write1byte(spi, CommandReg, PCD_CMD_CALCRC); 
	cnt = 5000;
	while (1) {
		ret = MFRC522_read1byte(spi, DivIrqReg);
		/* 0X04 = CRC IRQ */ 
		if (ret & 0x04) {
			printk("CalCRC: DivIrqReg:CRCIrq is Set!!\n");
			break;
		}
		if (cnt-- < 0) {
			printk("CalCRC: TIME OUT\n");
			return -1;
		}
	}
	MFRC522_write1byte(spi, CommandReg, 0x00);
	ret = MFRC522_read1byte(spi, CRCResultRegL); // LSB
	responsebuf[0] = ret;
	printk("CRC: responesbuf 0x%02x\n", ret);
	ret = MFRC522_read1byte(spi, CRCResultRegH); // MSB 
	responsebuf[1] = ret;
	printk("CRC: responsebuf 0x%02x\n", ret);
	return 0; 
}

int MFRC522_REQA(struct spi_device *spi, unsigned char *buf, unsigned char buflen, unsigned char *responsebuf, unsigned char responsebuflen, unsigned char bitframing)
{	
	int ret; 
	buf[0] = PICC_CMD_REQA;
	ret = MFRC522_Transceive(spi, buf, buflen, responsebuf, responsebuflen, bitframing);
	if (ret < 0) {
		printk("%s: REQA Failed\n", __func__);
		return -1;
	}

	// print 
	for(int i = 0; i < responsebuflen; i++) {
		printk("ATOA: %02x\n", responsebuf[i]);
	}
	return 0; 
}

/* anti collision loop
 * frame[0] = CMD
 * frame[1] = NVB
 * frame[2:5] = UID
 * frame[6] = BCC 
 */ 
void  MFRC522_anti_col_loop(struct spi_device *spi, unsigned char *buf, unsigned char buflen) 
{
	int ret; 
	unsigned char collpos = 0;
	unsigned char knownbits = 0;
	unsigned char knownindex = 2; // the index that the collision occur in frame or the response start posiion in frame. 

	unsigned char bitframing = 0; 

	unsigned char collpos_byteindex = 0; 
	unsigned char collpos_bitindex = 0; 

	unsigned char responselen = 5; 

	unsigned char mergeval = 0; 

	buf[0] = PICC_CMD_SEL_CL1; 
	buf[1] = 0x20; // init value for NVB

	MFRC522_clrRegMask(spi, CollReg, 0x80); // clear received bit after collision
	MFRC522_setRegMask(spi, BitFramingReg, 0x00); // RxAlign & TxLastBit == 0x00

	// loop 
	while (knownbits < 32) {
		printk("%s: LOOP START: NVM=%02x\n", __func__, buf[1]);
		ret = MFRC522_Transceive(spi, buf, buflen, &(buf[knownindex]), responselen, bitframing);

		ret = MFRC522_read1byte(spi, ErrorReg);
		if (ret & 0x08) {
			printk("%s: ErrorReg, CollErr occur\n", __func__);
		}

		ret = MFRC522_read1byte(spi, CollReg);
		printk("%s: CollReg = %02x\n", __func__, ret);

		if ((ret & 0x20) == 0x20) {
			// no collision, all uid, 32bits are known
			printk("%s: no collision, ret=%02x\n", __func__, ret);
			knownbits = 32; 
		}

		// merge the collision part. 
		buf[knownindex] = buf[knownindex] + mergeval; 

		if (knownbits < 32) {
		collpos = (ret & 0x1F); 
		collpos_bitindex = collpos % 8; 
		collpos_byteindex = collpos / 8;
		knownbits = collpos;
		knownindex = knownindex + collpos_byteindex; 
		buflen = buflen + collpos_byteindex + 1; 

		mergeval = buf[knownindex]; 

		// update NVB 
		buf[1] = buf[1] + (collpos_byteindex << 4);
		buf[1] = buf[1] + collpos_bitindex;
	
		// rxalign, txlastbit setting 
		bitframing = (collpos_bitindex << 4) + collpos_bitindex; 
		printk("%s: CollPos=%02x, knownbits=%02x, knownindex=%02x, NVM=%02x, BitFramingReg=%02x\n", __func__, collpos, knownbits, knownindex, buf[1], bitframing);
		
		printk("%s: LOOP END\n", __func__);
		}


	}
}

