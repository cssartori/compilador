
#include "cc_dict.h"

#ifndef _DICT_MISC_H
#define _DICT_MISC_H

#define bool char
#define TRUE 1
#define FALSE 0

typedef struct fun_param_lista {
	int tipo_param;
	int tamanho;
	bool e_const;
	struct variable* var;
	struct fun_param_lista* proximo;
}fun_param_lista;

typedef struct function {
	int num_param;
	char* rotulo_funcao;
	//char* reg_retorno;
	struct fun_param_lista* parametros;
	int bytes_declarados;
}function;

typedef struct variable {
	struct lista_int* dimensoes;
	bool e_const;
	bool e_global;
	int tamanho;
	int tamanho_total;
	int deslocamento; //deslocamento da variável em relação ao seu escopo
}variable;

typedef struct lista_int{
	int valor;
	struct lista_int* proximo;
}lista_int;

fun_param_lista* cria_lista_parametros();
fun_param_lista* insere_parametro(fun_param_lista* parametros, variable* var, int tipo_param, bool e_const);
int conta_parametros(fun_param_lista* parametros);
fun_param_lista* destroi_lista_parametros(fun_param_lista* parametros);

lista_int* cria_lista_int();
lista_int* insere_lista_int(lista_int* lista, int valor);
lista_int* destroi_lista_int(lista_int* lista);

#endif

