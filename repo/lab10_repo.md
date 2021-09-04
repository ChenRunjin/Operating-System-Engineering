1. mmap接受某个对象，并将其映射到调用者的地址空间中，每段映射的内存都应该有一个VMA与之对应，

lab:
1. 首先加两个system call
2. 然后再proc结构体当中增加vma，记得初始化vma
3. mmap实现的时候，我们先找到一个空的vma，然后记录目前map的addr还有len等情况，我们规定map到addr是p->sz 到 p->sz+len的位置，这里想到一个可能的问题，p->sz所在的page可能已经被map了，不会产生pagefault，本来我打算在mmap的时候，先读PGROUNDUP(p->sz) - p->sz 个字节，但是想到mmap的page的PTE flag可能和之前的不同，所以我打算浪费一点，直接浪费PGROUNDUP(p->sz) - p->sz这些字节，然后增加的sz也扩充到整字节数（不够我看mmtest里所有的len都是整page)
4. 当mmap的len大于文件本身的sz的时候，我们还是给len的空间
5. 和lazy lab一样，在trap的时候分配page，还要判断一下pagefault的va是不是在某个vma的地址范围内
6. 记得unmap的时候不要设置pte未map为page fault
7. 注意 PROT_WRITE 不一定文件可写，只有在(flags == MAP_SHARED) && (prot & PROT_WRITE) 同时满足的时候才需要写回去，而且只有同时满足但文件不可写的时候报错
8. 注意一个很容易犯的bug，map申请的时候的长度很可能是大于文件本身的长度的时候，这部分可能完全没被写也没被读，所以不会page fault进行map，而且文件中有些部分可能完全没有mao，所以MAP_SHARED模式下需要写东西回去的时候应该忽视这些完全没有map的地址
9. unlink 和 close的关系： unlink函 数删除目录项，并且减少一个链接数。如果 链接数达到0并且没有任何进程打开该文件，该文件内容才被真正删除。如果在unlilnk之前没有close，那么依旧可以访问文件内容。