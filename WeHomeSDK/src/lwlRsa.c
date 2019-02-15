//
//  lwlRsa.c
//  rsaDemo
//
//  Created by linkwil on 2018/12/19.
//  Copyright © 2018年 xlink. All rights reserved.
//

#include "lwlRsa.h"
#include "ECLog.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include "base64.h"

char publicKey[] =
"-----BEGIN PUBLIC KEY-----\n"\
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDNGUQNVHmwhZyzZ6F6cgO3KaJq\n"\
"iMILhFRTKB1ViCv+2J7/BEoG/PBf02AOMsD3zTmud+cSGMzezGR5WOeWQAtpQYhS\n"\
"moMxph/edp050OFtlzB2LsJQJldH4zEUOWyPBotVlaVecVPeXCyP0hc3+bgVloFw\n"\
"PFCM5PziOoWfyK0pzwIDAQAB\n"\
"-----END PUBLIC KEY-----\n";


//读取密钥
RSA* createRSA(unsigned char*key, int public)
{
    RSA *rsa= NULL;
    BIO* keybio ;
    keybio = BIO_new_mem_buf(key, -1);
    
    if(keybio==NULL)
    {
        LOGE("Failed to create key BIO");
        return 0;
    }
    if(public)
    {
        rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);
    }
    else
    {
        rsa= PEM_read_bio_RSAPrivateKey(keybio, &rsa,NULL, NULL);
    }
    if(rsa== NULL)
    {
        LOGE("Failed to create RSA");
    }
    return rsa;
}



int lwlrsa_encrypt(unsigned char *srcBuf, int srcLen, unsigned char *encryptedBuf, int *encryptedBufLen)
{
    if (!srcBuf)
    {
        LOGE("lwlrsa_encrypt, srcBuf is NULL, return fail");
        return LWLRSA_RETURN_FAIL;
    }
    
    if (srcLen <= 0)
    {
        LOGE("lwlrsa_encrypt, srcLen error, return fail");
        return LWLRSA_RETURN_FAIL;
    }
    
    if (!encryptedBuf)
    {
        LOGE("lwlrsa_encrypt, encryptedBuf is NULL, return fail");
        return LWLRSA_RETURN_FAIL;
    }

    RSA *pEnRsa = NULL;
    pEnRsa = createRSA(( unsigned char*)publicKey, 1);
    if( pEnRsa )
    {
        int rsaSize = RSA_size(pEnRsa);

        // RSA加密元数据长度要小于RSA_SIZE-11  https://blog.cnbluebox.com/blog/2014/03/19/rsajia-mi/
        if ( (srcLen+11) > rsaSize)
        {
            LOGE("lwlrsa_encrypt, srcLen error, return fail");
            RSA_free(pEnRsa);
            return LWLRSA_RETURN_FAIL;
        }

        int base64Len = BASE64_ENCODE_OUT_SIZE(rsaSize);
        if( *encryptedBufLen < base64Len )
        {
            LOGE("lwlrsa_encrypt, Encrypted buf size not enough");
            RSA_free(pEnRsa);
            return LWLRSA_RETURN_FAIL;
        }
        char *noBase64RsaEncStr = (char*)malloc(*encryptedBufLen);
        if (!noBase64RsaEncStr)
        {
            LOGE("lwlrsa_encrypt, malloc error, return fail");
            RSA_free(pEnRsa);
            return LWLRSA_RETURN_FAIL;
        }

        int result = RSA_public_encrypt(srcLen, srcBuf, (unsigned char*)noBase64RsaEncStr, pEnRsa, RSA_PKCS1_PADDING);
        if (result < 0)
        {
            LOGE("RSA_public_encrypt error, result:%d, return fail", result);
            RSA_free(pEnRsa);
            free(noBase64RsaEncStr);
            return LWLRSA_RETURN_FAIL;
        }

        base64_encode((const unsigned char*)noBase64RsaEncStr, (unsigned int)result, (char*)encryptedBuf);
        *encryptedBufLen = base64Len;
        RSA_free(pEnRsa);
        free(noBase64RsaEncStr);
    }

    return LWLRSA_RETURN_SUCCESS;
}

