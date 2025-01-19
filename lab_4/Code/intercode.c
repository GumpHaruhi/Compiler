#include "intercode.h"
#include <assert.h>

extern Symbol symbol_table[TABLE_SIZE];

int tempVarNO = 1;
int labelNO = 1;

Operand newOperand(){
    Operand op = (Operand)malloc(sizeof(Operand_t));
    op->type = NULL;
    op->is_addr = false;
    return op;
}
Operand newTemp(){
    Operand op = newOperand();
    op->kind = TEMP_VAR;
    op->no = tempVarNO ++;
    return op;
}
Operand newVar(char* name){
    Operand op = newOperand();
    op->kind = VARIABLE;
    strcpy(op->name, name);
    return op;
}
Operand newLabel(){
    Operand op = newOperand();
    op->kind = LABEL;
    op->no = labelNO ++;
    return op;
}
Operand newConst(int val){
    Operand op = newOperand();
    op->kind = CONSTANT;
    op->value = val;
    return op;
}

InterCode NULLCODE(){
    InterCode nullcode = (InterCode)malloc(sizeof(InterCode_t));
    nullcode->kind = NULL_IR;
    nullcode->prev = NULL;
    nullcode->next = NULL;
    return nullcode;
}

int getSizeof(Type type){
    if(type == NULL || type->kind == BASIC){
        return 4;
    } else if(type->kind == ARRAY){
        int size = 1;
        for(int i=0; i < type->array.dimension; i++){
            size = size * type->array.size[i];
        }
        size = size * getSizeof(type->array.elem);
        return size;
    } else if(type->kind == STRUCTURE || type->kind == STRUCT_DEF){
        int size = 0;
        FieldList ptr = type->structure->members;
        while(ptr != NULL){
            size += getSizeof(ptr->type);
            ptr = ptr->next;
        }
        return size;
    } else {
        printf("Oops! get size of a function\n");
        return 4;
    }
}

Operand getOperand(char* name){
    Symbol sym = search_symbol(name);
    assert(sym != NULL);
    if(sym->op != NULL){
        // 函数的参数已经创建了操作数类型，并且 is_addr = true
        return sym->op;
    }
    sym->op = newVar(name);
    sym->op->type = sym->type;
    return sym->op;
}

void copyOperand(Operand dst, Operand src){
    dst->kind = src->kind;
    dst->type = src->type;
    dst->is_addr = src->is_addr;
    
    if(dst->kind == TEMP_VAR || dst->kind == LABEL){
        dst->no = src->no;
    } else if(dst->kind == CONSTANT){
        dst->value = src->value;
    } else if(dst->kind == FUNC || dst->kind == VARIABLE){
        strcpy(dst->name, src->name);
    } else {
        dst->opr = src->opr;
    }
}

void insertCode(InterCode dst, InterCode src){
    // add src behind dst
    InterCode ptr = dst;
    while(ptr->next) { ptr = ptr->next; }
    ptr->next = src;
    src->prev = ptr;
}

InterCode generate_IR(Node program){
    init_symbol_table(symbol_table);

    // init read & write
    Symbol read_sym = search_symbol("read");
    Symbol write_sym = search_symbol("write");
    Operand read_op = newOperand();
    Operand write_op = newOperand();
    read_op->kind = FUNC; write_op->kind = FUNC;
    read_op->type = read_sym->type;
    write_op->type = write_sym->type;
    strcpy(read_op->name, "read");
    strcpy(write_op->name, "write");
    read_sym->op = read_op; write_sym->op = write_op;
    
    InterCode code = translate_ExtDefList(program->children[0]);
    // 格式化中间代码
    formatCode(code);
    return code;
}

InterCode translate_ExtDefList(Node node){
    if(node->kind == SYN_NULL){
        return NULLCODE();
    }
    InterCode code1 = translate_ExtDef(node->children[0]);
    InterCode code2 = translate_ExtDefList(node->children[1]);
    insertCode(code1, code2);
    return code1;
}

InterCode translate_ExtDef(Node node){
    Type type = Specifier(node->children[0]);
    if(strcmp(node->children[1]->name, "FunDec") != 0){
        // 由于不考虑全局变量，因此直接返回
        // 若引入全局变量，则需要在这里修改
        // 函数声明不需要生成什么中间代码
        return NULLCODE();
    }
    // 是函数定义
    Function func = FunDec(node->children[1]);
    func->ret_type = type;
    func->defined = true;
    // 加入到符号表
    Type ftype = (Type)malloc(sizeof(Type_t));
    ftype->kind = FUNCTION;
    ftype->func = func;
    Symbol sym = create_symbol(func->name, ftype, 0);
    // 翻译函数名
    Operand place = newOperand();
    place->kind = FUNC;
    strcpy(place->name, sym->name);
    place->type = sym->type;
    InterCode code1 = translate_Function(func, place);
    sym->op = place;
    insert_symbol(sym);

    InterCode code2 = translate_CompSt(node->children[2]);
    insertCode(code1, code2);
    return code1;
}

InterCode translate_Function(Function func, Operand place){
    // 函数名
    InterCode code1 = NULLCODE();
    code1->kind = FUNC_IR;
    code1->ops[0] = place;
    // 参数
    if(func->param != NULL){
        InterCode code2 = translate_ParamList(func->param);
        insertCode(code1, code2);
    }
    return code1;
}

InterCode translate_ParamList(FieldList param){
    // 参数按照正序排列
    // 初始化操作数
    Operand var = newVar(param->name);
    var->type = param->type;
    if(var->type->kind == STRUCTURE || var->type->kind == ARRAY){
        var->is_addr = true;
    }
    // 加入符号表
    Symbol sym = create_symbol(param->name, param->type, 0);
    sym->op = var;
    insert_symbol(sym);
    // 生成中间代码
    InterCode code1 = NULLCODE();
    code1->kind = PARAM_IR;
    code1->ops[0] = var;

    if(param->next != NULL){
        InterCode code2 = translate_ParamList(param->next);
        insertCode(code1, code2);
    }
    return code1;
}

InterCode translate_CompSt(Node node){
    InterCode code1 = translate_DefList(node->children[1]);
    InterCode code2 = translate_StmtList(node->children[2]);
    insertCode(code1, code2);
    return code1;
}

InterCode translate_DefList(Node node){
    if(node->kind == SYN_NULL){
        return NULLCODE();
    }
    InterCode code1 = translate_Def(node->children[0]);
    InterCode code2 = translate_DefList(node->children[1]);
    insertCode(code1, code2);
    return code1;
}

InterCode translate_Def(Node node){
    Type type = Specifier(node->children[0]);
    return translate_DecList(node->children[1], type);
}

InterCode translate_DecList(Node node, Type type){
    InterCode code1 = translate_Dec(node->children[0], type);
    if(node->childnum == 3){
        InterCode code2 = translate_DecList(node->children[2], type);
        insertCode(code1, code2);
    }
    return code1;
}

InterCode translate_Dec(Node node, Type type){
    FieldList variable = translate_VarDec(node->children[0], type, NULL, 0);
    // update symbol table
    Symbol sym = create_symbol(variable->name, variable->type, 0);
    insert_symbol(sym);

    InterCode code1 = NULLCODE();
    // intercode: DEC
    if(sym->type->kind == ARRAY || sym->type->kind == STRUCTURE){
        Operand var = newVar(sym->name);
        var->type = sym->type;
        sym->op = var;

        code1->kind = DEC_IR;
        code1->ops[0] = var;
        code1->size = getSizeof(var->type);
    }
    // VarDec = Exp
    if(node->childnum == 3){
        Operand var = getOperand(sym->name);
        sym->op = var;
        InterCode code2 = NULLCODE();
        code2->kind = ASSIGN_IR;
        code2->ops[0] = var;
        Operand place = newTemp();
        InterCode code3 = translate_Exp(node->children[2], place);
        code2->ops[1] = place;
        insertCode(code1, code3);
        insertCode(code1, code2);
    }
    return code1;
}

FieldList translate_VarDec(Node node, Type type, Type arr_type, int layer){
    if(node->childnum == 1){
        // ID
        char* id = node->children[0]->value.str;
        FieldList variable = (FieldList)malloc(sizeof(FieldList_t));
        strcpy(variable->name, id);
        if(layer == 0){
            variable->type = type;
        } else {
            variable->type = arr_type;
        }
        return variable;
    }
    else if(layer == 0){
        arr_type = (Type)malloc(sizeof(Type_t));
        arr_type->kind = ARRAY;
        arr_type->array.elem = type;
        arr_type->array.dimension = 1;
        arr_type->array.size[layer] = node->children[2]->value.ival;
        return translate_VarDec(node->children[0], type, arr_type, layer+1);
    }
    else {
        arr_type->array.dimension ++;
        arr_type->array.size[layer] = node->children[2]->value.ival;
        return translate_VarDec(node->children[0], type, arr_type, layer+1);
    }
}

InterCode translate_StmtList(Node node){
    if(node->kind == SYN_NULL){
        return NULLCODE();
    }
    InterCode code1 = translate_Stmt(node->children[0]);
    InterCode code2 = translate_StmtList(node->children[1]);
    insertCode(code1, code2);
    return code1;
}

InterCode translate_Stmt(Node node){
    if(strcmp(node->children[0]->name, "Exp") == 0){
        Operand place = newTemp();
        return translate_Exp(node->children[0], place);
    }
    else if(strcmp(node->children[0]->name, "CompSt") == 0){
        return translate_CompSt(node->children[0]);
    }
    else if(strcmp(node->children[0]->name, "RETURN") == 0){
        // RETURN
        Operand place = newTemp();
        InterCode code1 = translate_Exp(node->children[1], place);
        InterCode code2 = NULLCODE();
        code2->kind = RETURN_IR;
        code2->ops[0] = place;
        insertCode(code1, code2);
        return code1;
    }
    else if(strcmp(node->children[0]->name, "WHILE") == 0){
        // WHILE
        Operand label1 = newLabel();
        Operand label2 = newLabel();
        Operand label3 = newLabel();
        InterCode code1 = NULLCODE();
        code1->kind = LABEL_IR;
        code1->ops[0] = label1;
        InterCode code2 = translate_Condition(node->children[2], label2, label3);
        InterCode code3 = NULLCODE();
        code3->kind = LABEL_IR;
        code3->ops[0] = label2;
        InterCode code4 = translate_Stmt(node->children[4]);
        InterCode code5 = NULLCODE();
        code5->kind = GOTO_IR;
        code5->ops[0] = label1;
        InterCode code6 = NULLCODE();
        code6->kind = LABEL_IR;
        code6->ops[0] = label3;
        insertCode(code1, code2);
        insertCode(code1, code3);
        insertCode(code1, code4);
        insertCode(code1, code5);
        insertCode(code1, code6);
        return code1;
    }
    else if(node->childnum == 5){
        // IF
        Operand label1 = newLabel();
        Operand label2 = newLabel();
        InterCode code1 = translate_Condition(node->children[2], label1, label2);
        InterCode code2 = NULLCODE();
        code2->kind = LABEL_IR;
        code2->ops[0] = label1;
        InterCode code3 = translate_Stmt(node->children[4]);
        InterCode code4 = NULLCODE();
        code4->kind = LABEL_IR;
        code4->ops[0] = label2;
        insertCode(code1, code2);
        insertCode(code1, code3);
        insertCode(code1, code4);
        return code1;
    }
    else if(node->childnum == 7){
        // IF ELSE 
        Operand label1 = newLabel();
        Operand label2 = newLabel();
        Operand label3 = newLabel();
        InterCode code1 = translate_Condition(node->children[2], label1, label2);
        InterCode code2 = NULLCODE();
        code2->kind = LABEL_IR;
        code2->ops[0] = label1;
        InterCode code3 = translate_Stmt(node->children[4]);
        InterCode code4 = NULLCODE();
        code4->kind = GOTO_IR;
        code4->ops[0] = label3;
        InterCode code5 = NULLCODE();
        code5->kind = LABEL_IR;
        code5->ops[0] = label2;
        InterCode code6 = translate_Stmt(node->children[6]);
        InterCode code7 = NULLCODE();
        code7->kind = LABEL_IR;
        code7->ops[0] = label3;
        insertCode(code1, code2);
        insertCode(code1, code3);
        insertCode(code1, code4);
        insertCode(code1, code5);
        insertCode(code1, code6);
        insertCode(code1, code7);
        return code1;
    }
    return NULLCODE();
}

InterCode translate_Condition(Node exp, Operand labeltrue, Operand labelfalse){
    // IF EXP GOTO labeltrue
    // GOTO labelfalse
    if(strcmp(exp->children[0]->name, "NOT") == 0){
        return translate_Condition(exp->children[1], labelfalse, labeltrue);
    }
    else if(exp->childnum > 2 && strcmp(exp->children[1]->name, "RELOP") == 0){
        Operand exp1 = newTemp();
        Operand exp2 = newTemp();
        InterCode code1 = translate_Exp(exp->children[0], exp1);
        InterCode code2 = translate_Exp(exp->children[2], exp2);
        InterCode code3 = NULLCODE();
        code3->kind = IF_GOTO_IR;
        code3->ops[0] = exp1;
        code3->ops[1] = exp2;
        code3->ops[2] = labeltrue;
        strcpy(code3->relop, exp->children[1]->value.str);
        InterCode code4 = NULLCODE();
        code4->kind = GOTO_IR;
        code4->ops[0] = labelfalse;
        insertCode(code1, code2);
        insertCode(code1, code3);
        insertCode(code1, code4);
        return code1;
    }
    else if(exp->childnum > 2 && 
    (strcmp(exp->children[1]->name, "AND") == 0 || 
    strcmp(exp->children[1]->name, "OR") == 0)){
        Operand tmp = newLabel();
        InterCode code1;
        if(exp->children[1]->name[0] == 'A'){
            code1 = translate_Condition(exp->children[0], tmp, labelfalse);
        } else {
            code1 = translate_Condition(exp->children[0], labeltrue, tmp);
        }
        InterCode code2 = NULLCODE();
        code2->kind = LABEL_IR;
        code2->ops[0] = tmp;
        InterCode code3 = translate_Condition(exp->children[2], labeltrue, labelfalse);
        insertCode(code1, code2);
        insertCode(code1, code3);
        return code1;
    }
    else {
        // 单个值作为表达式
        Operand tmp = newTemp();
        InterCode code1 = translate_Exp(exp, tmp);
        InterCode code2 = NULLCODE();
        code2->kind = IF_GOTO_IR;
        code2->ops[0] = tmp;
        code2->ops[1] = newConst(0);
        code2->ops[2] = labeltrue;
        strcpy(code2->relop, "!=");
        InterCode code3 = NULLCODE();
        code3->kind = GOTO_IR;
        code3->ops[0] = labelfalse;
        insertCode(code1, code2);
        insertCode(code1, code3);
        return code1;
    }
}

InterCode translate_Exp(Node node, Operand place){
    if(node->childnum == 3 && strcmp(node->children[1]->name, "ASSIGNOP") == 0){
        // Exp ASSIGN Exp
        Operand left = newTemp();
        Operand right = newTemp();
        InterCode code1 = translate_Exp(node->children[0], left);
        InterCode code2 = translate_Exp(node->children[2], right);
        InterCode code3 = NULLCODE();
        code3->kind = ASSIGN_IR;
        code3->ops[0] = left;
        code3->ops[1] = right;
        // 可能是数组之间直接的地址赋值
        if(right->type != NULL && right->type->kind == ARRAY && !right->is_addr){
            Operand tmpr = newOperand();
            tmpr->kind = GET_ADDR;
            tmpr->opr = right;
            tmpr->is_addr = true;
            tmpr->type = right->type;
            code3->ops[1] = tmpr;
            if(left->type != NULL && left->type->kind == ARRAY && !left->is_addr 
            && node->children[0]->childnum == 1){
                // 左值是一个数组，将其标记为地址 
                Symbol sym = search_symbol(node->children[0]->children[0]->value.str);
                sym->op->is_addr = true;
            }
        }
        
        insertCode(code1, code2);
        insertCode(code1, code3);
        return code1;
    }
    else if(node->childnum >= 2 && (
        strcmp(node->children[0]->name, "NOT") == 0 ||
        strcmp(node->children[1]->name, "AND") == 0 ||
        strcmp(node->children[1]->name, "OR") == 0 ||
        strcmp(node->children[1]->name, "RELOP") == 0 )){
        // Exp && || ><= Exp
        InterCode code1 = NULLCODE();
        code1->kind = ASSIGN_IR;
        code1->ops[0] = place;
        code1->ops[1] = newConst(0);
        Operand label1 = newLabel();
        Operand label2 = newLabel();
        InterCode code2 = translate_Condition(node, label1, label2);
        InterCode code3 = NULLCODE();
        code3->kind = LABEL_IR;
        code3->ops[0] = label1;
        InterCode code4 = NULLCODE();
        code4->kind = ASSIGN_IR;
        code4->ops[0] = place;
        code4->ops[1] = newConst(1);
        InterCode code5 = NULLCODE();
        code5->kind = LABEL_IR;
        code5->ops[0] = label2;
        insertCode(code1, code2);
        insertCode(code1, code3);
        insertCode(code1, code4);
        insertCode(code1, code5);
        return code1;
    }
    else if(node->childnum == 3 && (
        strcmp(node->children[1]->name, "PLUS") == 0 ||
        strcmp(node->children[1]->name, "MINUS") == 0 ||
        strcmp(node->children[1]->name, "STAR") == 0 ||
        strcmp(node->children[1]->name, "DIV") == 0 )){
        // Exp +-*/ Exp
        Operand tmp1 = newTemp();
        Operand tmp2 = newTemp();
        InterCode code1 = translate_Exp(node->children[0], tmp1);
        InterCode code2 = translate_Exp(node->children[2], tmp2);
        InterCode code3 = NULL;
        switch (node->children[1]->name[0]){
            case 'P': code3 = arithmetic(place, tmp1, tmp2, PLUS_OP); break;
            case 'M': code3 = arithmetic(place, tmp1, tmp2, MINUS_OP); break;
            case 'S': code3 = arithmetic(place, tmp1, tmp2, STAR_OP); break;
            case 'D': code3 = arithmetic(place, tmp1, tmp2, DIV_OP); break;
        }
        insertCode(code1, code2);
        insertCode(code1, code3);
        return code1;
    }
    else if(strcmp(node->children[0]->name, "LP") == 0){
        // (Exp)
        return translate_Exp(node->children[1], place);
    }
    else if(strcmp(node->children[0]->name, "MINUS") == 0){
        // - Exp
        Operand tmp = newTemp();
        InterCode code1 = translate_Exp(node->children[1], tmp);
        InterCode code2 = arithmetic(place, newConst(0), tmp, MINUS_OP);
        insertCode(code1, code2);
        return code1;
    }
    else if(node->childnum > 1 && strcmp(node->children[1]->name, "LP") == 0){
        // ID()   ID(ARGS)
        Symbol sym = search_symbol(node->children[0]->value.str);
        Operand func = sym->op;

        // READ & WRITE
        if(strcmp(func->name, "read") == 0){
            InterCode read_code = NULLCODE();
            read_code->kind = READ_IR;
            read_code->ops[0] = place;
            return read_code;
        } else if(strcmp(func->name, "write") == 0){
            Operand tmp = newTemp();
            InterCode code1 = translate_Exp(node->children[2]->children[0], tmp);
            InterCode code2 = NULLCODE();
            code2->kind = WRITE_IR;
            code2->ops[0] = tmp;
            insertCode(code1, code2);
            return code1;
        }

        InterCode call_code = NULLCODE();
        call_code->kind = CALL_IR;
        call_code->ops[0] = place;
        call_code->ops[1] = func;
        if(node->childnum == 4){
            // ARGS
            InterCode arg_code = translate_Args(node->children[2]);
            insertCode(arg_code, call_code);
            return arg_code;
        }
        return call_code;
    }
    else if(node->childnum > 1 && strcmp(node->children[1]->name, "LB") == 0){
        // Exp[Exp]
        // 期望返回元素的值
        Operand tmp = newTemp();
        Operand arr_addr = NULL;     // 数组首地址
        Operand addr = NULL;         // 元素首地址
        Operand index = newTemp();   // INT 值
        Operand offset = newTemp();  // 偏移量
        // 获取数组首地址
        InterCode code1 = translate_Exp(node->children[0], tmp);
        if(tmp->kind == GET_VAL){
            arr_addr = tmp->opr;
            arr_addr->is_addr = true;
        } else if(!tmp->is_addr){
            arr_addr = newOperand();
            arr_addr->kind = GET_ADDR;
            arr_addr->opr = tmp;
            arr_addr->is_addr = true;
        } else {
            arr_addr = tmp;
        }
        // 多维数组的类型转换
        if(tmp->type->array.dimension > 1){
            Type low_arr = (Type)malloc(sizeof(Type_t));
            low_arr->kind = ARRAY;
            low_arr->array.dimension = tmp->type->array.dimension - 1;
            low_arr->array.elem = tmp->type->array.elem;
            for(int i=0; i < low_arr->array.dimension; i++){
                low_arr->array.size[i] = tmp->type->array.size[i];
            }
            arr_addr->type = low_arr;
        } else {
            arr_addr->type = tmp->type;
        }
        // 计算偏移量
        InterCode code2 = translate_Exp(node->children[2], index);
        insertCode(code1, code2);
        if(index->value == 0){
            // 数组首地址即为元素地址
            addr = arr_addr;
        } else {
            addr = newTemp();
            addr->is_addr = true;
            int move = 1;
            for(int i=0; i < arr_addr->type->array.dimension -1; i++){
                move = move * arr_addr->type->array.size[i];
            }
            InterCode code3 = arithmetic(offset, index, newConst(move * getSizeof(arr_addr->type->array.elem)), STAR_OP);
            InterCode code4 = arithmetic(addr, arr_addr, offset, PLUS_OP);
            insertCode(code1, code3);
            insertCode(code1, code4);
        }
        // 令 place 指向元素的值
        if(addr->kind == GET_ADDR){
            // 不能出现： *&t1, 将其更改为：
            // t2 := &t1
            // left :: *t2
            Operand tmp = newTemp();
            InterCode code5 = NULLCODE();
            code5->kind = ASSIGN_IR;
            code5->ops[0] = tmp;
            code5->ops[1] = addr;
            insertCode(code1, code5);
            place->kind = GET_VAL;
            place->opr = tmp;
            place->type = arr_addr->type->array.elem;
        } else {
            place->kind = GET_VAL;
            place->opr = addr;
            place->type = arr_addr->type->array.elem;
        }
        return code1;
    }
    else if(node->childnum > 1 && strcmp(node->children[1]->name, "DOT") == 0){
        // Exp.ID
        // 期望返回 struct.id 的值
        Operand tmp = newTemp();
        Operand exp_addr = NULL;       // exp 所表示的首地址，可能不是结构体 
        Operand addr = NULL;           // 成员首地址
        Operand offset = newConst(0);  // 偏移量

        InterCode code1 = translate_Exp(node->children[0], tmp);
        if(tmp->kind == GET_VAL){
            exp_addr = tmp->opr;
            exp_addr->type = tmp->type;
            exp_addr->is_addr = true;
        } else if(!tmp->is_addr){
            exp_addr = newOperand();
            exp_addr->kind = GET_ADDR;
            exp_addr->opr = tmp;
            exp_addr->type = tmp->type;
            exp_addr->is_addr = true;
        } else {
            exp_addr = tmp;
        }

        // 计算偏移量, 获取元素类型
        FieldList ptr = exp_addr->type->structure->members;
        char* id = node->children[2]->value.str;
        Type ret_type = NULL;
        while(ptr != NULL){
            if(strcmp(ptr->name, id) != 0){
                offset->value += getSizeof(ptr->type);
                ptr = ptr->next;
            } else {
                ret_type = ptr->type;
                break;
            }
        }

        if(offset->value != 0){
            // 偏移量不为 0 时生成额外的中间代码
            addr = newTemp();
            addr->is_addr = true;
            InterCode code2 = arithmetic(addr, exp_addr, offset, PLUS_OP);
            insertCode(code1, code2);
        } else {
            addr = exp_addr;
        }
        // 令 place 指向元素的值
        if(addr->kind == GET_ADDR){
            // 不能出现： *&t1, 将其更改为：
            // t2 := &t1
            // left :: *t2
            Operand tmp = newTemp();
            InterCode code3 = NULLCODE();
            code3->kind = ASSIGN_IR;
            code3->ops[0] = tmp;
            code3->ops[1] = addr;
            insertCode(code1, code3);
            place->kind = GET_VAL;
            place->opr = tmp;
            place->type = ret_type;
        } else {
            place->kind = GET_VAL;
            place->opr = addr;
            place->type = ret_type;
        }
        place->is_addr = false;
        return code1;
    }
    else if(node->childnum == 1 && strcmp(node->children[0]->name, "ID") == 0){
        // ID
        copyOperand(place, getOperand(node->children[0]->value.str));
        return NULLCODE();
    }
    else if(strcmp(node->children[0]->name, "INT") == 0 || 
        strcmp(node->children[0]->name, "FLOAT") == 0 ){
        // INT FLOAT
        copyOperand(place, newConst(node->children[0]->value.ival));
        return NULLCODE();
    }
    return NULLCODE();
}

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
    
    // 如果参数列表是： (arg1, arg2, arg3)，那么生成的代码应该是
    // exp1->exp2->exp3->arg3->arg2->arg1
    // exp 的顺序无所谓，但是参数的顺序必须与声明顺序相反
    insertCode(code1, code3);
    return code1;
}


InterCode transform_addr(Operand dst, Operand tmp){
    InterCode code = NULLCODE();
    code->kind = ASSIGN_IR;
    code->ops[0] = tmp;
    code->ops[1] = dst;
    return code;
}

bool is_memop(Operand op){
    return (op->kind == GET_ADDR || op->kind == GET_VAL);
}

InterCode arithmetic(Operand dst, Operand src1, Operand src2, enum Operator op){
    InterCode code = NULLCODE();
    switch (op){
        case PLUS_OP:   code->kind = PLUS_IR;   break;
        case MINUS_OP:  code->kind = SUB_IR;    break;
        case STAR_OP:   code->kind = MULTI_IR;  break;
        case DIV_OP:    code->kind = DIV_IR;    break;
    }
    code->ops[0] = dst;
    code->ops[1] = src1;
    code->ops[2] = src2;

    /**
     * 下面这段代码用于将算术运算中的引用和解引用消除
     * 比如   v1 := *t1 + &t2
     * 将被转化为 
     * t3 := *t1
     * t4 := &t2
     * v1 := t3 + t4
     */
    InterCode ptr = code;
    if(is_memop(src1)){
        Operand tmp = newTemp();
        InterCode code1 = transform_addr(src1, tmp);
        code->ops[1] = tmp;
        insertCode(code1, ptr);
        ptr = code1;
    }
    if(is_memop(src2)){
        Operand tmp = newTemp();
        InterCode code2 = transform_addr(src2, tmp);
        code->ops[2] = tmp;
        insertCode(code2, ptr);
        ptr = code2;
    }
    return ptr;
}

void formatCode(InterCode code){
    /**
     * 此函数用于将引用和解引用从以下几种代码类型中消除
     */
    InterCode curr = code, prev = NULL;
    while(curr != NULL){
        switch (curr->kind)
        {
        case ASSIGN_IR:
            if(is_memop(curr->ops[0]) && is_memop(curr->ops[1])){
                Operand tmp = newTemp();
                InterCode mid = transform_addr(curr->ops[1], tmp);
                curr->ops[1] = tmp;
                mid->next = curr; mid->prev = prev;
                prev->next = mid; curr->prev = mid;
                prev = mid;
            }
            break;
        case IF_GOTO_IR:
            if(is_memop(curr->ops[0])){
                Operand tmp = newTemp();
                InterCode mid = transform_addr(curr->ops[0], tmp);
                curr->ops[0] = tmp;
                mid->next = curr; mid->prev = prev;
                prev->next = mid; curr->prev = mid;
                prev = mid;
            }
            if(is_memop(curr->ops[1])){
                Operand tmp = newTemp();
                InterCode mid = transform_addr(curr->ops[1], tmp);
                curr->ops[1] = tmp;
                mid->next = curr; mid->prev = prev;
                prev->next = mid; curr->prev = mid;
                prev = mid;
            }
            break;
        case RETURN_IR:
            if(is_memop(curr->ops[0])){
                Operand tmp = newTemp();
                InterCode mid = transform_addr(curr->ops[0], tmp);
                curr->ops[0] = tmp;
                mid->next = curr; mid->prev = prev;
                prev->next = mid; curr->prev = mid;
                prev = mid;
            }
            break;
        case READ_IR || WRITE_IR:
            if(is_memop(curr->ops[0]) || curr->ops[0]->kind == CONSTANT){
                Operand tmp = newTemp();
                InterCode mid = transform_addr(curr->ops[0], tmp);
                curr->ops[0] = tmp;
                mid->next = curr; mid->prev = prev;
                prev->next = mid; curr->prev = mid;
                prev = mid;
            }
            break;
        case NULL_IR:
            if(prev == NULL){
                break;
            }
            InterCode ptr = curr;
            prev->next = ptr->next;
            if(ptr->next) { ptr->next->prev = prev; }
            curr = prev;
            free(ptr);
            break;    
        default:
            break;
        }

        prev = curr;
        curr = curr->next;
    }
}

void printInterCode(char* filepath, InterCode code){
    FILE* fp = fopen(filepath, "w");
    if(fp == NULL){
        printf("Failed to open file %s \n", filepath);
        return;
    }
    InterCode ptr = code;
    while(ptr != NULL){
        switch(ptr->kind){
            case LABEL_IR:
                fputs("LABEL ", fp);
                printOperand(ptr->ops[0], fp);
                fputs(" :", fp);
                break;
            case FUNC_IR:
                fputs("\n", fp);
                fputs("FUNCTION ", fp);
                printOperand(ptr->ops[0], fp);
                fputs(" :", fp);
                break;
            case ASSIGN_IR:
                printOperand(ptr->ops[0], fp);
                fputs(" := ", fp);
                printOperand(ptr->ops[1], fp);
                break;
            case PLUS_IR:
                printOperand(ptr->ops[0], fp);
                fputs(" := ", fp);
                printOperand(ptr->ops[1], fp);
                fputs(" + ", fp);
                printOperand(ptr->ops[2], fp);
                break;
            case SUB_IR:
                printOperand(ptr->ops[0], fp);
                fputs(" := ", fp);
                printOperand(ptr->ops[1], fp);
                fputs(" - ", fp);
                printOperand(ptr->ops[2], fp);
                break;
            case MULTI_IR:
                printOperand(ptr->ops[0], fp);
                fputs(" := ", fp);
                printOperand(ptr->ops[1], fp);
                fputs(" * ", fp);
                printOperand(ptr->ops[2], fp);
                break;
            case DIV_IR:
                printOperand(ptr->ops[0], fp);
                fputs(" := ", fp);
                printOperand(ptr->ops[1], fp);
                fputs(" / ", fp);
                printOperand(ptr->ops[2], fp);
                break;
            case MEM_IR:
                fputs("*", fp);
                printOperand(ptr->ops[0], fp);
                fputs(" := ", fp);
                printOperand(ptr->ops[1], fp);
                break;
            case GOTO_IR:
                fputs("GOTO ", fp);
                printOperand(ptr->ops[0], fp);
                break;
            case IF_GOTO_IR:
                fputs("IF ", fp);
                printOperand(ptr->ops[0], fp);
                fputs(" ", fp);
                fputs(ptr->relop, fp);
                fputs(" ", fp);
                printOperand(ptr->ops[1], fp);
                fputs(" GOTO ", fp);
                printOperand(ptr->ops[2], fp);
                break;
            case RETURN_IR:
                fputs("RETURN ", fp);
                printOperand(ptr->ops[0], fp);
                break;
            case DEC_IR:
                fputs("DEC ", fp);
                printOperand(ptr->ops[0], fp);
                char str[16];
                sprintf(str, " %d", ptr->size);
                fputs(str, fp);
                break;
            case ARG_IR:
                fputs("ARG ", fp);
                printOperand(ptr->ops[0], fp);
                break;
            case CALL_IR:
                printOperand(ptr->ops[0], fp);
                fputs(" := CALL ", fp);
                printOperand(ptr->ops[1], fp);
                break;
            case PARAM_IR:
                fputs("PARAM ", fp);
                printOperand(ptr->ops[0], fp);
                break;
            case READ_IR:
                fputs("READ ", fp);
                printOperand(ptr->ops[0], fp);
                break;
            case WRITE_IR:
                fputs("WRITE ", fp);
                printOperand(ptr->ops[0], fp);
                break;
            default:
                // NULL_IR
                break;
        }

        if(ptr->kind != NULL_IR){
            fputs("\n", fp);
        }
        fflush(fp);
        ptr = ptr->next;
    }
    fclose(fp);
}

void printOperand(Operand op, FILE* fp){
    if(op == NULL){
        fputs("null", fp);
        return;
    }
    char out[36];
    switch (op->kind){
        case VARIABLE:
            sprintf(out, "v%s", op->name);
            fputs(out, fp);
            break;
        case TEMP_VAR:
            sprintf(out, "t%d", op->no);
            fputs(out, fp);
            break;
        case CONSTANT:
            sprintf(out, "#%d", op->value);
            fputs(out, fp);
            break;
        case LABEL:
            sprintf(out, "label%d", op->no);
            fputs(out, fp);
            break;
        case FUNC:
            sprintf(out, "%s", op->name);
            fputs(out, fp);
            break;
        case GET_ADDR:
            fputs("&", fp);
            printOperand(op->opr, fp);
            break;
        case GET_VAL:
            fputs("*", fp);
            printOperand(op->opr, fp);
            break;
        default:
            break;
    }
}

void freeInterCode(InterCode code){
    // TODO
}