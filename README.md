思路：
1. fork copy page的时候直接将子进程映射到之前的物理内存，但是将父进程和子进程的pte均设置为只读（改uvmcopy就可以）
2. 处理page fault的时候，我们需要判断错误是不是因为cow,所以在pte的标志位我们要加一个cow标志位PTE_C
3. 如果是PTE_C，我们复制一个page，并且将该进程的设为可写可读
4. 我们需要对物理page进行引用计数，我们在kernel里加一个类似pagetable的引用计数器,在mappage的时候cite+1, 在unmap的时候cite-1；在free的时候，我们也只free引用为0的page
5. 我本来打算只记录在user pagetable里的引用数，内核的page都只有一个cite，那么我们可以初始化的时候赋0，mappage的时候+1， unmap的时候-1，但是发现kernelmap的时候也用了mappage，并且平行map了所有物理page，所以引用数一直是1，所以这条思路有误，改成在kalloc的时候赋值1，kfree的时候-1，在fork的时候+1
6. trap的时候，如果pagefault是15，那么判断是不是COW，然后copypage，在重新mappage之前记得unmap，并且map的时候
7. 为了提升效率，trap的时候，copy之前先检查一下引用，假如只有一个引用了，就不需要copy，直接改flag就可以
8. 比较坑的地方：使用pagetable记录引用比使用数组要省空间一点，但是注意在cite_table初始化之前一定不要调用walkcite函数，由于kalloc和kfree中都有walkcite，而citetable在kvminit中初始化，所以kinit里不能使用kfree，初始化也不能直接调用kalloc
9. 为了不循环调用，在walkcite里不能使用kalloc