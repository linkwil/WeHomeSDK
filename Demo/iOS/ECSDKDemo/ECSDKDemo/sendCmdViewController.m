//
//  sendCmdViewController.m
//  ECSDKDemo
//
//  Created by linkwil on 2018/7/23.
//  Copyright © 2018年 xlink. All rights reserved.
//

#import "sendCmdViewController.h"
#import <ECSDK/EasyCamSdk.h>

#import "cJSON.h"

@interface sendCmdViewController ()<UITextFieldDelegate>
{
    int _handle;
    int _seq;
}

@end

@implementation sendCmdViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    _cmdTextField.delegate = self;
    [_cmdResultLabel setNumberOfLines:0];
    
    cJSON * pJsonRoot = NULL;
    char* jsonStr = NULL;
    pJsonRoot = cJSON_CreateObject();
    cJSON_AddNumberToObject(pJsonRoot, "cmdId", EC_SDK_CMD_ID_GET_DEV_INFO);
    jsonStr = cJSON_Print(pJsonRoot);
    cJSON_Minify(jsonStr);
    
    NSString *cmdStr = [NSString stringWithFormat:@"%s", jsonStr];
    [_cmdTextField setText:cmdStr];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)setHandle:(int)handle seq:(int)seq
{
    _handle = handle;
    _seq = seq;
}


- (IBAction)sendCmdBtnFunc:(id)sender
{

    NSString *cmdStr = _cmdTextField.text;
    
    [_cmdResultLabel setText:@"cmd result:"];
    
    EC_SendCommand(_handle, (char*)[cmdStr UTF8String], _seq);
    
    [self LWLAlertWithTitle:@"send cmd success." message:@"" defaultActionTitle:@"OK" defaultActFunc:^(UIAlertAction *action) {
        //
    } cancelActionTitle:nil cancelActFunc:^(UIAlertAction *action) {
        //
    }];
    
    return;
}

- (void)setCmdResult:(NSString*)cmdResult
{
    [_cmdResultLabel setText:cmdResult];
    return;
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
