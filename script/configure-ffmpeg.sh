#!/bin/sh

export PATH=$DEVKITPPC/bin:$PATH
export ARCH="-mcpu=750 -meabi -mhard-float"
export CFLAGS="-fomit-frame-pointer -ffast-math -fno-math-errno -funsafe-math-optimizations -ffinite-math-only -fno-trapping-math -O3 -Ofast"

./configure --prefix=$DEVKITPRO/portlibs/ppc/ \
--enable-cross-compile \
--cross-prefix=$DEVKITPPC/bin/powerpc-eabi- \
--disable-shared \
--disable-runtime-cpudetect \
--disable-programs \
--disable-doc \
--disable-everything \
--enable-decoder=mpeg4,h264,aac,ac3,mp3 \
--enable-demuxer=mov,h264,sdp,rtp,rtsp \
--enable-filter=rotate,transpose \
--enable-protocol=file,udp,rtp \
--enable-static \
--arch=ppc \
--cpu=750 \
--target-os=none \
--extra-cflags=" -D__WIIU__ $CFLAGS $ARCH -I$WUT_ROOT/include " \
--extra-cxxflags=" -D__WIIU__ $CFLAGS -fno-rtti -fno-exceptions -std=gnu++11 $ARCH" \
--extra-ldflags=" -Wl,-q -Wl,-z,nocopyreloc -L$DEVKITPRO/portlibs/wiiu/lib -L$DEVKITPRO/wut/lib  -Wl,--no-undefined -I/opt/devkitpro/wut/include " \
--extra-libs=" -lwut -lm " \
--disable-debug \
--disable-bzlib \
--disable-iconv \
--disable-lzma \
--disable-securetransport \
--disable-xlib \
--disable-zlib \
--pkg-config="$DEVKITPRO/portlibs/wiiu/bin/powerpc-eabi-pkg-config"
