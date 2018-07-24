//
//  sendCmdViewController.h
//  ECSDKDemo
//
//  Created by linkwil on 2018/7/23.
//  Copyright © 2018年 xlink. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface sendCmdViewController : UIViewController
@property (weak, nonatomic) IBOutlet UITextField *cmdTextField;
@property (weak, nonatomic) IBOutlet UILabel *cmdResultLabel;

- (void)setHandle:(int)handle seq:(int)seq;
- (void)setCmdResult:(NSString*)cmdResult;

@end
