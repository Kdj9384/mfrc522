#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sysfs.h>

#include "MFRC522.h" 

#define BUF_LEN 4 

struct foo_sdev_data {
	struct spi_device *sdev;
	struct spi_controller *scont;
	unsigned char cmd; 	
	unsigned char wdata;
	
	uint8_t atoa_buf[2];
	uint8_t frame_buf[9]; // CMD, NVM, UID0~3, BCC, CRC_A(2byte) = 9byte
	uint8_t sak_buf[3];
};



/* attributes 
 * --------------------------------------------------------------------------
 * */ 
ssize_t DEBUG_show(struct device *dev, struct device_attribute *attr, char *buf) 
{
	printk("%s: Called\n", __func__);
	struct foo_sdev_data *data; 
	struct spi_controller *scont;
	struct spi_device *spi; 
	int ret = -1; 

	data = dev->driver_data;
	if (!data) {
		printk("%s: driver_data is NULL\n", __func__); 
		return 0; 
	}
	scont = data->scont;
	spi = data->sdev; 
	if (!spi) {
		printk("%s: spi_device is NULL\n", __func__);
		return 0;
	}

	ret = spi_w8r8(spi, data->cmd);
	if (ret < 0) {
		printk("%s: spi_w8r8 FAILED\n", __func__);
		return 0;
	}
	

	return scnprintf(buf, PAGE_SIZE, "tx:%02x, rx:0x%02x\n", data->cmd, ret);
}

ssize_t DEBUG_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int ret; 
	struct foo_sdev_data *data;
	struct spi_device *spi;
	unsigned char tmp_cmd; 

	data = dev->driver_data;
	spi = data->sdev; 
	ret = kstrtou8(buf, 16, &tmp_cmd);
	if (ret < 0) {
		printk("%s: kstrtoint FAILED\n", __func__);
		return count;
	}

	if (0x80 & tmp_cmd) {
		// its for read command that update data->cmd; 
		data->cmd = tmp_cmd;
		printk("%s: input:%s stored:%02x\n", __func__, buf, tmp_cmd);
		return count;
	} 

	// its for write command and buf is address and transmit random number to MFRC522
	ret = spi_write(spi, &(data->wdata), 1); 
	if (ret < 0) {
		printk("%s: spi_write FAILED\n", __func__);
	}
	return count;
}

// struct device_attribute dev_attr_name = {...}; 
DEVICE_ATTR_RW(DEBUG);

static struct attribute *DEBUG_attrs[] = {
	&dev_attr_DEBUG.attr, 
	NULL
};

ATTRIBUTE_GROUPS(DEBUG); // DEBUG_groups 



static struct class *mfrc522_class;

/* ------------------------------------------------------------------------------- */ 
int mfrc522_probe(struct spi_device *spi) 
{
	printk("%s: =============== probe started ==============\n", __func__); 
	int ret = 0;
	struct foo_sdev_data *data; 

	/* make driver data */ 
	data = devm_kzalloc(&spi->dev, sizeof(struct foo_sdev_data), GFP_KERNEL); 
	if (!data) {
		printk("%s: devm_kzalloc FAILED\n", __func__);
		return 0;
	}

	spi->mode = SPI_MODE_0;
	data->sdev = spi; 
	data->scont = spi->controller; 
	data->cmd = 0x92;
	data->wdata = 0x28;

	// TODO: should make it runnable, it has error on making device with given class name 
	// TODO: need to modify the formatter & lock for reentrant 
	struct device *dev;
	dev = device_create(mfrc522_class, &spi->dev, 154, data, "mfrc522%d", 0);
	if (IS_ERR(dev)) {
		printk("%s: device_create error %ld\n", __func__, PTR_ERR(dev));
	}

	dev_set_drvdata(&spi->dev, data); 

	MFRC522_Init(spi);
	printk("%s:Init() complete\n", __func__);

	MFRC522_AntennaOn(spi);
	printk("%s:AntennaOn() complete\n", __func__);
 
	MFRC522_REQA(spi, data->frame_buf, 1, data->atoa_buf, 2, 0x07);
	printk("%s:REQA complete\n", __func__);
	
	// spi, buf, buflen
	printk("%s: Anti-collision loop started\n", __func__);
	MFRC522_anti_col_loop(spi, data->frame_buf, 2);
	
	/***** Calculate CRC ******/ 
	printk("%s: CalCRC started\n", __func__);
	data->frame_buf[0] = PICC_CMD_SEL_CL1;
	data->frame_buf[1] = 0x70; 	
	ret = MFRC522_CalCRC(spi, data->frame_buf, 7, &(data->frame_buf[7]));
	
	/****** SELECT ******/ 	
	MFRC522_Transceive(spi, data->frame_buf, 9, data->sak_buf, 3, 0x00);

	ret = MFRC522_read1byte(spi, 0x06);
	printk("ErrorReg=0x%02x\n", ret); 

	printk("ATOA: 0x%02x %02x\n", data->atoa_buf[0], data->atoa_buf[1]); 
	printk("SAK: 0x%02x %02x %02x\n", data->sak_buf[0], data->sak_buf[1], data->sak_buf[2]); 
	for(int i = 0; i < 9; i++) {
		printk("FRAME: 0x%02x\n", data->frame_buf[i]);
	}

	/****** make Attribute ******/ 
	ret = sysfs_create_groups(&spi->dev.kobj, DEBUG_groups);
	if (ret < 0) {
		printk("%s: sysfs_create_groups FAILED!! %d\n", __func__, ret); 
	}

	printk("spi: spi's constoller=%s, max_speed_hz=%u, chip_select=%u, bits_per_word=%u, mode=%u, bus_num=%u\n", spi->controller->dev.kobj.name, spi->max_speed_hz, spi->chip_select, spi->bits_per_word, spi->mode, spi->controller->bus_num);

	return ret; 
}

void mfrc522_remove(struct spi_device *spi) 
{
	printk("%s: \n", __func__);
	sysfs_remove_groups(&spi->dev.kobj, DEBUG_groups);
}


static ssize_t spi_mfrc522_readuid(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t status = -ENXIO; 
	return status;
}

static int  spi_mfrc522_setup(struct inode *inode, struct file *filp)
{
	ssize_t status = -ENXIO;
	return status;
}

static const struct file_operations mfrc522_fops = {
	.owner = THIS_MODULE, 
	.read = spi_mfrc522_readuid,
	.open = spi_mfrc522_setup,
};

const struct of_device_id of_spi_table[] = {
	{.compatible="nxp,mfrc522_test", }, 
};

const struct spi_device_id id_spi_table[] = {
	{.name="mfrc_test", },
};

struct spi_driver mfrc522_drv = {
	.probe=mfrc522_probe,
	.remove=mfrc522_remove,
	.id_table =id_spi_table,
	.driver= {
		.name="mfrc522_drv",
		.of_match_table=of_spi_table,
	}, 
};

/* module 
 * ------------------------------------------------------------------
 * */ 
static int __init mfrc522_drv_init(void) 
{
	int status;

	
	status = register_chrdev(154, "mfrc522_cdev", &mfrc522_fops); 
	if (status < 0) {
		return status;
	}

	mfrc522_class = class_create("mfrc522_class");
	if (IS_ERR(mfrc522_class)) {
		unregister_chrdev(154, "mfrc522_chdev");
		return PTR_ERR(mfrc522_class);
	}
	
	status = spi_register_driver(&mfrc522_drv);
	if (status < 0) {
		unregister_chrdev(154, "mfrc522_cdev");
		class_destroy(mfrc522_class);
		printk("%s: spi_register_driver() FAILED\n", __func__);
	}	

	return status;
}

static void __exit mfrc522_drv_exit(void)
{
	unregister_chrdev(154, "mfrc522_cdev"); 
	class_destroy(mfrc522_class);
	spi_unregister_driver(&mfrc522_drv);
}

module_init(mfrc522_drv_init);
module_exit(mfrc522_drv_exit);
MODULE_LICENSE("GPL");
