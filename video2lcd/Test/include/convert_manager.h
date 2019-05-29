#ifndef _CONVERT_MANAGER_H
#define _CONVERT_MANAGER_H


#include <config.h>
#include <video_manager.h>
#include <linux/videodev2.h>

typedef struct VideoConvert {
	//是否支持从这种格式转换到另外一种格式
	char *name;
	int (*isSupport)(int iPixelFormatIn, int iPixelFormatOut);
	int (*Convert)(PT_VideoBuf ptVideoBufIn, PT_VideoBuf ptVideoBufOut);
	int (*ConvertExit)(PT_VideoBuf ptVideoBufOut);
	struct VideoConvert *ptNext;
}T_VideoConvert, *PT_VideoConvert;


PT_VideoConvert GetVideoConvertForFormats(int iPixelFormatIn, int iPixelFormatOut);
int VideoConvertInit(void);



int Yuv2RgbInit(void);
int Mjpeg2RgbInit(void);
int Rgb2RgbInit(void);
int RegisterVideoConvert(PT_VideoConvert ptVideoConvert);

#endif