/* 参考sound/soc/samsung/dma.c
*/

/* 1. 分配DMA BUFFER
 * 2. 从BUFFER取出period
 * 3. 启动DMA传输
 * 4. 传输完毕,更新状态(hw_ptr)
 *    2,3,4这部分主要有: request_irq, 触发DMA传输, 中断处理
 */

 
//定义dma信息
struct s3c2440_dma_info {
    unsigned int buf_max_size;
    unsigned int phy_addr;
    unsigned int virt_addr;
    
};

static int struct s3c2440_dma_info playback_dma_info;

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


static irqreturn_t s3c2440_dma2_irq(int irq, void *devid)
{
    /* 更新状态信息HW_PTR */

    /* 如果还有数据
     * 1. 加载下一个period 
     * 2. 再次启动DMA传输
     */

    return IRQ_HANDLED;
}



static int s3c2440_dma_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int ret;
	
	/* 设置属性 */
	snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);
	snd_soc_set_runtime_hwparams(substream, &s3c2440_dma_hardware);
	
	//flag为当发生中断的时候，在中断处理过程中，中断处于静止状态中
	ret = request_irq(IRQ_DMA2, s3c2440_dma2_irq, IRQF_DISABLED,  "myalsa for playback", NULL);
	
	if(ret)
	{
		printk("request_irq error!\n");
		return -EIO;
	}
	
	return 0;
}

static int s3c2440_dma_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned long totbytes = params_buffer_bytes(params);		//确定buffer有多大
    
    /* 根据params设置DMA */
	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	
	/* s3c2440_dma_new分配了很大的DMA BUFFER
     * params决定使用多大
     */
	runtime->dma_bytes = totbytes;	
	
    return 0;
}


static int s3c2440_dma_prepare(struct snd_pcm_substream *substream)
{
    /* 准备DMA传输 */

    /* 复位各种状态信息 */

	return 0;
}

static int s3c2440_dma_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret = 0;

    /* 根据cmd启动或停止DMA传输 */


	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        /* 启动DMA传输 */
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        /* 停止DMA传输 */
		break;

	default:
		ret = -EINVAL;
		break;
	}


	return ret;
}


static struct snd_pcm_ops s3c2440_dma_ops = {
	.open = s3c2440_dma_open,
	.hw_params = s3c2440_dma_hw_params,
	.prepare    = s3c2440_dma_prepare,
	.trigger	= s3c2440_dma_trigger,
};

static int s3c2440_dma_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	int ret = 0;
	
	 /* 1. 分配DMA BUFFER，只支持播放 */
	 if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
        dma_alloc_writecombine();
		//物理地址和虚拟地址都在这里分配
	    playback_dma_info.virt_addr = dma_alloc_writecombine(pcm->card->dev, s3c2440_dma_hardware.buffer_bytes_max,
					   &playback_dma_info.phy_addr, GFP_KERNEL);
        if (!playback_dma_info.virt_addr)
        {
            return -ENOMEM;
        }
        playback_dma_info.buf_max_size = s3c2440_dma_hardware.buffer_bytes_max;
	}

	return ret;
}

static void s3c2440_dma_free(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	//只释放掉播放功能的虚拟地址和物理地址
	dma_free_writecombine(pcm->card->dev, playback_dma_info.buf_max_size,
			      playback_dma_info.virt_addr, playback_dma_info.phy_addr);
}

static struct snd_soc_platform_driver s3c2440_dma_platform = {
	.ops		= &s3c2440_dma_ops,
	.pcm_new	= s3c2440_dma_new,
	.pcm_free	= s3c2440_dma_free,
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