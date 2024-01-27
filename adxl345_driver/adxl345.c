#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/i2c.h>
#include "adxl345_tp2.h"

char read_reg(struct i2c_client *client,
              char reg_id)
{
    char reg_value;

    i2c_master_send(client, &reg_id, 1);
    i2c_master_recv(client, &reg_value, 1);

    return reg_value;
}

void write_reg(struct i2c_client *client,
               char reg_id, char reg_value)
{
    char buf[2] = {reg_id, reg_value};
    i2c_master_send(client, buf, 2);
}

// https://docs.kernel.org/i2c/writing-clients.html
static int adxl345_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
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
    char currentValueFIFO = read_reg(client, FIFO_CTL);
    printk("previous value: %d\n", currentValueFIFO);
    write_reg(client, FIFO_CTL, 0x3F & currentValueFIFO);
    printk("current value: %d\n\n", read_reg(client, FIFO_CTL));

    printk("-------------------\nPOWER_CTL\n");
    char currentValuePOWER = read_reg(client, POWER_CTL);
    printk("previous value: %d\n", currentValuePOWER);
    write_reg(client, POWER_CTL, 0x08 | currentValuePOWER);
    printk("current value: %d\n\n", read_reg(client, POWER_CTL));
    //--------------------------------------------------------------------------------------------------------

    return 0;
}
static int adxl345_remove(struct i2c_client *client)
{
    printk("adxl345 has been removed...\n\n");

    // Lab 2 - Third Step
    //--------------------------------------------------------------------------------------------------------
    printk("-------------------\nPOWER_CTL\n");
    char power_value = read_reg(client, POWER_CTL);
    printk("previous value: %d\n", power_value);
    write_reg(client, POWER_CTL, power_value & 0xF7);
    printk("current value: %d\n\n", read_reg(client, POWER_CTL));
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
