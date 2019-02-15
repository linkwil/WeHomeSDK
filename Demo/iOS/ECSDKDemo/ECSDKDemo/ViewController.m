//
//  ViewController.m
//  ECSDKDemo
//
//  Created by linkwil on 2018/5/14.
//  Copyright © 2018年 linkwil. All rights reserved.
//

#import "ViewController.h"
#import <ECSDK/EasyCamSdk.h>
#import "sendCmdViewController.h"
#import <ifaddrs.h>
#import <arpa/inet.h>
#import "cJSON.h"
#import "LWLAudio.h"
#import "LWLCodecSDK.h"
#import "NSMutableArray+QueueStack.h"
#import "LWLAudioCapture.h"

typedef struct tagFrameHeadInfo
{
    unsigned long long seqNo;
    long long timeStamp;                // in ms
    unsigned int videoLen;
    unsigned int audioLen;
    unsigned int isKeyFrame;
    unsigned int videoWidth;
    unsigned int videoHeight;
    unsigned int audio_format;
    unsigned int frame_type;            //0:video;1:audio;2:start;3:seek;4:end
    unsigned int wifiQuality;
    int pbSessionNo;
    unsigned char encrypted; // 是否支持加密
    unsigned char reserved[51];
    char data[0];
}__attribute__((packed))  FrameHeadInfo;


static ViewController *gVC = nil;
static int gECSDKHandle = 0;
static int gSeq = 0;
static unsigned char *videoOutputData = nil;

NSMutableArray *videoDataQueue = nil;
NSMutableArray *audioDataQueue = nil;

char liveVideoDataTempBuf[2*1024*1024] = {0};
char liveAudioDataTempBuf[1*1024*1024] = {0};

void lpLoginResult(int handle, int errorCode, int seq, unsigned int notificationToken, unsigned int isCharging, unsigned int batPercent)
{
    NSLog(@"lpLoginResult enter");
    NSLog(@"lpLoginResult, handle:%d, errorCode:%d, seq:%d, notificationToken:%d, isCharging:%d, batPercent:%d", handle, errorCode, seq, notificationToken, isCharging, batPercent);
    
    if (gECSDKHandle == handle)
    {
        
        dispatch_async(dispatch_get_main_queue(), ^
                       {
                           
                           [gVC.loginBtn setEnabled:YES];
                           [gVC.loginBtn setTitle:@"Login" forState:UIControlStateNormal];
                           
                           if (errorCode >= 0)
                           {
                               [gVC.loginBtn setEnabled:NO];
                               [gVC.logoutBtn setEnabled:YES];
                               [gVC.cmdBtn setEnabled:YES];
                               [gVC.openTalkBtn setEnabled:YES];
                               [gVC.closeTalkBtn setEnabled:YES];
                               
                               [gVC LWLAlertWithTitle:@"login success." message:@"" defaultActionTitle:@"OK" defaultActFunc:^(UIAlertAction *action) {
                                   //
                               } cancelActionTitle:nil cancelActFunc:^(UIAlertAction *action) {
                                   //
                               }];
                               
                               
                           }else
                           {
                               EC_Logout(gECSDKHandle); //must logout
                               
                               [gVC LWLAlertWithTitle:@"login false!" message:@"" defaultActionTitle:@"OK" defaultActFunc:^(UIAlertAction *action) {
                                   //
                               } cancelActionTitle:nil cancelActFunc:^(UIAlertAction *action) {
                                   //
                               }];
                           }
                       });
        
       
    }

}

void lpCmdResult(int handle, char* data, int errorCode, int seq)
{
    NSLog(@"lpCmdResult enter");
    NSLog(@"lpCmdResult, handle:%d, errorCode:%d, seq:%d", handle, errorCode, seq);
    NSLog(@"lpCmdResult, data:%s", data);
    
    if (gECSDKHandle == handle)
    {
        dispatch_async(dispatch_get_main_queue(), ^
                       {
                           NSString *cmdResultStr = [NSString stringWithFormat:@"cmd result:\n\n%s", data];
                           [gVC setCmdResult:cmdResultStr];
                           
                       });
    }

}

void lpAudio_RecvData(int handle, char *data, int len, int payloadType, long long timestamp, int seq)
{
//    NSLog(@"lpAudio_RecvData enter");
//    NSLog(@"lpAudio_RecvData, handle:%d, len:%d, payloadType:%d, timestamp:%lld, seq:%d", handle, len, payloadType, timestamp, seq);
    
    
    FrameHeadInfo *pAudioFrame = (FrameHeadInfo*)liveAudioDataTempBuf;
    memset(pAudioFrame, 0, sizeof(FrameHeadInfo));
    pAudioFrame->audioLen = len;
    pAudioFrame->timeStamp = timestamp;
    pAudioFrame->seqNo = seq;
    pAudioFrame->frame_type = payloadType;
    memcpy(pAudioFrame->data, data, len);
    
    if (audioDataQueue)
    {
        NSData *audioData = [NSData dataWithBytes:liveAudioDataTempBuf length:(len + sizeof(FrameHeadInfo))];
        if (audioData)
        {
            [audioDataQueue queuePush:audioData];
        }
    }
    
    return;
    
    
}

void lpVideo_RecvData(int handle, char *data, int len, int payloadType, long long timestamp, int seq, int frameType, int videoWidth, int videoHeight, unsigned int wifiQuality)
{
    
    //    NSLog(@"lpVideo_RecvData enter");
    //    NSLog(@"lpVideo_RecvData, handle:%d, len:%d, payloadType:%d, timestamp:%lld, seq:%d, frameType:%d", handle, len, payloadType, timestamp, seq, frameType);
    
    FrameHeadInfo *pVideoFrame = (FrameHeadInfo*)liveVideoDataTempBuf;
    memset(pVideoFrame, 0, sizeof(FrameHeadInfo));
    pVideoFrame->videoLen = len;
    pVideoFrame->frame_type = frameType;
    pVideoFrame->timeStamp = timestamp;
    pVideoFrame->seqNo = seq;
    pVideoFrame->videoWidth = videoWidth;
    pVideoFrame->videoHeight = videoHeight;
    pVideoFrame->wifiQuality = wifiQuality;
    memcpy(pVideoFrame->data, data, len);
    
    if (videoDataQueue)
    {
        NSData *videoData = [NSData dataWithBytes:liveVideoDataTempBuf length:(len + sizeof(FrameHeadInfo))];
        if (videoData)
        {
            [videoDataQueue queuePush:videoData];
        }
    }
    return;

}

void lpPBAudio_RecvData(int handle, char *data, int len, int payloadType, long long timestamp, int seq, int pbSessionNo)
{
    
}

void lpPBVideo_RecvData(int handle, char *data, int len, int payloadType, long long timestamp, int seq, int frameType, int videoWidth, int videoHeight, int pbSessionNo)
{
    
}

void lpPBEnd(int handle, int pbSessionNo)
{
    
}

void AudioQueueInputCallbackFunc(
                                 void * __nullable               inUserData,
                                 AudioQueueRef                   inAQ,
                                 AudioQueueBufferRef             inBuffer,
                                 const AudioTimeStamp *          inStartTime,
                                 UInt32                          inNumberPacketDescriptions,
                                 const AudioStreamPacketDescription * __nullable inPacketDescs)
{

//    NSLog(@"AudioQueueInputCallbackFunc enter");
//    NSLog(@"inNumberPacketDescriptions:%d", inNumberPacketDescriptions);
    
    static char temp[1024*1024] = {0};
    static int sumLen = 0;
    
    memcpy(temp+sumLen, (char *)inBuffer->mAudioData, inBuffer->mAudioDataByteSize);
    
    sumLen += inBuffer->mAudioDataByteSize;
    
    while (sumLen >= 640)
    {
        char sendData[1024] = {0};
        memcpy(sendData, temp, 640);
        gSeq++;
        EC_SendTalkData(gECSDKHandle, sendData, 640, 0, gSeq);
        sumLen -= 640;
        memmove(temp, temp+640, sumLen);
    }
    
    AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL);
    return;
}


@interface ViewController ()
{
    sendCmdViewController *_sendCmdVC;
    LWLAudio *_audioPlayer;
    LWLAudioCapture *_audioCap;

}

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    
    _uidTextField.delegate = self;
    _usrNameTextField.delegate = self;
    _passwordTextField.delegate = self;
    _cmdResultLabel.numberOfLines = 0;
    
    [_uidTextField setText:@""];
    [_usrNameTextField setText:@""];
    [_passwordTextField setText:@""];
    
#pragma mark - init ECSDK
    EC_INIT_INFO sdkInitInfo;
    sdkInitInfo.lpLoginResult = lpLoginResult;
    sdkInitInfo.lpCmdResult = lpCmdResult;
    sdkInitInfo.lpVideo_RecvData = lpVideo_RecvData;
    sdkInitInfo.lpAudio_RecvData = lpAudio_RecvData;
    sdkInitInfo.lpPBVideo_RecvData = lpPBVideo_RecvData;
    sdkInitInfo.lpPBAudio_RecvData = lpPBAudio_RecvData;
    sdkInitInfo.lpPBEnd = lpPBEnd;
    EC_Initialize(&sdkInitInfo);
    
    gVC = self;
    
    
    [_logoutBtn setEnabled:NO];
    [_cmdBtn setEnabled:NO];
    [_openTalkBtn setEnabled:NO];
    [_closeTalkBtn setEnabled:NO];
    
    _audioPlayer = [[LWLAudio alloc] init];
    LWLH264DecoderInit(COLOR_FORMAT_RGB24);
    int h264DecoderVersion = LWLH264DecoderVersion();
    NSLog(@"h264DecoderVersion:%d", h264DecoderVersion);
    
    videoDataQueue = [NSMutableArray array];
    audioDataQueue = [NSMutableArray array];
    [self createAVThread];
    
}

- (void)createAVThread
{
    
#pragma mark - createAVThread
    BOOL videoThreadRun = YES;
    BOOL audioThreadRun = YES;
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^
    {
        
        char *data = nil;
        data = (char*)malloc(2*1024*1024);
        
        while (videoThreadRun)
        {
            NSData *videoNSData = [videoDataQueue queuePop];
            if (!videoNSData || 0 == [videoNSData length])
            {
                usleep(5*1000);    //5ms
                continue;
            }
            unsigned long len = [videoNSData length];
            memcpy(data, [videoNSData bytes], len);
            FrameHeadInfo *pVideoFrame =(FrameHeadInfo*)data;
            
            int outDataLen = 0;
            int decodeRet = LWLH264DecoderDecodeFrame((unsigned char*)pVideoFrame->data, pVideoFrame->videoLen, &videoOutputData, &outDataLen);
            
            if ( decodeRet <= 0 )
            {
                NSLog(@"decodeRet <= 0, return");
                continue;
            }
            
            int bpp = 3;
            NSData *imageData = [NSData dataWithBytes:videoOutputData length:outDataLen];
            CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault;
            CGDataProviderRef provider = CGDataProviderCreateWithCFData((__bridge CFDataRef)(imageData));
            CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
            CGImageRef cgImage = CGImageCreate(pVideoFrame->videoWidth,
                                               pVideoFrame->videoHeight,
                                               8,
                                               bpp*8,
                                               bpp*(pVideoFrame->videoWidth),
                                               colorSpace,
                                               bitmapInfo,
                                               provider,
                                               NULL,
                                               NO,
                                               kCGRenderingIntentDefault); //kCGRenderingIntentDefault
            CGColorSpaceRelease(colorSpace);
            UIImage *image = [UIImage imageWithCGImage:cgImage];//[[UIImage alloc] initWithCGImage:cgImage];
            CGImageRelease(cgImage);
            CGDataProviderRelease(provider);
            [gVC displayVideoWithImage:image height:pVideoFrame->videoHeight width:pVideoFrame->videoWidth];

        }
        
        if (data)
        {
            free(data);
            data = NULL;
        }
    });
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^
    {
        
        char *data = nil;
        data = (char*)malloc(1*1024*1024);
        
        while (audioThreadRun)
        {
            
            NSData *audioNSData = [audioDataQueue queuePop];
            
            if (!audioNSData || 0 == [audioNSData length])
            {
                
                usleep(5*1000); //5ms
                continue;
            }
            
            unsigned long len = [audioNSData length];
            memcpy(data, [audioNSData bytes], len);
            FrameHeadInfo *pAudioFrame =(FrameHeadInfo*)data;
            
            [self playAudio:pAudioFrame->data dataLen:pAudioFrame->audioLen];

        }
        
        if (data)
        {
            free(data);
            data = NULL;
        }
    });
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)loginBtnFunc:(id)sender
{
    [_loginBtn setEnabled:NO];
    [_loginBtn setTitle:@"Login..." forState:UIControlStateNormal];
    
    NSString *usrNameStr = @"admin";
    NSString *broadcastAddr = [self getIPAddress];
    
    gSeq++;
    gECSDKHandle = EC_Login([[_uidTextField text] UTF8String], [usrNameStr  UTF8String], [[_passwordTextField text] UTF8String], [broadcastAddr UTF8String] , gSeq, 1, 1, (CONNECT_TYPE_LAN | CONNECT_TYPE_P2P | CONNECT_TYPE_RELAY), 10);
    
    [_audioPlayer lwlStartPlay];
}

- (IBAction)logoutBtnFunc:(id)sender
{
    [self closeTalkBtnFunc:nil];
    
    EC_Logout(gECSDKHandle);
    [_cmdResultLabel setText:@""];
    [gVC LWLAlertWithTitle:@"logout success." message:@"" defaultActionTitle:@"OK" defaultActFunc:^(UIAlertAction *action)
    {
        //
    } cancelActionTitle:nil cancelActFunc:^(UIAlertAction *action)
    {
        //
    }];
    
    [_logoutBtn setEnabled:NO];
    [_cmdBtn setEnabled:NO];
    [_loginBtn setEnabled:YES];
    [_openTalkBtn setEnabled:NO];
    [_closeTalkBtn setEnabled:NO];
    
    [_audioPlayer lwlStopPlay];
    
}
- (IBAction)sendCmdFunc:(id)sender
{
    UIStoryboard *mainSB = [UIStoryboard storyboardWithName:@"Main" bundle:nil];
    _sendCmdVC = [mainSB instantiateViewControllerWithIdentifier:@"sendCmdViewController"];
    gSeq++;
    [_sendCmdVC setHandle:gECSDKHandle seq:gSeq];
    [self.navigationController pushViewController:_sendCmdVC animated:YES];
}
- (IBAction)openTalkBtnFunc:(id)sender
{
    cJSON * pJsonRoot = NULL;
    char* jsonStr = NULL;
    pJsonRoot = cJSON_CreateObject();
    cJSON_AddNumberToObject(pJsonRoot, "cmdId", EC_SDK_CMD_ID_OPEN_TALK);
    jsonStr = cJSON_Print(pJsonRoot);
    cJSON_Minify(jsonStr);
    gSeq++;
    EC_SendCommand(gECSDKHandle, jsonStr, gSeq);
    
    _audioCap = [[LWLAudioCapture alloc]  init];
    [_audioCap LWLAC_SetFormat:nil callback:AudioQueueInputCallbackFunc callbackSelf:self];
    [_audioCap LWLAC_StartCap];

}

- (IBAction)closeTalkBtnFunc:(id)sender
{
    cJSON * pJsonRoot = NULL;
    char* jsonStr = NULL;
    pJsonRoot = cJSON_CreateObject();
    cJSON_AddNumberToObject(pJsonRoot, "cmdId", EC_SDK_CMD_ID_CLOSE_TALK);
    jsonStr = cJSON_Print(pJsonRoot);
    cJSON_Minify(jsonStr);
    gSeq++;
    EC_SendCommand(gECSDKHandle, jsonStr, gSeq);
    
    [_audioCap LWLAC_StopCap];
    
}

- (void)setCmdResult:(NSString *)cmdResult
{
    [_sendCmdVC setCmdResult:cmdResult];
    return;
}

- (void)playAudio:(char*)data dataLen:(int)dataLen
{
    NSData *audioData = [NSData dataWithBytes:data length:dataLen];
    [_audioPlayer lwlPlayWithAudioData:audioData];
    return;
}

- (void)displayVideoWithImage:(UIImage*)image height:(int)height width:(int)width
{
    dispatch_async(dispatch_get_main_queue(), ^
                   {
                       [self->_videoShowImageView setImage:image];
                   });
}

- (NSString *)getIPAddress
{
    NSString *address = @"192.168.1.255";
    
    struct ifaddrs *interfaces = NULL;
    
    struct ifaddrs *temp_addr = NULL;
    
    int success = 0;
    
    success = getifaddrs(&interfaces);
    
    if (success == 0)
    {
        temp_addr = interfaces;
        while(temp_addr != NULL)
        {
            
            if(temp_addr->ifa_addr->sa_family == AF_INET)
                
            {
                // Check if interface is en0 which is the wifi connection on the iPhone
                if([[NSString stringWithUTF8String:temp_addr->ifa_name] isEqualToString:@"en0"])
                {
                    address = [NSString stringWithUTF8String:inet_ntoa(((struct sockaddr_in *)temp_addr->ifa_dstaddr)->sin_addr)];
                }
                
            }
            temp_addr = temp_addr->ifa_next;
        }
    }
    
    // Free memory
    freeifaddrs(interfaces);
    return address;
}


-(void)LWLAlertWithTitle:(NSString *)title message:(NSString *)msg defaultActionTitle:(NSString*)defaultActTitle defaultActFunc:(void (^) (UIAlertAction *action))defaultFunc cancelActionTitle:(NSString*)cancelActTitle cancelActFunc:(void (^) (UIAlertAction *action))cancelFunc
{
    UIAlertController *alertCtrl = nil;
    alertCtrl = [UIAlertController alertControllerWithTitle:title message:msg preferredStyle:UIAlertControllerStyleAlert];
    if (defaultActTitle)
    {
        UIAlertAction *okAction = [UIAlertAction actionWithTitle:defaultActTitle style:UIAlertActionStyleDefault handler:defaultFunc];
        [alertCtrl addAction:okAction];
    }
    if (cancelActTitle)
    {
        UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:cancelActTitle style:UIAlertActionStyleCancel handler:cancelFunc];
        [alertCtrl addAction:cancelAction];
    }
    [self presentViewController:alertCtrl animated:YES completion:^{}];
    
}

-(BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    return YES;
}


@end
