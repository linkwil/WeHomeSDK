/*
 * WeHome SDK
 * Copyright (c) Shenzhen Linkwil Intelligent Technology Co. Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef EASYCAMMSG_H_
#define EASYCAMMSG_H_

typedef struct tagEasyCamMsgHead
{
	char magic[8];
	unsigned int msgId;
	unsigned int seq;
	unsigned int dataLen;
	char data[0];
}EasyCamMsgHead;

enum EC_MSG_ID
{
	EC_MSG_ID_AV_DATA,
	EC_MSG_ID_TALK_DATA,
	EC_MSG_ID_CMD,
	EC_MSG_ID_CMD_RESP,
	EC_MSG_ID_PB_DATA, // Playback data
	EC_MSG_ID_FILE_DOWNLOAD_DATA, // File download data
	EC_MSG_ID_PIR_DATA,
};



enum EC_CMD_ID
{
    //login
    EC_CMD_ID_LOGIN = 0x00,
    EC_CMD_ID_LOGOUT,
    
    // talk
    EC_CMD_ID_OPEN_TALK = 0x100,
    EC_CMD_ID_CLOSE_TALK,
    
    //cmd
    EC_CMD_ID_GET_DEV_INFO = 0x200,    //获取设备信息，包括：设备固件版本；设备型号；设备电源频率；设备电量；设备时区；设备语言；
    EC_CMD_ID_SET_DEV_INFO,            //设置设备信息，包括：设备电源频率；设备时区；设备语言；
    EC_CMD_ID_CHANGE_PASSWORD,        //修改密码
    EC_CMD_ID_VIDEO_MIRROR,            //镜像
    EC_CMD_ID_VIDEO_FLIP,            //翻转
    EC_CMD_ID_GET_PIR_INFO,            //获取PIR的信息，包括：PIR的开关；PIR的灵敏度；PIR的生效时间；
    EC_CMD_ID_SET_PIR_INFO,            //设置PIR的信息，包括：PIR的开关；PIR的灵敏度；PIR的生效时间；
    EC_CMD_ID_RESTORE_TO_FACTORY,   //恢复出厂设置
    EC_CMD_ID_FORMAT_SD_CARD,         //格式化SD卡
    EC_CMD_ID_HEARTBEAT,             //APP的心跳信息
    EC_CMD_ID_GET_LED_FILL_LIGHT_SETTING, //获取补光灯设置
    EC_CMD_ID_SET_LED_FILL_LIGHT_SETTING, //设置补光灯设置
    EC_CMD_ID_OPEN_LOCK, // 开锁命令
    
    
    //play record
    EC_CMD_ID_GET_RECORD_BITMAP_BY_MON = 0x300, // 根据月份获取哪些天有录像
    EC_CMD_ID_GET_PLAY_RECORD_LIST,
    EC_CMD_ID_START_PLAY_EVENT_RECORD, //根据时间和消息ID回放事件录像
    EC_CMD_ID_START_PLAY_RECORD,
    EC_CMD_ID_STOP_PLAY_RECORD,
    EC_CMD_ID_CMD_PLAY_RECORD,
    
    EC_CMD_ID_UPGRADE = 0x400,                //升级

    // Debug
    EC_CMD_ID_SET_P2P_INFO = 0x600,
    EC_CMD_ID_SET_TIME,
};


enum RECORD_RETURN
{
    RECORD_RETURN_OK = 0,
    RECORD_RETURN_FALSE = -1,
};


#endif /* EASYCAMMSG_H_ */
