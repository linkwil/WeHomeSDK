//
//  wifiConfigViewController.m
//  ECSDKDemo
//
//  Created by linkwil on 2018/7/23.
//  Copyright © 2018年 xlink. All rights reserved.
//

#import "wifiConfigViewController.h"
#import <LinkWiFiConfig/LinkWiFiConfig.h>
@interface wifiConfigViewController ()<UITextFieldDelegate>

@end

@implementation wifiConfigViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    _wifiPswTextField.delegate = self;
    _wifiSsidTextField.delegate = self;
    _devPswTextField.delegate = self;
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)viewDidDisappear:(BOOL)animated
{
    [LinkWiFiConfig stopSmartConfig];
    [super viewDidDisappear:animated];
}

- (IBAction)startWiFiConfigFunc:(id)sender
{
    
    int startRet = [LinkWiFiConfig startSmartConfigWithWifiSsid:_wifiSsidTextField.text wifiPassword:_wifiPswTextField.text devPassword:_devPswTextField.text];
    NSLog(@"startRet:%d", startRet);
    if (startRet == LINK_WIFI_CONFIG_RETURN_FALES)
    {
        [self LWLAlertWithTitle:@"startSmartConfig fail!" message:@"" defaultActionTitle:@"Ok" defaultActFunc:^(UIAlertAction *action) {
            //
        } cancelActionTitle:nil cancelActFunc:^(UIAlertAction *action) {
            //
        }];
    }
}

- (IBAction)stopWiFiConfigFunc:(id)sender
{
    [LinkWiFiConfig stopSmartConfig];
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
