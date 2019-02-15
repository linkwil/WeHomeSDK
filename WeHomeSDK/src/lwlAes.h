//
//  lwlAes.h
//  rsaDemo
//
//  Created by linkwil on 2018/12/19.
//  Copyright © 2018年 xlink. All rights reserved.
//

#ifndef lwlAes_h
#define lwlAes_h

#include <stdio.h>


#ifdef __cplusplus
extern "C"
{
#endif /* End of #ifdef __cplusplus */
    
#define LEN_ALIGN(a, size)       (((a)+(size)-1) & (~ ((size)-1)))

    typedef enum TAG_LWLAES_RETURN
    {
        LWLAES_RETURN_SUCCESS,
        LWLAES_RETURN_FAIL,
    }LWLAES_RETURN;
    
    //out str no base64
    int lwlaes_genRandomAesKey(unsigned char* aesKey);
    
    //out str had base64
    int lwlaes_encrypt(unsigned char *aesKey, unsigned char *srcStr, int srcStrLen, unsigned char *encryptedStrBuf, int *encryptedStrBufLen);
    
    //in str had base64
    int lwlaes_decrypt(unsigned char *aesKey, unsigned char *srcStr, int srcStrLen, unsigned char *decryptedStrBuf, int *decryptedStrBufLen);
    
    
    //out str no base64
    int lwlaes_encrypt_nobase64(unsigned char *aesKey, unsigned char *srcData, int srcDataLen, unsigned char *encryptedBuf, int *encryptedBufLen);
    
    //in str no base64
    int lwlaes_decrypt_nobase64(unsigned char *aesKey, unsigned char *srcData, int srcDataLen, unsigned char *decryptedBuf, int *decryptedBufLen);
    
#ifdef __cplusplus
}
#endif /* End of #ifdef __cplusplus */


#endif /* lwlAes_h */
