//
//  LWLAudio.m
//  ECSDKDemo
//
//  Created by linkwil on 2018/7/24.
//  Copyright © 2018年 xlink. All rights reserved.
//

#import "LWLAudio.h"
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>


#define LWL_MIN_SIZE_PER_FRAME 960
#define LWL_QUEUE_BUFFER_SIZES 3
#define LWL_SAMPLE_RATE        16000


@interface LWLAudio()
{
    AudioQueueRef _audioQueue;
    AudioStreamBasicDescription _audioDescription;
    AudioQueueBufferRef _audioBuffer[LWL_QUEUE_BUFFER_SIZES];
    BOOL _audioBufferIsUsed[LWL_QUEUE_BUFFER_SIZES];
    NSLock *_audioLock;
    OSStatus osState;
}

@end

@implementation LWLAudio

- (id)init
{
    self = [super init];
    if (self)
    {
        [LWLAudio initialize];
        
        _audioLock = [[NSLock alloc] init];
        
        _audioDescription.mSampleRate = LWL_SAMPLE_RATE;
        _audioDescription.mFormatID = kAudioFormatLinearPCM;
        _audioDescription.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
        _audioDescription.mChannelsPerFrame = 1;
        _audioDescription.mFramesPerPacket = 1;
        _audioDescription.mBitsPerChannel = 16;
        _audioDescription.mBytesPerFrame = (_audioDescription.mBitsPerChannel / 8) * _audioDescription.mChannelsPerFrame;
        _audioDescription.mBytesPerPacket = _audioDescription.mBytesPerFrame * _audioDescription.mFramesPerPacket;
        
        AudioQueueNewOutput(&_audioDescription, AudioPlayerAQInputCallback, (__bridge void * _Nullable)(self), nil, 0, 0, &_audioQueue);
        
        AudioQueueSetParameter(_audioQueue, kAudioQueueParam_Volume, 1.0);
        
        for (int i = 0; i < LWL_QUEUE_BUFFER_SIZES; i++)
        {
            _audioBufferIsUsed[i] = NO;
            osState = AudioQueueAllocateBuffer(_audioQueue, LWL_MIN_SIZE_PER_FRAME, &_audioBuffer[i]);
        }
        
        osState = AudioQueueStart(_audioQueue, NULL);
        if (osState != noErr)
        {
            NSLog(@"AudioQueueStart Error");
        }
        
        
    }
    return self;
}

- (void)lwlPlayWithAudioData:(NSData*)audioData
{
//    [_audioLock lock];
    
    NSMutableData *tempData = [NSMutableData new];
    [tempData appendData:audioData];
    
    NSUInteger len = tempData.length;
    Byte *bytes = (Byte*)malloc(len);
    [tempData getBytes:bytes length: len];
    
    int i = 0;
    while (true)
    {
        if (!_audioBufferIsUsed[i])
        {
            _audioBufferIsUsed[i] = YES;
            break;
        }else
        {
            i++;
            if (i >= LWL_QUEUE_BUFFER_SIZES)
            {
                i = 0;
            }
        }
    }
    
    _audioBuffer[i] -> mAudioDataByteSize =  (unsigned int)len;
    memcpy(_audioBuffer[i] -> mAudioData, bytes, len);
    
    free(bytes);
    AudioQueueEnqueueBuffer(_audioQueue, _audioBuffer[i], 0, NULL);
    
//    [_audioLock unlock];
    return;
}

- (void)lwlStopPlay
{
    if (_audioQueue != nil)
    {
        AudioQueueStop(_audioQueue,true);
    }
    return;
}

- (void)lwlResetPlay
{
    if (_audioQueue != nil)
    {
        AudioQueueReset(_audioQueue);
    }
    return;
}


static void AudioPlayerAQInputCallback(void* inUserData, AudioQueueRef audioQueueRef, AudioQueueBufferRef audioQueueBufferRef)
{
    LWLAudio *player = (__bridge LWLAudio*)inUserData;
    [player resetBufferState:audioQueueRef and:audioQueueBufferRef];
}

- (void)resetBufferState:(AudioQueueRef)audioQueueRef and:(AudioQueueBufferRef)audioQueueBufferRef
{
    
    for (int i = 0; i < LWL_QUEUE_BUFFER_SIZES; i++)
    {
        if (audioQueueBufferRef == _audioBuffer[i])
        {
            _audioBufferIsUsed[i] = NO;
        }
    }
}

+ (void)initialize
{
    NSError *error = nil;

    BOOL ret = [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryMultiRoute error:&error];
    if (!ret)
    {
        NSLog(@"setCategory failed!");
        return;
    }
    
    ret = [[AVAudioSession sharedInstance] setActive:YES error:&error];
    if (!ret)
    {
        NSLog(@"setActive failed");
        return;
    }
}

@end
