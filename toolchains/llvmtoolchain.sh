#./bin/sh

# create directories
mkdir cross_llvm
cd cross_llvm
# retrieve llvm & clang
git clone https://github.com/llvm/llvm-project.git
cd llvm-project

# compile llvm & clang
cd ..
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE:STRING=Release ../llvm/
make
make install