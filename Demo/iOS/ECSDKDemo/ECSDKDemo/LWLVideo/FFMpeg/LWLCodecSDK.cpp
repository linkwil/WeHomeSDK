#include "LWLCodecSDK.h"
#include <map>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define LWLH264DECODERVERSION 1002         //-->str:1.0.0.2
static pthread_mutex_t lwlH264Mutex = PTHREAD_MUTEX_INITIALIZER;        //锁

typedef struct DecoderContext {
    int color_format;
    struct AVCodec *codec;
    struct AVCodecContext *codec_ctx;
    struct AVFrame *src_frame;
    struct AVFrame *dst_frame;
    struct SwsContext *convert_ctx;
    int frame_ready;
} DecoderContext;

static DecoderContext *ctx = NULL;
unsigned char *gOutBuffer = NULL;


int LWLH264DecoderInit(int colorFormat)
{
    pthread_mutex_lock(&lwlH264Mutex);
    if(colorFormat != COLOR_FORMAT_YUV420 && colorFormat != COLOR_FORMAT_RGB565LE && colorFormat != COLOR_FORMAT_BGR32 && colorFormat != COLOR_FORMAT_RGB24)
    {
        printf("LWLH264DecoderInit, colorFormat error.");
        pthread_mutex_unlock(&lwlH264Mutex);
        return LWLH264DECODER_RETURN_FALSE;
    }
    
    if (ctx)
    {
        free(ctx);
        ctx = NULL;
    }
    ctx = (DecoderContext *)malloc(sizeof(DecoderContext));
    if (!ctx)
    {
        printf("LWLH264DecoderInit, create ctx false.");
        pthread_mutex_unlock(&lwlH264Mutex);
        return LWLH264DECODER_RETURN_FALSE;
    }
    memset(ctx, 0, sizeof(DecoderContext));
    
    int ret = 0;
    switch (colorFormat)
    {
        case COLOR_FORMAT_YUV420:
            ctx->color_format = PIX_FMT_YUV420P;
            break;
        case COLOR_FORMAT_RGB565LE:
            ctx->color_format = PIX_FMT_RGB565LE;
            break;
        case COLOR_FORMAT_BGR32:
            ctx->color_format = PIX_FMT_BGR32;
            break;
        case COLOR_FORMAT_RGB24:
            ctx->color_format = PIX_FMT_RGB24;
            break;
    }
    
    avcodec_register_all();
    ctx->codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    ctx->codec_ctx = avcodec_alloc_context3(ctx->codec);
    
    ctx->codec_ctx->pix_fmt = PIX_FMT_YUV420P;
    ctx->codec_ctx->flags2 |= CODEC_FLAG2_CHUNKS;
    
    ctx->src_frame = av_frame_alloc();
    ctx->dst_frame = av_frame_alloc();
    
    ret = avcodec_open2(ctx->codec_ctx, ctx->codec, NULL);
    
    if (!ctx->codec || ret < 0)
    {
        printf("avcodec_open2 false, ret:%d\n", ret);
        avcodec_close(ctx->codec_ctx);
        av_free(ctx->codec_ctx);
        pthread_mutex_unlock(&lwlH264Mutex);
        return  LWLH264DECODER_RETURN_FALSE;
    }
    
    pthread_mutex_unlock(&lwlH264Mutex);
    return LWLH264DECODER_RETURN_OK;
}


int LWLH264DecoderDestroy()
{
    pthread_mutex_lock(&lwlH264Mutex);
    if (!ctx)
    {
        printf("LWLH264DecoderDestroy, ctx is NULL, return.");
        pthread_mutex_unlock(&lwlH264Mutex);
        return LWLH264DECODER_RETURN_FALSE;
    }
    
    avcodec_close(ctx->codec_ctx);
    av_free(ctx->codec_ctx);
    av_free(ctx->src_frame);
    av_free(ctx->dst_frame);
    free(ctx);
    ctx = NULL;
    
    if (gOutBuffer)
    {
        free(gOutBuffer);
        gOutBuffer = NULL;
    }
    
    pthread_mutex_unlock(&lwlH264Mutex);
    return LWLH264DECODER_RETURN_OK;
}


int LWLH264DecoderDecodeFrame(unsigned char *inputData, int dataLen, unsigned char **outBuffer, int *outBufferSize)
{
    pthread_mutex_lock(&lwlH264Mutex);
    if (!inputData)
    {
        printf("Received null buffer, sending empty packet to decoder");
        pthread_mutex_unlock(&lwlH264Mutex);
        return LWLH264DECODER_RETURN_FALSE;
    }
    if (!ctx)
    {
        printf("LWLH264DecoderDestroy, ctx is NULL, return.");
        pthread_mutex_unlock(&lwlH264Mutex);
        return LWLH264DECODER_RETURN_FALSE;
    }
    
    AVPacket packet;
    memset(&packet, 0, sizeof(AVPacket));
    packet.data = inputData;
    packet.size = dataLen;
    
    int decodedFrameDataSize = 0;
    
    int frameFinished = 0;
    avcodec_decode_video2(ctx->codec_ctx, ctx->src_frame, &frameFinished, &packet);
    
    if (frameFinished)
    {
        ctx->frame_ready = 1;
        
        decodedFrameDataSize = avpicture_get_size(PIX_FMT_RGB24, ctx->codec_ctx->width, ctx->codec_ctx->height);
        if (decodedFrameDataSize <= 0)
        {
            printf("decodedFrameDataSize <= 0, return");
            pthread_mutex_unlock(&lwlH264Mutex);
            return LWLH264DECODER_RETURN_FALSE;
        }
        if (ctx->convert_ctx == NULL)
        {
            gOutBuffer = (unsigned char *)malloc(decodedFrameDataSize);
            if((gOutBuffer) == NULL)
            {
                printf("outBuffer malloc false, return");
                pthread_mutex_unlock(&lwlH264Mutex);
                return LWLH264DECODER_RETURN_FALSE;
            }
            ctx->convert_ctx = sws_getContext(ctx->codec_ctx->width, ctx->codec_ctx->height, ctx->codec_ctx->pix_fmt,ctx->codec_ctx->width, ctx->codec_ctx->height, PIX_FMT_RGB24, SWS_FAST_BILINEAR, NULL, NULL, NULL);
        }
        avpicture_fill((AVPicture*)ctx->dst_frame, (uint8_t*)(gOutBuffer),PIX_FMT_RGB24, ctx->codec_ctx->width, ctx->codec_ctx->height);
        sws_scale(ctx->convert_ctx, (const uint8_t**)ctx->src_frame->data, ctx->src_frame->linesize, 0, ctx->codec_ctx->height,ctx->dst_frame->data, ctx->dst_frame->linesize);
        (*outBufferSize) = decodedFrameDataSize;
        (*outBuffer) = gOutBuffer;
        pthread_mutex_unlock(&lwlH264Mutex);
        return decodedFrameDataSize;
        
    }
    else
    {
        printf("frameFinished is false, return");
    }
    
    pthread_mutex_unlock(&lwlH264Mutex);
    return LWLH264DECODER_RETURN_FALSE;
}


int LWLH264DecoderVersion()
{
    printf("LWLH264DecoderVersion enter, version:%d", LWLH264DECODERVERSION);
    return LWLH264DECODERVERSION;
}
