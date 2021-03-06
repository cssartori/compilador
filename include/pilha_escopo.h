#ifndef __PILHA_ESCOPO_H
#define __PILHA_ESCOPO_H

#include "cc_dict.h"

typedef struct pilha_ids_escopo{
	comp_dict_t* tabela_escopo;
	struct pilha_ids_escopo* proximo;
}pilha_ids_escopo;


pilha_ids_escopo* cria_pilha_escopo();
pilha_ids_escopo* empilha_novo_escopo(pilha_ids_escopo* pilha, int deslocamento_base);
int deslocamento_base_escopo(pilha_ids_escopo* pilha);
void atualiza_deslocamento_escopo(pilha_ids_escopo* pilha, int novo_deslocamento_base);
comp_dict_item_t* insere_item_escopo(pilha_ids_escopo* pilha, comp_dict_item_t* item);
comp_dict_item_t* busca_item_escopo(pilha_ids_escopo* pilha, comp_dict_item_t* item);
comp_dict_item_t* busca_item_pilha(pilha_ids_escopo* pilha, comp_dict_item_t* item);
pilha_ids_escopo* desempilha_escopo(pilha_ids_escopo* pilha);
pilha_ids_escopo* destroi_pilha(pilha_ids_escopo* pilha);
void imprime_pilha(pilha_ids_escopo* pilha);


#endif
