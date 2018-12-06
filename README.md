# nachos-learn

nachos with log structured file system

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
    - 1.1增加 indirect data block 支持 *(stargazermiao)* **√**
    - 1.2增加 double indirect data block 支持 *(stargazermiao)* **√**
    - 1.3修改 `FileHeader::Allocate`使其分配时能向indirect block 和 double indirect block 上分配 *(stargazermiao)* **√**
    - 1.4修改 `FileHeader::Allocate`删除非直接的块 **x**
2. 使文件系统支持自适应的文件大小
    - 2.1修改`FileHeader::ByteToSector`访问大于文件1sector内容的块时增加新块 *(stargazermiao)* **√**
    - 2.2新增 `FileHeader::DeleteByte`删除文件中某一字节(~~不在明确题目要求中,不知道是不是强制要求，麻烦~~) **x**
    - 2.3处理创建新块/删除块过程中的磁盘freeMap并发访问问题 **x**
    - 2.4使文件夹支持超过10个文件 **x**

3. 多线程并发访问文件
    - 3.1 kernel中增加全局变量记录打开文件和操作情况 **x**
    - 3.2 原子读写操作，并保证读写一致性 **x**
    - 3.3 删除操作位于最低优先级，并发时先处理读写 **x**

## log structure file system

> 在makefile中删除-DLOG_FS标记并删除相关文件项（TODO，MakeFile要改）
1. 构建ADT 
    - FileHeaderMap, 保存filedeader在log中的位置， 实际上等于在磁盘上的位置，此处需缓存map！
    - Segment, 保存当前段在磁盘中的offset， seg 中存放一个块做segment sumary
    - SegmentSummary, 保存段内文件的编号和对应的文件块在段内的偏移， 操作时间，版本号
    - SegmentMap, 保存segment在磁盘上的起始地址, 段内的live byte, 该segment的最后操作时间
    - CheckPoint, 保存当前的FileHeaderMap和SegmentMap，当前日志的结尾指针

    > **NOTE：** 当前节点中只有文件名，考虑到文件名可以保证唯一性，使用文件名哈希值来作为文件编号可以保证唯一性（看下c++的md5运算, 需要引入c11支持）
2. 段清理
    - 检查SegmentMap, 找出live data 较少的块， 回收
    - 拷贝数据到新段上（刷写时块按修改的时间排序）， 此时触发写盘操作（包括新的fileheader）, 更新各种map
    - 设置守护线程检查当前clean的块数量，触发段清理

3. 缓存
    -  文件buffer使用循环buffer是实现，每次磁盘读写都会触发缓存更新.
    - log buffer 存储发生的写操作， buffer 满时触发一次磁盘连续写
    - 各种map同样需要在内存中做缓存
    - 标记已经缓存的日志末尾

4. 写操作
    - 向当前log buffer的末尾位置写入一条记录
    - 写盘操作被触发时， 刷写当前buffer到硬盘上的log结尾

5. 读操作
    - 查询fileheadermap, 可以打开文件
    - 读取sector时直接从文件中读取

6. 删除操作
    - 直接删除fileheadermap中的记录