#!/bin/bash

if [ ! -d "../build" ] ; then
	mkdir ../build
fi

if [ -d "../build/yu_mac.app" ] ; then
	rm -rf ../build/yu_mac.app
fi

cppCompilerFlags="-x c++ -DDEBUG=1 -std=gnu++11  -g  -arch x86_64 -c"
objCCompilerFlags="-x objective-c++ -DDEBUG=1 -std=gnu++11 -fno-exceptions -g -arch x86_64 -c"

pushd ../build

mkdir -p yu_mac.app
mkdir -p yu_mac.app/Contents
mkdir -p yu_mac.app/Contents/MacOS
mkdir -p yu_mac.app/Contents/Resources
cp -r ../../data yu_mac.app/Contents/Resources/


clang++ $objCCompilerFlags ../yu_mac/yu_mac/main.mm ../src/renderer/gl_mac.mm ../src/core/core_mac.mm
clang++ $cppCompilerFlags ../src/core/crc32.cpp ../src/renderer/renderer_gl.cpp ../src/yu.cpp ../src/stargazer/stargazer.cpp


clang++ core_mac.o crc32.o gl_mac.o main.o renderer_gl.o stargazer.o yu.o -mmacosx-version-min=10.9  -framework IOKit -framework Cocoa -framework OpenGL -framework ApplicationServices -o yu_mac.app/Contents/MacOS/yu_mac


popd