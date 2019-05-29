#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <asm/atomic.h>
#include <asm/unaligned.h>

#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>

static struct video_device *myuvc_vdev;


/* A2 */
static int myuvc_vidioc_querycap(struct file *file, void *priv, struct v4l2_capability *cap)
{
	return 0;
}

/* A3 列举支持的格式的摄像头 */
static int myuvc_vidioc_enum_fmt_vid_cap(struct file *file, void  *priv, struct v4l2_fmtdesc *f)
{
	
	return 0;
}

/* A4 返回当前所使用的格式 */
static int myuvc_vidioc_g_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	return 0;
}

/* A5 测试驱动程序是否支持某种格式 */
static int myuvc_vidioc_try_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	return 0;
}

/* A6 */
static int myuvc_vidioc_s_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	return 0;
}

/* A7 APP调用该ioctl让驱动程序分配若干缓存，APP将从这些缓存数据中读到这些视频数据 */
static int myuvc_vidioc_reqbufs(struct file *file, void *priv, struct v4l2_requestbuffers *p)
{
	
	return 0;
}

/* A8 查询缓存状态，比如地址信息（APP可以使用mmap函数进行映射） */
static int myuvc_vidioc_querybuf(struct file *file, void *priv, struct v4l2_buffer * p)
{
	return 0;
}


/* A10 把缓冲区放入队列，底层硬件操作函数会把数据放入这个队列的缓存中 */
static int myuvc_vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	return 0;
}

/* A11 启动传输 */
static int myuvc_vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
	return 0;
}


/* A13 APP通过poll/select命令确认有数据了，把缓存中队列中取出来 */
static int myuvc_vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	return 0;
}

/*
 * A14 之前已经通过mmap映射了缓存, APP可以直接读数据
 * A15 再次调用myuvc_vidioc_qbuf把缓存放入队列
 * A16 poll...
 */


/* A17 停止 */
static int myuvc_vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
	return 0;
}


static const struct v4l2_ioctl_ops myuvc_ioctl_ops = {
        // 表示它是一个摄像头设备
        .vidioc_querycap      = myuvc_vidioc_querycap,

        /* 用于列举、获得、测试、设置摄像头的数据的格式 */
        .vidioc_enum_fmt_vid_cap  = myuvc_vidioc_enum_fmt_vid_cap,
        .vidioc_g_fmt_vid_cap     = myuvc_vidioc_g_fmt_vid_cap,
        .vidioc_try_fmt_vid_cap   = myuvc_vidioc_try_fmt_vid_cap,
        .vidioc_s_fmt_vid_cap     = myuvc_vidioc_s_fmt_vid_cap,
        
        /* 缓冲区操作: 申请/查询/放入队列/取出队列 */
        .vidioc_reqbufs       = myuvc_vidioc_reqbufs,
        .vidioc_querybuf      = myuvc_vidioc_querybuf,
        .vidioc_qbuf          = myuvc_vidioc_qbuf,
        .vidioc_dqbuf         = myuvc_vidioc_dqbuf,
        
        // 启动/停止
        .vidioc_streamon      = myuvc_vidioc_streamon,
        .vidioc_streamoff     = myuvc_vidioc_streamoff,   
};

/* A1 */
static int myuvc_open(struct file *file)
{
	return 0;
}

/* A18 */
static int myuvc_close(struct file *file)
{
	return 0;
}

/* A9 把缓冲区映射到APP空间，以后APP就可以直接操作这块空间 */
static int myuvc_mmap(struct file *file, struct vm_area_struct *vma)
{
	return 0;
}


/* A12 APP调用POLL/select来确定缓存是否就绪(有数据) */
static int myuvc_poll(struct file *file,  struct poll_table_struct *wait)
{
	return 0;
}

static const struct v4l2_file_operations myuvc_fops = 
{
	.owner = THIS_MODULE,
	.open = myuvc_open,
	.release = myuvc_close,
	.mmap = myuvc_mmap,
	.ioctl = video_ioctl2, 		/* V4L2 ioctl handler */
	.poll = myuvc_poll,
};	


static void myuvc_release(struct video_device *vdev)
{
	
}

static int myuvc_probe(static struct usb_interface *intf, const struct usb_device_id *id)
{
	static int cnt=0;
	struct usb_device *dev = interface_to_usbdev(intf);
	
	printk("myuvc probe: cnt is %d\n", cnt++);
	
	if(cnt == 2)
	{
		/* 1、分配一个video_device 结构体 */
		myuvc_vdev = video_device_alloc();
		
		
		/* 2、设置 */
		/* 2.1 */
		myuvc_vdev->release = myuvc_release;
		
		/* 2.2 */
		myuvc_vdev->fops = &myuvc_fops;
		
		/* 2.3 */
		myuvc_vdev->ioctl_ops = &myuvc_ioctl_ops;
		
		/* 3、注册 */
		video_register_device(myuvc_vdev, VFL_TYPE_GRABBER, -1);
	}
	
	
	return 0;
}


static void myuvc_disconnect(struct usb_interface *intf)
{
	static int cnt=0;
	printk("myuvc_disconnect is: cnt = %d\n", cnt++);
	
	if(cnt == 2)
	{
		video_unregister_device(myuvc_driver);
	}
}



static struct usb_device_id myuvc_ids[] = {
	/* Generic USB Video Class */
	{ USB_INTERFACE_INFO(USB_CLASS_VIDEO, 1, 0) },  /* VideoControl Interface */
    { USB_INTERFACE_INFO(USB_CLASS_VIDEO, 2, 0) },  /* VideoStreaming Interface */
	{}
};

static struct usb_driver myuvc_driver = 
{
	.name = "myuvc",
	.probe = myuvc_probe,
	.disconnect = myuvc_disconnect,
	.id_table = myuvc_ids,
};

static int myuvc_init(void)
{
	usb_register(&myuvc_driver);
	return 0;
}

static void myuvc_exit(void)
{
	usb_deregister(&myuvc_driver);
}

module_init(myuvc_init);
module_exit(myuvc_exit);
MODULE_LICENSE("GPL");
