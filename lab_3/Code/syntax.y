%locations

%{
    #include <stdio.h>
    #include <stdlib.h>
    #include "tree.h"
    #include "lex.yy.c"

    extern Node root;
    extern int yylineno;
    int syn_error = 0;
    void yyerror(const char *msg);
%}

%union {
    Node node;
}

%token <node> INT FLOAT ID SEMI COMMA ASSIGNOP RELOP
%token <node> PLUS MINUS STAR DIV AND OR DOT NOT TYPE
%token <node> LP RP LB RB LC RC IF ELSE WHILE STRUCT RETURN

%right  ASSIGNOP
%left   OR
%left   AND
%left   RELOP
%left   PLUS MINUS
%left   STAR DIV
%right  NOT
%left   LP RP LB RB DOT

%nonassoc   LOWER_THAN_ELSE
%nonassoc   ELSE

%type <node> Program ExtDefList ExtDef ExtDecList Specifier 
%type <node> StructSpecifier OptTag Tag VarDec FunDec VarList ParamDec
%type <node> CompSt StmtList Stmt DefList Def DecList Dec Exp Args

%%

Program     : ExtDefList            {$$=createNode("Program", SYN_NORMAL, @$.first_line, 1, $1); root = $$; }
            ;
ExtDefList  : ExtDef ExtDefList     {$$=createNode("ExtDefList", SYN_NORMAL, @$.first_line, 2, $1, $2); }
            | /* empty */           {$$=createNode("ExtDefList", SYN_NULL, @$.first_line, 0); }
            ;
ExtDef      : Specifier ExtDecList SEMI     {$$=createNode("ExtDef", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | Specifier SEMI        {$$=createNode("ExtDef", SYN_NORMAL, @$.first_line, 2, $1, $2); }
            | Specifier FunDec CompSt       {$$=createNode("ExtDef", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | Specifier FunDec SEMI         {$$=createNode("ExtDef", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | error SEMI            { syn_error++; }
            ;
ExtDecList  : VarDec            {$$=createNode("ExtDecList", SYN_NORMAL, @$.first_line, 1, $1); }
            | VarDec COMMA ExtDecList       {$$=createNode("ExtDecList", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            ;
Specifier   : TYPE              {$$=createNode("Specifier", SYN_NORMAL, @$.first_line, 1, $1); }
            | StructSpecifier       {$$=createNode("Specifier", SYN_NORMAL, @$.first_line, 1, $1); }
            ;
StructSpecifier : STRUCT OptTag LC DefList RC   {
                            $$=createNode("StructSpecifier", SYN_NORMAL, @$.first_line, 5, $1, $2, $3, $4, $5); }
            | STRUCT Tag    {$$=createNode("StructSpecifier", SYN_NORMAL, @$.first_line, 2, $1, $2); }
            ;
OptTag      : ID            {$$=createNode("OptTag", SYN_NORMAL, @$.first_line, 1, $1); }
            | /* empty */   {$$=createNode("OptTag", SYN_NULL, @$.first_line, 0); }
            ;
Tag         : ID            {$$=createNode("Tag", SYN_NORMAL, @$.first_line, 1, $1); }
            ;
VarDec      : ID            {$$=createNode("VarDec", SYN_NORMAL, @$.first_line, 1, $1); }
            | VarDec LB INT RB      {$$=createNode("VarDec", SYN_NORMAL, @$.first_line, 4, $1, $2, $3, $4); }
            ;
FunDec      : ID LP VarList RP      {$$=createNode("FunDec", SYN_NORMAL, @$.first_line, 4, $1, $2, $3, $4); }
            | ID LP RP              {$$=createNode("FunDec", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | ID LP error RP        { syn_error++; }
            | error RP              { syn_error++; }
            ;
VarList     : ParamDec COMMA VarList        {$$=createNode("VarList", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | ParamDec              {$$=createNode("VarList", SYN_NORMAL, @$.first_line, 1, $1); }
            ;
ParamDec    : Specifier VarDec      {$$=createNode("ParamDec", SYN_NORMAL, @$.first_line, 2, $1, $2); }
            | Specifier error       { syn_error++; }
            ;
CompSt      : LC DefList StmtList RC        {$$=createNode("CompSt", SYN_NORMAL, @$.first_line, 4, $1, $2, $3, $4); }
            | error RC              { syn_error++; }
            ;
StmtList    : Stmt StmtList         {$$=createNode("StmtList", SYN_NORMAL, @$.first_line, 2, $1, $2); }
            | /* empty */           {$$=createNode("StmtList", SYN_NULL, @$.first_line, 0); }
            ;
Stmt        : Exp SEMI      {$$=createNode("Stmt", SYN_NORMAL, @$.first_line, 2, $1, $2); }
            | CompSt        {$$=createNode("Stmt", SYN_NORMAL, @$.first_line, 1, $1); }
            | RETURN Exp SEMI       {$$=createNode("Stmt", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE       {
                            $$=createNode("Stmt", SYN_NORMAL, @$.first_line, 5, $1, $2, $3, $4, $5); }
            | IF LP Exp RP Stmt ELSE Stmt   {
                            $$=createNode("Stmt", SYN_NORMAL, @$.first_line, 7, $1, $2, $3, $4, $5, $6, $7); }
            | WHILE LP Exp RP Stmt      {$$=createNode("Stmt", SYN_NORMAL, @$.first_line, 5, $1, $2, $3, $4, $5); }
            | Exp error             { syn_error++; }
            | error SEMI            { syn_error++; }
            ;
DefList     : Def DefList       {$$=createNode("DefList", SYN_NORMAL, @$.first_line, 2, $1, $2); }
            | /* empty */       {$$=createNode("DefList", SYN_NULL, @$.first_line, 0); }
            ;
Def         : Specifier DecList SEMI        {$$=createNode("Def", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | Specifier error SEMI          { syn_error++; }
            | error SEMI                    { syn_error++; }
            ;
DecList     : Dec           {$$=createNode("DecList", SYN_NORMAL, @$.first_line, 1, $1); }
            | Dec COMMA DecList         {$$=createNode("DecList", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            ;
Dec         : VarDec        {$$=createNode("Dec", SYN_NORMAL, @$.first_line, 1, $1); }
            | VarDec ASSIGNOP Exp       {$$=createNode("Dec", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            ;
Exp         : Exp ASSIGNOP Exp      {$$=createNode("Exp", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | Exp AND Exp           {$$=createNode("Exp", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | Exp OR Exp            {$$=createNode("Exp", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | Exp RELOP Exp         {$$=createNode("Exp", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | Exp PLUS Exp          {$$=createNode("Exp", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | Exp MINUS Exp         {$$=createNode("Exp", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | Exp STAR Exp          {$$=createNode("Exp", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | Exp DIV Exp           {$$=createNode("Exp", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | LP Exp RP             {$$=createNode("Exp", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | MINUS Exp             {$$=createNode("Exp", SYN_NORMAL, @$.first_line, 2, $1, $2); }
            | NOT Exp               {$$=createNode("Exp", SYN_NORMAL, @$.first_line, 2, $1, $2); }
            | ID LP Args RP         {$$=createNode("Exp", SYN_NORMAL, @$.first_line, 4, $1, $2, $3, $4); }
            | ID LP RP              {$$=createNode("Exp", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | Exp LB Exp RB         {$$=createNode("Exp", SYN_NORMAL, @$.first_line, 4, $1, $2, $3, $4); }
            | Exp DOT ID            {$$=createNode("Exp", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | ID            {$$=createNode("Exp", SYN_NORMAL, @$.first_line, 1, $1); }
            | INT           {$$=createNode("Exp", SYN_NORMAL, @$.first_line, 1, $1); }
            | FLOAT         {$$=createNode("Exp", SYN_NORMAL, @$.first_line, 1, $1); }
            | error         { syn_error++; }
            ;
Args        : Exp COMMA Args        {$$=createNode("Args", SYN_NORMAL, @$.first_line, 3, $1, $2, $3); }
            | Exp           {$$=createNode("Args", SYN_NORMAL, @$.first_line, 1, $1); }
            ;

%%

void yyerror(const char *msg){
    printf("Error type B at Line %d : %s\n",yylineno,msg);
}