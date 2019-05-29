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


/* ------------------------------------------------------------------
	IOCTL vidioc handling
   ------------------------------------------------------------------*/
static int myvivi_videoc_querycap(struct file *file, void  *priv,
					struct v4l2_capability *cap)
{
	//VIDIOC_QUERYCAP 命令通过结构 v4l2_capability 获取设备支持的操作模式：
	/*
		struct v4l2_capability {
		__u8	driver[16];	 	i.e. "bttv" 
		__u8	card[32];		i.e. "Hauppauge WinTV"
		__u8	bus_info[32];	"PCI:" + pci_name(pci_dev) 
		__u32   version;        should use KERNEL_VERSION() 
		__u32	capabilities; 	Device capabilities 
		__u32	reserved[4];
		};
	 * 
	 */
	strcpy(cap->driver, "myvivi");
	strcpy(cap->card, "myvivi");
	
	cap->version = 0x0001;
	cap->capabilities =	V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;		//V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING表示一个视
																			//频捕捉设备并且具有数据流控制模式
	
	return 0;
}

/* 列举支持哪种格式 */
static int myvivi_vidioc_enum_fmt_vid_cap(struct file *file, void  *priv,
					struct v4l2_fmtdesc *f)
{
	/*
	*	F O R M A T   E N U M E R A T I O N
	
	struct v4l2_fmtdesc {
		__u32		    index;             // Format number      , 需要填充，从0开始，依次上升。
		enum v4l2_buf_type  type;          // buffer type       Camera，则填写V4L2_BUF_TYPE_VIDEO_CAPTURE
		__u32               flags;			//	如果压缩的，则Driver 填写：V4L2_FMT_FLAG_COMPRESSED，
		__u8		    description[32];   // Description string ,image format的描述，如：YUV 4:2:2 (YUYV)
		__u32		    pixelformat;       // Format fourcc ,所支持的格式。 如：V4L2_PIX_FMT_UYVY    
		__u32		    reserved[4];
	};
	*/

	if(f->index >= 1)
	{
		return -EINVAL;
	}
	strcpy(f->description, "4:2:2, packed, YUYV");
	
	//从vivi_fmt结构体中可以看见：
	f->pixelformat = V4L2_PIX_FMT_YUYV;
	
	return 0;
}



struct v4l2_format myvivi_format;
/* 返回当前所使用的格式 */
static int myvivi_vidioc_g_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	
	memcpy(f, &myvivi_format, sizeof(myvivi_format));

	return (0);
}

/* 测试驱动程序是否支持某种格式 */
static int myvivi_vidioc_try_fmt_vid_cap(struct file *file, void *priv,
			struct v4l2_format *f)
{
	unsigned int maxw, maxh;
	enum v4l2_field field;
	
	
	if (f->fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV)
        return -EINVAL;

	field = f->fmt.pix.field;

	if (field == V4L2_FIELD_ANY) {
		field = V4L2_FIELD_INTERLACED;
	} else if (V4L2_FIELD_INTERLACED != field) {
		return -EINVAL;
	}
	
	maxw = 1024;
	maxh = 768;
	
	/*
	 v4l2_format:
	 struct v4l2_format {  
		enum v4l2_buf_type type;  
		union {  
        struct v4l2_pix_format         pix;     // V4L2_BUF_TYPE_VIDEO_CAPTURE 
        struct v4l2_window             win;     // V4L2_BUF_TYPE_VIDEO_OVERLAY   
        struct v4l2_vbi_format         vbi;     // V4L2_BUF_TYPE_VBI_CAPTURE 
        struct v4l2_sliced_vbi_format  sliced;  // V4L2_BUF_TYPE_SLICED_VBI_CAPTURE 
        __u8   raw_data[200];                   // user-defined   
    } fmt;  
	};

	其中  
	enum v4l2_buf_type 
	{  
		V4L2_BUF_TYPE_VIDEO_CAPTURE        = 1,  
		V4L2_BUF_TYPE_VIDEO_OUTPUT         = 2,  
		V4L2_BUF_TYPE_VIDEO_OVERLAY        = 3,  
		...  
		V4L2_BUF_TYPE_PRIVATE              = 0x80,  
	}; 
	
		struct v4l2_pix_format {  
		__u32                   width;  
		__u32                   height;  
		__u32                   pixelformat;  
		enum v4l2_field         field;  
		__u32                   bytesperline;   // for padding, zero if unused 
		__u32                   sizeimage;  
		enum v4l2_colorspace    colorspace;  
		__u32                   priv;           // private data, depends on pixelformat 
	};  
		
	常见的捕获模式为 V4L2_BUF_TYPE_VIDEO_CAPTURE 即视频捕捉模式，在此模式下 fmt 联合体采用域 v4l2_pix_format：其中 width 为
	视频的宽、height 为视频的高、pixelformat 为视频数据格式（常见的值有 V4L2_PIX_FMT_YUV422P | V4L2_PIX_FMT_RGB565）、
	bytesperline 为一行图像占用的字节数、sizeimage 则为图像占用的总字节数、colorspace 指定设备的颜色空间。	 

	*/
	
	
	//设置最小宽度和最大宽度等
	/* 调整format的width, height, 
     * 计算bytesperline, sizeimage
     */
	v4l_bound_align_image(&f->fmt.pix.width, 48, maxw, 2, &f->fmt.pix.height, 32, maxh, 0, 0);
	
	f->fmt.pix.bytesperline =
		(f->fmt.pix.width * 16) >> 3;		//颜色深度支持16
	f->fmt.pix.sizeimage =
		f->fmt.pix.height * f->fmt.pix.bytesperline;
	
}


static int myvivi_vidioc_s_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	int ret = myvivi_vidioc_try_fmt_vid_cap(file, NULL, f);
	if (ret < 0)
		return ret;

    memcpy(&myvivi_format, f, sizeof(myvivi_format));
    
	return ret;
}
/*------------------------------------------------------------------*/







/*
------------------------------------------------------------
struct video_device
	// device ops 
	const struct v4l2_file_operations *fops;
------------------------------------------------------------
				v4l2_file_operations=
				{
					struct module *owner;
					long (*ioctl) (struct file *, unsigned int, unsigned long);
				}



------------------------------------------------------------
	// callbacks 
	void (*release)(struct video_device *vdev);
------------------------------------------------------------
*/
static const struct v4l2_ioctl_ops myvivi_ioctl_ops = 
{
	//表示它是一个摄像头驱动
	.vidioc_querycap = myvivi_videoc_querycap,
	

	/* 用于列举、获得、测试、设置摄像头的数据的格式 */
	.vidioc_enum_fmt_vid_cap  = myvivi_vidioc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap     = myvivi_vidioc_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap   = myvivi_vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap     = myvivi_vidioc_s_fmt_vid_cap,
#if 0	
	/* 缓冲区操作: 申请/查询/放入队列/取出队列 */
	.vidioc_reqbufs       = myvivi_vidioc_reqbufs,
	.vidioc_querybuf      = myvivi_vidioc_querybuf,
	.vidioc_qbuf          = myvivi_vidioc_qbuf,
	.vidioc_dqbuf         = myvivi_vidioc_dqbuf,
	
	// 启动/停止
	.vidioc_streamon      = myvivi_vidioc_streamon,
	.vidioc_streamoff     = myvivi_vidioc_streamoff,  
#endif
};


static const struct v4l2_file_operations myvivi_fops = 
{
	.owner = THIS_MODULE,
	.ioctl = video_ioctl2, /* v4l2 ioctl handler，在这个函数中设置了ioctl的用法 */
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
	
	/* 2.1 */
    myvivi_device->release = myvivi_release;
	/* 2.2 */
    myvivi_device->fops    = &myvivi_fops;
	/* 2.3 */
    myvivi_device->ioctl_ops = &myvivi_ioctl_ops;

	
	/*
	#define VFL_TYPE_GRABBER	0		//表明是一个图像采集设备-包括摄像头、调谐器
	#define VFL_TYPE_VBI		1		//从视频消隐的时间段取得信息的设备
	#define VFL_TYPE_RADIO		2		//代表无线电设备
	#define VFL_TYPE_VTX		3		//代表视传设备
	#define VFL_TYPE_MAX		4
	@nr:   which device number (0 == /dev/video0, 1 == /dev/video1, ...
             -1 == first free)
	*/
	/* 3.注册，第二个参数为VFL_TYPE_GRABBER， -1 为 first free */
	error = video_register_device(myvivi_device, VFL_TYPE_GRABBER, -1);

	return error;
};


/*
 *	出口函数
 * 
 */
static void myvivi_exit(void)
{
	video_unregister_device(myvivi_device);
	video_device_release(myvivi_device);
}


module_init(myvivi_init);
module_exit(myvivi_exit);
MODULE_LICENSE("GPL");

