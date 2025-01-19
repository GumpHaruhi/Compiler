#include <stdio.h>
#include "tree.h"
#include "semantic.h"
#include "intercode.h"
#include "objectcode.h"

extern FILE * yyin;
extern int lex_error;       // 词法错误
extern int syn_error;       // 语法错误
extern bool sem_error;      // 语义错误

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
    // 词法分析与语法分析
    yyrestart(f);
    yyparse();
    fclose(f);

    if(lex_error == 0 && syn_error == 0){
        // 无语法错误， 打印语法树（到终端）
        upload_NULL_child(root);    // 空串的向上传递问题
        // print_SYN_Tree(root, 0);  
        
        // 语义分析
        semantic_transfer(root);

        if(!sem_error && argc >= 3){
            // 生成中间代码
            InterCode ir = generate_IR(root);

            if(argc == 4){
                // 输出中间代码
                printInterCode(argv[3], ir);
            }

            // 输出目标代码
            printObjectCode(argv[2], ir);
            freeInterCode(ir);
        }
    }
    freeTree(root);

    return 0;
}