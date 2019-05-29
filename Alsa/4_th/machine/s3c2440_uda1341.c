#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/module.h>

#include <sound/soc.h>

/*
*	用来表明platform是哪一个，cpu dai是哪一个，dma是哪一个
*	参考sound\soc\samsung\s3c24xx_uda134x.c
*/

/*
*		1、分配注册一个soc-audio的平台设备
*		2、这个平台设备有一个私有数据snd_soc_card
*		snd_soc_card有一项snd_soc_dai_link
*		snd_soc_dai_link被用来决定ASOC各部分的驱动
*		
*/

static struct snd_soc_ops s3c2440_uda1341_ops = {
	//.hw_params = s3c24xx_uda134x_hw_params,
};



static struct snd_soc_dai_link s3c2440_uda1341_dai_link = {
	.name = "100ask_UDA1341",
	.stream_name = "100ask_UDA1341",
	.codec_name = "uda1341-codec",
	.codec_dai_name = "uda1341-iis",
	.cpu_dai_name = "s3c2440-iis",
	.ops = &s3c2440_uda1341_ops,
	.platform_name = "s3c2440-dma",
};




static struct snd_soc_card myalsa_card = {
	.name = "S3C2440_UDA1341",
	.owner = THIS_MODULE,
	.dai_link = &s3c2440_uda1341_dai_link,
	.num_links = 1,
};


static void asoc_release(struct device *dev)
{
	
}


static struct platform_device asoc_dev = {
	.name = "soc-audio",
	.id   = -1,
	.dev = {
		.release = asoc_release,
	},
};


static int s3c2440_uda1341_init(void)
{
	platform_set_drvdata(&asoc_dev, &myalsa_card);
	platform_device_register(&asoc_dev);
	return 0;
}


static void s3c2440_uda1341_exit(void)
{
	platform_device_unregister(&asoc_dev);
}


module_init(s3c2440_uda1341_init);
module_exit(s3c2440_uda1341_exit);