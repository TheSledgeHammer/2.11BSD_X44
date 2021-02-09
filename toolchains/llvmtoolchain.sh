#./bin/sh

# retrieve llvm & clang
mkdir cross_llvm
cd cross_llvm
git clone http://llvm.org/git/llvm.git
cd llvm/tools
git clone http://llvm.org/git/clang.git
cd ../projects
git clone http://llvm.org/git/compiler-rt.git

# compile llvm & clang
cd ..
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE:STRING=Release ../llvm/
make
make install
