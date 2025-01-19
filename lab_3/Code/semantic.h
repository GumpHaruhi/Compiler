#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "tree.h"

#define TYPE_INT 1
#define TYPE_FLOAT 2
#define TABLE_SIZE 200

// 前向声明
typedef struct Operand_ Operand_t;
typedef Operand_t* Operand;

typedef struct Type_ Type_t;
typedef Type_t *Type;
typedef struct FieldList_ FieldList_t;
typedef FieldList_t *FieldList;
typedef struct Structure_ Structure_t;
typedef Structure_t *Structure;
typedef struct Function_ Function_t;
typedef Function_t *Function;
typedef struct Symbol_ Symbol_t;
typedef Symbol_t *Symbol;

struct Type_{
    enum {
        BASIC,
        ARRAY,
        STRUCTURE,
        STRUCT_DEF,
        FUNCTION
    } kind;
    union {
        int basic;
        struct{
            Type elem;
            int dimension;
            int size[10];  // for lab 3, 从低维到高维的大小
        } array;
        Structure structure;
        Function func;
    };
};

struct FieldList_{
    char name[32];
    Type type;
    int mark_line;
    FieldList next;
};

struct Structure_{
    char name[32];
    FieldList members;
};

struct Function_{
    char name[32];
    Type ret_type;
    int lineno;
    bool defined;
    FieldList param;
};

struct Symbol_{
    char name[32];
    Type type;
    int lineno;
    Symbol next;
    Operand op;   // for lab 3
};

typedef enum {
    IN_STRUCT,
    IN_COMPST,
    IN_FUNC_DEC
} Context ;

void semantic_transfer(Node root);
void init_symbol_table();
Symbol create_symbol(char* str, Type type, int lineno);
void insert_symbol(Symbol sym);
Symbol search_symbol(char* str);
void check_func_def();
bool typeEqual(Type t1, Type t2);
void Program(Node node);
void ExtDefList(Node node);
void ExtDef(Node node);
Type Specifier(Node node);
void ExtDecList(Node node, Type type);
Function FunDec(Node node);
Type StructSpecifier(Node node);
char* OptTag(Node node);
FieldList VarList(Node node);
FieldList ParamDec(Node node);
FieldList DefList(Node node, Context ctx);
FieldList Def(Node node, Context ctx);
FieldList DecList(Node node, Context ctx, Type type);
FieldList Dec(Node node, Context ctx, Type type);
FieldList VarDec(Node node, Context ctx, Type type, int layer);
void CompSt(Node node, Type ret_type);
void StmtList(Node node, Type ret_type);
void Stmt(Node node, Type ret_type);
Type Exp(Node node);
FieldList Args(Node node);
void check_field_in_one_struct(FieldList head);
void upload_func_params(FieldList param, FieldList mode);

#endif