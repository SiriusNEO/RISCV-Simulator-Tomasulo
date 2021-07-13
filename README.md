# Yakumo Yukari - CPU

### 简介

PPCA2021 first assignment, a simple  RISC-V Simulator with Tomasulo algorithm

Contributor: [@SiriusNEO](https://github.com/SiriusNEO)

> Yakumo Yukari - CPU，顾名思义，使用八云紫制作而成的乱序执行CPU.
>
> 八云紫，栖息在边境的隙间妖怪，幻想乡贤者。
>
> 种族说是隙间妖怪，实际上这个种族也只有她一个人组成。拥有操纵境界的能力，因此不需要像常规的CPU一样顺序执行——打开一道隙间，便可以让命令乱序地执行。
>
> 而即使是乱序的执行，也不会因为冒险等原因使得答案错误，不愧是大结界的守护者呢。



### 文件结构

```C++
include.hpp //一些预定义与头文件
cpu_core.hpp //CPU类以及一些调度函数的定义
cpu_stages.cpp //各个阶段的定义（包括时序电路和组合电路）
components.hpp //一块内存与一个ALU，沿用自五级流水
tomasulo_components.hpp //Tomasulo算法的元件，包括RF、RS、SLB、ROB、CDB以及结点
decoder.hpp //一个精巧的解码器，沿用自五级流水
main.cpp //main函数
```



### CPU设计

相比五级流水来说去掉了流水部分的寄存器以及高级元件（实际上托马斯洛应该也能加预测）

加入了托马斯洛特有的元件，

以及一些用于通信的 CDB 节点。

```C++
class CPU {
    uint32_t pc;
    Decoder id;
    Memory mem;
    ALU alu;

    Queue<IQNode, IQ_N> IQ;
    ReservationStation<RS_N> RS_prev, RS_nxt;
    RegFile<REG_N> RF_prev, RF_nxt;
    StoreLoadBuffer<SLB_N> SLB_prev, SLB_nxt;
    Queue<ROBNode, ROB_N> ROB_prev, ROB_nxt;

    //INPUT_OUTPUT
    CDBNode IS_RF, COM_RF, IS_RS, RS_EX, 
            IS_ROB, ROB_COM, IS_SLB, EX_PUB, 
            COM_PUB, SLB_PUB_prev, SLB_PUB_nxt;
	int memClock;
}
```



### 执行流程

```C++
Components Work 
//这部分是各个元件的工作部分，由于均采用两层设计因此可以random_shuffle
//具体来讲，就是run_rob() run_rs() 这些
    
update //更新数据，用nxt更新prev

issue
execute
commit
//组合电路
//时序电路和组合电路之间我的代码不可以换序，也许采用更好的设计也是可以random的，
//但是实际这部分就是按顺序执行，所以没必要徒增烦恼
```



### 特殊情况处理

- **如何处理内存访问三个周期的情况？**

  计数器停止两个周期的 SLB 即可，SLB 按顺序，只要保证其队首不弹掉就好

- **RS SLB ROB 满了怎么办？**

  先不 issue，保存在 IQ 中，直到一些指令 commit 掉留出空位

  不可能一直满，因为要符合拓扑序

- **SLB 的执行办法？**

  每次看队首，如果是 L 就执行并发给 ROB，如果是 S 就等 ROB commit

  因为 S 操作是不可逆的

  

### Debug Log

- commit：Q相同设为-1，Q不同不改
- 循环队列与容器满的问题
- CDB 清空问题



### 设计草稿

统一符号：Q，这个值对应的ROB位置；V，值

Q != -1 说明在拓扑序上要等 ROB[Q] commit 才算有效

Q V 只有一个字段有效

时序电路：各元件名字

逻辑电路：issue、execute、commit

时序和逻辑之间沟通要用 CDB

```C++
IS_RF IS_RS IS_SLB IS_ROB //issue
RS_EX
ROB_COM
EX_PUB
SLB_PUB (two version)
COM_RF COM_PUB
```

`reservation`  

根据 EX 与 SLB 结果对数据进行更新

IS发来的新开一个空间（for 一遍，没 busy 就插）

for一遍，第一个能执行的 RS_EX

`reorderBuffer`  

接受 IS 信息，分配空间进行储存

EX 和 SLB 阶段计算后，更新 ROB 中的值，更新后 ROB 为 ready 状态

尝试队首 commit？

`instructionQueue`  从内存读指令，放到 IQ 里

`regfile`  根据 IS 和 COM 发来的指令修改寄存器

`storeloadbuffer`  

根据 IS 结果进行插入（enqueue）

根据 EX 与 SLB 结果对数据进行更新

根据 COM 是否跳转进行清空

查看队头，若队头 ready，

注意三个周期延迟（先不实现）

若为 LOAD 操作，执行，并发给 PUB 执行结果

若为 STORE 操作，读内存

`issue`  Decode，

在 RS 或者 SLB 新开一个（等到它们阶段再干）

在 ROB 新开一个空间，RS 和 SLB 的 ROB_id 指向它（表示保留站对应的 ROB）

如果 ROB 满了就不 issue（此时仍在 IQ 中）

维护 RS、SLB 的 Q1、Q2（利用 RF 的 Q）

具体来讲，如果 RF 的 Q 为 -1，表示 RF ，那么这个source就是对的，赋值V

如果不是，Q 指向 即可。

根据指令类型（arthm mem）发给 RS 或者 SLB，

IS_RS IS_SLB

发给 RF ，因为 Q 一定是，”最新改这个寄存器“ 的指令的ROB，IS_RF 过去即可 

`execute`  

执行，RS_EX，算出需要的东西发给 PUB（运算结果，写回 rd；跳转地址 pc；是否跳转）

`commit`

发给 RF 让它更新（只有这里能改 RF 的值！IS 只能改Q！）

pc 跳转执行，如果跳转发给 PUB 说一声，清空有关信息