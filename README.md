# Yakumo Yukari

ROB 满？

Bug Record：

~~basicopt1~~

~~bulgarian~~ 

~~expr (WA)~~ （commit：Q相同设为-1，Q不同不改）

~~magic~~

~~multiarray (WA)~~

~~queen (WA)~~ （fix：clear some info when jump）

~~statement_test (WA)~~

superloop (WA->RE->WA)

~~pi~~





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