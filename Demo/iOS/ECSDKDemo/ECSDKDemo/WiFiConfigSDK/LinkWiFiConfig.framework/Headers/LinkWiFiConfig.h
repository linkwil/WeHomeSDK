//
//  LinkWiFiConfig.h
//  LinkWiFiConfig
//
//  Created by linkwil on 2018/5/17.
//  Copyright © 2018年 linkwil. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef enum TAG_LINK_WIFI_CONFIG_RETURN
{
    LINK_WIFI_CONFIG_RETURN_FALES = -1,
    LINK_WIFI_CONFIG_RETURN_OK = 0,
}LINK_WIFI_CONFIG_RETURN;

@interface LinkWiFiConfig : NSObject

+(NSString *)getVersion;

+(int)startSmartConfig:(NSString *)wifiSsid password:(NSString *)wifiPassword devPassword:(NSString *)devPassword;
+(int)stopSmartConfig;

@end
