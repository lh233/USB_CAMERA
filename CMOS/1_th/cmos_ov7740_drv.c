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
	.fops = cmos_ov7740_fops,
	.ioctl_ops = &cmos_ov7740_ioctl_ops,
	.release 	= cmos_ov7740_release,
	.name		= "cmos_ov7740",
};

static int __devexit cmos_ov7740_probe(struct id_client *client,
										const struct i2c_device_id *id)
{
	printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	
	/* 2.注册 */
	video_register_device(&cmos_ov7740_vdev, VFL_TYPE_GRABBER, -1);
}

static int __devexit cmos_ov7740_remove()
{
	printk("%s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
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
	}
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

static void cmos_ov7740_drv_exit()
{
	i2c_del_driver(&cmos_ov7740_driver);
}

module_init(cmos_ov7740_drv_init);
module_exit(cmos_ov7740_drv_exit);

MODULE_LICENSE("GPL");