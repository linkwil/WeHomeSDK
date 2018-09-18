//
//  ViewController.h
//  ECSDKDemo
//
//  Created by linkwil on 2018/5/14.
//  Copyright © 2018年 linkwil. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface ViewController : UIViewController<UITextFieldDelegate>
@property (weak, nonatomic) IBOutlet UITextField *uidTextField;
@property (weak, nonatomic) IBOutlet UITextField *usrNameTextField;
@property (weak, nonatomic) IBOutlet UITextField *passwordTextField;
@property (weak, nonatomic) IBOutlet UIButton *loginBtn;
@property (weak, nonatomic) IBOutlet UIButton *logoutBtn;
@property (weak, nonatomic) IBOutlet UILabel *cmdResultLabel;
@property (weak, nonatomic) IBOutlet UIButton *cmdBtn;
@property (weak, nonatomic) IBOutlet UIImageView *videoShowImageView;
@property (weak, nonatomic) IBOutlet UIButton *openTalkBtn;
@property (weak, nonatomic) IBOutlet UIButton *closeTalkBtn;


-(void)LWLAlertWithTitle:(NSString *)title message:(NSString *)msg defaultActionTitle:(NSString*)defaultActTitle defaultActFunc:(void (^) (UIAlertAction *action))defaultFunc cancelActionTitle:(NSString*)cancelActTitle cancelActFunc:(void (^) (UIAlertAction *action))cancelFunc;
- (void)setCmdResult:(NSString *)cmdResult;
- (void)playAudio:(char*)data dataLen:(int)dataLen;
- (void)displayVideoWithImage:(UIImage*)image height:(int)height width:(int)width;

@end

