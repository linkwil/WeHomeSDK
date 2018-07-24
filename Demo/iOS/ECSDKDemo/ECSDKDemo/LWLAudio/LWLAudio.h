//
//  LWLAudio.h
//  ECSDKDemo
//
//  Created by linkwil on 2018/7/24.
//  Copyright © 2018 xlink. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface LWLAudio : NSObject

- (id)init;
- (void)lwlPlayWithAudioData:(NSData*)audioData;
- (void)lwlStopPlay;
- (void)lwlResetPlay;

@end
