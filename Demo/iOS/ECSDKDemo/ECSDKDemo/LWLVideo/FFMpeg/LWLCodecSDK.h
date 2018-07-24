// CodecSDK.h
//
//////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C"
{
#endif
    
#ifndef __CodecSDK_h__
#define __CodecSDK_h__
    
#include "mypthread.h"
#include "./universal/include/libswresample/swresample.h"
#include "./universal/include/libavcodec/avcodec.h"
#include "./universal/include/libswscale/swscale.h"
    
#if 1
    
#define COLOR_FORMAT_YUV420 0
#define COLOR_FORMAT_RGB565LE 1
#define COLOR_FORMAT_BGR32 2
#define COLOR_FORMAT_RGB24 3
    
    
    typedef enum TAG_LWLH264DECODER_RETURN
    {
        LWLH264DECODER_RETURN_OK = 0,
        LWLH264DECODER_RETURN_FALSE = -1,
    }LWLH264DECODER_RETURN;
    
    
    
    /**
     初始化解码器
     
     @param colorFormat 输出数据的类型，此文件头部有宏定义
     @return 成功返回LWLH264DECODER_RETURN_OK，失败返回LWLH264DECODER_RETURN_FALSE
     */
    int LWLH264DecoderInit(int colorFormat);
    
    
    /**
     释放解码器
     
     @return 成功返回LWLH264DECODER_RETURN_OK，失败返回LWLH264DECODER_RETURN_FALSE
     */
    int LWLH264DecoderDestroy();
    
    
    
    /**
     解码接口
     
     @param inputData 输入数据
     @param dataLen 输入数据的大小
     @param outBuffer 输出数据缓冲区，此缓冲区由接口内容分配管理，调用者传入指针即可
     @param outBufferSize 输出数据的大小值
     @return 成功返回解码后数据的大小，该值大于0；失败返回LWLH264DECODER_RETURN_FALSE
     */
    int LWLH264DecoderDecodeFrame(unsigned char *inputData, int dataLen, unsigned char **outBuffer, int *outBufferSize);
    
    
    /**
     获取版本号
     
     @return 版本号，eg: 1001
     */
    int LWLH264DecoderVersion();
    
    
#endif
    
    
#endif
    
#ifdef __cplusplus
}
#endif




