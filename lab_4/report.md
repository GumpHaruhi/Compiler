# LAB_4: 目标代码生成



**Gump**

------------------------------

### 栈帧的实现

在将中间代码翻译为 MIPS32 目标代码的过程中，最核心的是关于栈帧的维护：

- 程序需要知道当前翻译进度所处的栈帧
- 需要知道每一个变量、临时变量等在栈空间中，对当前栈帧指针 $fp 的偏移量
- 在栈帧初始化时，需要知道使用的变量的大小（类型），从而知道栈顶 $sp 的偏移量
- 需要知道可使用的寄存器（t 系列和 s 系列）的状态

因此程序采用**两遍扫描、预先构建栈帧**的策略：扫描遍历中间代码两次，第一次用于记录、初始化所有可能出现的栈帧的信息，包括栈帧中的变量的初始化（相对于 $fp 的偏移量），第二遍遍历翻译为 MIPS 32 码：

```c
struct Register_{
    int no;             // 寄存器的编号
    bool ocupy;         // 是否被占用
    Variable var;       // 寄存器储存的变量
    int last_use;       // 距离上次使用的间隔
};
struct Variable_{
    Operand op;         // 代表的操作数
    int offset;         // 距离所在栈帧的 fp 指针的偏移量
};
struct Frame_{
    char name[32];     // 栈帧的名称 = 函数的名称
    Variable varlist;   // 栈帧的变量列表
};

// 计算变量的偏移量
if(frame->varlist == NULL)
    var->offset = getSizeof(var->op->type);
else
    var->offset = getSizeof(var->op->type) + frame->varlist->offset;
```

在首次扫描之后，栈帧与变量的信息都被记录。比如下面的源代码：

```c
// cmm
int fact(int n){
    if(n == 1)
        return n;
    else
        return (n * fact(n-1));
}
int main(){
    int a[8];
    int m, result;
    m = read();
    if(m > 1)
        result = fact(m);
    else
        result =1;
    write(result);
    return 0;
}

// 初始化后的栈帧信息
Frame ------ fact ------
  | vn  | 4     |
  | t7  | 8     |
  | t6  | 12    |
  | t4  | 16    |
Frame ------ main ------
  | va  | 32    |
  | t12 | 4     |
  | vm  | 8     |
  | t17 | 12    |
  | vresult | 16    |
-------------------------
```

### 寄存器的保存

统一由调用者保存，在翻译 CALL 语句时，由调用者在其栈上划分空间保存寄存器的值，调用函数返回后再恢复

```c
// 保存寄存器
pushRegsToStack(fp);
// 处理参数
// 维护全局栈帧指针
// 调用函数，保存 $ra
fprintf(fp, "  addi $sp, $sp, -4\n");
fprintf(fp, "  sw $ra, 0($sp)\n");
fprintf(fp, "  jal %s\n", code->ops[1]->name);
fprintf(fp, "  lw $ra, 0($sp)\n");
fprintf(fp, "  addi $sp, $sp, 4\n");
updateFramePointer(caller);
if(argCount > 4){
// 回收参数空间
}
// 恢复寄存器
popStackToRegs(fp);
```

### 寄存器的分配

采用分配**最近最少使用（LRU）**的策略：

- 若有空闲的寄存器，则分配
- 若没有，则选择最近最少使用的寄存器，将原来的值压栈，并分配

```c
int maxround = 0, flag = -1;
for(int i = 8; i < 26; i++){
    if(regs[i]->last_use > maxround){
       maxround = regs[i]->last_use;
       flag = i;
    }
}
// 将寄存器原来储存的值压栈
fprintf(fp, "  sw %s, %d($fp)\n", REG_NAME[flag], -regs[flag]->var->offset);
regs[flag]->ocupy = true;
regs[flag]->var = var;
updateRound(flag);
return flag;
```



### 运行程序

- Ubuntu 22.04
- 在 `/Code/` 路径下执行指令 `make` 进行编译
- 编译后，使用如下指令翻译源码
  `./parser ../Test/src/src.cmm ../Test/obj/obj.s`
  如果希望同时生成中间代码：
   `./parser ../Test/src/name.cmm ../Test/obj/name.s ../Test/ir/name.ir`
