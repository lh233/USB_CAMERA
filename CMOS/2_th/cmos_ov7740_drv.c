#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <asm/atomic.h>
#include <asm/unaligned.h>

#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf-core.h>


#define OV7740_INIT_REGS_SIZE (sizeof(ov7740_setting_30fps_VGA_640_480)/sizeof(ov7740_setting_30fps_VGA_640_480[0]))

typedef struct cmos_ov7740_i2c_value {
	unsigned char regaddr;
	unsigned char value;
}ov7740_t;

/* init: 640x480,30fps的,YUV422输出格式 */
ov7740_t ov7740_setting_30fps_VGA_640_480[] =
{
	{0x12, 0x80},
	{0x47, 0x02},
	{0x17, 0x27},
	{0x04, 0x40},
	{0x1B, 0x81},
	{0x29, 0x17},
	{0x5F, 0x03},
	{0x3A, 0x09},
	{0x33, 0x44},
	{0x68, 0x1A},

	{0x14, 0x38},
	{0x5F, 0x04},
	{0x64, 0x00},
	{0x67, 0x90},
	{0x27, 0x80},
	{0x45, 0x41},
	{0x4B, 0x40},
	{0x36, 0x2f},
	{0x11, 0x01},
	{0x36, 0x3f},
	{0x0c, 0x12},

	{0x12, 0x00},
	{0x17, 0x25},
	{0x18, 0xa0},
	{0x1a, 0xf0},
	{0x31, 0xa0},
	{0x32, 0xf0},

	{0x85, 0x08},
	{0x86, 0x02},
	{0x87, 0x01},
	{0xd5, 0x10},
	{0x0d, 0x34},
	{0x19, 0x03},
	{0x2b, 0xf8},
	{0x2c, 0x01},

	{0x53, 0x00},
	{0x89, 0x30},
	{0x8d, 0x30},
	{0x8f, 0x85},
	{0x93, 0x30},
	{0x95, 0x85},
	{0x99, 0x30},
	{0x9b, 0x85},

	{0xac, 0x6E},
	{0xbe, 0xff},
	{0xbf, 0x00},
	{0x38, 0x14},
	{0xe9, 0x00},
	{0x3D, 0x08},
	{0x3E, 0x80},
	{0x3F, 0x40},
	{0x40, 0x7F},
	{0x41, 0x6A},
	{0x42, 0x29},
	{0x49, 0x64},
	{0x4A, 0xA1},
	{0x4E, 0x13},
	{0x4D, 0x50},
	{0x44, 0x58},
	{0x4C, 0x1A},
	{0x4E, 0x14},
	{0x38, 0x11},
	{0x84, 0x70}
};



static struct i2c_client *cmos_ov7740_client;

// CAMIF GPIO
static unsigned long * GPJCON;		//摄像头控制接口
static unsigned long * GPJDAT;		//摄像头数据接口
static unsigned long * GPJUP;		//摄像头上拉接口


// CAMIF 相关寄存器
static unsigned long *CIGCTRL;		//摄像头软件复位寄存器



static irqreturn_t cmos_ov7740_camif_irq_c(int irq, void *dev_id) 
{
	return IRQ_HANDLED;
}
static irqreturn_t cmos_ov7740_camif_irq_p(int irq, void *dev_id) 
{
	
}



/* A2 参考 uvc_v4l2_do_ioctl */
static int cmos_ov7740_vidioc_querycap(struct file *file, void  *priv,
					struct v4l2_capability *cap)
{
	return 0;
}

/* A3 列举支持哪种格式
 * 参考: uvc_fmts 数组
 */
static int cmos_ov7740_vidioc_enum_fmt_vid_cap(struct file *file, void  *priv,
					struct v4l2_fmtdesc *f)
{
	return 0;
}

/* A4 返回当前所使用的格式 */
static int cmos_ov7740_vidioc_g_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	return 0;
}

/* A5 测试驱动程序是否支持某种格式, 强制设置该格式 
 * 参考: uvc_v4l2_try_format
 *       myvivi_vidioc_try_fmt_vid_cap
 */
static int cmos_ov7740_vidioc_try_fmt_vid_cap(struct file *file, void *priv,
			struct v4l2_format *f)
{
	return 0;
}

/* A6 参考 myvivi_vidioc_s_fmt_vid_cap */
static int cmos_ov7740_vidioc_s_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	return 0;
}

static int cmos_ov7740_vidioc_reqbufs(struct file *file, void *priv,
			  struct v4l2_requestbuffers *p)
{
	return 0;
}

/* A11 启动传输 
 * 参考: uvc_video_enable(video, 1):
 *           uvc_commit_video
 *           uvc_init_video
 */
static int cmos_ov7740_vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
	return 0;
}

/* A17 停止 
 * 参考 : uvc_video_enable(video, 0)
 */
static int cmos_ov7740_vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type t)
{
	return 0;
}

static const struct v4l2_ioctl_ops cmos_ov7740_ioctl_ops = {
        // 表示它是一个摄像头设备
        .vidioc_querycap      = cmos_ov7740_vidioc_querycap,

        /* 用于列举、获得、测试、设置摄像头的数据的格式 */
        .vidioc_enum_fmt_vid_cap  = cmos_ov7740_vidioc_enum_fmt_vid_cap,
        .vidioc_g_fmt_vid_cap     = cmos_ov7740_vidioc_g_fmt_vid_cap,
        .vidioc_try_fmt_vid_cap   = cmos_ov7740_vidioc_try_fmt_vid_cap,
        .vidioc_s_fmt_vid_cap     = cmos_ov7740_vidioc_s_fmt_vid_cap,
        
        /* 缓冲区操作: 申请/查询/放入队列/取出队列 */
        .vidioc_reqbufs       = cmos_ov7740_vidioc_reqbufs,

	/* 说明: 因为我们是通过读的方式来获得摄像头数据,因此查询/放入队列/取出队列这些操作函数将不在需要 */
#if 0
        .vidioc_querybuf      = myuvc_vidioc_querybuf,
        .vidioc_qbuf          = myuvc_vidioc_qbuf,
        .vidioc_dqbuf         = myuvc_vidioc_dqbuf,
#endif

        // 启动/停止
        .vidioc_streamon      = cmos_ov7740_vidioc_streamon,
        .vidioc_streamoff     = cmos_ov7740_vidioc_streamoff,   
};




/* A1 */
static int cmos_ov7740_open(struct file *file)
{
	return 0;
}

/* A18 关闭 */
static int cmos_ov7740_close(struct file *file)
{
	return 0;
}

/* 应用程序通过读的方式 */
static ssize_t cmos_ov7740_read(struct file *filep, char __user *buf, size_t count, loff_t *pos)
{
	return 0;
}


static const struct v4l2_file_operations cmos_ov7740_fops = {
	.owner = THIS_MODULE, 
	.open  = cmos_ov7740_open,
	.release = cmos_ov7740_close,
	.unlocked_ioctl = video_ioctl2,
	.read = cmos_ov7740_read,
};	


/*
*	注意该函数是必须的，否则在insmod的时候会发生错误。
*/


/* 2.1 分配、设置一个video_device结构体 */
static struct video_device cmos_ov7740_vdev = {
	.fops = &cmos_ov7740_fops,
	.ioctl_ops = &cmos_ov7740_ioctl_ops,
	.release 	= cmos_ov7740_release,
	.name		= "cmos_ov7740",
};

static void cmos_ov7740_gpio_cfg(void)
{
	/* 设置相应的GPIO用于CAMIF，需要将控制位设置为10 */
	*GPJCON = 0x2aaaaaa;
	*GPJDAT = 0;
	
	/* 使能上拉电阻 */
	*GPJUP = 0;
}

static void cmos_ov7740_camif_reset(void)
{
	/* 复位一下CAMIF控制器，把bit31设置为1 */
	*CIGCTRL |=  (1<<31);
	mdelay(10);
	*CIGCTRL &= ~(1<<31);
	mdelay(10);
}

static void cmos_ov7740_clk_cfg(void)
{
	struct clk *camif_clk;
	struct clk *camif_upll_clk;
	
	/* 使能CAMIF的时钟源 */
	camif_clk = clk_get(NULL, "camif");
	if(!camif_clk || IS_ERR(camif_clk))
	{
		printk(KERN_INFO "failed to get CAMIF clock source\n");
	}
	clk_enable(camif_clk);
	
	
	/* 使能并设置CAMCLK = 24MHZ */
	camif_upll_clk = clk_get(NULL, "camif-upll");
	/* 设置时钟的大小 */
	clk_set_rate(camif_upll_clk, 2400000);
	mdelay(100);
	
}


/*
*		注意：
*		1、S3C2440提供的复位时序通过设置CIGCTRL寄存器为：0——》1——》0表示正常工作的电平，1表示复位电平
*		但是实验证明，该复位时序与我们的ov7740需要复位时序（1——》0——》1）不符合
*		2、因此我们需要结合ov7740的具体复位时序来设置相应的寄存器；
*/		
static void cmos_ov7740_reset(void)
{
	//让第30位camset输出正常工作电平置一
	*CIGCTRL |= (1<<30);
	mdelay(30);
	//让复位信号持续一段时间，置零
	*CIGCTRL &= ~(1<<30);
	mdelay(30);
	//让第30位camset输出正常工作电平置一
	*CIGCTRL |= (1<<30);
	mdelay(30);	
}

static void cmos_ov7740_init(void)
{
	unsigned int mid;
	int i=0;
	
	/* 读操作，在OV7740 第7章 register tables中的0x0a就可以读到 Product ID信息，MSB代表高8位 */
	/* 读操作，0x0b为低8位 ，把高8位和低8位合并在一起 */
	mid = i2c_smbus_read_byte_data(cmos_ov7740_client, 0x0a) << 8;
	mid |= i2c_smbus_read_byte_data(cmos_ov7740_client, 0x0b);
	printk("manufacture ID = 0x%4x\n", mid)

	/* 写操作 */
	for(i=0; i<OV7740_INIT_REGS_SIZE; i++)
	{
		i2c_smbus_write_byte_data(cmos_ov7740_client, ov7740_setting_30fps_VGA_640_480[i].regaddr, ov7740_setting_30fps_VGA_640_480[i].value);
		mdelay(2);
	}
	
}


static int __devinit cmos_ov7740_probe(struct i2c_client *client,
										const struct i2c_device_id *id)
{
	printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	/* 2.3 硬件相关 */
	/* 2.3.1 映射相应的寄存器 */
	GPJCON = ioremap(0x560000d0, 4);
	GPJDAT = ioremap(0x560000d4, 4);
	GPJUP = ioremap(0x560000d8, 4);
	
	CIGCTRL = ioremap(0x4F000008, 4);
	
	/* 2.3.2 设置相应的GPIO用于CAMIF */
	cmos_ov7740_gpio_cfg();
	
	/* 2.3.3 复位一下CAMIF控制器 */
	cmos_ov7740_camif_reset();
	
	/* 2.3.4 设置、使能时钟（使能HCLK、使能并设置CAMCLK = 24MHZ） */
	cmos_ov7740_clk_cfg();
	
	/* 2.3.5 复位一下摄像头模块 */
	cmos_ov7740_reset();
	
	/* 2.3.6 通过IIC总线，初始化摄像头模块 */
	cmos_ov7740_client = client;
	cmos_ov7740_init();
	
	/* 2.3.7 注册中断 irq中断 对于摄像头来说有两个中断源 编码通道：IRQ_CAM_C IRQ_CAM_P */
	if(request_irq(IRQ_S3C2440_CAM_C, cmos_ov7740_camif_irq_c, IRQF_DISABLED,  "CAM_C", NULL))
	{
		printk("%s:request_irq failed\n", __func__);
	}
	
	//预览通道
	if (request_irq(IRQ_S3C2440_CAM_P, cmos_ov7740_camif_irq_p, IRQF_DISABLED , "CAM_P", NULL))
		printk("%s:request_irq failed\n", __func__);
	
	/* 2.注册 */
	video_register_device(&cmos_ov7740_vdev, VFL_TYPE_GRABBER, -1);
	return 0;
}

static int __devexit cmos_ov7740_remove(struct i2c_client *client)
{
	printk("%s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
	video_unregister_device(&cmos_ov7740_vdev);
	return 0;
}


//#define __devexit        __section(.devexit.text) __exitused __cold notrace
static const struct i2c_device_id cmos_ov7740_id_table[] = {
	{ "cmos_ov7740", 0 },
	{  }
};


/* 1.1 分配设置一个i2c_driver */
static struct i2c_driver cmos_ov7740_driver = {
	.driver = {
		.name = "cmos_ov7740",
		.owner = THIS_MODULE,
	},
	.probe = cmos_ov7740_probe,
	.remove = __devexit_p(cmos_ov7740_remove),
	.id_table = cmos_ov7740_id_table,
};


static int cmos_ov7740_drv_init(void)
{
	/* 1.2.注册 */
	i2c_add_driver(&cmos_ov7740_driver);

	return 0;
}

static void cmos_ov7740_drv_exit(void)
{
	i2c_del_driver(&cmos_ov7740_driver);
}

module_init(cmos_ov7740_drv_init);
module_exit(cmos_ov7740_drv_exit);

MODULE_LICENSE("GPL");