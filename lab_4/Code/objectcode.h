#ifndef OBJECTCODE_H
#define OBJECTCODE_H

#include "intercode.h"

typedef struct Register_ Register_t;
typedef Register_t* Register;
typedef struct Variable_ Variable_t;
typedef Variable_t* Variable;
typedef struct Frame_ Frame_t;
typedef Frame_t* Frame;

struct Register_{
    int no;             // 寄存器的编号
    bool ocupy;         // 是否被占用
    Variable var;       // 寄存器储存的变量
    int last_use;       // 距离上次使用的间隔
};

struct Variable_{
    Operand op;         // 代表的操作数
    int offset;         // 距离所在栈帧的 fp 指针的偏移量
    Variable next;
};

struct Frame_{
    char name[32];     // 栈帧的名称 = 函数的名称
    Variable varlist;   // 栈帧的变量列表
    Frame next;
};

void printObjectCode(char* filepath, InterCode ir);
void initRegisters();
void initFrames(InterCode ir);
void initObjectCode(FILE* fp);
Variable transVariable(Operand op, Frame frame);
bool opEqual(Operand op1, Operand op2);
void freeRegs();
void clearRegs();
void pushRegsToStack(FILE* fp);
void popStackToRegs(FILE* fp);
int handleOp(FILE* fp, Operand op, bool load);
int allocReg(FILE* fp, Variable var);
void updateRound(int no);
void updateFramePointer(char* name);

#endif