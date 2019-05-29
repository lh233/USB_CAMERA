#include <linux/module.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/random.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/highmem.h>
#include <linux/freezer.h>
#include <media/videobuf-vmalloc.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>

static struct video_device *myvivi_device;


/*
------------------------------------------------------------
struct video_device
	// device ops 
	const struct v4l2_file_operations *fops;
	// callbacks 
	void (*release)(struct video_device *vdev);
------------------------------------------------------------






*/
static const struct v4l2_file_operations myvivi_fops = 
{
	.owner = THIS_MODULE,
};
static void myvivi_release(struct video_device *vdev)
{
	
}

static int myvivi_init(void)
{
	int error=0;
	
	/* 1. 分配一个video_device函数，分配空间 */
	myvivi_device = video_device_alloc();
	
	/* 2.设置 */
	myvivi_device->release = myvivi_release;
	myvivi_device->fops = &myvivi_fops;
	
	/*
	#define VFL_TYPE_GRABBER	0		//表明是一个图像采集设备-包括摄像头、调谐器
	#define VFL_TYPE_VBI		1		//从视频消隐的时间段取得信息的设备
	#define VFL_TYPE_RADIO		2		//代表无线电设备
	#define VFL_TYPE_VTX		3		//代表视传设备
	#define VFL_TYPE_MAX		4
	
	@nr:   which device number (0 == /dev/video0, 1 == /dev/video1, ...
 *             -1 == first free)
	*/
	/* 3.注册，第二个参数为VFL_TYPE_GRABBER ，nr为 -1 */
	error = video_register_device(myvivi_device, VFL_TYPE_GRABBER, -1);

	return error;
};


/*
 *	出口函数
 * 
 */
static void myvivi_exit()
{
	video_unregister_device(myvivi_device);
	video_device_release(myvivi_device);
}


module_init(myvivi_init);
module_exit(myvivi_exit);
MODULE_LICENSE("GPL");
