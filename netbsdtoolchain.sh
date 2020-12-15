#./bin/sh

# retrieve NetBSD Source
git clone https://github.com/NetBSD/src.git
cd src
./build.sh -U -m i386 tools
 
