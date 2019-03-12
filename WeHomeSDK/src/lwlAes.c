//
//  lwlAes.c
//  rsaDemo
//
//  Created by linkwil on 2018/12/19.
//  Copyright © 2018年 xlink. All rights reserved.
//

#include "lwlAes.h"
#include "ECLog.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <openssl/aes.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include "base64.h"



#ifdef _WIN32
#include <time.h>  
#include <windows.h>
#else
#include <sys/time.h>
#endif

#ifdef WIN32
int gettimeofday(struct timeval *tp, void *tzp)
{
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	tp->tv_sec = clock;
	tp->tv_usec = wtm.wMilliseconds * 1000;
	return (0);
}
#endif

#define AES_KEY_LEN 16

int lwlRand(int min, int max)
{
    struct timeval time_now = {0};
    long time_mic = 0;
    gettimeofday(&time_now,NULL);
    time_mic = time_now.tv_sec*1000*1000 + time_now.tv_usec;
    
    clock_t time_clock;
    time_clock = clock();
    
    srand((unsigned)time_mic + (unsigned int)time_clock);
    return rand() % (max - min) + min;
}

int lwlaes_genRandomAesKey(unsigned char* aesKey)
{
    if (!aesKey)
    {
        LOGE("lwlaes_genRandomAesKey, aesKey is NULL, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    char keyMetaChar[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    for (int i = 0; i < AES_KEY_LEN; i++)
    {
        int randIndex = lwlRand(0, strlen(keyMetaChar));
        aesKey[i] = (unsigned char)keyMetaChar[randIndex];
    }
    aesKey[AES_KEY_LEN] = '\0';
    
    return LWLAES_RETURN_SUCCESS;
}


int lwlaes_encrypt(unsigned char *aesKey, unsigned char *srcStr, int srcStrLen, unsigned char *encryptedStrBuf, int *encryptedStrBufLen)
{
    if (!aesKey)
    {
        LOGE("lwlaes_encrypt, aesKey is NULL, return fail");
        return LWLAES_RETURN_FAIL;
    }

    if (strlen((const char*)aesKey) <= 0)
    {
        LOGE("lwlaes_encrypt, aesKey len error, return fail");
        return LWLAES_RETURN_FAIL;
    }

    if (!srcStr)
    {
        LOGE("lwlaes_encrypt, beforeEncryptStr is NULL, return fail");
        return LWLAES_RETURN_FAIL;
    }

    if (srcStrLen <= 0)
    {
        LOGE("lwlaes_encrypt, beforeStrLen error, return fail");
        return LWLAES_RETURN_FAIL;
    }

    if (encryptedStrBuf==NULL)
    {
        LOGE("lwlaes_encrypt, afterEncryptStr is NULL, return fail");
        return LWLAES_RETURN_FAIL;
    }

    int base64Len = srcStrLen * 1.5;
    if (*encryptedStrBufLen < base64Len)
    {
        LOGE("lwlaes_encrypt, afterStrBufLen error, return fail");
        return LWLAES_RETURN_FAIL;
    }

    char *noBase64AesEncStr = (char*)malloc((size_t)base64Len);
    if (!noBase64AesEncStr)
    {
        LOGE("malloc error, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    memset(noBase64AesEncStr, 0, base64Len);

    int ret = 0;
    AES_KEY sslAesEncKey;
    ret = AES_set_encrypt_key(aesKey, 128, &sslAesEncKey);
    if (ret < 0)
    {
        LOGE("lwlaes_encrypt, AES_set_encrypt_key error, ret:%d, return fail", ret);
        free(noBase64AesEncStr);
        return LWLAES_RETURN_FAIL;
    }


    char ivec[16] = {0};
    AES_cbc_encrypt(srcStr, (unsigned char*)noBase64AesEncStr, (size_t)srcStrLen, &sslAesEncKey, (unsigned char *)ivec, AES_ENCRYPT);
    
    base64_encode((const unsigned char *)noBase64AesEncStr, srcStrLen, (unsigned char*)encryptedStrBuf, &base64Len);
    *encryptedStrBufLen = base64Len;

    free(noBase64AesEncStr);

    return LWLAES_RETURN_SUCCESS;
}

int lwlaes_decrypt(unsigned char *aesKey, unsigned char *srcStr, int srcStrLen, unsigned char *decryptedStrBuf, int *decryptedStrBufLen)
{
    if (!aesKey)
    {
        LOGE("lwlaes_decrypt, aesKey is NULL, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    if (strlen((const char*)aesKey) <= 0)
    {
        LOGE("lwlaes_decrypt, aesKey len error, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    
    if (!srcStr)
    {
        LOGE("lwlaes_decrypt, srcStr is NULL, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    if (srcStrLen <= 0)
    {
        LOGE("lwlaes_decrypt, srcStrLen error, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    if (!decryptedStrBuf)
    {
        LOGE("lwlaes_decrypt, decryptedStrBuf is NULL, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    if (*decryptedStrBufLen <= 0)
    {
        LOGE("lwlaes_decrypt, decryptedStrBufLen error, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    int ret = 0;
    AES_KEY sslAesDecKey;
    ret = AES_set_decrypt_key(aesKey, 128, &sslAesDecKey);
    if (ret < 0)
    {
        LOGE("lwlaes_decrypt, AES_set_decrypt_key error, ret:%d, return fail", ret);
        return LWLAES_RETURN_FAIL;
    }
    
    int noBase64Len = srcStrLen;
    char *noBase64Str = (char*)malloc(noBase64Len);
    if (!noBase64Str)
    {
        LOGE("malloc error, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    memset(noBase64Str, 0, noBase64Len);

    base64_decode((const unsigned char *)srcStr, srcStrLen, (unsigned char *)noBase64Str, &noBase64Len);
    
    char ivec[16] = {0};
    AES_cbc_encrypt((unsigned char*)noBase64Str, decryptedStrBuf, noBase64Len, &sslAesDecKey, (unsigned char *)ivec, AES_DECRYPT);

    free(noBase64Str);
    
    return LWLAES_RETURN_SUCCESS;
}

//out str no base64
int lwlaes_encrypt_nobase64(unsigned char *aesKey, unsigned char *srcData, int srcDataLen, unsigned char *encryptedBuf, int *encryptedBufLen)
{

    if (!aesKey)
    {
        LOGE("lwlaes_encrypt, aesKey is NULL, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    if (strlen((const char*)aesKey) <= 0)
    {
        LOGE("lwlaes_encrypt, aesKey len error, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    
    if (!srcData)
    {
        LOGE("lwlaes_encrypt, srcData is NULL, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    if (srcDataLen <= 0)
    {
        LOGE("lwlaes_encrypt, srcDataLen error, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    if (!encryptedBuf)
    {
        LOGE("lwlaes_encrypt, encryptedBuf is NULL, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    if (*encryptedBufLen < srcDataLen)
    {
        LOGE("lwlaes_encrypt, encryptedBufLen error, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    int ret = 0;
    AES_KEY sslAesEncKey;
    ret = AES_set_encrypt_key(aesKey, 128, &sslAesEncKey);
    if (ret < 0)
    {
        LOGE("lwlaes_encrypt, AES_set_encrypt_key error, ret:%d, return fail", ret);
        return LWLAES_RETURN_FAIL;
    }
    
    char ivec[16] = {0};
    AES_cbc_encrypt(srcData, encryptedBuf, (size_t)srcDataLen, &sslAesEncKey, (unsigned char *)ivec, AES_ENCRYPT);
    
    *encryptedBufLen = srcDataLen;
    
    return LWLAES_RETURN_SUCCESS;
}

//in str no base64
int lwlaes_decrypt_nobase64(unsigned char *aesKey, unsigned char *srcData, int srcDataLen, unsigned char *decryptedBuf, int *decryptedBufLen)
{
    if (!aesKey)
    {
        LOGE("lwlaes_decrypt, aesKey is NULL, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    if (strlen((const char*)aesKey) <= 0)
    {
        LOGE("lwlaes_decrypt, aesKey len error, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    
    if (!srcData)
    {
        LOGE("lwlaes_decrypt, srcData is NULL, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    if (srcDataLen <= 0)
    {
        LOGE("lwlaes_decrypt, srcDataLen error, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    if (!decryptedBuf)
    {
        LOGE("lwlaes_decrypt, decryptedBuf is NULL, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    if (*decryptedBufLen <= 0)
    {
        LOGE("lwlaes_decrypt, decryptedBufLen error, return fail");
        return LWLAES_RETURN_FAIL;
    }
    
    int ret = 0;
    AES_KEY sslAesDecKey;
    ret = AES_set_decrypt_key(aesKey, 128, &sslAesDecKey);
    if (ret < 0)
    {
        LOGE("lwlaes_decrypt, AES_set_decrypt_key error, ret:%d, return fail", ret);
        return LWLAES_RETURN_FAIL;
    }
    
    char ivec[16] = {0};
    AES_cbc_encrypt(srcData, decryptedBuf, (size_t)srcDataLen, &sslAesDecKey, (unsigned char *)ivec, AES_DECRYPT);
    
    *decryptedBufLen = srcDataLen;
    
    return LWLAES_RETURN_SUCCESS;
}
