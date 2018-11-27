Building Instructions:
 * got to the directory build.<host>, where <host> is your working OS
 * do a "make depend" to build depenencies (DO IT!)
 * do a "make" to build NachOS

Usage:
see "nachos -u" for all command line options

Building and starting user-level programs in NachOS:
 * use Mips cross-compiler to build and link coff-binaries
 * use coff2noff to translate the binaries to the NachOS-format
 * start binary with nachos -x <path_to_file/file>


# TODO LIST

## Basic File System Part  
**code**
1. 使文件系统支持 `128kb` 的文件
    - 1.1增加 indirect data block 支持 *(stargazermiao)* **√** 
    - 1.2增加 double indirect data block 支持 *(stargazermiao)* **√**
    - 1.3修改 `FileHeader::Allocate`使其分配时能向indirect block 和 double indirect block 上分配 *(stargazermiao)* **√**
    - 1.4修改 `FileHeader::Allocate`删除非直接的块 **√**
2. 使文件系统支持自适应的文件大小
    - 2.1修改`FileHeader::ByteToSector`访问大于文件1sector内容的块时增加新块 *(stargazermiao)* **√**
    - 2.2新增 `FileHeader::DeleteByte`删除文件中某一字节(~~不在明确题目要求中,不知道是不是强制要求，麻烦~~) **x**
    - 2.3处理创建新块/删除块过程中的磁盘freeMap并发访问问题 **x**
    - 2.4使文件夹支持超过10个文件 **x**

3. 多线程并发访问文件
    - 3.1 kernel中增加全局变量记录打开文件和操作情况 **x**
    - 3.2 原子读写操作，并保证读写一致性 **x**
    - 3.3 删除操作位于最低优先级，并发时先处理读写 **x**


**test**
1. 使文件系统支持 `128kb` 的文件
    - 1.1增加 indirect data block 支持 *(stargazermiao)* **x**
    - 1.2增加 double indirect data block 支持 *(stargazermiao)* **x**
    - 1.3修改 `FileHeader::Allocate`使其分配时能向indirect block 和 double indirect block 上分配 *(stargazermiao)* **x**
    - 1.4修改 `FileHeader::Allocate`删除非直接的块 **x**
2. 使文件系统支持自适应的文件大小
    - 2.1修改`FileHeader::ByteToSector`访问大于文件1sector内容的块时增加新块 *(stargazermiao)* **x**
    - 2.2新增 `FileHeader::DeleteByte`删除文件中某一字节(~~不在明确题目要求中,不知道是不是强制要求，麻烦~~) **x**
    - 2.3处理创建新块/删除块过程中的磁盘freeMap并发访问问题 **x**
    - 2.4使文件夹支持超过10个文件 **x**

3. 多线程并发访问文件
    - 3.1 kernel中增加全局变量记录打开文件和操作情况 **x**
    - 3.2 原子读写操作，并保证读写一致性 **x**
    - 3.3 删除操作位于最低优先级，并发时先处理读写 **x**