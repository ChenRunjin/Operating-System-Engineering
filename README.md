锁：
1. 一个进程多次acquire一个锁，在释放之前再次申请，会造成死锁，这种死锁可以被检测
2. 多个进程申请资源ab，但是顺序不同，导致ab落入不同进程中，所有进程都等待，死锁，解决这个问题可以对所有锁进行排序，必须按顺序申请锁资源，但是在操作系统中定义一个全局的锁顺序比较困难，因为不同模块之间应该不可见，无法得知对方需要哪些锁资源
3. RISCV中锁的实现基于一个特殊的指令amoswap， amoswap接收三个参数 （address r1 r2) 该指令会先锁定住address，将address中的数据保存在一个临时变量中（tmp），之后将r1中的数据写入到地址中，之后再将保存在临时变量中的数据写入到r2中，最后再对于地址解锁。
4. __sync_lock_test_and_set(type \*ptr, type value, ...) 是gcc内置的一个原子操作，将 \*ptr设为value并返回\*ptr操作之前的值。__sync_lock_release (type *ptr, ...) 将\*ptr置0
5.  __sync_synchronize 执行mfence指令，保证了mfence前后的读写操作的顺序，同时要求mfence之后写操作结果全局可见之前，mfence之前写操作结果全局可见
6.  Memory allocator：这个任务比较简单，把kmem结构体改成一个NCPU长度的数组就可以，初始化的时候初始化列表里的所有锁，由于是cpu0调用了kinit，所以freearange首先把所有的空余page都初始化在了cpu0的list里，但是后续释放的时候，哪个cpu抢到执行kfree函数，哪个列表就会得到释放的page，kalloc的时候，假如本cpu page不够了，就去别的list里面拿

文件系统：
1. inode是一个文件的对象，不依赖于文件名，inode通过自身的编号进行区分，一份文件只能在link count和openfd count都为0的时候才能被删除
2. sector通常是磁盘驱动可以读写的最小单元，它过去通常是512字节
3. block通常是操作系统或者文件系统视角的数据。它由文件系统定义，在XV6中它是1024字节。所以XV6中一个block对应两个sector。通常来说一个block对应了一个或者多个sector。
4. xv6中inode 包含 type(文件/目录)， nink， size, direct block number（最多12个，指向了构成文件的前12个block）indirect block number，（它对应了磁盘上一个block，这个block包含了256个block number，这256个block number包含了文件的数据），所以xv6当中最大文件是268kb
5. 为什么bcache每个block使用的是sleeplock:读写需要较长时间，并且spinlock占有锁的时候需要关闭中断，但是我们对block进行操作的时候都需要占有锁，不适用，sleeplock在获取锁等待的时候是sleep的，并且获取锁之后不需要关闭中断

buffer cache思路：
1. 将所有的block分成几个bucket,每个bucket负责取余为对应bucket号的block
2. 假如一个bucket buffer不够了，就要从别的bucket偷，但是很可能死锁，为了避免死锁，我们偷的时候先释放本bucket的锁，然后从别的地方偷一个block到本bucket，但是重新拿到本bucket的锁之后还要重新扫描一下，防止刚刚不持有锁的时候，别的进程把相同的buf传进来了