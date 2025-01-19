#include "semantic.h"
#include <assert.h>

Symbol symbol_table[TABLE_SIZE];
bool sem_error = false;

unsigned int hash_pjw(char* name){
    unsigned int i, val = 0;
    for(; *name; ++name){
        val = (val << 2) + *name;
        if(i = val & ~0x3fff) val = (val ^ (i >> 12)) & 0x3fff;
    }
    return val % TABLE_SIZE;
}

void init_symbol_table(){
    for(int i=0; i < TABLE_SIZE; i++){
        if(symbol_table[i]){
            free(symbol_table[i]);
        }
        symbol_table[i] = NULL;
    }

    // 引入 read & write 函数
    Function read  = (Function)malloc(sizeof(Function_t));
    Function write = (Function)malloc(sizeof(Function_t));
    Type tint = (Type)malloc(sizeof(Type_t));
    tint->kind = BASIC;
    tint->basic = TYPE_INT;
    read->ret_type = tint;
    write->ret_type = tint;
    read->param = NULL;
    FieldList arg = (FieldList)malloc(sizeof(FieldList_t));
    arg->type = tint;
    write->param = arg;
    read->defined = true;
    write->defined = true;
    strcpy(read->name, "read");
    strcpy(write->name, "write");
    Type rtype = (Type)malloc(sizeof(Type_t));
    rtype->kind = FUNCTION;
    rtype->func = read;
    Type wtype = (Type)malloc(sizeof(Type_t));
    wtype->kind = FUNCTION;
    wtype->func = write;
    Symbol rsym = create_symbol(read->name, rtype, 0);
    Symbol wsym = create_symbol(write->name, wtype, 0);
    insert_symbol(rsym);
    insert_symbol(wsym);
}

Symbol create_symbol(char* str, Type type, int lineno){
    Symbol sym = (Symbol)malloc(sizeof(Symbol_t));
    strcpy(sym->name, str);
    sym->type = type;
    sym->lineno = lineno;
    sym->next = NULL;
    sym->op = NULL;
    return sym;
}

void insert_symbol(Symbol sym){
    unsigned int index = hash_pjw(sym->name);
    sym->next = symbol_table[index];
    symbol_table[index] = sym;
}

Symbol search_symbol(char* str){
    unsigned int index = hash_pjw(str);
    Symbol sym = symbol_table[index];
    while(sym != NULL){
        if(strcmp(sym->name, str) == 0){
            break;
        }
        sym = sym->next;
    }
    return sym;
}

bool fieldEqual(FieldList f1, FieldList f2){
    if(f1 == NULL && f2 == NULL){
        return true;
    } else if(f1 == NULL || f2 == NULL){
        return false;
    }
    return typeEqual(f1->type, f2->type) && fieldEqual(f1->next, f2->next);
}

bool typeEqual(Type t1, Type t2){
    if(t1 == NULL || t2 == NULL || t1->kind != t2->kind) { return false; }
    if(t1->kind == BASIC){
        return t1->basic == t2->basic;
    } else if(t1->kind == ARRAY){
        return typeEqual(t1->array.elem, t2->array.elem) 
            && t1->array.dimension == t2->array.dimension;
    } else if(t1->kind == FUNCTION){
        return (strcmp(t1->func->name, t2->func->name) == 0)
            && typeEqual(t1->func->ret_type, t2->func->ret_type)
            && fieldEqual(t1->func->param, t2->func->param);
    } else {
        return (strcmp(t1->structure->name, t2->structure->name) == 0)
            && fieldEqual(t1->structure->members, t2->structure->members);
    }
}

void check_func_def(){
    // 检查未定义函数
    for(int i=0; i < TABLE_SIZE; i++){
        if(symbol_table[i]){
            Symbol sym = symbol_table[i];
            while(sym != NULL){
                if(sym->type->kind == FUNCTION && !sym->type->func->defined){
                    printf("Error type 18 at line %d: function not define\n", sym->lineno);
                    sem_error = true;
                }
                sym = sym->next;
            }
        }
    }
}

void semantic_transfer(Node root){
    init_symbol_table();
    Program(root);
    check_func_def();
    // freeSymbolTable();
}

void Program(Node node){
    if(node == NULL)  return ;
    ExtDefList(node->children[0]);
}

void ExtDefList(Node node){
    if(node->childnum != 0){
        ExtDef(node->children[0]);
        ExtDefList(node->children[1]);
    }
}

void ExtDef(Node node){
    Type type = Specifier(node->children[0]);
    if(type == NULL){
        // type error
        return;
    } 
    if(strcmp(node->children[1]->name, "SEMI") == 0){
        return;
    } else if(strcmp(node->children[1]->name, "ExtDecList") == 0){
        ExtDecList(node->children[1], type);
        return;
    }

    // 关于函数声明、定义的处理
    assert(strcmp(node->children[1]->name, "FunDec") == 0);
    Function func = FunDec(node->children[1]);
    func->ret_type = type;
    func->defined = false;
    Type func_type = (Type)malloc(sizeof(Type_t));
    func_type->kind = FUNCTION;
    func_type->func = func;
    Symbol sym = search_symbol(func->name);

    if(strcmp(node->children[2]->name, "CompSt") == 0){
        // 函数定义
        if(sym == NULL){
            upload_func_params(func->param, NULL);
            Symbol new_sym = create_symbol(func->name, func_type, func->lineno);
            func->defined = true;
            insert_symbol(new_sym);
            CompSt(node->children[2], func->ret_type);
        }
        else {
            if(sym->type->kind != FUNCTION){
                // 普通的重名
                printf("Error type 4 at line %d: name repeat\n", node->lineno);
                sem_error = true;
            } else if(!typeEqual(func_type, sym->type)){
                if(sym->type->func->defined){
                    // 重复定义
                    printf("Error type 4 at line %d: function repeat define: %d\n", node->lineno, sym->lineno);
                    sem_error = true;
                } else {
                    // 定义与声明冲突
                    printf("Error type 19 at line %d: conflict define \'%s\' at %d\n", node->lineno, func->name, sym->lineno);
                    sem_error = true;
                }
            } else if(sym->type->func->defined){
                // 重复定义
                printf("Error type 4 at line %d: function repeat define: %d\n", node->lineno, sym->lineno);
                sem_error = true;
            } else {
                // 之前有声明，现在定义
                Function pre_func = sym->type->func;
                pre_func->defined = true;
                upload_func_params(func->param, pre_func->param);
                CompSt(node->children[2], pre_func->ret_type);
            }
        }
    }
    else if(strcmp(node->children[2]->name, "SEMI") == 0){
        // 函数声明
        if(sym == NULL){
            // 声明
            upload_func_params(func->param, NULL);
            Symbol new_sym = create_symbol(func->name, func_type, func->lineno);
            insert_symbol(new_sym);
        }
        else{
            if(sym->type->kind != FUNCTION){
                // 普通的重名
                printf("Error type 4 at line %d: name repeat\n", node->lineno);
                sem_error = true;
            } else if(!typeEqual(func_type, sym->type)){
                // 与之前声明/定义冲突
                printf("Error type 19 at line %d: conflict name \'%s\' at %d\n", func->lineno, func->name, sym->lineno);
                sem_error = true;
            } else {
                // 重复声明，允许
                upload_func_params(func->param, sym->type->func->param);
            }
        }
    }
    return; 
}

Type Specifier(Node node){
    if(strcmp(node->children[0]->name, "TYPE") == 0){
        Type type = (Type)malloc(sizeof(Type_t));
        type->kind = BASIC;
        char* str = node->children[0]->value.str;
        if(strcmp(str, "int") == 0){
            type->basic = TYPE_INT;
        } else {
            type->basic = TYPE_FLOAT;
        }
        return type;
    } 
    else {
        return StructSpecifier(node->children[0]);  // maybe NULL
    }
}

void ExtDecList(Node node, Type type){
    VarDec(node->children[0], IN_COMPST, type, 0);
    if(node->childnum == 3){
        ExtDecList(node->children[2], type);
    }
}

Function FunDec(Node node){
    // 需要构建函数的 name, lineno, param
    // 专注于函数的构建，避免在此阶段报错
    // 错误分析留给上层
    // 若无参数，则 param 的值为 NULL
    Function func = (Function)malloc(sizeof(Function_t));
    strcpy(func->name, node->children[0]->value.str);
    func->lineno = node->lineno;
    func->param = NULL;
    if(node->childnum == 4){
        func->param = VarList(node->children[2]);
    }
    return func;
}

Type StructSpecifier(Node node){
    Type type = (Type)malloc(sizeof(Type_t));
    type->kind = STRUCTURE;
    if(node->childnum == 2){
        // STRUCT Tag
        Node Tag = node->children[1];
        char* str = Tag->children[0]->value.str;
        Symbol sym = search_symbol(str);
        if(sym == NULL || sym->type->kind != STRUCT_DEF){
            // 未定义的结构体
            printf("Error type 17 at line %d: undefined struct\n", node->lineno);
            sem_error = true;
            return NULL;
        }
        type->structure = sym->type->structure;
    }
    else {
        // STRCUT OptTag LC DefList RC
        char* str = OptTag(node->children[1]);
        if(strcmp(str, "") != 0){
            Symbol sym = search_symbol(str);
            if(sym != NULL){
                // 结构体重名
                printf("Error type 16 at line %d: Duplicated struct name\n", node->lineno);
                sem_error = true;
                free(str); str = NULL;
                return NULL;
            }
            // 定义一个新结构体
            Type new_type = (Type)malloc(sizeof(Type_t));
            new_type->kind = STRUCT_DEF;
            new_type->structure = (Structure)malloc(sizeof(Structure_t));
            strcpy(new_type->structure->name, str);
            new_type->structure->members = DefList(node->children[3], IN_STRUCT);
            // 同一结构体中域名重复的错误（15）
            check_field_in_one_struct(new_type->structure->members);
            Symbol new_sym = create_symbol(str, new_type, node->lineno);
            insert_symbol(new_sym);

            type->structure = new_sym->type->structure;
        }
        else {
            // 匿名结构体
            type->structure = (Structure)malloc(sizeof(Structure_t));
            strcpy(type->structure->name, str);
            type->structure->members = DefList(node->children[3], IN_STRUCT);
            check_field_in_one_struct(type->structure->members);
        }
    }
    return type;
}

char* OptTag(Node node){
    char* name = (char*)malloc(sizeof(char) * 32);
    if(node->kind == SYN_NULL){
        strcpy(name, "");
    } else {
        strcpy(name, node->children[0]->value.str);
    }
    return name;
}

FieldList VarList(Node node){
    FieldList list = ParamDec(node->children[0]);
    if(node->childnum == 3){
        if(list == NULL){
            list = VarList(node->children[2]);
        } else {
            list->next = VarList(node->children[2]);
        }
    }
    return list;
}

FieldList ParamDec(Node node){
    Type type = Specifier(node->children[0]);
    if(type == NULL){
        return NULL;
    }
    return VarDec(node->children[1], IN_FUNC_DEC, type, 0);
}

FieldList DefList(Node node, Context ctx){
    if(node->kind == SYN_NULL){
        return NULL;
    }
    FieldList list = Def(node->children[0], ctx);
    if(list == NULL){
        return DefList(node->children[1], ctx);
    }
    FieldList ptr = list;
    while(ptr->next != NULL) { ptr = ptr->next; }
    ptr->next = DefList(node->children[1], ctx);
    return list;
}

FieldList Def(Node node, Context ctx){
    Type type = Specifier(node->children[0]);
    if(type == NULL){
        return NULL;
    }
    return DecList(node->children[1], ctx, type);
}

FieldList DecList(Node node, Context ctx, Type type){
    FieldList list = Dec(node->children[0], ctx, type);
    if(node->childnum > 1){
        if(list == NULL){
            return DecList(node->children[2], ctx, type);
        }
        FieldList ptr = list;
        while(ptr->next != NULL) { ptr = ptr->next; }
        ptr->next = DecList(node->children[2], ctx, type);
    }
    return list;
}

FieldList Dec(Node node, Context ctx, Type type){
    FieldList list = VarDec(node->children[0], ctx, type, 0);
    if(ctx == IN_STRUCT && node->childnum == 3){
        // 结构体定义不允许初始化
        printf("Error type 15 at line %d: initialize member in struct defination\n", node->lineno);
        sem_error = true;
    }
    if(list != NULL && node->childnum == 3){
        // 检查左右值类型是否相同
        Type right = Exp(node->children[2]);
        if(!typeEqual(list->type, right)){
            printf("Error type 5 at line %d: type mismatched\n", node->lineno);
            sem_error = true;
        }
    }
    return list;
}

// FieldList VarDec(Node node, Context ctx, Type type, int layer){
//     // layer 代表递归的层数，若递归发生，代表当前是一个数组类型的变量
//     char* str = node->children[0]->value.str;
//     if(node->childnum == 1){
//         // ID
//         Symbol sym = search_symbol(str);
//         if(sym != NULL && ctx != IN_FUNC_DEC){
//             printf("Error type 3 at line %d: var name confilct\n", node->lineno);
//             sem_error = true;
//             if(ctx != IN_STRUCT){
//                 return NULL;
//             }
//         }
//         FieldList list = (FieldList)malloc(sizeof(FieldList_t));
//         strcpy(list->name, str);
//         if(layer == 0){
//             list->type = type;
//         } else {
//             Type arr_type = (Type)malloc(sizeof(Type_t));
//             arr_type->kind = ARRAY;
//             arr_type->array.elem = type;
//             arr_type->array.dimension = layer;
//             list->type = arr_type;
//         }
//         list->mark_line = -1;           // 普通的域不需要行号，为-1
//         list->next = NULL;
//         if(sym == NULL && ctx != IN_FUNC_DEC){
//             Symbol new_sym = create_symbol(str, list->type, node->lineno);
//             insert_symbol(new_sym);
//         } else {
//             list->mark_line = node->lineno;
//         }
//         return list;
//     }
//     else {
//         // VarDec LB INT RB
//         return VarDec(node->children[0], ctx, type, layer+1);
//     }
// }

FieldList VarDec(Node node, Context ctx, Type type, int layer){
    if(node->childnum == 1){
        // ID
        char* id = node->children[0]->value.str;
        // 查重名
        Symbol sym = search_symbol(id);
        if(sym != NULL && ctx != IN_FUNC_DEC){
            printf("Error type 3 at line %d: var name confilct\n", node->lineno);
            sem_error = true;
            if(ctx != IN_STRUCT){
                return NULL;
            }
        }

        FieldList list = (FieldList)malloc(sizeof(FieldList_t));
        strcpy(list->name, id);
        list->type = type;
        list->mark_line = -1;           // 普通的域不需要行号，为-1
        list->next = NULL;
        if(sym == NULL && ctx != IN_FUNC_DEC){
            Symbol new_sym = create_symbol(id, list->type, node->lineno);
            insert_symbol(new_sym);
        } else {
            list->mark_line = node->lineno;
        }
        return list;
    }
    else if(layer == 0){
        Type arr_type = (Type)malloc(sizeof(Type_t));
        arr_type->kind = ARRAY;
        arr_type->array.elem = type;
        arr_type->array.dimension = 1;
        arr_type->array.size[layer] = node->children[2]->value.ival;
        return VarDec(node->children[0], ctx, arr_type, layer+1);
    }
    else {
        type->array.dimension ++;
        type->array.size[layer] = node->children[2]->value.ival;
        return VarDec(node->children[0], ctx, type, layer+1);
    }
}

void CompSt(Node node, Type ret_type){
    DefList(node->children[1], IN_COMPST);
    StmtList(node->children[2], ret_type);
}

void StmtList(Node node, Type ret_type){
    if(node->kind == SYN_NULL){
        return ;
    }
    Stmt(node->children[0], ret_type);
    StmtList(node->children[1], ret_type);
}

void Stmt(Node node, Type ret_type){
    if(strcmp(node->children[0]->name, "Exp") == 0){
        Exp(node->children[0]);
    }
    else if(strcmp(node->children[0]->name, "CompSt") == 0){
        CompSt(node->children[0], ret_type);
    }
    else if(strcmp(node->children[0]->name, "RETURN") == 0){
        Type exp_type = Exp(node->children[1]);
        if(!typeEqual(exp_type, ret_type)){
            printf("Error type 8 at line %d: error return type\n", node->lineno);
            sem_error = true;
        } 
    }
    else if(strcmp(node->children[0]->name, "WHILE") == 0){
        Exp(node->children[2]);
        Stmt(node->children[4], ret_type);
    }
    else {
        Exp(node->children[2]);
        Stmt(node->children[4], ret_type);
        if(node->childnum == 7){
            Stmt(node->children[6], ret_type);
        }
    }
}

Type Exp(Node node){
    if(strcmp(node->children[0]->name, "ID") == 0){
        char* str = node->children[0]->value.str;
        Symbol sym = search_symbol(str);
        if(node->childnum == 1){
            // ID
            if(sym == NULL){
                // 使用未定义的变量
                printf("Error type 1 at line %d: undefined var \'%s\'\n", node->lineno, str);
                sem_error = true;
                return NULL;
            }
            return sym->type;
        }
        else {
            // ID LP ... RP
            if(sym == NULL){
                // 使用未定义函数
                printf("Error type 2 at line %d: undefined function\n", node->lineno);
                sem_error = true;
                return NULL;
            } else if(sym->type->kind != FUNCTION){
                // 对其他类型变量使用 ()
                printf("Error type 11 at line %d: () for other type var\n", node->lineno);
                sem_error = true;
                return NULL;
            } else if(!sym->type->func->defined){
                // 函数仅声明未定义
                printf("Error type 2 at line %d: undefined function\n", node->lineno);
                sem_error = true;
                return NULL;
            }
            FieldList args = NULL;
            if(node->childnum == 4){
                args = Args(node->children[2]);
            }
            FieldList params = sym->type->func->param;
            if(!fieldEqual(args, params)){
                printf("Error type 9 at line %d: args and params mismatched\n", node->lineno);
                sem_error = true;
            }
            return sym->type->func->ret_type;
        }
    }
    else if(strcmp(node->children[0]->name, "INT") == 0){
        // INT
        Type type = (Type)malloc(sizeof(Type_t));
        type->kind = BASIC;
        type->basic = TYPE_INT;
        return type;
    }
    else if(strcmp(node->children[0]->name, "FLOAT") == 0){
        // FLOAT
        Type type = (Type)malloc(sizeof(Type_t));
        type->kind = BASIC;
        type->basic = TYPE_FLOAT;
        return type;
    }
    else if(strcmp(node->children[0]->name, "LP") == 0){
        // LP EXP RP
        return Exp(node->children[1]);
    }
    else if(strcmp(node->children[0]->name, "MINUS") == 0){
        // MINUS EXP
        Type exp_type = Exp(node->children[1]);
        if(exp_type != NULL){
            if(exp_type->kind != BASIC){
                // 非 int float 变量使用负号
                printf("Error type 7 at line %d: illegal -\n", node->lineno);
                sem_error = true;
                return NULL;
            }
        }
        return exp_type;
    }
    else if(strcmp(node->children[0]->name, "NOT") == 0){
        // NOT EXP
        Type exp_type = Exp(node->children[1]);
        if(exp_type != NULL){
            if(exp_type->kind != BASIC || exp_type->basic != TYPE_INT){
                // 只有 int 可以做逻辑运算
                printf("Error type 7 at line %d: only INT\n", node->lineno);
                sem_error = true;
                return NULL;
            }
        }
        return exp_type;
    }
    else if(strcmp(node->children[1]->name, "ASSIGNOP") == 0){
        // EXP == EXP
        Type right = Exp(node->children[2]);
        Node left_child = node->children[0];
        Type left = Exp(left_child);
        if((left_child->childnum == 1 && strcmp(left_child->children[0]->name, "ID") == 0)
        || (left_child->childnum == 3 && strcmp(left_child->children[1]->name, "DOT") == 0)
        || (left_child->childnum == 4 && strcmp(left_child->children[1]->name, "LB") == 0)){
        } else {
            // 赋值号左边是右值表达式
            printf("Error type 6 at line %d: right exp in left\n", node->lineno);
            sem_error = true;
            return NULL;
        }
        if(left != NULL && right != NULL && (!typeEqual(left, right) || left->kind == FUNCTION)){
            // 赋值类型不匹配
            printf("Error type 5 at line %d: mismatched type\n", node->lineno);
            sem_error = true;
            return NULL;
        }
        return left;
    }
    else if(strcmp(node->children[1]->name, "AND") == 0
    || strcmp(node->children[1]->name, "OR") == 0){
        // EXP && || EXP
        Type left = Exp(node->children[0]);
        Type right = Exp(node->children[2]);
        if(left != NULL && right != NULL){
            if(left->kind == BASIC && right->kind == BASIC
            && left->basic == TYPE_INT && right->basic == TYPE_INT){
                return left;
            }
            // 只有 int 可以做逻辑运算
            printf("Error type 7 at line %d: only INT\n", node->lineno);
            sem_error = true;
        }
        return NULL;
    }
    else if(strcmp(node->children[1]->name, "DOT") == 0){
        // EXP DOT ID
        Type stk = Exp(node->children[0]);
        if(stk == NULL)  { return NULL; }
        else if(stk->kind != STRUCTURE){
            // 非结构体数据用 .
            printf("Error type 13 at line %d: illegal .\n", node->lineno);
            sem_error = true;
            return NULL;
        }
        char* str = node->children[2]->value.str;
        FieldList ptr = stk->structure->members;
        int flag = 0;
        while(ptr != NULL){
            if(strcmp(ptr->name, str) == 0){
                flag = 1;
                break;
            }
            ptr = ptr->next;
        }
        if(flag == 0){
            // 访问结构体中没有的域
            printf("Error type 14 at line %d: mismatch \'%s\'\n", node->lineno, str);
            sem_error = true;
            return NULL;
        }
        return ptr->type;
    }
    else if(strcmp(node->children[1]->name, "LB") == 0){
        // EXP LB EXP RB
        Type base = Exp(node->children[0]);
        Type offset = Exp(node->children[2]);
        if(base == NULL || offset == NULL){
            return NULL;
        } else if(base->kind != ARRAY){
            // 对非数组变量使用 []
            printf("Error type 10 at line %d: illegal []\n", node->lineno);
            sem_error = true;
            return NULL;
        } else if(offset->kind != BASIC || offset->basic != TYPE_INT){
            //  数组的索引不是整型
            printf("Error type 12 at line %d: index must be INT\n", node->lineno);
            sem_error = true;
            return NULL;
        }
        if(base->array.dimension == 1){
            return base->array.elem;
        } else {
            Type type = (Type)malloc(sizeof(Type_t));
            type->kind = ARRAY;
            type->array.dimension = base->array.dimension - 1;
            type->array.elem = base->array.elem;
            return type;
        }
    }
    else {
        // EXP ><= EXP
        // EXP +-*/ EXP
        Type left = Exp(node->children[0]);
        Type right = Exp(node->children[2]);
        if(left == NULL || right == NULL){
            return NULL;
        } else if(left->kind != BASIC || right->kind != BASIC){
            // 只有 basic 数据可以做算术运算
            printf("Error type 7 at line %d: only basic type for math\n", node->lineno);
            sem_error = true;
            return NULL;
        } else if(left->basic != right->basic){
            // 算数类型不匹配
            printf("Error type 7 at line %d: mismatched type\n", node->lineno);
            sem_error = true;
            return NULL;
        }
        if(strcmp(node->children[1]->name, "RELOP") == 0){
            Type type = (Type)malloc(sizeof(Type_t));
            type->kind = BASIC;
            type->basic = TYPE_INT;
            return type;
        }
        return left;
    }
}

FieldList Args(Node node){
    // Args 不关注 name
    FieldList list = (FieldList)malloc(sizeof(FieldList_t));
    list->type = Exp(node->children[0]);
    list->next = NULL;
    if(node->childnum == 3){
        list->next = Args(node->children[2]);
    }
    return list;
}

void check_field_in_one_struct(FieldList head){
    /**
     * head 是一个结构体定义的成员的链表
     * 此函数用于检测是否存在结构体内部的重名，即链表内的元素重名
     * 将一切重名成员剔除
     */
    if(head == NULL){
        return;
    }
    FieldList pre_ptr = head, ptr = pre_ptr->next;
    while(ptr != NULL){
        if(ptr->mark_line >= 0){
            FieldList roll = head;
            while(roll != ptr){
                if(strcmp(roll->name, ptr->name) == 0){
                    printf("Error type 15 at line %d: Duplicated var name in one struct\n", ptr->mark_line);
                    sem_error = true;
                    break;
                }
                roll = roll->next;
            }
            pre_ptr->next = ptr->next;
            FieldList trash = ptr;
            ptr = ptr->next;
            free(trash); trash = NULL;
        } else {
            pre_ptr = ptr;
            ptr = ptr->next;
        }
    }
}

void upload_func_params(FieldList param, FieldList mode){
    // 载入函数的形参
    // mode 可能是 NULL
    // 若 param 中有重名，但是与 mode 中的重名，则不报错
    if(param == NULL){
        return;
    } else if(mode != NULL && strcmp(param->name, mode->name) == 0){
        upload_func_params(param->next, mode->next);
    } else {
        Symbol sym = search_symbol(param->name);
        if(sym != NULL){
            // 命名重复
            printf("Error type 3 at line %d: var name confilct\n", param->mark_line);
            sem_error = true;
        } else {
            Symbol new_sym = create_symbol(param->name, param->type, param->mark_line);
            insert_symbol(new_sym);
        }
        if(mode == NULL){
            upload_func_params(param->next, NULL);
        } else {
            upload_func_params(param->next, mode->next);
        }
    }
}