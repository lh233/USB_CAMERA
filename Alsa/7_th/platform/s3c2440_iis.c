/* 参考sound\soc\samsung\s3c24xx-i2s.c
*/


struct s3c2440_iis_regs {
    unsigned int iiscon ; 
    unsigned int iismod ; 
    unsigned int iispsr ; 
    unsigned int iisfcon; 
    unsigned int iisfifo; 
};

static volatile unsigned int *gpecon;
static volatile struct s3c2440_iis_regs *iis_regs;


static int s3c2440_i2s_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *dai)
{
    /* 根据params设置IIS控制器 */

    int tmp_fs;
    int i;
    int min = 0xffff;
    int pre = 0;
    unsigned int fs;		//采样率
    struct clk *clk = clk_get(NULL, "pclk");

    /* 配置GPIO用于IIS */
    *gpecon &= ~((3<<0) | (3<<2) | (3<<4) | (3<<6) | (3<<8));
    *gpecon |= ((2<<0) | (2<<2) | (2<<4) | (2<<6) | (2<<8));
    
    
    /* bit[9] : Master clock select, 0-PCLK
     * bit[8] : 0 = Master mode
     * bit[7:6] : 10 = Transmit mode
     * bit[4] : 0-IIS compatible format
     * bit[2] : 384fs, 确定了MASTER CLOCK之后, fs = MASTER CLOCK/384
     * bit[1:0] : Serial bit clock frequency select, 32fs
     */
     
    if (params_format(params) == SNDRV_PCM_FORMAT_S16_LE)
        iis_regs->iismod = (2<<6) | (0<<4) | (1<<3) | (1<<2) | (1);
    else if (params_format(params) == SNDRV_PCM_FORMAT_S8)
        iis_regs->iismod = (2<<6) | (0<<4) | (0<<3) | (1<<2) | (1);
    else
        return -EINVAL;

    /* Master clock = PCLK/(n+1)
     * fs = Master clock / 384
     * fs = PCLK / (n+1) / 384
     */
    fs = params_rate(params);
    for (i = 0; i <= 31; i++)
    {
        tmp_fs = clk_get_rate(clk)/384/(i+1);
        if (ABS(tmp_fs, fs) < min)
        {
            min = ABS(tmp_fs, fs);
            pre = i;
        }
    }
    iis_regs->iispsr = (pre << 5) | (pre);

    /*
     * bit15 : Transmit FIFO access mode select, 1-DMA
     * bit13 : Transmit FIFO, 1-enable
     */
    iis_regs->iisfcon = (1<<15) | (1<<13);
    
    /*
     * bit[5] : Transmit DMA service request, 1-enable
     * bit[1] : IIS prescaler, 1-enable
     */
    iis_regs->iiscon = (1<<5) | (1<<1) ;

    clk_put(clk);
    
    return 0;
}


static const struct snd_soc_dai_ops s3c2440_i2s_dai_ops = {
	.hw_params = s3c2440_i2s_hw_params,
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

struct platform_driver s3c2440_iis_drv = {
	.probe = s3c2440_iis_probe,
	.remove = s3c2440_iis_remove,
	.driver = {
		.name = "s3c2440-iis",
	}
};

static int s3c2440_iis_init(void)
{
	gpecon   = ioremap(0x56000040, 4);
    iis_regs = ioremap(0x55000000, sizeof(struct s3c2440_iis_regs));
	platform_device_register(&s3c2440_iis_dev);
    platform_driver_register(&s3c2440_iis_drv);
    return 0;
}

static void s3c2440_iis_exit(void)
{
	platform_device_unregister(&s3c2440_iis_dev);
	platform_driver_unregister(&s3c2440_iis_drv);
	iounmap(gpecon);
    iounmap(iis_regs);
}


module_init(s3c2440_iis_init);
module_exit(s3c2440_iis_exit);
 