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

static struct timer_list myvivi_timer;
static struct list_head myvivi_vb_local_queue;				//链表的头结点
static struct video_device * myvivi_device;


static void myvivi_timer_function(unsigned long data)
{
    struct videobuf_buffer *vb;
	void *vbuf;
	struct timeval ts;
    
    /* 1. 构造数据: 从队列头部取出第1个videobuf到本地队列中, 填充数据
     */

    /* 判断是否为空节点 */
    if (list_empty(&myvivi_vb_local_queue)) {
        goto out;
    }
    
	
	//取出节点
	/* 1.1 从本地队列取出第1个videobuf */
    vb = list_entry(myvivi_vb_local_queue.next,
             struct videobuf_buffer, queue);
    
    /* Nobody is waiting on this buffer, return */
    if (!waitqueue_active(&vb->done))
        goto out;
    

    /* 1.2 填充数据 */
    vbuf = videobuf_to_vmalloc(vb);
    memset(vbuf, 0xff, vb->size);
    vb->field_count++;
    do_gettimeofday(&ts);
    vb->ts = ts;
    vb->state = VIDEOBUF_DONE;

    /* 1.3 把videobuf从本地队列中删除 */
    list_del(&vb->queue);

    /* 2. 唤醒进程: 唤醒videobuf->done上的进程 */
    wake_up(&vb->done);
    
out:
    /* 3. 修改timer的超时时间 : 30fps, 1秒里有30帧数据
     *    每1/30 秒产生一帧数据
     */
    mod_timer(&myvivi_timer, jiffies + HZ/30);
}
/* ---------------*/



/*
*	IOCTL vidioc handling
*/

//表明它是一个摄像头驱动模块
static int myvivi_videoc_querycap(struct file *file, void *priv, struct v4l2_capability *cap)
{
	strcpy(cap->driver, "myvivi");
	strcpy(cap->card, "myvivi");

	cap->version = 0x0001;
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;

	return 0;
}

//列举支持哪种格式
static int myvivi_vidioc_enum_fmt_vid_cap(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	if(f->index >= 1)
	{
		return -EINVAL;
	}

	strcpy(f->description, "4:2:2, packed, YUYV");

	f->pixelformat = V4L2_PIX_FMT_YUYV;
	return 0;
}
struct v4l2_format myvivi_format;
//返回当前所使用的格式
static int myvivi_vidioc_g_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	memcpy(f, &myvivi_format, sizeof(myvivi_format));
	return 0;
}
//测试驱动程序是否支持某种格式
static int myvivi_vidioc_try_fmt_vid_cap(struct file *file, void * priv, struct v4l2_format *f)
{
	unsigned int maxw, maxh;
	enum v4l2_field field;

	if(f->fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV)
	{
		return -EINVAL;
	}

	maxw=1024;
	maxh=768;

	/*
		调整最小宽度和最大宽度等
	*/
	v4l_bound_align_image(&f->fmt.pix.width, 48, maxw, 2, &f->fmt.pix.height, 32, maxh, 0, 0);

	f->fmt.pix.bytesperline = (f->fmt.pix.width * 16) >> 3;
	f->fmt.pix.sizeimage = f->fmt.pix.height * f->fmt.pix.bytesperline;
}
//设置驱动程序的格式
static int myvivi_vidioc_s_fmt_vid_cap(struct file *file, void *priv,
										struct v4l2_format *f)
{
	int ret = myvivi_vidioc_try_fmt_vid_cap(file, NULL, f);
	if(ret < 0)
		return ret;
	memcpy(&myvivi_format, f, sizeof(myvivi_format));

	return ret;
}



//队列操作1：定义
static struct videobuf_queue myvivi_vb_vidqueue;
//自旋锁
static spinlock_t myvivi_queue_slock;
/***************buffer operations****************/

/*****************buffer operations***********************/
/* APP调用ioctl VIDIOC_REQBUFS(videobuf_reqbufs)时会导致此函数被调用,
 * 它重新调整count和size
 */
static int myvivi_buffer_setup(struct videobuf_queue *vq, unsigned int *count, unsigned int * size)
{
	//第一个参数
	*size = myvivi_format.fmt.pix.sizeimage;

	if(0 == *count)
	{
		*count = 32;
	}

	return 0;
}
/* APP调用ioctl VIDIOC_QBUF时导致此函数被调用,
 * 它会填充video_buffer结构体并调用videobuf_iolock来分配内存
 * 
 */
static int myvivi_buffer_prepare(struct videobuf_queue *vq, struct videobuf_buffer *vb,
									enum v4l2_field field)
{
	 /* 0. 设置videobuf */
	vb->size = myvivi_format.fmt.pix.sizeimage;
    vb->bytesperline = myvivi_format.fmt.pix.bytesperline;
	vb->width  = myvivi_format.fmt.pix.width;
	vb->height = myvivi_format.fmt.pix.height;
	vb->field  = field;
	//1.做一些准备工作

#if 0
	//2.调用videobuf_iolock为类型为V4L2_MEMORY_USERPTR的videobuf分配内存
	if(VIDEOBUF_NEEDS_INIT == buf->vb.state)
	{
		rc = videobuf_iolock(vq, &buf->vb, NULL);
		if(rc < 0)
		{
			goto fail;
		}
	}
#endif
	//3.设置状态
	vb->state = VIDEOBUF_PREPARED;
	return 0;
}
/* APP调用ioctlVIDIOC_QBUF时:
 * 1. 先调用buf_prepare进行一些准备工作
 * 2. 把buf放入队列
 * 3. 调用buf_queue(起通知作用)通知应用程序正在请求调用
 */
static void myvivi_buffer_queue(struct videobuf_queue *vq, struct videobuf_buffer *vb)
{
	vb->state = VIDEOBUF_QUEUED;
	list_add_tail(&vb->queue, &myvivi_vb_local_queue);
			//在链表尾部插入节点增加videobuf的东西
}

//APP不再使用队列时，用它来释放内存
static void myvivi_buffer_release(struct videobuf_queue *vq, struct videobuf_buffer *vb)
{
	videobuf_vmalloc_free(vb);
	vb->state = VIDEOBUF_NEEDS_INIT;
}

//videobuf operations
static struct videobuf_queue_ops myvivi_video_qops = 
{
	.buf_setup = myvivi_buffer_setup, 	//计算大小以免浪费
	.buf_prepare = myvivi_buffer_prepare,
	.buf_queue = myvivi_buffer_queue,
	.buf_release = myvivi_buffer_release,
};
//videobuf operations

//APP调用ioctl VIDICOC_QBUF时
//缓冲区操作：申请、查询、放入队列，取出队列
static int myvivi_vidioc_reqbufs(struct file *file, void *priv, 
								struct v4l2_requestbuffers *p)
{
	return (videobuf_reqbufs(&myvivi_vb_vidqueue, p));
}
static int myvivi_vidioc_querybuf(struct file *file, void *priv, 
								struct v4l2_buffer *p)
{
	return (videobuf_querybuf(&myvivi_vb_vidqueue, p));
}
static int myvivi_vidioc_qbuf(struct file *file, void *priv, 
								struct v4l2_buffer *p)
{
	return (videobuf_qbuf(&myvivi_vb_vidqueue, p));
}
static int myvivi_vidioc_dqbuf(struct file *file, void *priv,
								struct v4l2_buffer *p)
{
	return (videobuf_dqbuf(&myvivi_vb_vidqueue, p, file->f_flags & O_NONBLOCK));
}
//缓冲区操作：申请、查询、放入队列、取出队列




/*-------------------开启与关闭--------------------------*/
static int myvivi_vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
		return videobuf_streamon(&myvivi_vb_vidqueue);
}

static int myvivi_vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
		videobuf_streamoff(&myvivi_vb_vidqueue);
		return 0;
}
/*-------------------开启与关闭--------------------------*/

/*
*	IOCTL vidioc handling
*/
static const struct v4l2_ioctl_ops myvivi_ioctl_ops = 
{
	// 表示它是一个摄像头设备
	.vidioc_querycap      = myvivi_videoc_querycap,



	//用于列举、获得、测试、设置摄像头的数据的格式
	.vidioc_enum_fmt_vid_cap  = myvivi_vidioc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap     = myvivi_vidioc_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap   = myvivi_vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap     = myvivi_vidioc_s_fmt_vid_cap,


	//申请、查询、放入队列
	.vidioc_reqbufs       = myvivi_vidioc_reqbufs,
	.vidioc_querybuf      = myvivi_vidioc_querybuf,
	.vidioc_qbuf          = myvivi_vidioc_qbuf,
	.vidioc_dqbuf         = myvivi_vidioc_dqbuf,


	//查询、获得、设置属性
	//	.vidioc_queryctrl     = vidioc_queryctrl,
	//	.vidioc_g_ctrl        = vidioc_g_ctrl,
	//	.vidioc_s_ctrl        = vidioc_s_ctrl,


	.vidioc_streamon      = myvivi_vidioc_streamon,
	.vidioc_streamoff     = myvivi_vidioc_streamoff,


};






// file operations for the device--------------------------------------
static int myvivi_open(struct file *file)
{
	//队列操作：初始化
	videobuf_queue_vmalloc_init(&myvivi_vb_vidqueue, &myvivi_video_qops,
			NULL, &myvivi_queue_slock, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_FIELD_INTERLACED,
			sizeof(struct videobuf_buffer), NULL); /* V4L2_BUF_TYPE_VIDEO_CAPTURE用于视频捕获设备,倒数第2个参数是buffer的头部大小 */
	//myvivi_video_qops这个结构体我们怎么去处理它呢？
								
								
	myvivi_timer.expires = jiffies + 1;			//1ms后开始进入mytimer function函数中
	add_timer(&myvivi_timer);					//定时器要生效的话，需要连接到内核专门的链表中,跳到timer 函数之后的一个函数中去
	
	return 0;
}
static int myvivi_close(struct file *file)
{
	//在myvivi_close中关闭timer
	del_timer(&myvivi_timer);
	
	
	//队列操作2： 初始化
	videobuf_stop(&myvivi_vb_vidqueue);		//stop the buf
	videobuf_mmap_free(&myvivi_vb_vidqueue);
	return 0;
}
//当内核空间缓冲的流IO请求完成后，驱动还必须支持mmap系统调用以使能用户空间可以访问data数据。在v4l2驱动中，通常很复杂的mmap的实现被简化了，只需要调用下面这个函数就可以了：
static int myvivi_mmap(struct file *file, struct vm_area_struct *vma)
{
	return videobuf_mmap_mapper(&myvivi_vb_vidqueue, vma);
}
//　　poll提供的功能与select类似，不过在处理流设备时，它能够提供额外的信息。
static unsigned int myvivi_poll(struct file *file, struct poll_table_struct *wait)
{
	return videobuf_poll_stream(file, &myvivi_vb_vidqueue, wait);
}
//----------------------------------------------------------------------------
// file operations for the device--------------------------------------

static const struct v4l2_file_operations myvivi_fops = 
{
	.owner = THIS_MODULE,
	.ioctl = video_ioctl2,	/* V4L2 ioctl handler */
	.open = myvivi_open,
	.release = myvivi_close,
	.mmap = myvivi_mmap,
	.poll = myvivi_poll,
};

static void myvivi_release(struct video_device *vdev)
{

}

static int myvivi_init(void)
{
	int error=0;
	/* 1. 分配一个video_device函数，分配空间 */
	myvivi_device = video_device_alloc();
	
    /* 2. 设置 */
	myvivi_device->release = myvivi_release;
	myvivi_device->fops = &myvivi_fops;
	myvivi_device->ioctl_ops = &myvivi_ioctl_ops;

	/*
	*	VFL_TYPE_GRABBER 0 表明一个图像采集设备的情况
	*	2.4 队列操作
	*	a. 定义/初始化一个队列（会用到一个自旋锁）
	*
	*/
	spin_lock_init(&myvivi_queue_slock);

	/* 3.注册 */
	error = video_register_device(myvivi_device, VFL_TYPE_GRABBER, -1);
	
	//用定时器产生数据并唤醒进程
	init_timer(&myvivi_timer);
	//定义内核定时器的作用函数
	myvivi_timer.function = myvivi_timer_function;
	
	
	INIT_LIST_HEAD(&myvivi_vb_local_queue);
				//初始化内核链表
	
	

	return error;
	
	
}

static void myvivi_exit(void)
{
	video_unregister_device(myvivi_device);
	video_device_release(myvivi_device);
}

module_init(myvivi_init);
module_exit(myvivi_exit);
MODULE_LICENSE("GPL");
