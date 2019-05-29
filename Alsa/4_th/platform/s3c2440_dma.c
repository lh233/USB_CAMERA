/* ²Î¿¼sound/soc/samsung/dma.c
*/

static struct snd_pcm_ops s3c2440_dma_ops = {
	
};

static struct snd_soc_platform_driver s3c2440_dma_platform = {
	.ops		= &s3c2440_dma_ops,
	//.pcm_new	= dma_new,
	//.pcm_free	= dma_free_dma_buffers,
};

static int s3c2440_dma_probe(struct platform_device *pdev)
{
	return snd_soc_register_platform(&pdev->dev, &s3c2440_dma_platform);
}

static int s3c2440_dma_remove(struct platform_device *pdev)
{
	return snd_soc_unregister_platform(&pdev->dev);
}


static void s3c2440_dma_release(struct device *dev)
{
}


static struct platform_device s3c2440_dma_dev = {
	.name         = "s3c2440-dma",
    .id       = -1,
    .dev = { 
    	.release = s3c2440_dma_release, 
	},
};

struct platform_driver s3c2440_dma_drv = {
	.probe = s3c2440_dma_probe,
	.remove = s3c2440_dma_remove,
	.driver = {
		.name = "s3c2440-dma",
	}
};


static int s3c2440_dma_init(void)
{
	platform_device_register(&s3c2440_dma_dev);
	platform_driver_register(&s3c2440_dma_drv);
	return 0;
}


static void s3c2440_dma_exit(void)
{
	platform_device_unregister(&s3c2440_dma_dev);
	platform_driver_unregister(&s3c2440_dma_drv);
}

module_init(s3c2440_dma_init);
module_exit(s3c2440_dma_exit);