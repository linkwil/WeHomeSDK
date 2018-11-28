//
//  LinkWiFiConfig.h
//  LinkWiFiConfig
//
//  Created by LinkWil on 2018/11/17.
//  Copyright © 2018年 xlink. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

typedef enum TAG_LINK_CONFIG_NETWORK_RETURN
{
    LINK_WIFI_CONFIG_RETURN_FALES = -1,
    LINK_WIFI_CONFIG_RETURN_OK = 0,
}LINK_WIFI_CONFIG_RETURN;

@interface LinkWiFiConfig : NSObject

+(NSString *)getVersion;

+(int)startSmartConfigWithWifiSsid:(NSString *)wifiSsid wifiPassword:(NSString *)wifiPassword devPassword:(NSString *)devPassword;
+(int)stopSmartConfig;
@end
