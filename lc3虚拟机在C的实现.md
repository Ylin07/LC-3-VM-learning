项目地址 https://www.jmeiners.com/lc3-vm/#running-the-vm

# 什么是虚拟机

虚拟机是一种像计算机一样的程序。它模拟CPU和一些其他的硬件组件的功能，使其能够实现算术运算，内存读写，还有与I/O设备的交互。最重要的是，你可以理解你为其定义的机器语言，然后用它来编程。

# LC-3架构

这次编写的虚拟机将模拟LC-3架构。和X86架构相比，它的指令集更加简单

## 内存

LC-3中有65536个内存位置（这是16位无符号整数能表示的最大地址），每个位置存储一个16位的值。这意味它只能存储128KB的值，我们将其存储在一个数组中

```c
// Memory Storage
#define MEMORY_MAX (1<<16)
uint16_t memory[MEMORY_MAX];	//65536个16位的内存空间
```

## 寄存器

LC-3共有10个寄存器，每个寄存器都为16位的。其功能分布如下：

+ 通用寄存器（R0-R7）—— 8：执行程序计算
+ 程序计数器（PC）—— 1：表示内存中将要执行的下一条指令
+ 条件寄存器（COND）—— 1：计算的信息

```c
// Registers
enum{
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC,
    R_COND,
    R_COUNT
};
```

我们也将其保存在一个数组中

```c
// Register Storage
uint16_t reg[R_COUNT];
```

## 指令集

LC-3中只有16个操作指令，我们将用他们实现计算机中的各种内容。每个指令的长度是16位，我们用左边的4位用来存储操作码（操作指令对应的机器码）,其它位置用来存放参数

接下来定义操作码，并分配对应的枚举值：

```c
// Opcodes
enum{
    OP_BR = 0, 	//条件分支
    OP_ADD,		//加
    OP_LD,		//加载数据
    OP_ST,		//存储数据
    OP_JSR,		//子程序调用
    OP_AND,		//按位与
    OP_LDR,		//基址加载
    OP_STR,		//基址存储
    OP_RTI,		//不使用
    OP_NOT,		//按位或
    OP_LDI,		//间接加载
    OP_STI,		//间接存储
    OP_JMP,		//跳转
    OP_RES,		//不使用
    OP_LEA,		//加载有效地址
    OP_TRAP		//系统调用
};
```

## 条件标志

R_COND寄存器存储条件标志，这些标志提供了最近的计算的信息，使得程序能检查逻辑条件。这里LC-3至使用3个条件标志，这些条件标志指示前一次的计算符号

```c
// Condition Flags
enum{
    FL_POS = 1 << 0,	//P
    FL_ZRO = 1 << 1,	//Z
    FL_NEG = 1 << 2,	//N
}
```

现在我们已经完成虚拟机中硬件组件的设置。可以进一步尝试虚拟机的实现了，此时文件结构应该是这样

```c
@{Includes}

@{Registers}
@{Condition Flags}
@{Opcodes}
```

# 汇编示例

现在我们尝试写一个LC-3汇编的程序，以了解虚拟机上发生的事情

```assembly
.ORIG x3000			;程序将被加载到内存的这个位置
LEA R0,HELLO_STR	 ;将字符串加载到R0中
PUTs				;将R0指向的字符串输出
HALT				;中断
HELLO_STR .STRINGZ "Hello,World!"	;程序中字符串存储于此
.END				;结束标记
```

+ 这个指令是线性执行的，和C语言以及其他语言中的`{}`作用域不同
+ `.ORIG`和`.STRINGZ`是汇编器指令，用于生成一段代码或数据（像宏一样）

尝试循环和条件语句的使用通常使用类似goto的指令实现。比如计数到10：

```assembly
AND R0, R0, 0		;清空R0
LOOP				;设置跳转标签
ADD R0, R0, 1		;R0 = R0 + 1
ADD R1, R0, -10		 ;R1 = R0 - 10
BRn LOOP			;若为负数则跳转
```

# 执行程序

上面的例子让你大致理解了这个虚拟机的功能。使用LC-3的汇编语言，你甚至能在你的虚拟机上浏览网页，或者Linux系统

## 过程

以下是我们编写程序的步骤：

+ 从PC寄存器存储的内存地址中加载一条指令
+ 递增PC寄存器
+ 查看指令的操作码以确定它执行那种类型的指令
+ 使用指令中的参数执行指令
+ 返回第一步

当然我们也需要`while`和`if`之类的指令，帮我们实现指令的跳转，不然编程量会很大。所以在这里我们会通过类似于`goto`的指令来跳转PC从而改变执行流程，我们开始编写：

```c
// Main Loop
int main(int argc,const char * argv[]){
    @{Load Arguments}
    @{Setup}
    
    //无论何时都应该设定一个条件标志，这里设置Z标志
    reg[R_COND] = FL_ZRO;
    
    //为PC设置一个起始的内存加载地址，0x3000为默认
    enum{PC_START = 0x3000};
    reg[R_PC] = PCSTART;
    
    int running = 1;
    while (running){
        //读取
        uint16_t instr = mem_read(reg[R_PC]++);
        uint16_t op = instr >> 12;
        
        switch(op){
            case OP_ADD:
                @{ADD}
                break;
            case OP_AND:
                @{AND}
                break;
            case OP_NOT:
                @{NOT}
                break;
            case OP_BR:
                @{BR}
                break;
            case OP_JMP:
                @{JMP}
                break;
            case OP_JSR:
                @{JSR}
                break;
            case OP_LD:
                @{LD}
                break;
            case OP_LDI:
                @{LDI}
                break;
            case OP_LDR:
                @{LDR}
                break;
            case OP_LEA:
                @{LEA}
                break;
            case OP_ST:
                @{ST}
                break;
            case OP_STI:
                @{STI}
                break;
            case OP_STR:
                @{STR}
                break;
            case OP_TRAP:
                @{TRAP}
                break;
            case OP_RES:
            case OP_RTI:
            default:
                @{BAD OPCADE}
                break;
        }
    }
    @{Shutdown}
}
```

我们在主循环中，处理命令行参数，使我们的程序可用。我们期望输入的参数是一个虚拟机镜像，如果不是的话，则给出参数输入的用法

```c
// Load Arguments
if(argc < 2){
    //展示用法
    printf("lc3 [image-file1] ...\n");
    exit(2);
}

for(int j = 1;j < argc; ++j){
    if(!read_image(argv[j])){
        printf("failed to load image: %s\n",argv[j]);
        exit(1);
    }
}
```

# 指令实现

我们现在需要将每个操作码情况都填充为正确的实现，我们可以在LC-3中找到指令的正确实现。下面会演示两个指令的实现

## ADD

ADD指令取两个数字相加，并将结果存储在寄存器中。其规范如下：

![image.png](https://s2.loli.net/2025/03/17/J5tKwMghPWG7iul.png)

编码显示了两行，因为这条指令有两种模式。可以看到前四位（这里是小端序）都是一样的，`0001`是OP_ADD的操作码。接下来3位都是DR，这代表目标寄存器。目标寄存器是用来存储加和的位置。再接下来3位是SR1,这是包含第一个要加的数字的寄存器。

区别在于最后五位，注意第5位，这个位表示ADD是立即数模式还是寄存器模式，在寄存器模式中，则是将两个寄存器相加，第二个寄存器被标记为SR2。其使用方法如下：

```assembly
ADD R2 R0 R1	;R2 = R0 + R1
```

立即数模式则是将第二个值嵌入指令本身，如图中所示标识为imm5，消除了编写将值从内存中读取的需要。不过限于指令的长度，数字最多$2^5 = 32$（无符号），这使得即时模式主要用于递增和递减，其使用如下：

```assembly
ADD R0 R0 1		;R0 = R0 + 1
```

其具体规范如下：

If bit [5] is 0, the second source operand is obtained from SR2. If bit [5] is 1, the second source operand is obtained by sign-extending the imm5 field to 16 bits. In both cases, the second source operand is added to the contents of SR1 and the result stored in DR. (Pg. 526)

这里的话和我们刚刚分析的差不多，但是在于它提到的`"sign-extending"`。这是啥用的呢？要知道，我们的加法是16位的，但我们的立即数是5位的，所以我们需要将它拓展到16位以匹配另一个数字。对于正数我们只需要补充0即可，但是对于负数我们需要用1填充，从而保持原始值

```c
// Sign Extend
uint16_t sign_extend(uint16_t x, int bit_count){
    if((x>>(bit_count-1))&1){
        x |= (0xFFFF << bit_count);
    }
    return x;
}
```

规范中还有一句填充：

The condition codes are set, based on whether the result is negative, zero, or positive. (Pg. 526)

条件码根据结果是否为负，0，正来设置。我们之前定义了一个条件标志枚举，现在我们需要用到他们。每当有一个值被写入寄存器中，我们需要更新条件标志寄存器来标识它的符号。我们写一个函数来进行这个过程：
```c
// Update Flags
void update_flags(uint16_t r){
    if(reg[r] == 0)
        reg[R_COND] = FL_ZRO;
    else if(reg[r] >> 15)	//当1是最高位时说明这是一个负数
        reg[R_COND] = FL_NEG;	
    else
        reg[R_COND] = FL_POS
}
```

至此我们可以编写ADD了：

```c
// ADD
{
    //目标寄存器
    uint16_t r0 = (instr >> 9) & 0x7;
    //第一操作数
    uint16_t r1 = (instr >> 6) & 0x7;
    //是否是立即数
    uint16_t imm_flag = (instr >> 5) & 0x1;
    
   if(imm_flag){
       uint16_t imm5 = sign_extend(instr & 0x1F,5);
       reg[r0] = reg[r1] + imm5;
   }else{
       uint16_t r2 = instr & 0x7;
       reg[r0] = reg[r1] + reg[r2];
   }
    update_flags[r0]
}
```

至此，我们的ADD指令编写完成了，接下来我们将使用符号拓展，不同模式和更新标志的组合去实现其他的指令

## LDI

LDI的话是间接加载的意思。这个指令将内存地址中的值加载到寄存器中，下面是它的指令样式：

![image.png](https://s2.loli.net/2025/03/17/W9nZpKRscT52XkD.png)

可以看到前面的结构和ADD差不多，一个操作码，加上一个目标寄存器，剩余的位被标记为`PCoffset9`。这是一个嵌入在指令中的立即值（类似imm5）。由于这个指令从内存中读取，所以推测这个数字是一个地址，告诉我们从哪里加载：

An address is computed by sign-extending bits `[8:0]` to 16 bits and adding this value to the incremented `PC`. What is stored in memory at this address is the address of the data to be loaded into `DR`. (Pg. 532)

大概的意思是，我们需要将这个9位的值进行符号拓展，将其加到当前的PC上,结果之和则是一个内存的地址，这个地址包含另一个值，也就是我们将要加载的值的地址

这种方式读取内存可能很绕，但实际上这是必不可少的。`LD`指令仅限于9位地址偏移量，但是内存需要16位寻址，`LDI`对于加载存储在离当前PC位置较远的数据时更有用。与之前一样，将值放到`DR`后需要更新标志，其实现如下：

```c
// LDI
{
    //目标寄存器
    uint16_t r0 = (instr >> 9) & 0x7;
    //初始化PCoffset
    uint16_t pc_offset = sign_extend(instr & 0x1FF,9);
    //将偏移值与PC相加，并读取对应的内存中的值到寄存器中
    reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
    update_flags(r0);
}
```

现在我们实现了两个主要的指令，接下来的用差不多的方式就可以写出来了

## 其他

### RTI & RES

并不使用

```c
// BAD OPCODE
abort();
```

### AND

```c
// AND
{
	uint16_t r0 = (instr >> 9) & 0x7;
	uint16_t r1 = (instr >> 6) & 0x7;
	uint16_t imm_flag = (instr >> 5) & 0x1;

	if(imm_flag){
    	uint16_t imm5 = sign_extend(instr & 0x1F,5);
	    reg[r0] = reg[r1] & imm5;
	}else{
    	uint16_t r2 = instr & 0x7;
	    reg[r0] = reg[r1] & reg[r2];
	}
	update_flags(r0)
}

```

### NOT

```c
// NOT
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t r1 = (instr >> 6) & 0x7;
    
    reg[r0] = ~reg[r1];
    update_flags(r0);
}
```

### BR

```c
// BR
{
    uint16_t pc_offset = sign_extend(instr & 0x1FF,9);
    uint16_t cond_flag = (instr >> 9) & 0x7;
    if(cond_flag & reg[R_COND]){	//cond_flag设置了跳转的条件
        reg[R_PC] += pc_offset;
    }
}
```

### JMP

这里我们将RET视作JMP的一种特殊情况

```c
// JMP
{
    uint16_t r1 = (instr >> 6) & 0x7;
    reg[R_PC] = reg[r1];
}
```

### JSR

```c
// JSR
{
    uint16_t long_flag = (instr >> 11) & 1;
    reg[R_R7] = reg[R_PC];
    if(long_flag){
        uint16_t long_pc_offset = sign_extend(intstr & 0x7FF,11);
        reg[R_PC] += long_pc_offset;	//JSR,加上偏移值
    }else{
        uint16_t r1 = (instr >> 6) & 0x7;
        reg[R_PC] = reg[r1];			//JSRR，跳转到寄存器指定地址
    }
}
```

### LD

```c
// LD
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset = sign_extend(instr & 0x1FF,9);
    reg[r0] = mem_read(reg[R_PC] + pc_offset);	//在PC上偏移
    update_flags(r0);
}
```

### LDR 

```c
// LDR
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t r1 = (instr >> 6) & 0x7;
    uint16_t offset = sign_extend(instr & 0x3F,6);
    reg[r0] = mem_read(reg[r1] + offset);
    update_flags(r0);
}
```

### LEA

```c
// LEA
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset = sign_extend(instr & 0x1FF,9);
    reg[r0] = reg[R_PC] + pc_offset;
    update_flags(r0);
}
```

### ST

```c
// ST
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset = sign_extend(instr & 0x1FF,9);
    mem_write(reg[R_PC] + pc_offset,reg[r0]);
}
```

### STI 

```c
// STI
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset = sign_extend(instr & 0x1FF,9);
    mem_write(mem_read(reg[R_PC] + pc_offset),reg[r0]);
}
```

### STR

```c
// STR
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t r1 = (instr >> 6) & 0x7;
    uint16_t offset = sign_extend(intstr & 0x3F,6);
    mem_write(reg[r1] + offset,reg[r0])
}
```

## 中断例程 Trap routines

LC-3中提供了一些预定的例程，用来执行常见的任务，以及与I/O间的设备交互。例如，从键盘获取输入和向控制台显示字符串的中断例程。可以将其作为LC-3的操作系统API理解。每个例程分配了一个中断码，我们定义一个枚举来实现他们，下面是`TRAP`指令的格式

![image.png](https://s2.loli.net/2025/03/19/ZiLJBOyFST43fWc.png)

其中的0~7位用于存放中断码，我们据此来实现他们：

```c
// TRAP Codes
enum{
    TRAP_GETC = 0x20,		//从键盘中读取输入，但是不打印至终端
    TRAP_OUT = 0x21,		//输出字符
    TRAP_PUTS = 0x22,		//输出字符串（双字节单字符）
    TRAP_IN = 0x23,			//从键盘中读取输入，且打印至终端
    TRAP_PUTSP = 0x24,		//输出字符串（单字节单字符）
    TRAP_HALT = 0x25		//中断程序
};
```

中断例程并不是指令，但是它为LC-3的运行提供了各种快捷的方式。在LC-3中，这些例程实际上是使用汇编编写的，当中断例程被调用了，程序被跳转到这里，执行后返回程序。（为什么程序从0x3000开始加载而不是0x0也是因为这个原因，需要一部分空间用来存储中断例程的代码）

在LC-3中我们直接使用C语言进行中断例程的编写，而不是汇编。我们不用从汇编开始写自己的I/O设备，我们可以发挥虚拟机的优势，使用更好的更搞层次的设备去仿真这个过程

我们在`TRAP`中使用一个switch来进一步的编写这个指令：

```c
// TRAP
reg[R_R7] = reg[R_PC];
switch(instr & 0xFF){
    case TRAP_GETC:
        @{TRAP_GETC}
        break;
    case TRAP_OUT:
        @{TRAP_OUT}
        break;
    case TRAP_PUTS:
        @{TRAP_PUTS}
        break;
    case TRAP_IN:
        @{TRAP_IN}
        break;
    case TRAP_PUTSP:
        @{TRAP_PUTSP}
        break;
    case TRAP_HALT:
        @{TRAP_HALT}
        break;
}
```

接下来进一步的实现例程中的内容

### PUTS

功能的话类似于C语言中的`printf`，但是我们字符串和C中的有所不同，LC-3中的字符并不是被单独存储为一个字节，而是被存储为一个内存位置。LC-3中的内存位置并不是16位，所以字符串中的每个字符都是16位的。为了用C的方法将其打印出来，我们需要将它转换为一个字符并单独输出

```c
// TRAP PUTS
{
    // 每个字符占一个字(16bits)
    uint16_t *c = memory + reg[R_R0];
    while(*c){
        putc((char)*c,stdout);	//强制类型转换
        ++c;
    }
    fflush(stdout);
}
```

以这个为例，我们可以进一步的编写其他的例程

### GETC

```c
// TRAP GETC
{
    // 读取一个ASCII字符
    reg[R_R0] = (uint16_t)getchar();
    update_flags(R_R0);
}
```

### OUT

```c
// TRAP OUT
{
    putc((char)reg[R_R0],stdout);
    fflush(stdout);
}
```

### IN

```c
// TRAP IN
{
    printf("Enter a character: ");
    char c = getchar();
    putc(c,stdout);
    fflush(stdout);
    reg[R_R0] = (uint16_t)c;
    update_flags(R_R0);
}
```

### PUTSP

```c
// TRAP PUTSP
{
    //刚刚的字符串打印是适用于双字节单字符的情况，现在是单字节单字符的情况
    uint16_t *c = memory + reg[R_R0];
    while(*c){
        char char1 = (*c) & 0xFF;
        putc(char1,stdout);
        char char2 = (*c) >> 8;
        if(char2) putc(char2,stdout);
        ++c;
    }
    fflush(stdout);
}
```

### HALT

```c
//HALT	
{
    puts("HALT");
    fflush(stdout);
    running = 0;
}
```



# 程序加载

我们也许可以注意到指令是从内存中加载和进行执行的，但是指令是怎么进入内存的呢？当汇编被转换为机器指令时，它变成了一个包含了一系列的数据与指令的文件，可以将它复制到内存中进行加载。

16位的程序文件头指出了程序在内存中起始的地址。这个地址被称之为`起始地址(origin)`。它首先被读取，然后从起始地址开始将其余数据从文件读入内存。以下是LC-3中加载内存的程序：

```c
// Reas Image File
void read_image_file(FILE * file){
    //起始地址让我们知道内存从什么地方开始加载
    uint16_t origin;
    fread(&origin,sizeof(orgin),1,file);
    origin = swap16(origin);
    
    //由于我们有最大的内存范围，所以我们可以通过起始位置，确定加载范围
    uint16_t max_read = MEMORY_MAX - origin;
    uint16_t *p = memory + origin;
    size_t read = fread(p,sizeof(uint16_t),max_read,file);
    
    //转换为小端序
    while(read-- > 0){
        *p = swap16(*p);
        ++p;
    }
}
```

注意这里`swap16`被调用于每一个数据块，这是因为LC-3是大端序结构，但是大多数现代计算机采用的是小端序结构，所以我们需要将每个`uint16`数据都转换一遍

```c
uint16_t swap16(uint16_t x){
    return (x << 8)|(x >> 8);
}
```

我们进一步的封装`read_image_file()`程序：

```c
int read_image(const char * image_path){
    FILE *file = fopen(image_path,"rb");
    if(!file){return 0;};
    read_image_file(file);
    fclose(file);
    return 1;
}
```

# 内存映射寄存器

有一些特殊的寄存器是不能通过普通的寄存器组去访问的。相反，在内存中为其保留了特殊的地址。要读写这些寄存器只需要读写他们的内存地址。这个过程便被称为`内存映射寄存器`。这通常用于一些特殊的硬件设备操作

LC-3有两个内存映射寄存器需要实现。一个是键盘状态寄存器（KBSR），一个是键盘数据寄存器（KBDR）。KBSR用来指示按键是否被按下，KBDR用来指示哪个按键被按下了

虽然可以使用`GETC`来完成对键盘输入的读取，但是在读取到输入之前，它会一直阻塞程序的执行。而KBSR和KBDR则可以实现对键盘设备的轮询，以确保程序在等待输入的时候持续执行

```c
//Memory Mapped Registers
enum{
    MR_KBSR = 0xFE00,
    MR_KBDR = 0xFE02
};
```

内存映射寄存器使得内存访问变得有点复杂，使得你不能直接对内存数组进行访问。所以我们需要编写函数以实现对内存的读写。当从KBSR读取内存时，读取函数将检查键盘并更新两个内存位置

```c
//Memory Access
void mem_write(uint16_t address,uint16_t val){
    memory[address] = val;
}

uint16_t mem_read(uint16_t address){
    if(address == MR_KBSR){
        if(check_key()){
            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = getchar();
        }else{
            memory[MR_KBSR] = 0;
        }
    }
    return memory[address];
}
```

到此为止，VM的最后一个组件也模拟实现了，我们可以尝试运行它了。

# 平台特定信息

不同系统中的键盘读入和终端打印的实现可能有所不同

```c
// Input Buffering
struct termios original_tio;

void disable_input_buffering(){
    tcgetattr(STDIN_FILENO,&original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);
}

void restore_input_buffering(){
    tcsetattr(STDIN_FILENO,TCSANOW,&original_tio);
}

uint16_t check_key(){
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO,&readfds);
    
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1,&readfds,NULL,NULL,&timeout) != 0;
}
```

# 组合程序

我们利用刚刚写好的程序来对缓冲进行设置，以实现正确处理终端输入。

在main函数开头设置代码：

```c
//Setup
signal(SIGINT,handle_interrupt);
disable_input_buffering();
```

当程序结束后，我们将中断设置为正常状态：

```c
//Shutdown
restore_input_buffering();
```

设置也应该在接收到结束信号时恢复：

```c
//Handle Interrupt
void handle_interrupt(int signal){
    restore_input_buffering();
    printf("\n");
    exit(-2);
}
```

OK 我们的程序将要实现，我们将其组合便可得到：

```c
@{Memory Mapped Registers}
@{TRAP Codes}

@{Memory Storage}
@{Register Storage}

@{Input Buffering}
@{Handle Interrupt}
@{Sign Extend}
@{Swap}
@{Update Flags}
@{Read Image File}
@{Read Image}
@{Memory Access}

@{Main Loop}
```



# 完整代码

```c
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>

#define MEMORY_MAX (1<<16)


//Memory Mapped Registers
enum{
    MR_KBSR = 0xFE00,
    MR_KBDR = 0xFE02
};

enum{
    TRAP_GETC = 0x20,		//从键盘中读取输入，但是不打印至终端
    TRAP_OUT = 0x21,		//输出字符
    TRAP_PUTS = 0x22,		//输出字符串（双字节单字符）
    TRAP_IN = 0x23,			//从键盘中读取输入，且打印至终端
    TRAP_PUTSP = 0x24,		//输出字符串（单字节单字符）
    TRAP_HALT = 0x25		//中断程序
};
enum{
    OP_BR = 0, 	//条件分支
    OP_ADD,		//加
    OP_LD,		//加载数据
    OP_ST,		//存储数据
    OP_JSR,		//子程序调用
    OP_AND,		//按位与
    OP_LDR,		//基址加载
    OP_STR,		//基址存储
    OP_RTI,		//不使用
    OP_NOT,		//按位或
    OP_LDI,		//间接加载
    OP_STI,		//间接存储
    OP_JMP,		//跳转
    OP_RES,		//不使用
    OP_LEA,		//加载有效地址
    OP_TRAP		//系统调用
};
uint16_t memory[MEMORY_MAX];	//65536个16位的内存空间
enum{
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC,
    R_COND,
    R_COUNT
};
// Register Storage
uint16_t reg[R_COUNT];
enum{
    FL_POS = 1 << 0,	//P
    FL_ZRO = 1 << 1,	//Z
    FL_NEG = 1 << 2,	//N
};

struct termios original_tio;

void disable_input_buffering(){
    tcgetattr(STDIN_FILENO,&original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);
}

void restore_input_buffering(){
    tcsetattr(STDIN_FILENO,TCSANOW,&original_tio);
}

uint16_t check_key(){
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO,&readfds);
    
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1,&readfds,NULL,NULL,&timeout) != 0;
}

//Handle Interrupt
void handle_interrupt(int signal){
    restore_input_buffering();
    printf("\n");
    exit(-2);
}

uint16_t sign_extend(uint16_t x, int bit_count){
    if((x>>(bit_count-1))&1){
        x |= (0xFFFF << bit_count);
    }
    return x;
}

uint16_t swap16(uint16_t x){
    return (x << 8)|(x >> 8);
}

void update_flags(uint16_t r){
    if(reg[r] == 0)
        reg[R_COND] = FL_ZRO;
    else if(reg[r] >> 15)	//当1是最高位时说明这是一个负数
        reg[R_COND] = FL_NEG;	
    else
        reg[R_COND] = FL_POS;
}

void read_image_file(FILE * file){
    //起始地址让我们知道内存从什么地方开始加载
    uint16_t origin;
    fread(&origin,sizeof(origin),1,file);
    origin = swap16(origin);
    
    //由于我们有最大的内存范围，所以我们可以通过起始位置，确定加载范围
    uint16_t max_read = MEMORY_MAX - origin;
    uint16_t *p = memory + origin;
    size_t read = fread(p,sizeof(uint16_t),max_read,file);
    
    //转换为小端序
    while(read-- > 0){
        *p = swap16(*p);
        ++p;
    }
}

int read_image(const char * image_path){
    FILE *file = fopen(image_path,"rb");
    if(!file){return 0;};
    read_image_file(file);
    fclose(file);
    return 1;
}

void mem_write(uint16_t address,uint16_t val){
    memory[address] = val;
}

uint16_t mem_read(uint16_t address){
    if(address == MR_KBSR){
        if(check_key()){
            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = getchar();
        }else{
            memory[MR_KBSR] = 0;
        }
    }
    return memory[address];
}

int main(int argc,const char * argv[]){
    if(argc < 2){
        //展示用法
        printf("lc3 [image-file1] ...\n");
        exit(2);
    }
    
    for(int j = 1;j < argc; ++j){
        if(!read_image(argv[j])){
            printf("failed to load image: %s\n",argv[j]);
            exit(1);
        }
    }
    signal(SIGINT,handle_interrupt);
    disable_input_buffering();
    
    //无论何时都应该设定一个条件标志，这里设置Z标志
    reg[R_COND] = FL_ZRO;
    
    //为PC设置一个起始的内存加载地址，0x3000为默认
    enum{PC_START = 0x3000};
    reg[R_PC] = PC_START;
    
    int running = 1;
    while (running){
        //读取
        uint16_t instr = mem_read(reg[R_PC]++);
        uint16_t op = instr >> 12;
        
        switch(op){
            case OP_ADD:
            {
                //目标寄存器
                uint16_t r0 = (instr >> 9) & 0x7;
                //第一操作数
                uint16_t r1 = (instr >> 6) & 0x7;
                //是否是立即数
                uint16_t imm_flag = (instr >> 5) & 0x1;
                
               if(imm_flag){
                   uint16_t imm5 = sign_extend(instr & 0x1F,5);
                   reg[r0] = reg[r1] + imm5;
               }else{
                   uint16_t r2 = instr & 0x7;
                   reg[r0] = reg[r1] + reg[r2];
               }
                update_flags(r0);
            }
                break;
            case OP_AND:
            {
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t r1 = (instr >> 6) & 0x7;
                uint16_t imm_flag = (instr >> 5) & 0x1;
            
                if(imm_flag){
                    uint16_t imm5 = sign_extend(instr & 0x1F,5);
                    reg[r0] = reg[r1] & imm5;
                }else{
                    uint16_t r2 = instr & 0x7;
                    reg[r0] = reg[r1] & reg[r2];
                }
                update_flags(r0);
            }
                break;
            case OP_NOT:
            {
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t r1 = (instr >> 6) & 0x7;
                
                reg[r0] = ~reg[r1];
                update_flags(r0);
            }
                break;
            case OP_BR:
            {
                uint16_t pc_offset = sign_extend(instr & 0x1FF,9);
                uint16_t cond_flag = (instr >> 9) & 0x7;
                if(cond_flag & reg[R_COND]){	//cond_flag设置了跳转的条件
                    reg[R_PC] += pc_offset;
                }
            }
                break;
            case OP_JMP:
            {
                uint16_t r1 = (instr >> 6) & 0x7;
                reg[R_PC] = reg[r1];
            }
                break;
            case OP_JSR:
            {
                uint16_t long_flag = (instr >> 11) & 1;
                reg[R_R7] = reg[R_PC];
                if(long_flag){
                    uint16_t long_pc_offset = sign_extend(instr & 0x7FF,11);
                    reg[R_PC] += long_pc_offset;	//JSR,加上偏移值
                }else{
                    uint16_t r1 = (instr >> 6) & 0x7;
                    reg[R_PC] = reg[r1];			//JSRR，跳转到寄存器指定地址
                }
            }
                break;
            case OP_LD:
            {
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t pc_offset = sign_extend(instr & 0x1FF,9);
                reg[r0] = mem_read(reg[R_PC] + pc_offset);	//在PC上偏移
                update_flags(r0);
            }
                break;
            case OP_LDI:
            {
                //目标寄存器
                uint16_t r0 = (instr >> 9) & 0x7;
                //初始化PCoffset
                uint16_t pc_offset = sign_extend(instr & 0x1FF,9);
                //将偏移值与PC相加，并读取对应的内存中的值到寄存器中
                reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
                update_flags(r0);
            }
                break;
            case OP_LDR:
            {
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t r1 = (instr >> 6) & 0x7;
                uint16_t offset = sign_extend(instr & 0x3F,6);
                reg[r0] = mem_read(reg[r1] + offset);
                update_flags(r0);
            }
                break;
            case OP_LEA:
            {
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t pc_offset = sign_extend(instr & 0x1FF,9);
                reg[r0] = reg[R_PC] + pc_offset;
                update_flags(r0);
            }
                break;
            case OP_ST:
            {
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t pc_offset = sign_extend(instr & 0x1FF,9);
                mem_write(reg[R_PC] + pc_offset,reg[r0]);
            }
                break;
            case OP_STI:
            {
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t pc_offset = sign_extend(instr & 0x1FF,9);
                mem_write(mem_read(reg[R_PC] + pc_offset),reg[r0]);
            }
                break;
            case OP_STR:
            {
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t r1 = (instr >> 6) & 0x7;
                uint16_t offset = sign_extend(instr & 0x3F,6);
                mem_write(reg[r1] + offset,reg[r0]);
            }
                break;
            case OP_TRAP:
            {
                reg[R_R7] = reg[R_PC];
                switch(instr & 0xFF){
                    case TRAP_GETC:
                    {
                        // 读取一个ASCII字符
                        reg[R_R0] = (uint16_t)getchar();
                        update_flags(R_R0);
                    }
                        break;
                    case TRAP_OUT:
                    {
                        putc((char)reg[R_R0],stdout);
                        fflush(stdout);
                    }
                        break;
                    case TRAP_PUTS:
                    {
                        // 每个字符占一个字(16bits)
                        uint16_t *c = memory + reg[R_R0];
                        while(*c){
                            putc((char)*c,stdout);	//强制类型转换
                            ++c;
                        }
                        fflush(stdout);
                    }
                        break;
                    case TRAP_IN:
                    {
                        printf("Enter a character: ");
                        char c = getchar();
                        putc(c,stdout);
                        fflush(stdout);
                        reg[R_R0] = (uint16_t)c;
                        update_flags(R_R0);
                    }
                        break;
                    case TRAP_PUTSP:
                    {
                        //刚刚的字符串打印是适用于双字节单字符的情况，现在是单字节单字符的情况
                        uint16_t *c = memory + reg[R_R0];
                        while(*c){
                            char char1 = (*c) & 0xFF;
                            putc(char1,stdout);
                            char char2 = (*c) >> 8;
                            if(char2) putc(char2,stdout);
                            ++c;
                        }
                        fflush(stdout);
                    }
                        break;
                    case TRAP_HALT:
                    {
                        //刚刚的字符串打印是适用于双字节单字符的情况，现在是单字节单字符的情况
                        uint16_t *c = memory + reg[R_R0];
                        while(*c){
                            char char1 = (*c) & 0xFF;
                            putc(char1,stdout);
                            char char2 = (*c) >> 8;
                            if(char2) putc(char2,stdout);
                            ++c;
                        }
                        fflush(stdout);
                    }
                        break;
                }
            }
                break;
            case OP_RES:
            case OP_RTI:
            default:
                abort();
                break;
        }
    }
    restore_input_buffering();
}

```

至此为止，对LC-3的虚拟机仿真结束

