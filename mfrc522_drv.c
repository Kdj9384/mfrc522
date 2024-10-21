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

	struct list_head device_entry; 
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
static LIST_HEAD(device_list); 

/* ------------------------------------------------------------------------------- */ 
int mfrc522_probe(struct spi_device *spi) 
{
	int ret = 0; int status; 
	struct foo_sdev_data *data; 

	data = devm_kzalloc(&spi->dev, sizeof(struct foo_sdev_data), GFP_KERNEL); 
	if (!data) {
		printk("%s: devm_kzalloc FAILED\n", __func__);
		return 0;
	}

	spi->mode = SPI_MODE_0;
	data->sdev = spi; 
	data->scont = spi->controller; 

	INIT_LIST_HEAD(&(data->device_entry));

	struct device *dev;
	dev = device_create(mfrc522_class, &spi->dev, MKDEV(154,0), data, "mfrc522%d", 0);
	if (IS_ERR(dev)) {
		printk("%s: device_create error %ld\n", __func__, PTR_ERR(dev));
		return 0;
	}
	status = PTR_ERR_OR_ZERO(dev);
	if (status == 0) {
		list_add(&data->device_entry, &device_list);
	}

	if (status == 0) {
		dev_set_drvdata(&spi->dev, data); 
	} else {
		kfree(data);
	}
	
	// add attribute groups dynamically 
	ret = sysfs_create_groups(&spi->dev.kobj, DEBUG_groups);
	if (ret < 0) {
		printk("%s: sysfs_create_groups FAILED!! %d\n", __func__, ret); 
	}

	return ret; 
}

void mfrc522_remove(struct spi_device *spi) 
{
	struct foo_sdev_data *data;
	data = spi_get_drvdata(spi);

	// data->sdev = NULL; 
	list_del(&data->device_entry);
	
	device_destroy(mfrc522_class, MKDEV(154, 0));
	sysfs_remove_groups(&spi->dev.kobj, DEBUG_groups);
}


static ssize_t spi_mfrc522_readuid(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t status = -ENXIO; 
	struct spi_device *spi;
	struct foo_sdev_data *data;

	data = filp->private_data;
	spi = data->sdev;

	// return 0 on success 
	MFRC522_REQA(spi, data->frame_buf, 1, data->atoa_buf, 2, 0x07);

	// void, filling the frame_buf 	
	MFRC522_anti_col_loop(spi, data->frame_buf, 2);
	
	// return 0 on success
	MFRC522_CalCRC(spi, data->frame_buf, 7, &(data->frame_buf[7]));

	// void
	MFRC522_Select(spi, data->frame_buf, 9, data->sak_buf, 3, 0x00);

	unsigned long missing;
	missing = copy_to_user(buf, data->frame_buf, count); 
	
	if (missing == 0) {
		return 0;
	}
	if (missing == count) {
		return -EFAULT;
	}
	
	return count - missing;
}

// Return 0 on Success.  
static int  spi_mfrc522_setup(struct inode *inode, struct file *filp)
{
	ssize_t status = -ENXIO;
	struct foo_sdev_data *data;
	struct spi_device *spi;

	// ptr, type, member
	data = list_first_entry(&device_list, typeof(*data), device_entry);

	filp->private_data = data;
	spi = data->sdev; 

	status = MFRC522_Init(spi);
	printk("Init(): status=%ld\n", status);

	status = MFRC522_AntennaOn(spi);
	printk("AntennaOn(): status=%ld\n", status);

	return 0;
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

	printk("%s: driver init\n", __func__);

	return status;
}

static void __exit mfrc522_drv_exit(void)
{
	spi_unregister_driver(&mfrc522_drv);	
	class_destroy(mfrc522_class);
	unregister_chrdev(154, "mfrc522_cdev"); 

	printk("%s: driver exit\n", __func__);

}

module_init(mfrc522_drv_init);
module_exit(mfrc522_drv_exit);
MODULE_LICENSE("GPL");
