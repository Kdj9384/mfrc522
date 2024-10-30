#include <linux/spi/spi.h>
#include <linux/delay.h>

#define CommandReg 0x01
#define ComIrqReg 0x04
#define DivIrqReg 0x05
#define ErrorReg 0x06
#define FIFODataReg 0x09
#define FIFOLevelReg 0x0A
#define BitFramingReg 0x0D
#define CollReg 0x0E
#define ModeReg 0x11
#define TxControlReg 0x14
#define TxASKReg 0x15
#define CRCResultRegH 0x21
#define CRCResultRegL 0x22

#define PICC_CMD_REQA  0x26
#define PICC_CMD_SEL_CL1  0x93
#define PCD_CMD_CALCRC 0x03
#define PCD_CMD_TRANSCEIVE 0x0C
#define PCD_CMD_SOFTRESET 0x0F 

ssize_t MFRC522_Init(struct spi_device *spi); 
ssize_t MFRC522_AntennaOn(struct spi_device *spi);

int MFRC522_REQA(struct spi_device *spi, unsigned char *buf, unsigned char buflen,\
	       	unsigned char *responsebuf, unsigned char responsebuflen, unsigned char bitframing);
void  MFRC522_anti_col_loop(struct spi_device *spi, unsigned char *buf, unsigned char buflen);
int MFRC522_Transceive(struct spi_device *spi, unsigned char *buffer, unsigned char bufferlen, \
		unsigned char *responsebuf, unsigned char responsebuflen, unsigned char bitframing); 

int MFRC522_CalCRC(struct spi_device *spi, unsigned char *buffer, unsigned char bufferlen, \
		unsigned char *responsebuf);
void MFRC522_Select(struct spi_device *spi, unsigned char *buf, unsigned char buflen, \
		unsigned char  *responsebuf, unsigned char responsebuflen, unsigned char bitframing);

unsigned char MFRC522_read1byte(struct spi_device *spi, unsigned char reg);
ssize_t  MFRC522_write1byte(struct spi_device *spi, unsigned char reg, unsigned char data); 

