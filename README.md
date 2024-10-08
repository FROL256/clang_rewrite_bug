# clang_rewrite_bug
Simple rewrite example to reprocude clang internal bugs

# Install
install llvm-17 (or other)

* wget https://apt.llvm.org/llvm.sh
* chmod +x llvm.sh
* sudo ./llvm.sh 17
* sudo apt-get install llvm-17-dev
* sudo touch /usr/lib/llvm-17/bin/yaml-bench
* sudo apt-get install libclang-17-dev

# Build
* cmake -DCMAKE_BUILD_TYPE=debug .
* make

# Run

* ./rtest input/test1.cpp
