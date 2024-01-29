#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/i2c.h>

#include <linux/ioctl.h>
#include "adxl345_tp.h"

//--------------------------------------------------------------------------------------------------------
char read_reg(struct i2c_client *client, char reg_id);
void write_reg(struct i2c_client *client, char reg_id, char reg_value);
static int adxl345_probe(struct i2c_client *client, const struct i2c_device_id *id);

char addr[2];

// Lab 3 - First Step
//--------------------------------------------------------------------------------------------------------
#include <linux/miscdevice.h>

struct adxl345_device
{
    struct miscdevice misc_dev;
};

char x = 0; // Counter for the connected devices
//--------------------------------------------------------------------------------------------------------

// Lab 3 - Second Step
//--------------------------------------------------------------------------------------------------------
static ssize_t adxl345_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset)
{

    int result;
    struct adxl345_device *dev;
    struct i2c_client *client;
    char *buf;
    char len;

    printk("READING FUNCTION WAS CALLED\n");

    dev = (struct adxl345_device *)file->private_data;
    client = to_i2c_client(dev->misc_dev.parent);
    buf = kmalloc(sizeof(char) * 2, GFP_KERNEL);
    len = size / sizeof(char);

    if (len > 0)
    {
        printk("addr %x %x", addr[0], addr[1]);

        buf[0] = read_reg(client, addr[0]);

        if (len > 1)
            buf[1] = read_reg(client, addr[1]);
    }

    result = buf[0] | buf[1] << 8;

    if (copy_to_user(user_buffer, buf, sizeof(buf)))
        return -EFAULT;

    return size;
}

static long adxl345_write_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    printk("IOCTL %ld", arg);
    if (cmd == WR_VALUE)
    {
        switch (arg)
        {
        case X_IOCTL:
            addr[0] = DATAX0;
            addr[1] = DATAX1;
            break;

        case Y_IOCTL:
            addr[0] = DATAY0;
            addr[1] = DATAY1;
            break;

        case Z_IOCTL:
            addr[0] = DATAZ0;
            addr[1] = DATAZ1;
            break;

        default:
            return -1;
            break;
        }
    }

    return 0;
}

static const struct file_operations adxl345_fops = {
    .owner = THIS_MODULE,
    .read = adxl345_read,
    .unlocked_ioctl = adxl345_write_ioctl};

//--------------------------------------------------------------------------------------------------------

// Lab 2 - Third Step
//--------------------------------------------------------------------------------------------------------
char read_reg(struct i2c_client *client, char reg_id)
{
    char reg_value;
    int ret;

    ret = i2c_master_send(client, &reg_id, 1);
    if (ret < 0)
    {
        pr_err("[ERROR] Failed to send register address\n");
        return ret;
    }

    ret = i2c_master_recv(client, &reg_value, 1);
    if (ret < 0)
    {
        pr_err("[ERROR] Failed to receive register value\n");
        return ret;
    }

    return reg_value;
}

void write_reg(struct i2c_client *client, char reg_id, char reg_value)
{
    int ret;
    char buf[2];

    buf[0] = reg_id;
    buf[1] = reg_value;

    ret = i2c_master_send(client, buf, sizeof(buf));

    if (ret < 0)
    {
        pr_err("[ERROR] Failed to write register\n");
    }
}
//--------------------------------------------------------------------------------------------------------

// https://docs.kernel.org/i2c/writing-clients.html
static int adxl345_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    char currentValuePOWER;
    char currentValueFIFO;
    char *name;
    struct adxl345_device *adxl345_dev;

    printk("adxl345 has been detected...\n\n");

    // Lab 2 - Second Step
    //--------------------------------------------------------------------------------------------------------
    /*
        char buf[2];
        const char id2[1] = {id->driver_data};

        i2c_master_send(client, id2, 1);
        i2c_master_recv(client, buf, 1);

        printk("accelerometer: %d | expected value: %d\n", buf[0], 0xe5);
    */

    // Lab 2 - Third Step
    //--------------------------------------------------------------------------------------------------------
    read_reg(client, id->driver_data);

    printk("-------------------\nBW_RATE\n");
    printk("previous value: %d\n", read_reg(client, BW_RATE));
    write_reg(client, BW_RATE, RATE_CODE_0100);
    printk("current value: %d\n\n", read_reg(client, BW_RATE));

    printk("-------------------\nINT_ENABLE\n");
    printk("previous value: %d\n", read_reg(client, INT_ENABLE));
    write_reg(client, INT_ENABLE, 0);
    printk("current value: %d\n\n", read_reg(client, INT_ENABLE));

    printk("-------------------\nDATA_FORMAT\n");
    printk("previous value: %d\n", read_reg(client, DATA_FORMAT));
    write_reg(client, DATA_FORMAT, 0);
    printk("current value: %d\n\n", read_reg(client, DATA_FORMAT));

    printk("-------------------\nFIFO_CTL\n");
    currentValueFIFO = read_reg(client, FIFO_CTL);
    printk("previous value: %d\n", currentValueFIFO);
    write_reg(client, FIFO_CTL, 0x3F & currentValueFIFO);
    printk("current value: %d\n\n", read_reg(client, FIFO_CTL));

    printk("-------------------\nPOWER_CTL\n");
    currentValuePOWER = read_reg(client, POWER_CTL);
    printk("previous value: %d\n", currentValuePOWER);
    write_reg(client, POWER_CTL, 0x08 | currentValuePOWER);
    printk("current value: %d\n\n", read_reg(client, POWER_CTL));
    //--------------------------------------------------------------------------------------------------------

    // Lab 3 - First Step
    //--------------------------------------------------------------------------------------------------------
    // Dynamically allocating memory for an instance of the struct adxl345_device
    adxl345_dev = kmalloc(sizeof(struct adxl345_device), GFP_KERNEL);
    if (!adxl345_dev)
        return -ENOMEM;

    // Associating this instance with the struct i2c_client
    i2c_set_clientdata(client, adxl345_dev);

    // Filling the content of the miscdevice structure contained in the instance of the adxl345_device structure
    name = kasprintf(GFP_KERNEL, "adxl345-%d", x);
    if (!name)
    {
        pr_err("[ERROR] allocation failure\n");
        return -ENOMEM;
    }
    x++;

    adxl345_dev->misc_dev.minor = MISC_DYNAMIC_MINOR;
    adxl345_dev->misc_dev.name = name;
    adxl345_dev->misc_dev.parent = &client->dev;
    adxl345_dev->misc_dev.fops = &adxl345_fops; // Register your device's file operations (second step)
    adxl345_dev->misc_dev.this_device = NULL;
    adxl345_dev->misc_dev.groups = NULL;
    adxl345_dev->misc_dev.nodename = NULL;

    // Registering with the misc framework
    if (misc_register(&adxl345_dev->misc_dev))
    {
        pr_err("[ERROR] misc_register failed\n");
        return -1;
    }

    printk("%s has been detected\n", name);
    //--------------------------------------------------------------------------------------------------------

    // Lab 3 - Second Step
    //--------------------------------------------------------------------------------------------------------

    //--------------------------------------------------------------------------------------------------------

    return 0;
}
static int adxl345_remove(struct i2c_client *client)
{
    char power_value;
    struct adxl345_device *adxl345_dev;

    // Lab 2 - Third Step
    //--------------------------------------------------------------------------------------------------------
    printk("-------------------\nPOWER_CTL\n");
    power_value = read_reg(client, POWER_CTL);
    printk("previous value: %d\n", power_value);
    write_reg(client, POWER_CTL, power_value & 0xF7);
    printk("current value: %d\n\n", read_reg(client, POWER_CTL));
    //--------------------------------------------------------------------------------------------------------

    // Lab 3 - First Step
    //--------------------------------------------------------------------------------------------------------
    adxl345_dev = i2c_get_clientdata(client); // Getting the device pointer from the client
    misc_deregister(&adxl345_dev->misc_dev);  // Unregistering the misc device
    printk("%s has been removed\n", adxl345_dev->misc_dev.name);

    kfree(adxl345_dev->misc_dev.name); // Free the memory reserved for the device name
    kfree(adxl345_dev);                // Free the memory allocated for the device structure

    x--; // Decreasing the number of connected devices
    //--------------------------------------------------------------------------------------------------------

    return 0;
}
/* The following list allows the association between a device and its driver
 driver in the case of a static initialization without using
 device tree.
 Each entry contains a string used to make the association
 association and an integer that can be used by the driver to
 driver to perform different treatments depending on the physical
 the physical device detected (case of a driver that can manage
 different device models).*/
static struct i2c_device_id adxl345_idtable[] = {
    {"adxl345", 0},
    {}};
MODULE_DEVICE_TABLE(i2c, adxl345_idtable);
#ifdef CONFIG_OF
/* If device tree support is available, the following list
 allows to make the association using the device tree.
 Each entry contains a structure of type of_device_id. The field
 compatible field is a string that is used to make the association
 with the compatible fields in the device tree. The data field is
 a void* pointer that can be used by the driver to perform different
 perform different treatments depending on the physical device detected.
 device detected.*/
static const struct of_device_id adxl345_of_match[] = {
    {.compatible = "qemu,adxl345",
     .data = NULL},
    {}};

MODULE_DEVICE_TABLE(of, adxl345_of_match);
#endif
static struct i2c_driver adxl345_driver = {
    .driver = {
        /* The name field must correspond to the name of the module
        and must not contain spaces. */
        .name = "adxl345",
        .of_match_table = of_match_ptr(adxl345_of_match),
    },
    .id_table = adxl345_idtable,
    .probe = adxl345_probe,
    .remove = adxl345_remove,
};
module_i2c_driver(adxl345_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("adxl345 driver");
MODULE_AUTHOR("Alaf");
