/* 参考sound\soc\samsung\s3c24xx-i2s.c
*/

static const struct snd_soc_dai_ops s3c2440_i2s_dai_ops = {
};


static struct snd_soc_dai_driver s3c2440_i2s_dai = {
	.playback = {
		.channels_min = 2,		//只支持双声道
		.channels_max = 2,
		.rates = S3C24XX_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE,},
	.capture = {
		.channels_min = 2,
		.channels_max = 2,
		.rates = S3C24XX_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE,},
	.ops = &s3c2440_i2s_dai_ops,
};


static int s3c2440_iis_probe(struct platform_device *pdev)
{
	return snd_soc_register_dai(&pdev->dev, &s3c2440_i2s_dai);
}
static int s3c2440_iis_remove(struct platform_device *pdev)
{
	return snd_soc_unregister_dai(&pdev->dev);
}

static void s3c2440_iis_release(struct device * dev)
{
}

static struct platform_device s3c2440_iis_dev = {
    .name         = "s3c2440-iis",
    .id       = -1,
    .dev = { 
    	.release = s3c2440_iis_release, 
	},
};

static platform_driver s3c2440_iis_drv = {
	.probe = s3c2440_iis_probe,
	.remove = s3c2440_iis_remove,
	.driver = {
		.name = "s3c2440-iis",
	}
};

static int s3c2440_iis_init(void)
{
	platform_device_register(&s3c2440_iis_dev);
    platform_driver_register(&s3c2440_iis_drv);
    return 0;
}

static void s3c2440_iis_exit(void)
{
	platform_device_unregister(&s3c2440_iis_dev);
	platform_driver_unregister(&s3c2440_iis_drv);
}


module_init(s3c2440_iis_init);
module_exit(s3c2440_iis_exit);
 