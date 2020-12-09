# 环境配置
关于环境配置可以在[这里](https://github.com/NKU-EmbeddedSystem/riscv64-ucore/blob/k210-lab0/README.md)找到

# 与运行在qemu上版本的代码改动
[这里](https://github.com/NKU-EmbeddedSystem/riscv64-ucore/commit/e3035abf300cbcc96b2f54275da02e9e337620ff)有对每一处修改的说明
### 添加低地址内核映射
初始内核页表（entry.S中hard code的部分）只设置了高地址的内存映射，而刚开启分页时，内核仍然运行在低地址，随后才跳转到高地址，因此低地址部分的内核映射是必要的。qemu模拟器对此没有要求，而对实际环境这是必须的。

### 对memlayout的修改
qemu版本使用opensbi作为bootloader，将控制权移交给内核时跳转地址为0x80200000，同时内核运行的起始虚拟地址为0xFFFFFFFFC0200000，模拟器模拟出的最大物理地址为0x88000000。k210版本使用rustsbi作为bootloader，跳转地址为0x80020000，内核运行的起始虚拟地址为0xFFFFFFFFC0020000，实际物理地址最大为0x80600000（见[k210规格说明书](https://s3.cn-north-1.amazonaws.com.cn/dl.kendryte.com/documents/kendryte_datasheet_20180919020633.pdf)）。

因此，要修改两部分代码：一方面要修改memlayout中宏的定义，使得符号地址能够被正确计算；另一方面要修改linker script中的链接地址，使得代码运行的虚拟地址正确。

### 不同版本指令集带来的兼容性问题
risc-v ucore编写时基于[v1.11](https://github.com/riscv/riscv-isa-manual/releases/download/Ratified-IMFDQC-and-Priv-v1.11/riscv-privileged-20190608.pdf)的特权指令集，而k210实现时基于[v1.9](https://people.eecs.berkeley.edu/~krste/papers/riscv-privileged-v1.9.1.pdf)的特权指令集，两者大体上相同，但对我们的代码产生了两处影响
- v1.11中sstatus寄存器的SUM bit设置时允许S-mode访问用户态地址，然而在v1.9中，这个bit代表PUM，设置时会反而禁止这样的访问，因此需要删除原本设置这一bit的语句，同时修改对这个位的宏名称。
- 相比于v1.11，v1.9没有定义sstatus的UXL位，在宏定义中删去这一项