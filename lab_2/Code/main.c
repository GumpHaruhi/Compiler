#include <stdio.h>
#include "tree.h"
#include "semantic.h"

extern FILE * yyin;
extern int lex_error;
extern int syn_error;

void perror(const char *__s);
int yyparse();
void yyrestart(FILE* f);
Node root = NULL;

extern void print_SYN_Tree(Node node, int index);
extern bool upload_NULL_child(Node node);

int main(int argc, char** argv){
    if(argc <= 1) { return 1; }
    FILE *f = fopen(argv[1], "r");
    if(!f){
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    yyparse();
    fclose(f);

    if(lex_error == 0 && syn_error == 0){
        // no syntax error
        upload_NULL_child(root);
        // print_SYN_Tree(root, 0);
        semantic_transfer(root);
    }
    freeTree(root);
       
    return 0;
}