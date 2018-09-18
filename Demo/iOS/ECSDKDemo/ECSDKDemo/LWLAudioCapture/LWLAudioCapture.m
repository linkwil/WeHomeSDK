//
//  LWLAudioCapture.m
//  LWLAudioCaptureDemo
//
//  Created by linkwil on 2018/9/17.
//  Copyright © 2018年 xlink. All rights reserved.
//

#import "LWLAudioCapture.h"


#define LWL_MIN_SIZE_PER_FRAME 960
#define LWL_QUEUE_BUFFER_SIZES 3
#define LWL_SAMPLE_RATE        16000

@implementation LWLAudioCapture
{
    AudioQueueRef _audioQueueRef;
    AudioStreamBasicDescription _inFormat;
    AudioQueueInputCallback _inCallbackProc;
    AudioQueueBufferRef _audioBuffers[LWL_QUEUE_BUFFER_SIZES];
    OSStatus osState;
}


- (BOOL)LWLAC_SetFormat:(AudioStreamBasicDescription*)format callback:(AudioQueueInputCallback)callback callbackSelf:(id)callbackSelf
{
    NSError *error = nil;
    BOOL ret = [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord error:&error];
    if (!ret)
    {
        NSLog(@"setCategory failed!");
        return NO;
    }
    
    //启用audio session
    ret = [[AVAudioSession sharedInstance] setActive:YES error:&error];
    if (!ret)
    {
        NSLog(@"setActive failed");
        return NO;
    }
    
    if (!format)
    {
        _inFormat.mSampleRate          = LWL_SAMPLE_RATE;
        _inFormat.mFormatID            = kAudioFormatLinearPCM;
        _inFormat.mFormatFlags         = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
        _inFormat.mFramesPerPacket     = 1;
        _inFormat.mChannelsPerFrame    = 1;
        _inFormat.mBitsPerChannel      = 16;
        _inFormat.mBytesPerPacket      = 2;
        _inFormat.mBytesPerFrame       = 2;
    }else
    {
        _inFormat = *format;
    }
    
    _inCallbackProc = callback;
    
    AudioQueueNewInput(&_inFormat, _inCallbackProc, (__bridge void * _Nullable)(callbackSelf), NULL, NULL, 0, &_audioQueueRef);
    
    for (int i = 0; i < LWL_QUEUE_BUFFER_SIZES; i++)
    {
        AudioQueueAllocateBuffer(_audioQueueRef, LWL_MIN_SIZE_PER_FRAME, &_audioBuffers[i]);
        AudioQueueEnqueueBuffer(_audioQueueRef, _audioBuffers[i], 0, NULL);
    }
    
    return YES;
}

- (BOOL)LWLAC_StartCap
{
    osState = AudioQueueStart(_audioQueueRef, NULL);
    if (osState != noErr)
    {
        NSLog(@"AudioQueueStart Error");
        return NO;
    }
    return YES;
}

- (BOOL)LWLAC_StopCap
{
    AudioQueueStop(_audioQueueRef, TRUE);
//    AudioQueueDispose(_audioQueueRef, TRUE);
//    [[AVAudioSession sharedInstance] setActive:NO error:nil];
    return YES;
}

@end
