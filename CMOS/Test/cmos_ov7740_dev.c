#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/regmap.h>
#include <linux/slab.h>

static struct i2c_board_info cmos_ov7740_info = {
	I2C_BORAD_INFO("cmos_ov7740", 0x21);
};

static struct i2c_client *cmos_ov7740_client;

static int i2c_client *cmos_ov7740_client;

static int cmos_ov7740_dev_init()
{
	struct i2c_adapter *i2c_adap;
	
	i2c_adap = i2c_get_adaptor();
	
	cmos_ov7740_client = i2c_new_device(i2c_adap, &cmos_ov7740_info);
	
	i2c_put_adaptor(i2c_adap);
	
	return 0;
	
}


static void cmos_ov7740_dev_exit(void)
{
	i2c_unregister_device(cmos_ov7740_client);
}


module_init(cmos_ov7740_dev_init);
module_exit(cmos_ov7740_dev_exit);

MODULE_LICENSE("GPL");