#ifndef INTERCODE_H
#define INTERCODE_H

#include <stdio.h>
#include <stdlib.h>
#include "semantic.h"

typedef struct Operand_ Operand_t;
typedef Operand_t* Operand;
typedef struct InterCode_ InterCode_t;
typedef InterCode_t* InterCode;

struct Operand_ {
    enum {
        VARIABLE, TEMP_VAR, CONSTANT,
        GET_ADDR, GET_VAL, LABEL, FUNC
    } kind ;
    union {
        int no;
        int value;
        char name[32];
        Operand opr;
    };
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
    union {
        char relop[4];
        int size;
    };
    InterCode prev;
    InterCode next;
};

enum Operator{
    PLUS_OP,
    MINUS_OP,
    STAR_OP,
    DIV_OP
};


InterCode generate_IR(Node program);
void printInterCode(char* filepath, InterCode code);
void freeInterCode(InterCode code);

void printOperand(Operand op, FILE* fp);
void insertCode(InterCode dst, InterCode src);
void formatCode(InterCode code);
int getSizeof(Type type);
InterCode translate_ExtDefList(Node node);
InterCode translate_ExtDef(Node node);
InterCode translate_Function(Function func, Operand place);
InterCode translate_CompSt(Node node);
InterCode translate_ParamList(FieldList param);
InterCode translate_DefList(Node node);
InterCode translate_Def(Node node);
InterCode translate_DecList(Node node, Type type);
InterCode translate_Dec(Node node, Type type); 
FieldList translate_VarDec(Node node, Type type, Type arr_type, int layer);
InterCode translate_StmtList(Node node);
InterCode translate_Stmt(Node node);
InterCode translate_Condition(Node exp, Operand labeltrue, Operand labelfalse);
InterCode translate_Exp(Node node, Operand place);
InterCode translate_Args(Node node);
InterCode arithmetic(Operand dst, Operand src1, Operand src2, enum Operator op);

#endif