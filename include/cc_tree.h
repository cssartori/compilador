#ifndef __CC_TREE_H
#define __CC_TREE_H

#include "cc_ast.h"
#include "cc_dict.h"
#include "cc_list.h"

/*Estrutura do nó da árvore de sintaxe abstrata (AST)*/
typedef struct comp_tree_t {
	int tipo_ast;
	int tipo;
	int coercao;

	char* t; //labels para true e false
	char* f;
	char* next; // rotulo para o proximo comando
	char* reg_temp;
	char* vec_reg_temp; //offset a partir da primeira posição do vetor
	lista_instrucoes* codigo;

	comp_dict_item_t* conteudo;
	struct comp_tree_t* filho[3];
	struct comp_tree_t* proximo;
}comp_tree_t;


/*Função que cria um nó da árvore AST. Retorna um ponteiro para esse novo nó criado.*/
comp_tree_t* cria_no(int tipo_ast, int tipo, comp_dict_item_t* item);
/*Função que insere um nó filho na posição (filho_pos) do nó pai.*/
comp_tree_t* insere_no(comp_tree_t* no_pai, comp_tree_t* no_filho, int filho_pos);
/*Função que insere o próximo nó do nó em questão (formando uma lista).*/
comp_tree_t* insere_proximo(comp_tree_t* no, comp_tree_t* no_proximo);
/*Função que destrói toda a árvore, liberando espaço em memória.*/
comp_tree_t* destroi_tree(comp_tree_t* raiz);
/*Função que seta um tipo de coercao para um determinado no*/
comp_tree_t* define_coercao(comp_tree_t* no, int coercao);

#endif
