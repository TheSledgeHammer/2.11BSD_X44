#./bin/sh

# retrieve NetBSD Source
mkdir cross_netbsd
cd cross_netbsd
git clone https://github.com/NetBSD/src.git
cd src
# compile netbsd toolchain for i386
./build.sh -U -m i386 tools
 
