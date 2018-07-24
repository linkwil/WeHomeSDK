//
//  wifiConfigViewController.h
//  ECSDKDemo
//
//  Created by linkwil on 2018/7/23.
//  Copyright © 2018年 xlink. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface wifiConfigViewController : UIViewController
@property (weak, nonatomic) IBOutlet UITextField *wifiSsidTextField;
@property (weak, nonatomic) IBOutlet UITextField *wifiPswTextField;
@property (weak, nonatomic) IBOutlet UITextField *devPswTextField;

@end
