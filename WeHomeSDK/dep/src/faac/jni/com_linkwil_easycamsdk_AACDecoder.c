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

#include <jni.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <faac.h>

static faacEncHandle sFaacEncoder;
static unsigned char *sFaacBuf = NULL;
static unsigned long sMaxOutputSamples = 0;

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {

    return JNI_VERSION_1_6;
}

JNIEXPORT jint JNICALL Java_com_linkwil_easycamsdk_AACDecoder_init
  (JNIEnv * env, jobject thiz, jint sample_rate, jint channels)
{
	int pcmBufSize = 0;
	unsigned long inputSamples = 0;
	
	if( sFaacEncoder != NULL )
	{
		return pcmBufSize;
	}

	sFaacEncoder = faacEncOpen(sample_rate, channels, &inputSamples, &sMaxOutputSamples);
	
	pcmBufSize = inputSamples * 2;
	sFaacBuf = (unsigned char* )malloc(sMaxOutputSamples);
	
	faacEncConfigurationPtr pConfiguration; 
	pConfiguration = faacEncGetCurrentConfiguration(sFaacEncoder);
	pConfiguration->inputFormat = FAAC_INPUT_16BIT;
	pConfiguration->outputFormat = 1;
	pConfiguration->useTns= 1;
	pConfiguration->useLfe= 0;
	pConfiguration->aacObjectType= LOW;
	pConfiguration->shortctl= SHORTCTL_NORMAL;
	pConfiguration->quantqual= 100;
	pConfiguration->bandWidth= 0;
	pConfiguration->bitRate= 0;
	faacEncSetConfiguration(sFaacEncoder, pConfiguration);
	
	return pcmBufSize;
}

JNIEXPORT jint JNICALL Java_com_linkwil_easycamsdk_AACDecoder_deinit
  (JNIEnv * env, jobject thiz)
{
	if (sFaacEncoder!=NULL)
	{
		faacEncClose(sFaacEncoder);
		sFaacEncoder=NULL;
	}
	
	if( sFaacBuf != NULL )
	{
		free(sFaacBuf);
		sFaacBuf = NULL;
	}
	
	return 0;
}

JNIEXPORT jint JNICALL Java_com_linkwil_easycamsdk_AACDecoder_encode
  (JNIEnv * env, jobject thiz, jbyteArray inBuffer, jint bufSize, jbyteArray outBuf)
{
	int decodedSize = 0;
	
	jbyte* inBufArrayBody = (*env)->GetByteArrayElements( env, inBuffer, 0);
    
	decodedSize = faacEncEncode(sFaacEncoder, (int*)inBufArrayBody, bufSize/2, sFaacBuf, sMaxOutputSamples);

    (*env)->ReleaseByteArrayElements( env, inBuffer, inBufArrayBody, 0 );
	
	if( decodedSize > 0 )
	{
		(*env)->SetByteArrayRegion( env, outBuf, 0, decodedSize, (jbyte *)sFaacBuf );
	}
	
	return decodedSize;
}

// EOF



