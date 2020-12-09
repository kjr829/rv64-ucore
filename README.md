# 环境配置
关于环境配置可以在[这里](https://github.com/NKU-EmbeddedSystem/riscv64-ucore/blob/k210-lab0/README.md)找到

# 与运行在qemu上版本的代码改动
对每一处修改的说明：
- [不包括终端的改动](https://github.com/Kirhhoff/riscv64-ucore/commit/82db2c9b26aff29d82aed49b53b5fd611d3e30d9)
- [添加SD卡驱动](https://github.com/Kirhhoff/riscv64-ucore/commit/671c0baf00365361793694413ce2fb95bbc1813b)
- [对终端的改动](https://github.com/Kirhhoff/riscv64-ucore/commit/7680ebb35c247c356f3846e88b57f37ee68ce4a8)

### 添加低地址内核映射
初始内核页表（entry.S中hard code的部分）只设置了高地址的内存映射，而刚开启分页时，内核仍然运行在低地址，随后才跳转到高地址，因此低地址部分的内核映射是必要的。qemu模拟器对此没有要求，而对实际环境这是必须的。

### 对memlayout的修改
qemu版本使用opensbi作为bootloader，将控制权移交给内核时跳转地址为0x80200000，同时内核运行的起始虚拟地址为0xFFFFFFFFC0200000，模拟器模拟出的最大物理地址为0x88000000。k210版本使用rustsbi作为bootloader，跳转地址为0x80020000，内核运行的起始虚拟地址为0xFFFFFFFFC0020000，实际物理地址最大为0x80600000（见[k210规格说明书](https://s3.cn-north-1.amazonaws.com.cn/dl.kendryte.com/documents/kendryte_datasheet_20180919020633.pdf)）。

因此，要修改两部分代码：一方面要修改memlayout中宏的定义，使得符号地址能够被正确计算；另一方面要修改linker script中的链接地址，使得代码运行的虚拟地址正确。

### 不同版本指令集带来的兼容性问题
risc-v ucore编写时基于[v1.11](https://github.com/riscv/riscv-isa-manual/releases/download/Ratified-IMFDQC-and-Priv-v1.11/riscv-privileged-20190608.pdf)的特权指令集，而k210实现时基于[v1.9](https://people.eecs.berkeley.edu/~krste/papers/riscv-privileged-v1.9.1.pdf)的特权指令集，两者大体上相同，但对我们的代码产生了两处影响
- v1.11中sstatus寄存器的SUM bit设置时允许S-mode访问用户态地址，然而在v1.9中，这个bit代表PUM，设置时会反而禁止这样的访问，因此需要删除原本设置这一bit的语句，同时修改对这个位的宏名称。
- 相比于v1.11，v1.9没有定义sstatus的UXL位，在宏定义中删去这一项

### 对齐问题
risc-v对内存读写指令有着较强的对齐要求，如当读取8字节指令作用在仅4字节对齐的地址上时，会触发misalignment异常，而在qemu版本中用户态syscall的内联汇编使用了错误对齐的内存读写指令，会触发异常。对齐在模拟器中不是强制要求的，但在实际环境中是必须的。

### 初始化部分
qemu版本在初始化时有一个get_char操作，但在实际环境下会阻塞内核的正常运行，因此删去这条语句。

### 用户栈的对齐
在给用户态进程准备初始栈以及参数时，除argc外其他数据类型均为8字节长，最后位于栈顶的argc为4字节，这会导致控制权刚移交到用户态时的栈指针不是8字节对齐的，但用户态并不知道这一点，会默认刚进入用户态的初始状态为8字节对齐。因此这里使用一个trick在内核准备栈参数以及传递这个参数时临时使用8字节的long类型，而在umain移交给main时再将该类型cast回4字节的int，同时避免的截断（因为最开始类型为4字节）。

### SD卡驱动
默认booting时会尝试检测SD卡，如果存在且成功初始化则会打印：
```
Successfully initialize SD card!
```
失败或超时则打印：
```
Fail to connect to SD card!
```
初始化时默认**不会**进行读写测试，以防出现意料外的数据损失；也可以通过取消注释kern/driver/sdcard.c的sd_test来进行读写测试，这个测试会对倒数第10、9两个扇区备份后进行读写测试，然后恢复其数据，但仍可能会造成数据损失，因此需要谨慎。

### 控制台改动
qemu版本的控制台输入使用sbi调用来获得，在每个时钟tick时检查，并将其feed进缓冲区。实际环境中通过外部中断获得，这里注册外部中断handler需要使用rustsbi提供的ecall扩展，同时键盘中断通过UARTHS传递到处理器，在handler中通过其中断号来识别。另外，在中断handler中不能直接feed输入，而只将其放在缓冲区中，而在每个时钟tick才尝试唤醒等待io的进程，从而避免在handler中触发调度。