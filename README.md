# risc-v ucore环境
- risc-v ucore的源代码可以在[这里](https://github.com/Liurunda/riscv64-ucore)下载
- risc-v ucore的环境配置可以循着[这里](https://1790865014.gitbook.io/ucore-step-by-step/intro/3_startdash)的指引
# 物理环境
- 一块risc-v指令集的K210芯片开发板（[Maix Dock](https://item.taobao.com/item.htm?spm=a1z10.5-c-s.w4002-21410578033.26.5de964c1c3hGix&id=591616120470)）
- 一根功能强大（**能跑串口协议的**）type-c数据线（一定要确保不是只能充电的，否则串口驱动不会自动安装，也无法与开发板通信，别问是怎么发现的..）

# Python环境
- 确保python3/pip3可用即可
>  如缺乏python环境可以以按下步骤安装
        1. 安装ssl等必需的库:
                ```
                sudo apt-get install libreadline-gplv2-dev libncursesw5-dev libssl-dev libsqlite3-dev tk-dev libgdbm-dev libc6-dev libbz2-dev
                ```
        2. 前往[Python官网](https://www.python.org/downloads/)下载需要版本的Python包，这里以[Python3.7.9](https://www.python.org/ftp/python/3.7.9/Python-3.7.9.tgz)的gzip包为例
        3. 下载完成后解压到任意目录，进入并执行
        ```
        ./configure --prefix=/usr/lib/python3.7.9
        ```
        ```
        make
        ```
        ```
        sudo make install
        ```
        这里的prefix是你要安装的目录，建议手动制定，方便未来必要时的卸载、删除。
        4. 将Python所在目录加入到PATH中（可加入）
        ```
        export PATH=/usr/lib/python3.7.9/bin:$PATH
        ```
        这里添加的是你安装路径下的bin文件夹

# k210环境
因为兼容性、乱码以及驱动问题，建议在Linux环境下进行k210实验，本指南也仅仅以Ubuntu下的配置为例。
总体上来说，k210需要的环境包括以下几个组件:
1. 串口转USB驱动
2. 串口终端工具
3. 烧录工具
4. bootloader与内核镜像共同打包的镜像文件

k210开发板通过其上的type-c串口与外界通信，这也是我们获取它输入输出的途径（类似屏幕接口），因此USB转串口驱动是必须的，但不用担心，Ubuntu是自带这个驱动的，无需我们安装。连接到电脑后，我们可以通过
```
ls /dev/ttyUSB*
```
来查看设备（一般就是/dev/ttyUSB0）
```
lumin@lumin:~$ ls /dev/ttyUSB*
/dev/ttyUSB0
```
我们想要与k210通信，除了驱动，还需要一个类似终端的东西来进行输入/输出，也就是串口终端工具。在[k210官网](https://cn.maixpy.sipeed.com/zh/get_started/env_serial_tools.html)说明上给出了几个可选的工具，但实测除了miniterm其他均存在兼容、乱码问题，因此这里我们以miniterm为例。miniterm是python3-serial这个package中附带的工具，这里我们通过apt安装python3-serial
```
sudo apt install python3-serial
```
然后就可以连接设备，用miniterm与之通信了
```
miniterm --eol LF --dtr 0 --rts 0 --filter direct /dev/ttyUSB0 115200
```
可以看到欢迎界面如下
```
lumin@lumin:~$ miniterm --eol LF --dtr 0 --rts 0 --filter direct /dev/ttyUSB0 115200
--- forcing DTR inactive
--- forcing RTS inactive
--- Miniterm on /dev/ttyUSB0  115200,8,N,1 ---
--- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---

[MAIXPY]Pll0:freq:832000000
[MAIXPY]Pll1:freq:398666666
[MAIXPY]Pll2:freq:45066666
[MAIXPY]cpu:freq:416000000
[MAIXPY]kpu:freq:398666666
[MAIXPY]Flash:0xc8:0x17
open second core...
gc heap=0x80319c40-0x80399c40(524288)
[MaixPy] init end

 __  __              _____  __   __  _____   __     __
|  \/  |     /\     |_   _| \ \ / / |  __ \  \ \   / /
| \  / |    /  \      | |    \ V /  | |__) |  \ \_/ /
| |\/| |   / /\ \     | |     > <   |  ___/    \   /
| |  | |  / ____ \   _| |_   / . \  | |         | |
|_|  |_| /_/    \_\ |_____| /_/ \_\ |_|         |_|

Official Site : https://www.sipeed.com
Wiki          : https://maixpy.sipeed.com

MicroPython v0.5.0-177-g40d52663f-dirty on 2020-08-20; Sipeed_M1 with kendryte-k210
Type "help()" for more information.
>>> 
```

接下来是烧录工具。这里先介绍下k210的运行流程，根据其[规格说明书](https://s3.cn-north-1.amazonaws.com.cn/dl.kendryte.com/documents/kendryte_datasheet_20180919020633.pdf)，k210有两部分通用sram内存，分别是0x80000000-0x805FFFFF的6MB与0x40000000-x405FFFFF的6MB，共12MB。

SRAM 映射分布：
| 模块名称 | 映射类型 | 开始地址 |结束地址  | 空间大小 | 
|  - | -  | -  | -  | -  |
|通用 SRAM 存储器 | 经 CPU 缓存| 0x80000000 |0x805FFFFF| 0x600000|
|AI SRAM 存储器| 经 CPU 缓存| 0x80600000| 0x807FFFFF |0x200000|
|通用 SRAM 存储器 |非 CPU 缓存 |0x40000000| 0x405FFFFF| 0x600000|
|AI SRAM 存储器| 非 CPU 缓存 |0x40600000 |0x407FFFFF| 0x200000|
系统启动时，ROM负责将内置SPI Flash中的内容拷贝到sram并跳转到0x80000000处开始执行。

因此我们的烧录实际上是将我们的程序写入内置SPI Flash，然后启动时由ROM将其拷贝到0x80000000处开始开始执行。意识到这一点，我们需要做的就是将bootloader的bin文件与内核内核镜像的bin文件打包成一个bin文件，使用一定的烧录工具将其写入到SPI Flash中即可。

烧录工具可以在[这里](https://github.com/sipeed/kflash_gui)下载，他提供了两个版本，分别是kflash_gui和kflash_py/kflash.py，前者是后者的GUI版本。由于前者需要qt等组件，我们以后者为例。

进行烧录前，我们需要先安装烧录需要的python库
```
pip3 install pyserial
```
至此，烧录程序的准备就可以了，但巧妇难为无米之炊，我们还没有制作镜像，也就没办法烧录。接下来我们进行镜像的制作。

镜像分为两部分，分别是bootloader和内核镜像。bootloader可以在[这里](https://github.com/wyfcyx/rCore-Tutorial/blob/multicore/bootloader/rustsbi-k210.bin)下载，内核镜像就使用对lab0进行make时产生的bin/ucore.img即可。我们将下载好的rustsbi-k210.bin与ucore.img都放在烧录工具的kflash_py目录下（也就是kflash.py所在的目录），将他们打包到一起
```
cp rustsbi-k210.bin kernel.img
dd if=ucore.img of=kernel.img bs=128K seek=1
```
这里bs=128K也就是0x20000，而seek=1意味着我们将ucore.img拷贝到了0x20000起始处。这是因为我们会将生成的kernel.img放在0x80000000处，那么相应的，原先的ucore.img就会放在0x80020000处，这个地址是我们在制作rustsbi-k210.bin是配置的bootloader跳转地址，当bootloader结束自己的工作后，他会跳转到0x80020000，将控制权转交给内核。我们可以查看这三个文件的大小
```
lumin@lumin:~/Documents/kflash_gui/kflash_py$ ll rustsbi-k210.bin ucore.img kernel.img -h
-rw-r--r-- 1 lumin lumin 141K 10月 21 09:43 kernel.img
-rw-r--r-- 1 lumin lumin  71K 10月 20 16:13 rustsbi-k210.bin
-rwxr-xr-x 1 lumin lumin  13K 10月 21 09:42 ucore.img*
```
rustsbi-k210.bin只有71K，因此我们不用担心ucore.img会覆盖之，同时最后的kernel.img大小也与我们的预期符合：128K+13K=141K

这样，我们就可以进行烧录了
```
python3 kflash.py -p /dev/ttyUSB0 -b 1500000 kernel.img 
```
这里-p指定端口，-b制定写入时的波特率，kernel.img是我们要写入的镜像文件。我们也可以什么参数都不加查看他的参数说明
```
lumin@lumin:~/Documents/kflash_gui/kflash_py$ python3 kflash.py
usage: kflash.py [-h] [-p PORT] [-f FLASH] [-b BAUDRATE] [-l BOOTLOADER]
                 [-k KEY] [-v] [--verbose] [-t] [-n] [-s] [-B BOARD] [-S SLOW]
                 [-A ADDR] [-L LENGTH]
                 firmware
```
烧录时可以看到如下效果
```
lumin@lumin:~/Documents/kflash_gui/kflash_py$ python3 kflash.py -p /dev/ttyUSB0 -b 1500000 kernel.img 
[INFO] COM Port Selected Manually:  /dev/ttyUSB0 
[INFO] Default baudrate is 115200 , later it may be changed to the value you set. 
[INFO] Trying to Enter the ISP Mode... 
.
[INFO] Automatically detected dan/bit/trainer 

[INFO] Greeting Message Detected, Start Downloading ISP 
Downloading ISP: |=============================================| 100.0% 10kiB/s
[INFO] Booting From 0x80000000 
[INFO] Wait For 0.1 second for ISP to Boot 
[INFO] Boot to Flashmode Successfully 
[INFO] Selected Baudrate:  1500000 
[INFO] Baudrate changed, greeting with ISP again ...  
[INFO] Boot to Flashmode Successfully 
[INFO] Selected Flash:  On-Board 
[INFO] Initialization flash Successfully 
------------------------------
ok
------------------------------
------------------------------========-------------------------| 45.7% 59kiB/s
ok
------------------------------
------------------------------=============================----| 91.4% 59kiB/s
ok
------------------------------
[INFO] Rebooting... ============================================| 102.8% 54kiB/s
```
至此，原则上我们的系统就可以跑起来了，我们再次运行miniterm
```
lumin@lumin:~$ miniterm --eol LF --dtr 0 --rts 0 --filter direct /dev/ttyUSB0 115200
--- forcing DTR inactive
--- forcing RTS inactive
--- Miniterm on /dev/ttyUSB0  115200,8,N,1 ---
--- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---
[rustsbi] Version 0.1.0
.______       __    __      _______.___________.  _______..______   __
|   _  \     |  |  |  |    /       |           | /       ||   _  \ |  |
|  |_)  |    |  |  |  |   |   (----`---|  |----`|   (----`|  |_)  ||  |
|      /     |  |  |  |    \   \       |  |      \   \    |   _  < |  |
|  |\  \----.|  `--'  |.----)   |      |  |  .----)   |   |  |_)  ||  |
| _| `._____| \______/ |_______/       |__|  |_______/    |______/ |__|

[rustsbi] Platform: K210
[rustsbi] misa: RV64ACDFIMSU
[rustsbi] mideleg: 0x222
[rustsbi] medeleg: 0x1ab
[rustsbi] Kernel entry: 0x80020000
(THU.CST) os is loading ...
```
可以看到lab0如预期的能够正常运行了！

# Makefile
为了方便开发与调试，在Makefile中有一个目标k210包含了上述所有命令，并且所需要的bootloader文件与kflash脚本均存放在tools文件夹下，这样一来，编译、生成镜像、刷固件、运行、终端连接一键完成：
```
lumin@lumin:~/Documents/riscv64-ucore/lab0$ make k210 -n
...（编译命令）
riscv64-unknown-elf-objcopy bin/kernel --strip-all -O binary bin/ucore.img
cp tools/rustsbi-k210.bin bin/kernel.img
dd if=bin/ucore.img of=bin/kernel.img bs=128K seek=1
python3 tools/kflash.py -p /dev/ttyUSB0 -b 1500000 bin/kernel.img
miniterm --eol LF --dtr 0 --rts 0 --filter direct /dev/ttyUSB0 115200
```
可能出现串口设备不是/dev/ttyUSB0的情况，可以在命令中指定：
```
lumin@lumin:~/Documents/riscv64-ucore/lab0$ make k210 PORT=/dev/ttyUSB1 -n
...（编译命令）
riscv64-unknown-elf-objcopy bin/kernel --strip-all -O binary bin/ucore.img
cp tools/rustsbi-k210.bin bin/kernel.img
dd if=bin/ucore.img of=bin/kernel.img bs=128K seek=1
python3 tools/kflash.py -p /dev/ttyUSB1 -b 1500000 bin/kernel.img
miniterm --eol LF --dtr 0 --rts 0 --filter direct /dev/ttyUSB1 115200
```
也可以在Makefile中修改，方便后续使用：
```
# create kernel.img
...
BOOTLOADER	:= tools/rustsbi-k210.bin
PORT		:= /dev/ttyUSB1
...
```