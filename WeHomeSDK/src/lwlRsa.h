//
//  lwlRsa.h
//  rsaDemo
//
//  Created by linkwil on 2018/12/19.
//  Copyright © 2018年 xlink. All rights reserved.
//

#ifndef lwlRsa_h
#define lwlRsa_h

#include <stdio.h>



#ifdef __cplusplus
extern "C"
{
#endif /* End of #ifdef __cplusplus */
    
    typedef enum TAG_LWLRSA_RETURN
    {
        LWLRSA_RETURN_SUCCESS,
        LWLRSA_RETURN_FAIL,
    }LWLRSA_RETURN;

    //out str had base64
    int lwlrsa_encrypt(unsigned char *srcBuf, int srcLen, unsigned char *encryptedBuf, int *encryptedBufLen);
    
#ifdef __cplusplus
}
#endif /* End of #ifdef __cplusplus */




#endif /* lwlRsa_h */
