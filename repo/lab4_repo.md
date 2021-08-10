### 个人理解
1. 中断步骤：uservec->usertrap->usertrapret->userret
2. uservec:需要处理来自用户态的中断，难点在于用户页表没法处理，所以内核页表中需要和用户页表中共享相同的映射，最顶端有一个trampoline页，又uservec的地址
3. usertrap: 在usertrap函数中判断中断来源， 如果是时间中断（which_dev=2)，那么进入yeild函数，yeild函数先将进程状态修改为就绪，然后进入sched, sched切换上下文：
`swtch(&p->context, &mycpu()->context)` 将寄存器上下文转化为cpu调度器程序（包括ra寄存器)，这样子sched函数在返回的时候就会直接回到cpu->context中存储的ra地址，即cpu调度程序, 该程序会继续在process表中寻找下一个可以执行的进程，当某个cpu再次执行到这个进程的时候，调度器函数运行 `swtch(&c->context, &p->context)`切换回之前的上下文，这时的ra为上次调用的yeild函数sched后的位置，继续运行回到usertrap，然后进入usertrapret, 在usertrapret中，进程需要回到中断前的用户态，首先存储kernel pagetabel地址到trapframe（下次进入uservec的时候会用）

3. context和trapframe的区别：结构体trapframe用于切换优先级、页表目录等，而context则是用于轻量级的上下文切换。从技术上来看，两者的区别在于context仅仅能够切换普通寄存器，而trapframe可以切换包括普通寄存器、段寄存器以及少量的控制寄存器。

4. alarm 思路：在调用sys_aigalarm的时候，进入uservec，之前的用户态寄存器全部存在trapframe里，然后运行sys_aigalarm函数，我们设置好ticks和handler,然后将interval置零。下次时间中断进入usertrap的时候，trapframe也存储了中断前用户态的全部寄存器，这个时候我们要把trapframe的epc改成handler的地址，这样usertrapret里就会把sepc设置成handler，函数会直接回到handler,但是再次调用sig_sigreturn的时候，trapframe中寄存器的值会被覆盖，所以在sigalarm中，我们需要开辟新的空间存储原trapframe寄存器的信息，在sigreturn中恢复，所以我们在proc结构体当中增加trapframe_bk的字段，并更改proc初始化的代码（trapframe_bk字段只在内核态用，不用在用户页表中增加映射）

5. 但是检查很多遍发现test1当中 ij的值都无法正确恢复，暂时先放着