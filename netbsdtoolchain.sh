#./bin/sh

#NetBSD: Building the crosscompiler
#The first step to do cross-development is to get all the necessary tools available. In NetBSD terminology, this is called the "toolchain", and it includes BSD-compatible make(1), 
#C/C++ compilers, linker, assembler, config(8), as well as a fair number of tools that are only required when crosscompiling a full NetBSD release, which we won't cover here.
#The command to create the crosscompiler is quite simple, using NetBSD's new src/build.sh script. Please note that all the commands here can be run as normal (non-root) user:
#$ cd /usr/src
#$ ./build.sh -m i386 tools
#
#non-root:
#$ cd /usr/src
#$ ./build.sh -U -m i386 tools

# retrieve NetBSD Source
git clone https://github.com/NetBSD/src.git
cd src
./build.sh -U -m i386 tools
 
