#ifndef __CC_DICT_H
#define __CC_DICT_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dict_misc.h"

#define TAM_HASH 	1000
#define PASSO_HASH 	17

#define bool char
#define TRUE 1
#define FALSE 0

/*Códigos de retorno das funções da tabela hash*/
#define TABELA_CHEIA	-1
#define RETORNO_ERRO	-1
#define RETORNO_OK	 0

/*DEF que IDs podem assumir*/
#define FUNCAO 1
#define VARIAVEL_COMUM 2
#define VARIAVEL_VETOR 3


typedef union {
	struct function* _funcao;
	struct variable* _variavel;	
} valor_id;

/*Union que define os tipos literais possíveis a serem armazenados no dicionário*/
typedef union {
	int _int;
	float _float;
	char _char;
	char* _string;
	bool _bool;
	char* _id;
} valor_token;

typedef struct comp_dict_item_t
{
	int linha_lexema;
	int tipo_token;
	valor_token valor;
	char* lexema;

	/*Campos acessíveis em caso de se tratar de um Identificador*/
	int def_id; //O que o ID é: FUNCAO, VARIAVEL_COMUM, VARIAVEL_VETOR
	int tipo_id;
	bool e_static;
	valor_id vid;

} comp_dict_item_t;

typedef struct 
{
	char* chave; // lexema
	int chave_tipo;
	comp_dict_item_t conteudo;
}entrada_hash_t;

typedef struct 
{
	entrada_hash_t* dicionario[TAM_HASH];
	int deslocamento; //flag de marcação do deslocamento atual na memória em relação as variáveis já declaradas neste escopo
} comp_dict_t;

/*Função que cria uma tabela hash de TAM_HASH posições.*/
comp_dict_t* cria_hash(int deslocamento_base);

/*Função que insere um lexema na tabela hash. Utiliza busca_posição para calcular a posição de inserção.
Se a entrada já existir, atualiza a linha da mesma com o novo valor. Retorna um ponteiro para entrada.*/
comp_dict_item_t* insere_entrada(comp_dict_t *tabela_hash, char* lexema, int tipo_token, int linha_lexema);

comp_dict_item_t* busca_item_hash(comp_dict_t* tabela_hash, char* lexema, int tipo_token);

/*Função que lista as entradas da tabela em ordem de posição.*/
void lista_hash(comp_dict_t tabela_hash);

/*Função que destroi a tabela hash, liberando memória.*/
comp_dict_t* destroi_hash(comp_dict_t *tabela_hash);


void atualiza_item(comp_dict_item_t* item, int def_id, int tipo_id, bool e_static);

void atualiza_item_funcao(comp_dict_item_t* item, int num_param, struct fun_param_lista* parametros);

void atualiza_item_variavel(comp_dict_item_t* item, bool e_const, bool e_global, lista_int* dimensoes, int deslocamento_base);

void atualiza_deslocamento_dicionario(comp_dict_t* dicionario, int adicao);

void definir_rotulo_funcao(comp_dict_item_t* item, char* rotulo);

//void definir_reg_retorno_funcao(comp_dict_item_t* funcao, char* reg_retorno);

void atualiza_deslocamento_base(comp_dict_t* dict, int novo_deslocamento_base);

void atualiza_numero_bytes_declarados(comp_dict_item_t* funcao, int bytes_declarados);

int numero_declaracoes(comp_dict_item_t* funcao);

#endif
