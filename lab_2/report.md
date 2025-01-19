# LAB_2 语义分析



**Gump**

------------------------------

### 实验内容&程序功能

本次实验基于 LAB_1 的词法分析与语法分析所构造的语法树，进行语义分析。

**语法树**：在词法分析与语法分析阶段，依照 `syntax.y` 中的语法结构所构建

**语义分析**

C头文件和源文件 `semantic.h`  `semantic.c` ，提供接口：`semantic_transfer()` ，接受语法树的根节点，对其进行语义分析

语义分析的主逻辑：

```c
void semantic_transfer(Node root){
    init_symbol_table();  // 初始化符号表
    Program(root);  // 语义分析
    check_func_def();  // 检查仅声明未定义的函数
    freeSymbolTable();  // 释放内存
}
```

**符号表**

在语义分析阶段，逐步构建符号表。符号表之间使用“名等价”

- **数据结构**：使用哈希表，链表处理冲突的数据结构

```c
struct Symbol_{
    char name[32];  // 名称
    Type type;  // 数据类型
    int lineno;  // 所在行号
    Symbol next;  // 哈希表处理冲突
};
```

- **数据类型**

设计如下几种数据类型。考虑到结构体类型拥有成员、函数类型拥有参数，设计链表结构的“域”：

```c
struct Type_{
    enum {
        BASIC,  // 整型或浮点数
        ARRAY,  // 数组
        STRUCTURE,  // 结构体变量
        STRUCT_DEF,  // 结构体的定义
        FUNCTION  // 函数
    } kind;
    union {
        int basic;
        struct{
            Type elem;
            int dimension;
        } array;
        Structure structure;
        Function func;
    };
};
```

**亮点设计：不同顶层的底层分析区别对待**

在分析表达式即变量时，不同的顶层情景下任务目标不同：

普通的变量需要避免名冲突。结构体的成员不仅需要检测到名冲突，还要判断是否是同一结构体内的冲突。函数的参数则不在乎名冲突，而是关注类型等价（形参与实参）。

但是 `FunDec` `StructSpecifier` 和普通变量的底层语法分析是相同的结构：`VarDec` 。在底层分析时，必须根据顶层的情景不同，区别分析。向下传递顶层情景：

```c
typedef enum {
    IN_STRUCT,  // 在结构体内
    IN_COMPST,  // 一般
    IN_FUNC_DEC  // 函数的参数
} Context ;
// 将 context 作为参数传递给底层
FieldList VarDec(Node node, Context ctx, Type type);
```

遇到重名冲突时的不同策略：

- 普通 `Specifier` ：即刻报错，并舍弃当前词素
- 结构体成员：可以报错（也可以不报），但是保留当前域，传递给上层，在上层检测是否为同一结构体内的重名
- 函数参数：不报错，保留当前域，交由上层判断

```c
void check_field_in_one_struct(FieldList head){
    // head 是一个结构体定义的成员的链表
    // 此函数用于检测是否存在结构体内部的重名，即链表内的元素重名
    // 若是结构体内部重名，则报出错误类型 15
    // 将一切重名成员剔除
}
void upload_func_params(FieldList param, FieldList mode){
    // 函数的参数分析阶段，一切参数域都未更新质符号表，在此处更新
    // 载入函数的形参。param 是待分析的参数，mode 是可能存在的形参（可能是 NULL）
    // 若 param 中有重名，但是与 mode 中的重名，则不报错
    // 将不存在重名的参数添加到符号表
}
```

**亮点设计：多维数组的构建与类型传递**

多维数组构建：考虑以下情形，假如存在多维数组

```c
int arr[6][9];
// 其底层语法树结构
VarDec
    > VarDec LB INT RB
    	> VarDec LB INT RB
    		> ID
// 因此在函数 VarDec 中假如参数 layer：标记当前嵌套层数，0层为其他类型，多层则是数组类型
FieldList VarDec(Node node, Context ctx, Type type, int layer){ ... }
```

多维数组的类型传递：考虑以下几种 `Exp` ，以及它们对应的返回类型

```c
int arr[6][9];
arr[1][2]  =>  int
arr[1]     =>  array {elem=int, dimention=1}
arr        =>  array {elem=int, dimention=2}
```

因此在 `Exp` 函数中如下处理：

```c
Type Exp(Node node){
    ...
    if(/* EXP LB EXP RB */){
    	Type base = Exp(node->children[0]);
        if(base->array.dimension == 1){
            // 当前是一维数组，则返回元素的类型
            return base->array.elem;
        } else {
            // 当前是多维数组，返回低一维数组
            type->kind = ARRAY;
            type->array.dimension = base->array.dimension - 1;
            type->array.elem = base->array.elem;
            return type;
        }   
    }
}
```



### 运行环境&程序编译

- OS: Ubuntu 22.04

- gcc, flex, bsion

- 实验提供的 Makefile 即可： `make`

  或者手动编译：

  ```
  bison -d syntax.y
  flex lexical.l
  gcc main.c syntax.tab.c tree.c semantic.c -lfl -o parser
  ```

