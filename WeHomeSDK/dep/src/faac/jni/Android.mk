
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# source file
LOCAL_MODULE    := faac
LOCAL_SRC_FILES := ./libfaac/aacquant.c \
            ./libfaac/backpred.c \
			./libfaac/bitstream.c \
			./libfaac/channels.c \
			./libfaac/fft.c \
			./libfaac/filtbank.c \
			./libfaac/frame.c \
			./libfaac/huffman.c \
			./libfaac/ltp.c \
			./libfaac/midside.c \
			./libfaac/tns.c \
			./libfaac/util.c \
			./libfaac/psychkni.c \
			./com_linkwil_easycamsdk_AACDecoder.c 、


LOCAL_C_INCLUDES := $(LOCAL_PATH)/src/ \
		    $(LOCAL_PATH)/include \
		    $(LOCAL_PATH)/libfaac
		    

LOCAL_CFLAGS := -Wall -fvisibility=hidden

LOCAL_LDLIBS := -llog


include $(BUILD_SHARED_LIBRARY)


