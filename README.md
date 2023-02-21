obfuscator-pass
=========
Obfuscating LLVM passes based on **LLVM 15**

**Note: This project is purely for educational purposes.**

**obfuscator-pass** is a collection of LLVM passes for obfuscating. Key features:

* **Control Flow Flattening** - The purpose of this pass is to completely flatten the control flow graph of a program.

## Overview

LLVM implements a very rich, powerful and popular API. Its pass system makes it
very easy to add additional code transformations. While these transformations
are usually for code optimization, they can be used for different purposes,
e.g. code obfuscation.

The aim of this project is to provide a collection of LLVM passes to provide
increased software security through
[code obfuscation](https://en.wikipedia.org/wiki/Obfuscation_%28software%29)
and tamper-proofing. As LLVM pass mostly work at the
[Intermediate Representation](http://en.wikipedia.org/wiki/Intermediate_language)
(IR) level, these tools should be compatible with all programming languages and
target platforms supported by LLVM 15.

**Note**: The project is using and focusing on the _New Pass Manager_ aka Pass Manager
(that's how it is referred to in the
[code](https://github.com/llvm/llvm-project/blob/release/15.x/llvm/include/llvm/IR/PassManager.h#L469)
base), which is the default pass manager for the optimisation pipeline in LLVM as the _Legacy Pass Manager_
is deprecated and removed in LLVM 14.

<!-- === -->

Requirements
========
This project has been tested on **Ubuntu 20.04**. In order to build **obfuscator-pass** you will need:
  * LLVM 15
  * C++ compiler that supports C++17
  * CMake 3.13.4 or higher

In order to run the passes, you will need:
  * **clang-15** (to generate input LLVM files)
  * [**opt**](http://llvm.org/docs/CommandGuide/opt.html) (to run the passes)

There are additional requirements for tests (these will be satisfied by
installing LLVM 15):
  * [**lit**](https://llvm.org/docs/CommandGuide/lit.html) (aka **llvm-lit**,
    LLVM tool for executing the tests)
  * [**FileCheck**](https://llvm.org/docs/CommandGuide/FileCheck.html) (LIT
    requirement, it's used to check whether tests generate the expected output)

<!-- ## Installing LLVM 15 on Mac OS X
On Darwin you can install LLVM 15 with [Homebrew](https://brew.sh/):

```bash
brew install llvm@15
```

If you already have an older version of LLVM installed, you can upgrade it to
LLVM 15 like this:

```bash
brew upgrade llvm
```

Once the installation (or upgrade) is complete, all the required header files,
libraries and tools will be located in `/usr/local/opt/llvm/`. -->

## Installing LLVM 15 on Ubuntu
On Ubuntu Focal Fossa, you can install modern LLVMfrom the official
[repository](http://apt.llvm.org/):

```bash
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
sudo apt-add-repository "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-15 main"
sudo apt-get update
sudo apt-get install -y llvm-15 llvm-15-dev llvm-15-tools clang-15
```
This will install all the required header files, libraries and tools in
`/usr/lib/llvm-15/`.

## Building LLVM 15 From Sources
Building from sources can be slow and tricky to debug. It is not necessary, but
might be your preferred way of obtaining LLVM 15. The following steps will work
on Linux and Mac OS X:

```bash
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
git checkout release/15.x
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD=host -DLLVM_ENABLE_PROJECTS=clang <llvm-project/root/dir>/llvm/
cmake --build .
```
For more details read the [official
documentation](https://llvm.org/docs/CMake.html).

<!-- === -->

Building
========
You can build **obfuscator-pass** as follows:

```bash
cd <build/dir>
cmake -DLT_LLVM_INSTALL_DIR=<installation/dir/of/llvm/15> <source/dir>
make
```

The `LT_LLVM_INSTALL_DIR` variable should be set to the root of either the
installation or build directory of LLVM 15. It is used to locate the
corresponding `LLVMConfig.cmake` script that is used to set the include and
library paths.

Every LLVM pass is implemented in a separate shared object.
These shared objects are essentially dynamically loadable plugins for **opt**.
All plugins are built in the `<build/dir>/lib` directory.

Note that the extension of dynamically loaded shared objects differs between
Linux and Mac OS. For example, for the **HelloWorld** pass you will get:

* `libHelloWorld.so` on Linux
* `libHelloWorld.dylib` on MacOS.

For the sake of consistency, in this README.md file all examples use the `*.so`
extension. When working on Mac OS, use `*.dylib` instead.

<!-- === -->

References and Credits
========
Below is a list of resources and projects that this project is based on and I have found it very helpful.
* [llvm-tutor](https://github.com/banach-space/llvm-tutor)
* [Writing an LLVM Pass: 101](https://www.youtube.com/watch?v=ar7cJl2aBuU)
* [obfuscator](https://github.com/obfuscator-llvm/obfuscator)
* [Obfuscating C++ Programs via Control Flow Flattening](http://ac.inf.elte.hu/Vol_030_2009/003.pdf)

<!-- === -->

License
========
MIT License

Copyright (c) 2023 Yuerino

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

