//
//  LWLAudioCapture.h
//  LWLAudioCaptureDemo
//
//  Created by linkwil on 2018/9/17.
//  Copyright © 2018年 xlink. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@interface LWLAudioCapture : NSObject

- (BOOL)LWLAC_SetFormat:(AudioStreamBasicDescription*)format callback:(AudioQueueInputCallback)callback callbackSelf:(id)callbackSelf;
- (BOOL)LWLAC_StartCap;
- (BOOL)LWLAC_StopCap;

@end
