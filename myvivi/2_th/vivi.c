/*仿照vivi.c*/
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


/*
video_device结构体在v4l2-dev.h中函数中：
void (*release)(struct video_device *vdev);
const struct v4l2_ioctl_ops *ioctl_ops;
const struct v4l2_file_operations *fops;



v4l2_file_operations 
	static const struct v4l2_file_operations vivi_fops = {
	.owner		= THIS_MODULE,
	.open           = vivi_open,
	.release        = vivi_close,
	.read           = vivi_read,
	.poll		= vivi_poll,
	.ioctl          = video_ioctl2, // V4L2 ioctl handler 
	.mmap           = vivi_mmap,
};	

*/

static int myvivi_vidioc_querycap(struct file *file, void  *priv,
					struct v4l2_capability *cap)
{

	strcpy(cap->driver, "myvivi");
	strcpy(cap->card, "myvivi");
//	strlcpy(cap->bus_info, dev->v4l2_dev.name, sizeof(cap->bus_info));
	cap->version = 0x0001;
	cap->capabilities =	V4L2_CAP_VIDEO_CAPTURE |		\
				V4L2_CAP_STREAMING   ;
	/*			V4L2_CAP_READWRITE;  //暂时不提供read函数*/
	return 0;
}



static struct video_device *myvivi_device;

static void myvivi_release(struct video_device *vdev)
{
	
}


//需要设置一个ioctl的结构体
static const struct v4l2_ioctl_ops myvivi_ioctl_ops = {
	// 表示它是一个摄像头设备
	.vidioc_querycap      = myvivi_vidioc_querycap,
	
	 
#if 0
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
#endif 

};

static const struct v4l2_file_operations myvivi_fops = {
	.owner		= THIS_MODULE,
	//必须增加这个才能写video_ioctl_fops

    .ioctl      = video_ioctl2, /* V4L2 ioctl handler */
};










static int myvivi_init(void)
{
	int error = 0;
	
	/* 1. 分配一个video_device  字符设备驱动结构体 */
	myvivi_device = video_device_alloc();
	
	/* 2. 设置 */
	myvivi_device->release =  myvivi_release; 
	myvivi_device->fops = &myvivi_fops;
	myvivi_device->ioctl_ops = &myvivi_ioctl_ops;
	
	/* 3. 注册 */
	//VFL_TYPE_GRABBER表明是一个图像采集设备-包括摄像头、调谐器
	//VFL_TYPE_VBI  从视频消隐的时间段取得信息的设备
	//VFL_TYPE_RADIO 代表无线电设备
	//VFL_TYPE_VTX   代表视传设备
	//
	/*
	*	#define VFL_TYPE_GRABBER	0
	*	#define VFL_TYPE_VBI		1
	*	#define VFL_TYPE_RADIO		2
	*	#define VFL_TYPE_VTX		3
	*	#define VFL_TYPE_MAX		4
	*
	
		第三个参数为video_regiseter_device的参数
		@nr:   which device number (0 == /dev/video0, 1 == /dev/video1, ...
 *             -1 == first free)
	*/
	error = video_register_device(myvivi_device, VFL_TYPE_GRABBER, -1);
	
	
	//通过dmesg命令可以查看
	/*
		[ 3751.302082] WARNING: at /build/buildd/linux-2.6.31/drivers/media/video/v4l2-dev.c:395 video_register_device_index+0x46c/0x4e0 [videodev]()
			在395行中，缺少一个_release()
		[ 3751.302110] Hardware name: VMware Virtual Platform
		[ 3751.302113] Modules linked in: vivi(+) videobuf_vmalloc videobuf_core v4l2_common videodev v4l1_compat isofs udf crc_itu_t binfmt_misc snd_ens1371 gameport snd_ac97_codec ac97_bus snd_pcm_oss snd_mixer_oss nfsd snd_pcm exportfs snd_seq_dummy nfs snd_seq_oss lockd iptable_filter snd_seq_midi snd_rawmidi snd_seq_midi_event nfs_acl ip_tables snd_seq auth_rpcgss snd_timer pl2303 x_tables ppdev snd_seq_device usbserial sunrpc parport_pc snd soundcore snd_page_alloc psmouse i2c_piix4 serio_raw lp shpchp parport floppy pcnet32 mii intel_agp agpgart mptspi mptscsih mptbase scsi_transport_spi [last unloaded: vivi]
		[ 3751.302166] Pid: 2990, comm: insmod Not tainted 2.6.31-14-generic #48-Ubuntu
		[ 3751.302169] Call Trace:
		[ 3751.302192]  [<c014518d>] warn_slowpath_common+0x6d/0xa0
		[ 3751.302199]  [<f822276c>] ? video_register_device_index+0x46c/0x4e0 [videodev]
		[ 3751.302205]  [<f822276c>] ? video_register_device_index+0x46c/0x4e0 [videodev]
		[ 3751.302211]  [<c01451d5>] warn_slowpath_null+0x15/0x20
		[ 3751.302217]  [<f822276c>] video_register_device_index+0x46c/0x4e0 [videodev]
		[ 3751.302223]  [<f822298e>] ? video_device_alloc+0x1e/0x80 [videodev]
		[ 3751.302229]  [<f82227f2>] video_register_device+0x12/0x20 [videodev]
		[ 3751.302234]  [<f80df039>] myvivi_init+0x19/0x1c [vivi]
		[ 3751.302238]  [<c010112c>] do_one_initcall+0x2c/0x190
		[ 3751.302242]  [<f80df020>] ? myvivi_init+0x0/0x1c [vivi]
		[ 3751.302249]  [<c0173751>] sys_init_module+0xb1/0x1f0
		[ 3751.302253]  [<c010336c>] syscall_call+0x7/0xb
		[ 3751.302257] ---[ end trace afd31a470d40d180 ]---
	*/
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



/*
实验结果：
Warning: Missing charsets in String to FontSet conversion
Warning: Cannot convert string "-*-lucidatypewriter-bold-r-normal-*-14-*-*-*-m-*-iso8859-*,           -*-courier-bold-r-normal-*-14-*-*-*-m-*-iso8859-*,          -gnu-unifont-bold-r-normal--16-*-*-*-c-*-*-*,        -efont-biwidth-bold-r-normal--16-*-*-*-*-*-*-*,                 -*-*-bold-r-normal-*-16-*-*-*-m-*-*-*,                 -*-*-bold-r-normal-*-16-*-*-*-c-*-*-*,                         -*-*-*-*-*-*-16-*-*-*-*-*-*-*, *" to type FontSet
ioctl: VIDIOC_G_STD(std=0xb78261d000cb3326 [PAL_B1,PAL_G,PAL_D,PAL_M,PAL_N,NTSC_M,NTSC_M_JP,SECAM_B,SECAM_D,SECAM_H,SECAM_L,?ATSC_8_VSB,(null),(null),(null),(null),(null),(null),(null),(null),(null),(null),(null),(null),(null),(null)]): Invalid argument
ioctl: VIDIOC_G_INPUT(int=0): Invalid argument
ioctl: VIDIOC_S_INPUT(int=0): Invalid argument
ioctl: VIDIOC_S_STD(std=0x0 []): Invalid argument
no way to get: 384x288 32 bit TrueColor (LE: bgr-)


*/

