1. inode是一个文件的对象，不依赖于文件名，而靠编号进行区分，每个inode有一个link count 来跟踪指向这个inode的文件数量，一个inode只有在link count=0的时候才能被删除
2. 文件系统最底层是磁盘，提供持久化存储，上面是block cache，这些cache保存常用block到内存
3. sector通常是磁盘驱动可以读写的最小单元，它过去通常是512字节， block通常是操作系统或者文件系统视角的数据。它由文件系统定义，在XV6中它是1024字节。所以XV6中一个block对应两个sector。通常来说一个block对应了一个或者多个sector。我们可以将磁盘看作是一个巨大的block的数组，数组从0开始，一直增长到磁盘的最后。
4. 磁盘分布：block0要么没有用，要么被用作boot sector来启动操作系统。block1通常被称为super block，它描述了文件系统。它可能包含磁盘上有多少个block共同构成了文件系统这样的信息。我们之后会看到XV6在里面会存更多的信息，你可以通过block1构造出大部分的文件系统信息。在XV6中，log从block2开始，到block32结束。实际上log的大小可能不同，这里在super block中会定义log就是30个block。接下来在block32到block45之间，XV6存储了inode，一个inode是64字节（难道最多有14*（1024/64)=224个inode?)。之后是bitmap block，这是我们构建文件系统的默认方法，它只占据一个block。它记录了数据block是否空闲(所以最多1024*8个block?)。之后就全是数据block了，数据block存储了文件的内容和目录的内容。
5. 一个inode结构包括：type(目录还是文件)，nlink，size(包括多少字节)，文件所在block编号，最多12个，当文件大小多于12个block的时候，后面会紧跟一个indirect block number，这个number指向的block当中包含之多256个block编码，所以目前xv6最大文件是（256+12）*1024字节。
6. block log:一个系统调用并不直接导致对磁盘上文件系统的写操作，相反，他会把一个对磁盘写操作的描述包装成一个日志写在磁盘中。当系统调用把所有的写操作都写入了日志，它就会写一个特殊的提交记录到磁盘上，代表一次完整的操作。log_write像是 bwrite的一个代理；它把块中新的内容记录到日志中，并且把块的扇区号记录在内存中。log_write仍将修改后的块留在内存中的缓冲区中，因此相继的本会话中对这一块的读操作都会返回已修改的内容。log_write能够知道在一次会话中对同一块进行了多次读写，并且覆盖之前同一块的日志。
7. 写磁盘必须原子化，一个大操作下(比如写文件，但有很多block)的每个小操作都会被写log里，然后完成大操作之后一起commit(还会记录属于一个文件的操作数), commit会把log block写到磁盘，之后install操作会统一从log把数据移到磁盘，最后会清空log
8. bread函数都是从block cache 当中读数据，cache中没有必须要踢掉一个block，然后返回内存中一个cache block, 并且返回的cache block是带锁的，使用完之后，需要brelse释放锁，并且减少引用次数，假如引用=0就把这个block放到block cache列表的最后
9. begin_op()开始了一个操作，到end_op()结束，中间的操作是原子的
10. 所有在log中的block在log被清空前引用数都是大于0的，所以不会被release,也不会被占用 (超过cache数量的block文件读写怎么办？？解答：这是不允许的，所有的文件系统操作都必须适配log的大小，在真实的文件系统中会有大得多的log空间，大部分的文件操作都只会涉及到几个block的写入，当处理大文件的时候，都是几个block，几个block地写) 
11. commit的时候先把log中改动过的block编号对应的block写到log磁盘
12. buffer cache和log block的个书都一定要大于最大的文件操作长度
13. hard link：不增加inode,只会增加对应inode的link数，soft link:一个独立文件，内容是指向另一个文件的指针(注意：即使指向的文件不存在，也可以创建软链接！！)