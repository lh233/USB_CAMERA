#ifndef _VIDEO_MANAGER_H
#define _VIDEO_MANAGER_H


/* 以面向对象的形式编写C语言，以单向链表存储相应的函数指针结构体，以注册函数为
 * 建立首节点，遍历函数为注册多个视频模块的名字为起点，相应遍历它；
 * GetVideoOpr()则以指定的名字为索引，来读取相应的模块
 */

#include <config.h>				//config.h位于include文件中，具体描述宏定义了FB_DEVICE_NAME "/dev/fb0“， 图标的位置等信息
#include <pic_operation.h>
#include <linux/videodev2.h>

#define NB_BUFFER 4


struct VideoDevice;
struct VideoOpr;

typedef struct VideoDevice T_VideoDevice, *PT_VideoDevice;
typedef struct VideoOpr T_VideoOpr, * PT_VideoOpr;				//函数操作集


struct VideoDevice{
	int iFd;					//文件描述符
	int iPixelFormat;			//像素格式，mjep，YUV，RGB等格式
	int iWidth;					//图片宽度
	int iHeight;				//图片高度
	
	int iVideoBufCnt;
	int iVideoBufMaxLen;
	int iVideoBufCurIndex;
	unsigned char *pucVideBuf[NB_BUFFER];				//指针数组
	
	/* 函数 */
	PT_VideoOpr ptOPr;
	
};




typedef struct VideoBuf{
	T_PixelDatas tPixelDatas;		//这个是保存图像像素信息，具体看pic_operation.h
	int iPixelFormat;
}T_VideoBuf, *PT_VideoBuf;

 
 struct VideoOpr{
	char *name;					//名字为多个视频模块注册之后的情况
	int (*InitDevice)(char *strDevName, PT_VideoDevice ptVideoDevice);
	int (*ExitDevice)(PT_VideoDevice ptVideoDevice);
	int (*GetFrame)(PT_VideoDevice ptVideoDevice, PT_VideoBuf ptVideoBuf);
	int (*GetFormat)(PT_VideoDevice ptVideoDevice);
	int (*PutFrame)(PT_VideoDevice ptVideoDevice, PT_VideoBuf ptVideoBuf);
	int (*StartDevice)(PT_VideoDevice ptVideoDevice);
	int (*StopDevice)(PT_VideoDevice ptVideoDevice);
	struct VideoOpr *ptNext;
};


//在头文件中声明
int VideoDeviceInit(char *strDevName, PT_VideoDevice ptVideoDevice);
int V4l2Init(void);
int RegisterVideoOpr(PT_VideoOpr ptVideoOpr);
int VideoInit(void);



#endif