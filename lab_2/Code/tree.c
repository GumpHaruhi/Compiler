#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"

Node createNode(char *name, syn_state state, int lineno, int childnum, ...){
    Node node = (Node)malloc(sizeof(Node_t));
    node->name = (char*)malloc(sizeof(char) * strlen(name));
    strcpy(node->name, name);
    node->type = state;
    node->lineno = lineno;
    node->childnum = childnum;

    for(int i=0; i < MAX_CHILD; i++){
        node->children[i] = NULL;
    }
    if(node->type == SYN_NULL || node->childnum == 0){
        return node;
    }

    va_list param;
    va_start(param, childnum);
    for(int i=0; i < childnum; i++){
        node->children[i] = va_arg(param, Node);
    }
    return node;
}

void print_SYN_Tree(Node node, int index){
    if(node == NULL || node->type == SYN_NULL) { return; }

    for(int i=0; i < index*2; i++){
        printf(" ");
    }

    switch (node->type){
    case SYN_INT:
        printf("INT: %d", node->value.ival);
        break;
    case SYN_FLOAT:
        printf("FLOAT: %lf", node->value.fval);
        break;
    case SYN_ID:
        printf("ID: %s", node->value.str);
        break;
    case SYN_TYPE:
        printf("TYPE: %s", node->value.str);
        break;
    case SYN_NORMAL:
        printf("%s (%d)", node->name, node->lineno);
        break;
    case SYN_TOKEN:
        printf("%s", node->name);
        break;
    default:
        break;
    }
    printf("\n");

    for(int i=0; i < node->childnum; i++){
        print_SYN_Tree(node->children[i], index+1);
    }
}

bool upload_NULL_child(Node node){
    if(node->type == SYN_NULL){
        // 是空串
        return true;
    } else if(node->childnum == 0){
        // 是终结符，在最底层
        return false;
    }

    bool flag = true;
    for(int i=0; i < node->childnum; i++){
        flag = flag && upload_NULL_child(node->children[i]);
    }
    if(flag){
        node->type = SYN_NULL;
        return true;
    } else {
        return false;
    }
}

void freeTree(Node root){
    if(root == NULL){
        return ;
    }
    for(int i=0; i < root->childnum; i++){
        freeTree(root->children[i]);
    }
    free(root->name);
    free(root);
}