1. 本项目在Linux平台下编译
2. NDK安装好后先建立standalone编译环境，否则编译不通过
   python make_standalone_toolchain.py --api 21 --install-dir /home/ffmpeg/work/toolchain/android/linux-x86_64/ndk-r16/android-21/arm --arch arm --stl libc++ --force
3. 修改ffmpeg_build_android.sh中的standalong工具链目录
4. 执行ffmpeg_build_android.sh生成ffmpeg静态库
5. jni目录中执行 ndk-build 生成解码库