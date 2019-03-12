#ifndef BASE64_H
#define BASE64_H

#ifdef __cplusplus
extern "C"
{
#endif /* End of #ifdef __cplusplus */
    
    int base64_encode(const unsigned char *indata, int inlen, unsigned char *outdata, int *outlen);
    int base64_decode(const unsigned char *indata, int inlen, unsigned char *outdata, int *outlen);
    
#ifdef __cplusplus
}
#endif /* End of #ifdef __cplusplus */


#endif /* BASE64_H */
