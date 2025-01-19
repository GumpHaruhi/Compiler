## lab_3 中间代码生成



**Gump**

----------------------

### 设计介绍

设计中包含两个基础的结构体类：操作数和中间代码句，其中中间代码句以双向链表的形式存储

```c
struct Operand_ {
    enum {
        VARIABLE, TEMP_VAR, CONSTANT,
        GET_ADDR, GET_VAL, LABEL, FUNC
    } kind ;
    union { int no; int value; char name[32]; Operand opr; };
    Type type;
    bool is_addr;
};

struct InterCode_ { 
    enum {
        LABEL_IR, FUNC_IR, ASSIGN_IR, PLUS_IR, SUB_IR,
        MULTI_IR, DIV_IR, MEM_IR, GOTO_IR, IF_GOTO_IR,
        RETURN_IR, DEC_IR, ARG_IR, CALL_IR, PARAM_IR,
        READ_IR, WRITE_IR, NULL_IR
    } kind ;
    Operand ops[3];
    union { char relop[4]; int size; };
    InterCode prev;
    InterCode next;
};
```

在我的设计中，引用与解引用、地址与取地址是操作数的一个属性，因此将会存在下面的情况：

```
*v1 := *t1 + *t2
```

同时也设计了 `formatCode(InterCode code)` 函数用于转换此种类型语句。转换效果：

```
t3 := *t1
t4 := *t2
*v1 := t3 + t4
```

为了能够做到这一点，需要修改 lab_2 中语法单元的结构体 Symbol, 在其中添加一个`Operand`属性：`sym->op`

与实验二不同，实验三要求不仅记录多维数组的维数、元素类型，还要记录数组每一维的大小。添加 size 属性，同时更改翻译数组的代码：

```c
FieldList translate_VarDec(Node node, Type type, Type arr_type, int layer){
    if(node->childnum == 1){
        // ID
        char* id = node->children[0]->value.str;
        FieldList variable = (FieldList)malloc(sizeof(FieldList_t));
        strcpy(variable->name, id);
        if(layer == 0)
            variable->type = type;
        else 
            variable->type = arr_type;
        return variable;
    } else if(layer == 0){
        arr_type = (Type)malloc(sizeof(Type_t));
        arr_type->kind = ARRAY;
        arr_type->array.elem = type;
        arr_type->array.dimension = 1;
        arr_type->array.size[layer] = node->children[2]->value.ival;
        return translate_VarDec(node->children[0], type, arr_type, layer+1);
    } else {
        arr_type->array.dimension ++;
        arr_type->array.size[layer] = node->children[2]->value.ival;
        return translate_VarDec(node->children[0], type, arr_type, layer+1);
    }
}
```

在翻译 Exp 的过程中，对于数组类型、结构体特定域，一律采用返回地址原则，并且只有在偏移量不是 0 的情况下生成对应的中间代码（优化）。

翻译 `Args` 语句时，必须做到：参数 exp 表达式必须在前面，一个函数调用的 ARG 语句中间不能被打断：

```c
// 比如参数列表： (int a, int b, int c)
// 必须翻译为： (expa,expb,expc)->arg c->arg b->arg a ， 其中 exp 的顺序随意
// 设计 Args 代码翻译函数：

InterCode translate_Args(Node node){
    Operand tmp = newTemp();
    InterCode code1 = translate_Exp(node->children[0], tmp);
    if(node->childnum == 3){
        InterCode code2 = translate_Args(node->children[2]);
        insertCode(code1, code2);
    }
    InterCode code3 = NULLCODE();
    code3->kind = ARG_IR;
    // 如果是结构体或数组的类型的值，需要传递它们的地址
    if(tmp->type != NULL && !tmp->is_addr && 
    (tmp->type->kind == STRUCTURE || tmp->type->kind == ARRAY)){
        Operand mid = newOperand();
        mid->is_addr = true;
        if(tmp->kind == GET_VAL){
            copyOperand(mid, tmp->opr);
        } else {
            mid->kind = GET_ADDR;
            mid->opr = tmp;
            mid->type = tmp->type;
        }
        code3->ops[0] = mid;
    } else {
        code3->ops[0] = tmp;
    }
    insertCode(code1, code3);
    return code1;
}
```



--------------------------

- OS: Ubuntu 22.04

- gcc 11

- 在 Code 目录下执行 `make` 即可编译，所用 Makefile 文件即提供的文件

- 执行下面命令来运行程序

  ```
  cd Code
  ./parser /path/to/input/file /path/to/output/file.ir
  ```

  

