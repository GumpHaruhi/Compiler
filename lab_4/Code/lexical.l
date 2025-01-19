%{
    #include "syntax.tab.h"
    #include <stdlib.h>

    int lex_error = 0;
    int yycolumn = 1;
#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;

    int ato_hex(char *text);
    int ato_oct(char *text);
%}

%option yylineno
space   [ \t\v\f]+
CRLF    \r?\n
digit   [0-9]
letter  [_a-zA-Z]
relop   ">"|"<"|">="|"<="|"=="|"!="
type    "int"|"float"
id      {letter}({letter}|{digit})*
dec     0|[1-9]{digit}*
hex     0[xX][0-9a-fA-F]+
oct     0[0-7]+
float   ({digit}+\.{digit}+)

%%

{space} { }
{CRLF}  { yycolumn = 1; }
"//"    {   char c = input(); 
            while (c != '\n') c = input(); }
"/*"    {   char a = input();
            char b = input();
            while (!(a == '*' && b == '/')) { a = b; b = input(); } }
";"     { yylval.node = createNode("SEMI", SYN_TOKEN, yylineno, 0); return SEMI; }
","     { yylval.node = createNode("COMMA", SYN_TOKEN, yylineno, 0); return COMMA; }
"="     { yylval.node = createNode("ASSIGNOP", SYN_TOKEN, yylineno, 0); return ASSIGNOP; }
{relop} { yylval.node = createNode("RELOP", SYN_TOKEN, yylineno, 0);
        strcpy(yylval.node->value.str, yytext);
        return RELOP; }
"+"     { yylval.node = createNode("PLUS", SYN_TOKEN, yylineno, 0); return PLUS; }
"-"     { yylval.node = createNode("MINUS", SYN_TOKEN, yylineno, 0); return MINUS; }
"*"     { yylval.node = createNode("STAR", SYN_TOKEN, yylineno, 0); return STAR; }
"/"     { yylval.node = createNode("DIV", SYN_TOKEN, yylineno, 0); return DIV; }
"&&"    { yylval.node = createNode("AND", SYN_TOKEN, yylineno, 0); return AND; }
"||"    { yylval.node = createNode("OR", SYN_TOKEN, yylineno, 0); return OR; }
"."     { yylval.node = createNode("DOT", SYN_TOKEN, yylineno, 0); return DOT; }
"!"     { yylval.node = createNode("NOT", SYN_TOKEN, yylineno, 0); return NOT; }
{type}  { yylval.node = createNode("TYPE", SYN_TYPE, yylineno, 0);
        strcpy(yylval.node->value.str, yytext);
        return TYPE; }
"("     { yylval.node = createNode("LP", SYN_TOKEN, yylineno, 0); return LP; }
")"     { yylval.node = createNode("RP", SYN_TOKEN, yylineno, 0); return RP; }
"["     { yylval.node = createNode("LB", SYN_TOKEN, yylineno, 0); return LB; }
"]"     { yylval.node = createNode("RB", SYN_TOKEN, yylineno, 0); return RB; }
"{"     { yylval.node = createNode("LC", SYN_TOKEN, yylineno, 0); return LC; }
"}"     { yylval.node = createNode("RC", SYN_TOKEN, yylineno, 0); return RC; }
if      { yylval.node = createNode("IF", SYN_TOKEN, yylineno, 0); return IF; }
else    { yylval.node = createNode("ELSE", SYN_TOKEN, yylineno, 0); return ELSE; }
while   { yylval.node = createNode("WHILE", SYN_TOKEN, yylineno, 0); return WHILE; }
struct  { yylval.node = createNode("STRUCT", SYN_TOKEN, yylineno, 0); return STRUCT; }
return  { yylval.node = createNode("RETURN", SYN_TOKEN, yylineno, 0); return RETURN; }

{dec}   { yylval.node = createNode("INT", SYN_INT, yylineno, 0); 
        yylval.node->value.ival = atoi(yytext); 
        return INT; }
{oct}   { yylval.node = createNode("INT", SYN_INT, yylineno, 0); 
        yylval.node->value.ival = ato_oct(yytext); 
        return INT; }
{hex}   { yylval.node = createNode("INT", SYN_INT, yylineno, 0); 
        yylval.node->value.ival = ato_hex(yytext); 
        return INT; }

{float} { yylval.node = createNode("FLOAT", SYN_FLOAT, yylineno, 0); 
        yylval.node->value.fval = strtof(yytext, NULL); 
        return FLOAT; }
{id}    { yylval.node = createNode("ID", SYN_ID, yylineno, 0); 
        strcpy(yylval.node->value.str, yytext); 
        return ID; }

0[0-9]+     {
    printf("Error type A at Line %d: illegal oct: \'%s\'\n", yylineno, yytext);
    lex_error += 1; return INT;
    }
0[xX][0-9a-zA-Z]+   {
    printf("Error type A at Line %d: illegal hex: \'%s\'\n", yylineno, yytext);
    lex_error += 1; return INT;
    }
[0-9]*[.][0-9]*[eE][+-]?[0-9]*  {
    printf("Error type A at Line %d: illegal float: \'%s\'\n", yylineno, yytext);
    lex_error += 1; return FLOAT;
    }
.   { 
    printf("Error type A at Line %d: Mysterious characters: \'%s\'\n", yylineno, yytext); 
    lex_error += 1; 
    }

%%

int yywrap() {
    return 1;
}

int tempChar(char c){
    if(c >= '0' && c <= '9'){
		return c - '0';
	} else if(c >= 'a' && c <= 'z'){
		return c - 'a' + 10;
	} else if(c >= 'A' && c <= 'Z'){
		return c - 'A' + 10;
	}
}

int ato_oct(char *text){
    int ret = 0;
    int lenth = strlen(text);
    for(int i=0; i < lenth; i++){
        ret *= 8;
        ret += tempChar(text[i]);
    }
    return ret;
}

int ato_hex(char *text){
    int ret = 0;
    int lenth = strlen(text);
    for(int i=2; i < lenth; i++){
        ret *= 16;
        ret += tempChar(text[i]);
    }
    return ret;
}