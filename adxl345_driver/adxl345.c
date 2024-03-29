#include "adxl345_tp.h"

static char x = 0; // Counter for the connected devices

struct mutex m_int;
DEFINE_MUTEX(m_int);

spinlock_t spin_ioctl;
DEFINE_SPINLOCK(spin_ioctl);

static const struct file_operations adxl345_fops = {
    .owner = THIS_MODULE,
    .read = adxl345_read,
#ifndef FINAL_VERSION
#ifndef IOCTL_V2
    .unlocked_ioctl = adxl345_write_ioctl
#else
    .unlocked_ioctl = adxl345_write_read_ioctl
#endif
#endif
};

irqreturn_t adxl345_int(int irq, void *dev_id)
{
    // Declare variables
    struct fifo_element fifo;
    int samples_cnt;
    int i;
    struct adxl345_device *adxldevice;
    struct i2c_client *client;
    char buffer[6];
    int bytes_read;
    char reg_address;
    int err;

    // Get device and client information
    adxldevice = (struct adxl345_device *)dev_id;
    client = (struct i2c_client *)to_i2c_client(adxldevice->misc_dev.parent);

    // Read the number of samples in the FIFO
    samples_cnt = read_reg(client, FIFO_STATUS) & 0x3F;

    // Iterate over the elements in the FIFO
    bytes_read = 0;
    reg_address = DATAX0;

    err = mutex_lock_killable(&m_int);

    if (err)
    {
        pr_err("[ERROR] failed to take the mutex");
        return err;
    }

    for (i = 0; i < samples_cnt; i++)
    {
        // Send command to read data from the device
        i2c_master_send(client, &reg_address, 1);

        // Receive data from the device
        while (bytes_read < 6)
            bytes_read += i2c_master_recv(client, buffer + bytes_read, 6 - bytes_read);

        // Copy data to FIFO element and put it into the FIFO
        memcpy(fifo.data, buffer, 6);
        kfifo_put(&adxldevice->samples_fifo, fifo);
    }
    mutex_unlock(&m_int);

    // Put FIFO element into the FIFO
    if (!kfifo_put(&adxldevice->samples_fifo, fifo))
        return IRQ_HANDLED;

    // Wake up any processes waiting on the queue
    wake_up_interruptible(&adxldevice->waiting_queue);

    return IRQ_HANDLED;
}

static int read_reg(struct i2c_client *client, char reg_id)
{
    // Declare variables
    char reg_value;
    int ret;

    // Send register address to the device
    ret = i2c_master_send(client, &reg_id, 1);
    if (ret < 0)
    {
        pr_err("[ERROR] Failed to send register address\n");
        return ret;
    }

    // Receive register value from the device
    ret = i2c_master_recv(client, &reg_value, 1);
    if (ret < 0)
    {
        pr_err("[ERROR] Failed to receive register value\n");
        return ret;
    }

    // Return the register value
    return reg_value;
}

static int write_reg(struct i2c_client *client, char reg_id, char reg_value)
{
    // Declare variables
    int ret;
    char buf[2];

    // Prepare buffer with register address and value
    buf[0] = reg_id;
    buf[1] = reg_value;

    // Send buffer to the device
    ret = i2c_master_send(client, buf, sizeof(buf));

    // Check for errors during sending
    if (ret < 0)
    {
        pr_err("[ERROR] Failed to write register\n");
        return -EBUSY;
    }

    // Return success
    return 0;
}

static ssize_t adxl345_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset)
{
    // Declare variables
    struct adxl345_device *dev;
    struct i2c_client *client;
    struct fifo_element fifo;
    char fifo_len;
    int i, bytes_read;
    int axis;

// Debug: Print that the reading function was called
#ifdef DEBUG
    printk("READING FUNCTION WAS CALLED\n");
#endif

    // Get device and client information
    dev = (struct adxl345_device *)file->private_data;
    client = to_i2c_client(dev->misc_dev.parent);

    // Wait until there is data available in the FIFO
    wait_event_interruptible(dev->waiting_queue, !kfifo_is_empty(&dev->samples_fifo));

    // Determine the length of the FIFO
    fifo_len = kfifo_len(&dev->samples_fifo);

    // Limit size to the length of the FIFO
    if (size > fifo_len)
        size = fifo_len;

    // Read data from FIFO and copy to user buffer
#ifndef FINAL_VERSION
    bytes_read = 0;
    axis = dev->addr[0] - DATAX0; // Axis offset
    for (i = 0; i < size; i++)
    {
        if (!kfifo_get(&dev->samples_fifo, &fifo))
        {
            pr_err("[ERROR] failed to get data from FIFO");
            return -1;
        }

        // Copy data from FIFO to user buffer
        if (copy_to_user(user_buffer, fifo.data + axis, 2 * sizeof(char)))
        {
            pr_err("[ERROR] failed to copy data to user buffer");
            return -1;
        }

        // Update bytes read
        bytes_read += sizeof(fifo.data);
    }
#else
    if (!kfifo_get(&dev->samples_fifo, &fifo))
    {
        pr_err("[ERROR] failed to get data from FIFO");
        return -1;
    }
    // Copy data from FIFO to user buffer
    if (copy_to_user(user_buffer, fifo.data, sizeof(fifo.data)))
    {
        pr_err("[ERROR] failed to copy data to user buffer");
        return -1;
    }
    bytes_read = sizeof(fifo.data);
#endif

    // Return the total size of data read
    return bytes_read;
}

#ifndef FINAL_VERSION
#ifndef IOCTL_V2
static long adxl345_write_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct adxl345_device *dev;
    dev = (struct adxl345_device *)file->private_data;

#ifdef DEBUG
    printk("IOCTL %ld", arg);
#endif

    if (cmd == WR_VALUE)
    {
        switch (arg)
        {
        case X_IOCTL:
            if (!spin_trylock(&spin_ioctl))
            {
                pr_err("[ERROR] failed to take the spin");
                return -1;
            }
            dev->addr[0] = DATAX0;
            dev->addr[1] = DATAX1;
            spin_unlock(&spin_ioctl);
            break;

        case Y_IOCTL:
            if (!spin_trylock(&spin_ioctl))
            {
                pr_err("[ERROR] failed to take the spin");
                return -1;
            }
            dev->addr[0] = DATAY0;
            dev->addr[1] = DATAY1;
            spin_unlock(&spin_ioctl);
            break;

        case Z_IOCTL:
            if (!spin_trylock(&spin_ioctl))
            {
                pr_err("[ERROR] failed to take the spin");
                return -1;
            }
            dev->addr[0] = DATAZ0;
            dev->addr[1] = DATAZ1;
            spin_unlock(&spin_ioctl);
            break;

        default:
            return -1;
            break;
        }
    }

    return 0;
}
#else
static long adxl345_write_read_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct adxl345_device *dev;
    struct i2c_client *client;
    struct ioctl_data io_data;
    char len;

#ifdef DEBUG
    printk("READING FUNCTION WAS CALLED\n");
#endif

    dev = (struct adxl345_device *)file->private_data;
    client = to_i2c_client(dev->misc_dev.parent);

#ifdef DEBUG
    printk("IOCTL %ld", arg);
#endif

    if (copy_from_user(&io_data, (struct ioctl_data *)arg, sizeof(struct ioctl_data)) != 0)
    {
        return -EFAULT; // Bad address
    }

    len = io_data.write_data[1]; // number of bytes to be read

    if (cmd == RWR_VALUE)
    {
        switch (io_data.write_data[0])
        {
        case X_IOCTL:
            io_data.read_data[0] = read_reg(client, DATAX0);
            if (len > 1)
                io_data.read_data[1] = read_reg(client, DATAX1);
            break;

        case Y_IOCTL:
            io_data.read_data[0] = read_reg(client, DATAY0);
            if (len > 1)
                io_data.read_data[1] = read_reg(client, DATAY1);
            break;

        case Z_IOCTL:
            io_data.read_data[0] = read_reg(client, DATAZ0);
            if (len > 1)
                io_data.read_data[1] = read_reg(client, DATAZ1);
            break;
        default:
            return -1;
            break;
        }
    }

    if (copy_to_user((struct ioctl_data *)arg, &io_data, sizeof(struct ioctl_data)))
    {
        return -EFAULT;
    }

    return 0;
}
#endif
#endif

static int adxl345_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    char currentValuePOWER;
    char *name;
    struct adxl345_device *adxl345_dev;

#ifdef DEBUG
    printk("adxl345 has been detected...\n\n");
#endif

    read_reg(client, id->driver_data);

#ifdef DEBUG
    printk("-------------------\nBW_RATE\n");
    printk("previous value: %d\n", read_reg(client, BW_RATE));
#endif
    write_reg(client, BW_RATE, RATE_CODE_0100);

#ifdef DEBUG
    printk("current value: %d\n\n", read_reg(client, BW_RATE));
#endif

#ifdef DEBUG
    printk("-------------------\nINT_ENABLE\n");
    printk("previous value: %d\n", read_reg(client, INT_ENABLE));
#endif

    if (write_reg(client, INT_ENABLE, 0x02) != 0) // Watermark
    {
        pr_err("adxl345 : write_reg failed \n");
        return -1;
    }

#ifdef DEBUG
    printk("current value: %d\n\n", read_reg(client, INT_ENABLE));
#endif

#ifdef DEBUG
    printk("-------------------\nDATA_FORMAT\n");
    printk("previous value: %d\n", read_reg(client, DATA_FORMAT));
#endif
    write_reg(client, DATA_FORMAT, 0);
#ifdef DEBUG
    printk("current value: %d\n\n", read_reg(client, DATA_FORMAT));
#endif
#ifdef DEBUG
    printk("-------------------\nFIFO_CTL\n");
    printk("previous value: %d\n", read_reg(client, FIFO_CTL));
#endif
    if (write_reg(client, FIFO_CTL, 0b10010100) != 0)
    {
        pr_err("adxl345 : write_reg failed \n");
        return -1;
    }

#ifdef DEBUG
    printk("current value: %d\n\n", read_reg(client, FIFO_CTL));
#endif
#ifdef DEBUG
    printk("-------------------\nPOWER_CTL\n");
#endif
    currentValuePOWER = read_reg(client, POWER_CTL);
#ifdef DEBUG
    printk("previous value: %d\n", currentValuePOWER);
#endif
    write_reg(client, POWER_CTL, 0x08 | currentValuePOWER);
#ifdef DEBUG
    printk("current value: %d\n\n", read_reg(client, POWER_CTL));
#endif
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

    INIT_KFIFO(adxl345_dev->samples_fifo);
    init_waitqueue_head(&adxl345_dev->waiting_queue);

    // Registering with the misc framework
    if (misc_register(&adxl345_dev->misc_dev))
    {
        pr_err("[ERROR] misc_register failed\n");
        return -EBUSY;
    }

    if (devm_request_threaded_irq(&client->dev, client->irq, NULL, adxl345_int, IRQF_ONESHOT, name, adxl345_dev))
    {
        pr_err("[ERROR] Failed to register an interruption handler\n");
        misc_deregister(&adxl345_dev->misc_dev);
        return -1;
    }

#ifdef DEBUG
    printk("%s has been detected\n", name);
#endif
    return 0;
}

static int adxl345_remove(struct i2c_client *client)
{
    // Declare variables
    char power_value;
    struct adxl345_device *adxl345_dev;

// Debug: Print POWER_CTL section
#ifdef DEBUG
    printk("-------------------\nPOWER_CTL\n");
#endif
    // Read the value of POWER_CTL register
    power_value = read_reg(client, POWER_CTL);

// Debug: Print previous value
#ifdef DEBUG
    printk("previous value: %d\n", power_value);
#endif
    // Clear the measurement bit in POWER_CTL register
    write_reg(client, POWER_CTL, power_value & 0xF7);

// Debug: Print current value
#ifdef DEBUG
    printk("current value: %d\n\n", read_reg(client, POWER_CTL));
#endif
    // Retrieve device pointer from the client
    adxl345_dev = i2c_get_clientdata(client);

    // Unregister the misc device
    misc_deregister(&adxl345_dev->misc_dev);

// Debug: Print device removal
#ifdef DEBUG
    printk("%s has been removed\n", adxl345_dev->misc_dev.name);
#endif
    // Free memory for device name and device structure
    kfree(adxl345_dev->misc_dev.name);
    kfree(adxl345_dev);

    // Decrease the number of connected devices
    x--;

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
MODULE_AUTHOR("Alaf D. N. SANTOS");
