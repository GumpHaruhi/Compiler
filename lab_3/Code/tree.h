#ifndef TREE_H
#define TREE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#define MAX_CHILD 16

extern int yylineno;
typedef enum Syn_state syn_state;
typedef struct Node_ Node_t;
typedef Node_t* Node;

enum Syn_state {
    SYN_NORMAL = 1,
    SYN_NULL,
    SYN_INT,
    SYN_FLOAT,
    SYN_ID,
    SYN_TYPE,
    SYN_TOKEN
};

struct Node_ {
    char *name;
    syn_state kind;
    int lineno;
    int childnum;
    union {
        int ival;
        float fval;
        char str[32];
    } value ;
    Node children[MAX_CHILD];
};

Node createNode(char *name, syn_state state, int lineno, int childno, ...);
void print_SYN_Tree(Node node, int index);
bool upload_NULL_child(Node node);
void freeTree(Node root);

#endif