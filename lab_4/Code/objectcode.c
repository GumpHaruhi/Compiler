#include "objectcode.h"

const char* REG_NAME[32] = {
    "$0", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", 
    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", 
    "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", 
    "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra" };

Register regs[32];
Frame frameStack = NULL;
Frame currentFrame = NULL;

#ifdef DEBUG
void printVar(Variable var){
    if(var->op->kind == VARIABLE){
        printf("  | v%s\t| %d\t|\n", var->op->name, var->offset);
    } else if(var->op->kind == TEMP_VAR){
        printf("  | t%d\t| %d\t|\n", var->op->no, var->offset);
    }
}
#endif

void updateFramePointer(char* name){
    Frame ptr = frameStack;
    while(ptr != NULL){
        if(strcmp(ptr->name, name) == 0){
            currentFrame = ptr;
            return;
        }
        ptr = ptr->next;
    }
#ifdef DEBUG
    printf("Oops! failed to find Frame: %s\n", name);
#endif
    currentFrame = NULL;
}

void initRegisters(){
    // 初始化 32 个寄存器
    for(int i=0; i < 32; i++){
        Register reg = (Register)malloc(sizeof(Register_t));
        reg->no = i;
        reg->ocupy = false;
        reg->var = NULL;
        reg->last_use = 0;
        regs[i] = reg;
    }
}

void initFrames(InterCode ir){
    /**
     * 初始化所有可能出现的栈帧的信息
     * 添加所有变量，将中间代码代表的变量联系起来
     */
    InterCode code = ir;
    while(code != NULL){
        switch (code->kind)
        {
        case RETURN_IR:
        case DEC_IR:
        case ARG_IR:
        case CALL_IR:
        case PARAM_IR:
        case READ_IR:
        case WRITE_IR:
            if(code->ops[0]->kind == VARIABLE || code->ops[0]->kind == TEMP_VAR)
                transVariable(code->ops[0], frameStack);
            break;
        case ASSIGN_IR:
        case MEM_IR:
        case IF_GOTO_IR:
            if(code->ops[0]->kind == VARIABLE || code->ops[0]->kind == TEMP_VAR)
                transVariable(code->ops[0], frameStack);
            if(code->ops[1]->kind == VARIABLE || code->ops[1]->kind == TEMP_VAR)
                transVariable(code->ops[1], frameStack); 
            break;
        case PLUS_IR:
        case SUB_IR:
        case MULTI_IR:
        case DIV_IR:
            if(code->ops[0]->kind == VARIABLE || code->ops[0]->kind == TEMP_VAR)
                transVariable(code->ops[0], frameStack);
            if(code->ops[1]->kind == VARIABLE || code->ops[1]->kind == TEMP_VAR)
                transVariable(code->ops[1], frameStack);
            if(code->ops[2]->kind == VARIABLE || code->ops[2]->kind == TEMP_VAR)
                transVariable(code->ops[2], frameStack); 
            break;
        case FUNC_IR: {
            Frame f = (Frame)malloc(sizeof(Frame_t));
            strcpy(f->name, code->ops[0]->name);
            f->varlist = NULL;
            f->next = frameStack;
            frameStack = f;
#ifdef DEBUG
            printf("Frame ------ %s ------\n", f->name);
#endif      
            break;
        }
        default:
            break;
        }
        code = code->next;
    }
#ifdef DEBUG
    printf("-------------------------\n");
#endif  
}

Variable transVariable(Operand op, Frame frame){
    /**
     * 添加操作数 op 为栈帧 frame 的一个变量
     * 变量可能之前就存在 
     */
    if(frame == NULL || op == NULL) {
#ifdef DEBUG
        printf("Oops! transVariable return NULL\n");
#endif
        return NULL;
    }
    Variable ptr = frame->varlist;
    while(ptr != NULL){
        if(opEqual(ptr->op, op)){
            return ptr;
        }
        ptr = ptr->next;
    }
    // 新建一个变量
    Variable var = (Variable)malloc(sizeof(Variable_t));
    var->op = op;
    if(frame->varlist == NULL){
        var->offset = getSizeof(var->op->type);
    } else {
        var->offset = getSizeof(var->op->type) + frame->varlist->offset;
    }
    var->next = frame->varlist;
    frame->varlist = var;
#ifdef DEBUG
    printVar(var);
#endif
    return var;
}

bool opEqual(Operand op1, Operand op2){
    // 判断两个操作数是否相等
    if(op1 == NULL && op2 == NULL){
        return true;
    } else if(op1 == NULL || op2 == NULL){
        return false;
    } 
    if(op1->kind == op2->kind){
        switch(op1->kind)
        {
        case VARIABLE:
        case FUNC:
            return strcmp(op1->name, op2->name) == 0;
        case TEMP_VAR:
        case LABEL:
            return op1->no == op2->no;
        case CONSTANT:
            return op1->value == op2->value;
        case GET_ADDR:
        case GET_VAL:
            return opEqual(op1->opr, op2->opr);
        default:
            break;
        }
    }
    return false;
}

void initObjectCode(FILE* fp){
    // 写入必要的前置代码
    // 数据段标记
    fputs(".data\n", fp);
    // 输入提示语
    fputs("_prompt: .asciiz \"Enter an integer:\"\n", fp);
    // 换行符
    fputs("_ret: .asciiz \"\\n\"\n", fp);
    fputs(".globl main\n", fp);
    // 代码段标记
    fputs(".text\n", fp);

    // read函数目标代码
    fputs("read:\n", fp);
    // 打印输入提示语
    fputs("  li $v0, 4\n", fp);
    fputs("  la $a0, _prompt\n", fp);
    fputs("  syscall\n", fp);
    // 读入一个整型
    fputs("  li $v0, 5\n", fp);
    fputs("  syscall\n", fp);
    // 跳转返回地址
    fputs("  jr $ra\n", fp);
    fputs("\n", fp);

    // write函数目标代码
    fputs("write:\n", fp);
    fputs("  li $v0, 1\n", fp);
    fputs("  syscall\n", fp);
    fputs("  li $v0, 4\n", fp);
    fputs("  la $a0, _ret\n", fp);
    fputs("  syscall\n", fp);
    fputs("  move $v0, $0\n", fp);
    fputs("  jr $ra\n", fp);
}

void freeRegs(){
    for(int i=8; i <= 25; i++){
        regs[i]->ocupy = false;
    }
}

void clearRegs(){
    for(int i=8; i <= 25; i++){
        regs[i]->ocupy = false;
        regs[i]->var = NULL;
        regs[i]->last_use = 0;
    }
}

int handleOp(FILE* fp, Operand op, bool load){
    // 将操作数装在到一个寄存器中（t 或 s 系列）, 结果返回寄存器编号
    if(op->kind == CONSTANT){
        if(op->value == 0){
            return 0;
        }
        Variable var = transVariable(op, currentFrame);
        int regno = allocReg(fp, var);
        if(load){
            fprintf(fp, "  li %s, %d\n", REG_NAME[regno], op->value);
        }
        return regno;
    } else if(op->kind == VARIABLE || op->kind == TEMP_VAR){
        Variable var = transVariable(op, currentFrame);
        int regno = allocReg(fp, var);
        if(load){
            fprintf(fp, "  lw %s, %d($fp)\n", REG_NAME[regno], -var->offset);
        }
        return regno;
    } else if(op->kind == GET_VAL){
        int regno = handleOp(fp, op->opr, load);
        fprintf(fp, "  lw %s, 0(%s)\n", REG_NAME[regno], REG_NAME[regno]);
        return regno;
    } else if(op->kind == GET_ADDR){
        int regno = handleOp(fp, op->opr, load);
        Variable var = transVariable(op->opr, currentFrame);
        fprintf(fp, "  addi %s, $fp, %d\n", REG_NAME[regno], -var->offset);
        return regno;
    }
}

int allocReg(FILE* fp, Variable var){
    // 从 t 或 s 系列中分配一个寄存器
    for(int i = 8; i < 26; i++){
        if(!regs[i]->ocupy){
            regs[i]->ocupy = true;
            regs[i]->var = var;
            updateRound(i);
            return i;
        }
    }
    // 没有空闲的寄存器
    // 从“已被占有”的寄存器中分配“最久未被使用”的寄存器
    int maxround = 0, flag = -1;
    for(int i = 8; i < 26; i++){
        if(regs[i]->last_use > maxround){
            maxround = regs[i]->last_use;
            flag = i;
        }
    }
#ifdef DEBUG
    if(flag == -1){
        printf("Oops! failed to alloc register\n");
    }
#endif 
    // 将寄存器原来储存的值压栈
    fprintf(fp, "  sw %s, %d($fp)\n", REG_NAME[flag], -regs[flag]->var->offset);
    regs[flag]->ocupy = true;
    regs[flag]->var = var;
    updateRound(flag);
    return flag;
}

void updateRound(int no){
    for(int i = 8; i < 26; i++){
        regs[i]->last_use ++;
    }
    regs[no]->last_use = 0;
}

void pushRegsToStack(FILE* fp){
    // 将寄存器上的值压栈
    // 以当前 sp 为栈顶, 编号小的靠近栈顶
    fprintf(fp, "  addi $sp, $sp, -72\n");
    for(int i = 25; i >= 8; i--){
        fprintf(fp, "  sw %s, %d($sp)\n", REG_NAME[i], 4*(i-8));
    }
    fprintf(fp, "\n");
}

void popStackToRegs(FILE* fp){
    // 以当前 sp 为栈顶，恢复寄存器
    fprintf(fp, "\n");
    for(int i = 8; i <= 25; i++){
        fprintf(fp, "  lw %s, %d($sp)\n", REG_NAME[i], 4*(i-8));
    }
    fprintf(fp, "  addi $sp, $sp, 72\n\n");
}

// 翻译中间代码为 MIPS32 代码
void printObjectCode(char* filepath, InterCode ir){
    FILE* fp = fopen(filepath, "w");
    if (fp == NULL) {
        printf("Cannot open file %s", filepath);
        return;
    }

#ifdef DEBUG
    printf("Start init\n");
#endif
    // 在翻译前需要初始化，扫描一遍IR代码，获取栈帧信息
    initRegisters();
    initFrames(ir);
    initObjectCode(fp);
#ifdef DEBUG
    printf("Finish init\n");
#endif

    InterCode code = ir;
    while(code != NULL){
        switch (code->kind)
        {
        case CALL_IR: {
            /**
             * 如果要保存现场，在这里由调用者将寄存器压栈
             * 但是这并不必要，原因之后阐述
             */
            // pushRegsToStack(fp);

            // 处理参数
            InterCode arg = code->prev;
            int argCount = 0;
            int arg_reg[18];
            while(arg != NULL && arg->kind == ARG_IR){
                argCount ++;
                int regno = handleOp(fp, arg->ops[0], true);
                if(argCount <= 4){
                    // 前四个参数放在 $a
                    fprintf(fp, "  move %s, %s\n", REG_NAME[argCount+3], REG_NAME[regno]);
                } else {
                    // 其他参数压栈
                    // 为了使小端靠近栈顶，先将储存参数的寄存器编号记录下来，之后再压栈
                    arg_reg[argCount-5] = regno;
                }
                arg = arg->prev;
            }
            if(argCount > 4){
                fprintf(fp, "  addi $sp, $sp, %d\n", -4*(argCount-4));
            }
            for(int i = argCount-5; i >= 0; i--){
                fprintf(fp, "  sw %s, %d($sp)\n", REG_NAME[arg_reg[i]], i*4);
            }
            fprintf(fp, "\n");
            // 维护全局栈帧指针
            char caller[32];
            strcpy(caller, currentFrame->name);

            // 调用函数，保存 $ra
            fprintf(fp, "  addi $sp, $sp, -4\n");
            fprintf(fp, "  sw $ra, 0($sp)\n");
            fprintf(fp, "  jal %s\n", code->ops[1]->name);
            fprintf(fp, "  lw $ra, 0($sp)\n");
            fprintf(fp, "  addi $sp, $sp, 4\n");
            
            updateFramePointer(caller);
            if(argCount > 4){
                fprintf(fp, "  addi $sp, $sp, %d\n", (argCount - 4)*4);
            }
            // 恢复寄存器
            // popStackToRegs(fp);
            
            // 处理函数返回值
            Operand retop = code->ops[0];
            if(retop->kind == VARIABLE || retop->kind == TEMP_VAR){
                Variable var = transVariable(retop, currentFrame);
                fprintf(fp, "  sw $v0, %d($fp)\n", -var->offset);
            } else if(retop->kind == GET_VAL){
                int regno = handleOp(fp, retop->opr, true);
                fprintf(fp, "  sw $v0, 0(%s)\n", REG_NAME[regno]);
            }
            break;
        }
        case FUNC_IR: {
            updateFramePointer(code->ops[0]->name);
            fprintf(fp, "%s: \n", code->ops[0]->name);
            // 储存 $fp 旧值，更新 $fp
            fprintf(fp, "  addi $sp, $sp, -4\n");
            fprintf(fp, "  sw $fp, 0($sp)\n");
            fprintf(fp, "  move $fp, $sp\n");
            // 为变量预留空间
            fprintf(fp, "  addi $sp, $sp, %d\n", -currentFrame->varlist->offset);
            clearRegs();
            // 处理参数
            InterCode param = code->next;
            int paramCount = 0;
            while(param != NULL && param->kind == PARAM_IR){
                paramCount ++;
                Variable var = transVariable(param->ops[0], currentFrame);
                if(paramCount <= 4){
                    fprintf(fp, "  sw %s, %d($fp)\n", REG_NAME[paramCount+3], -var->offset);
                } else {
                    fprintf(fp, "  lw $t0, %d($fp)\n", 4*(paramCount-4) +4);
                    fprintf(fp, "  sw $t0, %d($fp)\n", -var->offset);
                }
                param = param->next;
            }
            break;
        }
        case RETURN_IR: {
            // 保存返回值
            Operand retop = code->ops[0];
            if(retop->kind == VARIABLE 
            || retop->kind == TEMP_VAR 
            || retop->kind == CONSTANT){
                int regno = handleOp(fp, retop, true);
                fprintf(fp, "  move $v0, %s\n", REG_NAME[regno]);
            } else if(retop->kind == GET_VAL){
                int regno = handleOp(fp, retop->opr, true);
                fprintf(fp, "  lw $v0, 0(%s)\n", REG_NAME[regno]);
            }
            // 恢复调用者的旧值 $sp $fp
            fprintf(fp, "  move $sp, $fp\n");
            fprintf(fp, "  lw $fp, 0($fp)\n");
            fprintf(fp, "  addi $sp, $sp, 4\n");
            fprintf(fp, "  jr $ra\n");
            break;
        }
        case READ_IR: {
            // read 和 write 未使用 t s 寄存器
            fprintf(fp, "  addi $sp, $sp, -4\n");
            fprintf(fp, "  sw $ra, 0($sp)\n");
            fprintf(fp, "  jal read\n");
            fprintf(fp, "  lw $ra, 0($sp)\n");
            fprintf(fp, "  addi $sp, $sp, 4\n");
            Operand retop = code->ops[0];
            if(retop->kind == VARIABLE || retop->kind == TEMP_VAR){
                int regno = handleOp(fp, retop, false);
                fprintf(fp, "  move %s, $v0\n", REG_NAME[regno]);
                fprintf(fp, "  sw %s, %d($fp)\n", REG_NAME[regno], -regs[regno]->var->offset);
            } else if(retop->kind == GET_VAL){
                int regno = handleOp(fp, retop->opr, true);
                fprintf(fp, "  sw $v0, 0(%s)\n", REG_NAME[regno]);
            }
            break;
        }
        case WRITE_IR: {
            int regno = handleOp(fp, code->ops[0], true);
            fprintf(fp, "  move $a0, %s\n", REG_NAME[regno]);
            fprintf(fp, "  addi $sp, $sp, -4\n");
            fprintf(fp, "  sw $ra, 0($sp)\n");
            fprintf(fp, "  jal write\n");
            fprintf(fp, "  lw $ra, 0($sp)\n");
            fprintf(fp, "  addi $sp, $sp, 4\n");
            break;
        }

        case PARAM_IR:
        case ARG_IR:
        case DEC_IR:
            break;

        case ASSIGN_IR: {
            Operand left = code->ops[0];
            int rightreg = handleOp(fp, code->ops[1], true);
            if(left->kind == VARIABLE || left->kind == TEMP_VAR){
                Variable leftvar = transVariable(left, currentFrame);
                fprintf(fp, "  sw %s, %d($fp)\n", REG_NAME[rightreg], -leftvar->offset);
            } else if(left->kind == GET_VAL){
                int leftreg = handleOp(fp, left->opr, true);
                fprintf(fp, "  sw %s, 0(%s)\n", REG_NAME[rightreg], REG_NAME[leftreg]);
            }
            break;
        }
        case PLUS_IR: {
            Operand left = code->ops[0];
            int reg1 = handleOp(fp, code->ops[1], true);
            int reg2 = handleOp(fp, code->ops[2], true);
            if(left->kind == VARIABLE || left->kind == TEMP_VAR){
                int reg0 = handleOp(fp, left, false);
                fprintf(fp, "  add %s, %s, %s\n", REG_NAME[reg0], REG_NAME[reg1], REG_NAME[reg2]);
                fprintf(fp, "  sw %s, %d($fp)\n", REG_NAME[reg0], -regs[reg0]->var->offset);
            } else if(left->kind == GET_VAL){
                int reg0 = handleOp(fp, left->opr, true);
                fprintf(fp, "  add %s, %s, %s\n", REG_NAME[reg1], REG_NAME[reg1], REG_NAME[reg2]);
                fprintf(fp, "  sw %s, 0(%s)\n", REG_NAME[reg1], REG_NAME[reg0]);
            }
            break;
        }
        case SUB_IR: {
            Operand left = code->ops[0];
            int reg1 = handleOp(fp, code->ops[1], true);
            int reg2 = handleOp(fp, code->ops[2], true);
            if(left->kind == VARIABLE || left->kind == TEMP_VAR){
                int reg0 = handleOp(fp, left, false);
                fprintf(fp, "  sub %s, %s, %s\n", REG_NAME[reg0], REG_NAME[reg1], REG_NAME[reg2]);
                fprintf(fp, "  sw %s, %d($fp)\n", REG_NAME[reg0], -regs[reg0]->var->offset);
            } else if(left->kind == GET_VAL){
                int reg0 = handleOp(fp, left->opr, true);
                fprintf(fp, "  sub %s, %s, %s\n", REG_NAME[reg1], REG_NAME[reg1], REG_NAME[reg2]);
                fprintf(fp, "  sw %s, 0(%s)\n", REG_NAME[reg1], REG_NAME[reg0]);
            }
            break;
        }
        case MULTI_IR: {
            Operand left = code->ops[0];
            int reg1 = handleOp(fp, code->ops[1], true);
            int reg2 = handleOp(fp, code->ops[2], true);
            if(left->kind == VARIABLE || left->kind == TEMP_VAR){
                int reg0 = handleOp(fp, left, false);
                fprintf(fp, "  mul %s, %s, %s\n", REG_NAME[reg0], REG_NAME[reg1], REG_NAME[reg2]);
                fprintf(fp, "  sw %s, %d($fp)\n", REG_NAME[reg0], -regs[reg0]->var->offset);
            } else if(left->kind == GET_VAL){
                int reg0 = handleOp(fp, left->opr, true);
                fprintf(fp, "  mul %s, %s, %s\n", REG_NAME[reg1], REG_NAME[reg1], REG_NAME[reg2]);
                fprintf(fp, "  sw %s, 0(%s)\n", REG_NAME[reg1], REG_NAME[reg0]);
            }
            break;
        }
        case DIV_IR: {
            Operand left = code->ops[0];
            int reg1 = handleOp(fp, code->ops[1], true);
            int reg2 = handleOp(fp, code->ops[2], true);
            if(left->kind == VARIABLE || left->kind == TEMP_VAR){
                int reg0 = handleOp(fp, left, false);
                fprintf(fp, "  div %s, %s\n", REG_NAME[reg1], REG_NAME[reg2]);
                fprintf(fp, "  mflo %s\n", REG_NAME[reg0]);
                fprintf(fp, "  sw %s, %d($fp)\n", REG_NAME[reg0], -regs[reg0]->var->offset);
            } else if(left->kind == GET_VAL){
                int reg3 = handleOp(fp, left->opr, false);
                fprintf(fp, "  div %s, %s\n", REG_NAME[reg1], REG_NAME[reg2]);
                fprintf(fp, "  mflo %s\n", REG_NAME[reg3]);
                int reg0 = handleOp(fp, left->opr, true);
                fprintf(fp, "  sw %s, 0(%s)\n", REG_NAME[reg3], REG_NAME[reg0]);
            }
            break;
        }
        case MEM_IR: {
            int reg0 = handleOp(fp, code->ops[0], true);
            int reg1 = handleOp(fp, code->ops[1], true);
            fprintf(fp, "  sw %s, 0(%s)\n", REG_NAME[reg1], REG_NAME[reg0]);
            break;
        }
        case LABEL_IR:
            fprintf(fp, "label%d:\n", code->ops[0]->no);
            break;
        case GOTO_IR:
            fprintf(fp, "  j label%d\n", code->ops[0]->no);
            break;
        case IF_GOTO_IR: {
            int reg0 = handleOp(fp, code->ops[0], true);
            int reg1 = handleOp(fp, code->ops[1], true);
            int label = code->ops[2]->no;
            if(strcmp(code->relop, "==") == 0){
                fprintf(fp, "  beq %s, %s, label%d\n", REG_NAME[reg0], REG_NAME[reg1], label);
            } else if(strcmp(code->relop, "!=") == 0){
                fprintf(fp, "  bne %s, %s, label%d\n", REG_NAME[reg0], REG_NAME[reg1], label);
            } else if(strcmp(code->relop, ">") == 0){
                fprintf(fp, "  bgt %s, %s, label%d\n", REG_NAME[reg0], REG_NAME[reg1], label);
            } else if(strcmp(code->relop, "<") == 0){
                fprintf(fp, "  blt %s, %s, label%d\n", REG_NAME[reg0], REG_NAME[reg1], label);
            } else if(strcmp(code->relop, ">=") == 0){
                fprintf(fp, "  bge %s, %s, label%d\n", REG_NAME[reg0], REG_NAME[reg1], label);
            } else if(strcmp(code->relop, "<=") == 0){
                fprintf(fp, "  ble %s, %s, label%d\n", REG_NAME[reg0], REG_NAME[reg1], label);
            }
            break;
        }

        default:
            break;
        }

        fputs("\n", fp);   // 方便阅读
        fflush(fp);
        code = code->next;
        /**
         * 这里采取一种偷懒的策略：每执行一次完整的操作，就及时的将寄存器
         * 中的值写回栈中。这样会造成一些不必要的、频繁的内存读写，但是不必
         * 考虑寄存器中值的有效性、栈帧切换时的现场保存
         */
        freeRegs();
    }

    fclose(fp);
}