/*
Grupo Tibuga:
	-Carlo Sulzbach Sartori
	-Marcelo Magno Rodrigues
*/

%{
#include "parser.h" //arquivo automaticamente gerado pelo bison
#include "../include/cc_misc.h"
#include "../include/main.h"
#include "../include/cc_dict.h"
#include <stdio.h>
%}

/*Permite pegar o numero da linha atual no scanner atraves da variavel yylineno*/
%option yylineno

/*Estado para reconhecer bloco de comentários*/
%x E_COMENTARIO_BLOCO



/*Definições básicas para utilizar nas ER's*/

/*Digitos sao numeros de 0 a 9*/
digito [0-9] 

/*Letras sao quaisquer letras maiusculas ou minusculas: 'A' a 'Z' e 'a' a 'z'*/
qualquer_letra [a-zA-Z]

/*Espaços em branco sao tanto ' ' quanto '\t'*/
espaco_branco [ \t\n]

/*Comentario em linha pode conter qualquer caracter ate o FIM daquela mesma linha ($)*/
comentario_linha "//"(.)*$

/*Inicio de bloco de comentario*/
inicio_bloco_comentario "/*"
/*Fim de bloco de comentario*/
fim_bloco_comentario "*/"

/*Caractere e qualquer simbolo unico*/
chr ['][^'][']

/*Literais booleanos*/
true_l true
false_l false

/*String pode conter qualquer caractere um numero "infinito" de vezes exceto as ", que delimitam seu fim*/
str ["][^"]*["]

/*Inteiros podem ser positivos ou negativos e sao formados por um ou mais digitos*/
inteiro {digito}+

/*Ponto flutuante pode ser positivo ou negativo, formado por zero ou mais digitos iniciais, seguidos de ponto e um ou mais digitos*/
ponto_flutuante {digito}+\.{digito}+

/*Identificadores podem iniciar com qualquer letra ou '_' e depois serem seguidos por qualquer letra, digito ou '_' um numero "infinito" de vezes*/
id ({qualquer_letra}|_)+({qualquer_letra}|_|{digito})*

%%

{comentario_linha} { }
{inicio_bloco_comentario}		{ BEGIN(E_COMENTARIO_BLOCO); } /*Quando detecta o inicio de bloco de comentario, inicia o estado E_COMENTARIO_BLOCO e só aceitará regras que tenham a tag <E_COMENTARIO_BLOCO> na frente*/
<E_COMENTARIO_BLOCO>(.|\n) { } /*Enquanto estiver no estado de comentario de bloco, aceita qualquer coisa e não faz nada*/
<E_COMENTARIO_BLOCO><<EOF>> 	{return TOKEN_ERRO;}
<E_COMENTARIO_BLOCO>{fim_bloco_comentario} 	{ BEGIN(INITIAL); } /*Quando encontrar o fim de bloco de comentario, seta o estado como inicial (pre definido pelo flex) e volta ao "normal"*/
{espaco_branco} { }

"," {return yytext[0];} 
";" {return yytext[0];}
":" {return yytext[0];}
"(" {return yytext[0];}
")" {return yytext[0];}
"[" {return yytext[0];}
"]" {return yytext[0];}
"{" {return yytext[0];}
"}" {return yytext[0];}
"+" {return yytext[0];}
"-" {return yytext[0];}
"*" {return yytext[0];}
"/" {return yytext[0];}
"<" {return yytext[0];}
">" {return yytext[0];}
"=" {return yytext[0];}
"!" {return yytext[0];}
"&" {return yytext[0];}
"$" {return yytext[0];}

"int" {return TK_PR_INT;}
"float" {return TK_PR_FLOAT;}
"bool" {return TK_PR_BOOL;}
"char" {return TK_PR_CHAR;}
"string" {return TK_PR_STRING;}
"if" {return TK_PR_IF;}
"then" {return TK_PR_THEN;}
"else" {return TK_PR_ELSE;}
"while" {return TK_PR_WHILE;}
"do" {return TK_PR_DO;}
"input" {return TK_PR_INPUT;}
"output" {return TK_PR_OUTPUT;}
"return" {return TK_PR_RETURN;}
"static" {return TK_PR_STATIC;}
"const" {return TK_PR_CONST;}

"<=" {return TK_OC_LE;}
">=" {return TK_OC_GE;}
"==" {return TK_OC_EQ;}
"!=" {return TK_OC_NE;}
"&&" {return TK_OC_AND;}
"||" {return TK_OC_OR;}
"<<" {return TK_OC_SL;}
">>" {return TK_OC_SR;}
"=>" {return TK_OC_IN;}

{true_l} {yylval.valor_simbolo_lexico = insere_entrada(dicionario_global, yytext, SIMBOLO_LITERAL_BOOL, getLineNumber());return TK_LIT_TRUE;}
{false_l} {yylval.valor_simbolo_lexico = insere_entrada(dicionario_global, yytext, SIMBOLO_LITERAL_BOOL, getLineNumber());return TK_LIT_FALSE;}
{inteiro} {yylval.valor_simbolo_lexico = insere_entrada(dicionario_global, yytext, SIMBOLO_LITERAL_INT, getLineNumber());return TK_LIT_INT;}
{ponto_flutuante} {yylval.valor_simbolo_lexico = insere_entrada(dicionario_global, yytext, SIMBOLO_LITERAL_FLOAT, getLineNumber());return TK_LIT_FLOAT;}
{chr} {char* sem_aspa = remove_aspa(yytext); yylval.valor_simbolo_lexico = insere_entrada(dicionario_global, sem_aspa, SIMBOLO_LITERAL_CHAR, getLineNumber()); free(sem_aspa); return TK_LIT_CHAR;}
{str} {char* sem_aspa = remove_aspa(yytext); yylval.valor_simbolo_lexico = insere_entrada(dicionario_global, sem_aspa, SIMBOLO_LITERAL_STRING, getLineNumber()); free(sem_aspa); return TK_LIT_STRING;}

{id} {yylval.valor_simbolo_lexico = insere_entrada(dicionario_global, yytext, SIMBOLO_IDENTIFICADOR, getLineNumber()); return TK_IDENTIFICADOR;}

. {return TOKEN_ERRO;}

%%

/*Funcao que retorna o valor da variavel de contagem de linhas yylineno*/
int getyylineno(){
	return yylineno;
}
