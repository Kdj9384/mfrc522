

int MFRC522_read1byte(struct spi_device *spi, uint8_t reg);
int MFRC522_write1byte(struct spi_device *spi, uint8_t reg, uint8_t data);
int MFRC522_setRegMask(struct spi_device *spi, uint8_t reg, uint8_t mask);

enum RA {
	CommandReg = 0x01,
	ComIrqReg = 0x04,
	DivIrqReg = 0x05, 
	ErrorReg = 0x06, 
	FIFODataReg = 0x09,
	FIFOLevelReg = 0x0A,
	BitFramingReg = 0x0D,
	CollReg = 0x0E,
	ModeReg = 0x11, 
	TxControlReg = 0x14, 
	TxASKReg = 0x15, 
	CRCResultRegH = 0x21,
	CRCResultRegL = 0x22, 
};

enum PICC_CMD {
	PICC_CMD_REQA = 0x26,
	PICC_CMD_SEL_CL1 = 0x93,
};

enum PCD_CMD {
	PCD_CMD_CALCRC = 0x03,
	PCD_CMD_TRANSCEIVE = 0x0C, 
	PCD_CMD_SOFTRESET = 0x0F, 
};


int MFRC522_clrRegMask(struct spi_device *spi, uint8_t reg, uint8_t mask)
{
	int tmp;
	tmp = MFRC522_read1byte(spi,reg);
	if (tmp < 0) {
		printk("%s: read failed\n", __func__);
		return -1;
	}

	tmp = MFRC522_write1byte(spi, reg, ((uint8_t)tmp &  (~mask)) );
	if (tmp < 0) {
		printk("%s: write failed\n", __func__);
		return tmp;
	}

	return tmp;
}

int MFRC522_setRegMask(struct spi_device *spi, uint8_t reg, uint8_t mask) 
{
	int tmp;
	tmp = MFRC522_read1byte(spi, reg);
	if (tmp < 0) {
		printk("%s: read failed\n", __func__);
		return -1;
	}

	tmp = MFRC522_write1byte(spi, reg, ( (uint8_t)tmp | mask ) );
	if (tmp < 0) {
		printk("%s: write failed\n", __func__);
		return tmp;
	}

	return tmp;
}
int MFRC522_write1byte(struct spi_device *spi, uint8_t reg, uint8_t data) 
{
	int ret = 0; 
	uint8_t buf[2]; 
	buf[0] = (reg << 1) & 0x7e; // 0x7e for makes MSB & LSB zero
	buf[1] = data;

	ret = spi_write(spi,buf, 2);
       	if (ret < 0) {
		printk("%s: %02x, %02x FAILED\n", __func__, reg, data);
		return ret;
	}	
	return ret; 
}

int  MFRC522_read1byte(struct spi_device *spi, uint8_t reg)
{
	int ret = 0;
	uint8_t addr[2];
	uint8_t recv[2]; 
       	addr[0]	= (0x80 | ((reg << 1) & 0x7e));
	
	ret = spi_w8r8(spi, addr[0]);
	if (ret < 0) {
		printk("%s: read1byte failed, ret=%02x\n", __func__, ret);
	}
	return ret;
}


void MFRC522_Init(struct spi_device *spi) {
	int ret; 
	/* 1. Soft Reset */ 
	ret = MFRC522_write1byte(spi, CommandReg, PCD_CMD_SOFTRESET);
	mdelay(50);

	/* 2. Init rest registers */
	ret = MFRC522_write1byte(spi, TxASKReg, 0x40);
	ret = MFRC522_write1byte(spi, ModeReg, 0x3D);
}

void MFRC522_AntennaOn(struct spi_device *spi) {
	int ret; 
	/* 1. Turn on Antenna */ 
	ret = MFRC522_read1byte(spi, TxControlReg);
	if (ret < 0) {
		printk("read TxControlReg Failed\n"); 
	}
	/* 0x03 = Enable Tx1, Tx2 output signal */ 
	printk("Init: TxControlReg:0x%02x 0x03 should be set\n", ret);
	if ((ret & 0x03) != 0x03) {	
		ret = MFRC522_write1byte(spi, TxControlReg, (ret | 0x03));
	}
}

int MFRC522_Transceive(struct spi_device *spi, uint8_t *buffer, uint8_t bufferlen, uint8_t *responsebuf, uint8_t responsebuflen, uint8_t bitframing) {
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
	for(int i = 0; i < bufferlen; i++) {
		MFRC522_write1byte(spi, FIFODataReg, buffer[i]);
	}

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
	}
	return 0; 
}

/* Store CRC_A 2byte on responsebuf buffer */ 
int MFRC522_CalCRC(struct spi_device *spi, uint8_t *buffer, uint8_t bufferlen, uint8_t *responsebuf) {
	int cnt; int ret; 

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
