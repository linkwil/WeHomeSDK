
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# source file
LOCAL_MODULE    := EasyCamSdk
LOCAL_SRC_FILES :=  ./com_linkwil_easycamsdk_EasyCamApi.c \
                    ./../../../src/JNIEnv.cpp \
                    ./../../../src/ECLog.cpp \
                    ./../../../src/cJSON.c \
                    ./../../../src/CEasyCamClient.cpp \
                    ./../../../src/EasyCamSdk.cpp \
                    ./../../../src/YUV2RGB.cpp \
                    ./../../../src/adpcm.c \
                    ./../../../src/Discovery.cpp \
                    ./../../../src/audio_process/analog_agc.c \
                    ./../../../src/audio_process/complex_bit_reverse.c \
                    ./../../../src/audio_process/complex_fft.c \
                    ./../../../src/audio_process/copy_set_operations.c \
                    ./../../../src/audio_process/cross_correlation.c \
                    ./../../../src/audio_process/digital_agc.c \
                    ./../../../src/audio_process/division_operations.c \
                    ./../../../src/audio_process/dot_product_with_scale.c \
                    ./../../../src/audio_process/downsample_fast.c \
                    ./../../../src/audio_process/energy.c \
                    ./../../../src/audio_process/fft4g.c \
                    ./../../../src/audio_process/get_scaling_square.c \
                    ./../../../src/audio_process/min_max_operations.c \
                    ./../../../src/audio_process/noise_suppression.c \
                    ./../../../src/audio_process/noise_suppression_x.c \
                    ./../../../src/audio_process/ns_core.c \
                    ./../../../src/audio_process/nsx_core.c \
                    ./../../../src/audio_process/nsx_core_c.c \
                    ./../../../src/audio_process/nsx_core_neon_offsets.c \
                    ./../../../src/audio_process/real_fft.c \
                    ./../../../src/audio_process/resample.c \
                    ./../../../src/audio_process/resample_48khz.c \
                    ./../../../src/audio_process/resample_by_2.c \
                    ./../../../src/audio_process/resample_by_2_internal.c \
                    ./../../../src/audio_process/resample_by_2_mips.c \
                    ./../../../src/audio_process/resample_fractional.c \
                    ./../../../src/audio_process/ring_buffer.c \
                    ./../../../src/audio_process/spl_init.c \
                    ./../../../src/audio_process/spl_sqrt.c \
                    ./../../../src/audio_process/spl_sqrt_floor.c \
                    ./../../../src/audio_process/splitting_filter.c \
                    ./../../../src/audio_process/vector_scaling_operations.c \


LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../src/ \
            $(LOCAL_PATH)/../../../src/platform \
		    $(LOCAL_PATH)/../../../include \
		    $(LOCAL_PATH)/../../../dep/include/
		    

ifeq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_LDFLAGS := -L$(LOCAL_PATH)/../../../dep/lib/android/armeabi
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_LDFLAGS := -L$(LOCAL_PATH)/../../../dep/lib/android/armeabi-v7a
endif
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_LDFLAGS := -L$(LOCAL_PATH)/../../../dep/lib/android/arm64-v8a
endif

LOCAL_CFLAGS := -Wall -fvisibility=hidden
LOCAL_CFLAGS += -D_ANDROID -DLINUX -DWEBRTC_POSIX

LOCAL_LDLIBS := -llog -lstdc++ -lLINK_API -lPPCS_API -lNDT_API_PPCS


include $(BUILD_SHARED_LIBRARY)


