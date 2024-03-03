#ifndef ADXL345_TP
#define ADXL345_TP

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>

// REGISTERS
#define BW_RATE 0x2C
#define INT_ENABLE 0x2E
#define DATA_FORMAT 0x31
#define FIFO_CTL 0x38
#define POWER_CTL 0x2D
#define DATAX1 0x33
#define DATAX0 0x32
#define DATAY0 0x34
#define DATAY1 0x35
#define DATAZ0 0x36
#define DATAZ1 0x37

#define RATE_CODE_3200 0x0F
#define RATE_CODE_1600 0x0E
#define RATE_CODE_0800 0x0D
#define RATE_CODE_0400 0x0C
#define RATE_CODE_0200 0x0B
#define RATE_CODE_0100 0x0A

#define WR_VALUE _IOW(10, 0, char)   // (type,nr,size) - The default major number of all the misc drivers is 10.
#define RWR_VALUE _IOWR(10, 2, char) // (type,nr,size) - The default major number of all the misc drivers is 10.
#define X_IOCTL 0
#define Y_IOCTL 1
#define Z_IOCTL 2

#define FIFO_STATUS 0x39
#define INT_SOURCE 0x30
#define INT_ENABLE 0x2E

// #define IOCTL_V2
#define DEBUG 0

static int read_reg(struct i2c_client *client, char reg_id);
static int write_reg(struct i2c_client *client, char reg_id, char reg_value);
static ssize_t adxl345_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset);
static int adxl345_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int adxl345_remove(struct i2c_client *client);
irqreturn_t adxl345_int(int irq, void *dev_id);

#ifdef IOCTL_V2
static long adxl345_write_read_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#else
static long adxl345_write_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#endif

struct fifo_element
{
    char data[6];
};

struct adxl345_device
{
    struct miscdevice misc_dev;
    char addr[2];
    DECLARE_KFIFO(samples_fifo, struct fifo_element, 64);
    wait_queue_head_t waiting_queue;
};

#ifdef IOCTL_V2
struct ioctl_data
{
    char write_data[2];
    char read_data[2];
};
#endif

#endif
