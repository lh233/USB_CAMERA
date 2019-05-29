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


static int myuvc_probe(struct usb_interface *intf,
		     const struct usb_device_id *id)
{
    static int cnt = 0;
    printk("myuvc_probe : cnt = %d\n", cnt++);
    return 0;
}

static void myuvc_disconnect(struct usb_interface *intf)
{
    static int cnt = 0;
    printk("myuvc_disconnect : cnt = %d\n", cnt++);
}

static struct usb_device_id myuvc_ids[] = 
{
	/* Generic USB Video Class */
	{ USB_INTERFACE_INFO(USB_CLASS_VIDEO, 1, 0) },  /* VideoControl Interface */
    { USB_INTERFACE_INFO(USB_CLASS_VIDEO, 2, 0) },  /* VideoStreaming Interface */
	{}
};

/* 1. 分配usb_driver */
/* 2. 设置 */
static struct usb_driver myuvc_driver = {
	.name = "myuvc",
	.probe = myuvc_probe,
	.disconnect = myuvc_disconnect,
	.id_table = myuvc_ids,
};

static int myuvc_init(void)
{
    /* 3. 注册 */
    usb_register(&myuvc_driver);
    return 0;
}

static void myuvc_exit(void)
{
    usb_deregister(&myuvc_driver);
}

module_int(myuvc_init);
module_exit(myuvc_exit);
MODULE_LICENSE("GPL");
