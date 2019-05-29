
/* 参考 sound\soc\codecs\uda134x.c
 */

/* 1. 构造一个snd_soc_dai_driver	DAI接口
 * 2. 构造一个snd_soc_codec_driver	控制接口
 * 3. 注册它们
 */

 
/* 所有寄存器的默认值 */
static const char uda1341_reg[] = {
};

static void uda1341_init_regs(void);

static int uda1341_soc_probe(struct snd_soc_codec *codec)
{
    uda1341_init_regs();
    return 0;
}


/*
 * The codec has no support for reading its registers except for peak level...
 */
static inline unsigned int uda1341_read_reg_cache(struct snd_soc_codec *codec,
	unsigned int reg)
{
	u8 *cache = codec->reg_cache;

	if (reg >= UDA1341_REGS_NUM)
		return -1;
	return cache[reg];
}

static int uda1341_write(struct snd_soc_codec *codec, unsigned int reg,
	unsigned int value)
{
    return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_uda1341 = {
    .probe = uda1341_soc_probe,

    /* UDA1341的寄存器不支持读操作
     * 要知道某个寄存器的当前值,
     * 只能在写入时保存起来
     */
	.reg_cache_size = sizeof(uda1341_reg),
	.reg_word_size = sizeof(u8),
	.reg_cache_default = uda1341_reg,
	.reg_cache_step = 1,
	.read  = uda1341_read_reg_cache,
	.write = uda1341_write,  /* 写寄存器 */
};


static void uda1341_init_regs(void)
{

	/* GPB 4: L3CLOCK */
	/* GPB 3: L3DATA */
	/* GPB 2: L3MODE */


	set_mod(1);
    set_clk(1);

	uda1341_l3_address(UDA1341_REG_STATUS);
	uda1341_l3_data(0x40 | STAT0_SC_384FS | STAT0_DC_FILTER); // reset uda1341
	uda1341_l3_data(STAT1 | STAT1_ADC_ON | STAT1_DAC_ON);

	uda1341_l3_address(UDA1341_REG_DATA0);
	uda1341_l3_data(DATA0 |DATA0_VOLUME(0x0)); // maximum volume
	uda1341_l3_data(DATA1 |DATA1_BASS(0)| DATA1_TREBLE(0));
	uda1341_l3_data((DATA2 |DATA2_DEEMP_NONE) &~(DATA2_MUTE));
	uda1341_l3_data(EXTADDR(EXT2));
}


static int uda1341_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
    /* 根据params的值,设置UDA1341的寄存器 */
    return 0;
}
 
 
static const struct snd_soc_dai_ops uda1341_dai_ops = {
	.hw_params	= uda1341_hw_params,
};
 
 static struct snd_soc_dai_driver uda1341_dai = {
	.name = "uda1341-iis",		//根据machine中的codec_dai_name
	/* playback capabilities */
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = UDA134X_RATES,		//采样率
		.formats = UDA134X_FORMATS,		//格式
	},
	/* capture capabilities */
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = UDA134X_RATES,
		.formats = UDA134X_FORMATS,
	},
	/* pcm operations */
	.ops = &uda1341_dai_ops,
 };
 
 /* 通过注册平台设备、平台驱动来实现对snd_soc_register_codec的调用
 *
 */
static void uda1341_dev_release(struct device *dev)
{
 
}
 
static int uda1341_probe(struct platform_device *pdev)
{
	//注册codec dai 驱动，注册codec driver
	return snd_soc_register_codec(&pdev->dev,
						&soc_codec_dev_uda1341, &uda1341_dai, 1);
}
 
 
static int uda1341_remove(struct platform_device *pdev)
{
	return snd_soc_unregister_codec(&pdev->dev);
}
 
static struct platform_device uda1341_dev = {
	.name = "uda1341-codec",		//与drv的名字是一致的
	.id = -1,
	.dev = {
		.release = uda1341_dev_release,
	},
};
 
 
struct platform_driver uda1341_drv = {
	.probe = uda1341_probe,
	.remove = uda1341_remove,
	.driver = {
		.name = "uda1341-codec",	//与machine中的名字是一致的
	}
};
 
 
static int uda1341_init(void)
{
	platform_device_register(&uda1341_dev);
	platform_driver_register(&uda1341_drv);
	return 0;
}


static void uda1341_exit(void)
{
	platform_device_unregister(&uda1341_dev);
	platform_driver_unregister(&uda1341_drv);
}


module_init(uda1341_init);
module_exit(uda1341_exit);