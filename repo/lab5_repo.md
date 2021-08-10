思路：
1. 前两个任务比较简单，直接去掉sys_sbrk函数当中growproc部分，然后在trap.c当中添加新增page的代码，就可以实现用时再分配物理内存了。还需要改一下unmap的代码，去掉PTE_V位无效时的报错
2. 根据hint:
   1. Handle negative sbrk() arguments. 那么我在sbrk中设置加入n>0，只增加sz，不分配页，但n<0时，回收page,并且回收的时候注意忽略PTE_V无效和pte=0的情况
   2. fork正确复制：忽略uvmcopy当中页面不存在的情况
   3. 杀死使用大于sz页面的proc:在trap中加一个判断，假如大于sz就killed=1
   4. 杀死page fault小于用户栈的程序：因为用户栈顶在进入trap的时候存在trapframe->sp里，所以小于sp的page fault应该杀死
   5. 比较坑的一个细节（怪自己不小心吧）：因为物理内存耗尽而杀死进程，除了设置p->killed=1之外，可以立马exit杀死，不然会进入下一步memset出错.
   6. 注意：这里你会发现sbrkarg无法通过，这是因为copyin里用了walk函数，在page还没有映射的时候，walk很可能返回0， 在walkaddr返回0的时候进行mappage