#./bin/sh

# retrieve gcc
mkdir cross_gcc
cd cross_gcc
git clone https://github.com/travisg/toolchains.git

# compile gcc for i386
./doit -f -a "i386"