/* 参考sound/soc/samsung/dma.c
*/



//这些都是相应的属性
static const struct snd_pcm_hardware s3c2440_dma_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED |		//数据存放的格式
				    SNDRV_PCM_INFO_BLOCK_TRANSFER |
				    SNDRV_PCM_INFO_MMAP |
				    SNDRV_PCM_INFO_MMAP_VALID |
				    SNDRV_PCM_INFO_PAUSE |
				    SNDRV_PCM_INFO_RESUME,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE |		//音频数据格式
				    SNDRV_PCM_FMTBIT_U16_LE |
				    SNDRV_PCM_FMTBIT_U8 |
				    SNDRV_PCM_FMTBIT_S8,
	.channels_min		= 2,
	.channels_max		= 2,
	.buffer_bytes_max	= 128*1024,
	.period_bytes_min	= PAGE_SIZE,
	.period_bytes_max	= PAGE_SIZE*2,
	.periods_min		= 2,
	.periods_max		= 128,
	.fifo_size		= 32,
};


static int s3c2440_dma_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	
	/* 设置属性 */
	snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);
	snd_soc_set_runtime_hwparams(substream, &s3c2440_dma_hardware);
	
	
	return 0;
}

static int s3c2440_dma_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
    /* 根据params设置DMA */
    return 0;
}

static struct snd_pcm_ops s3c2440_dma_ops = {
	.open = s3c2440_dma_open,
	.hw_params = s3c2440_dma_hw_params,
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