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
