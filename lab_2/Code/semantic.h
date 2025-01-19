#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "tree.h"

#define TYPE_INT 1
#define TYPE_FLOAT 2

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
};

typedef enum {
    IN_STRUCT,
    IN_COMPST,
    IN_FUNC_DEC
} Context ;

typedef enum{
    STRICT,
    RELAXED
} Standard ;

void semantic_transfer(Node root);
void check_func_def();
bool typeEqual(Type t1, Type t2);
void freeType(Type type, Standard standard);
void freeField(FieldList list, Standard standard);
void freeSymbolTable();
void freeSymbol(Symbol sym);
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