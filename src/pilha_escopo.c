#include "pilha_escopo.h"
#include <stdio.h>

pilha_ids_escopo* cria_pilha_escopo()
{
	return NULL;
}

pilha_ids_escopo* empilha_novo_escopo(pilha_ids_escopo* pilha, int deslocamento_base)
{
	pilha_ids_escopo* p = pilha;

	pilha = (pilha_ids_escopo*)malloc(sizeof(pilha_ids_escopo));
	
	if(pilha != NULL)
	{
		pilha->tabela_escopo = cria_hash(deslocamento_base);
		pilha->proximo = p;
	}

	return pilha;
}

int deslocamento_base_escopo(pilha_ids_escopo* pilha)
{
	if(pilha == NULL)
		return 0;

	return pilha->tabela_escopo->deslocamento;
}

void atualiza_deslocamento_escopo(pilha_ids_escopo* pilha, int novo_deslocamento_base)
{
	if(pilha != NULL)
	{
		atualiza_deslocamento_base(pilha->tabela_escopo, novo_deslocamento_base);
	}
}

comp_dict_item_t* insere_item_escopo(pilha_ids_escopo* pilha, comp_dict_item_t* item)
{
	if(pilha == NULL)
		return NULL;

	return insere_entrada(pilha->tabela_escopo, item->lexema, item->tipo_token, item->linha_lexema); 
}

comp_dict_item_t* busca_item_escopo(pilha_ids_escopo* pilha, comp_dict_item_t* item)
{
	comp_dict_item_t* item_encontrado=NULL;
	
	if(pilha != NULL)
		item_encontrado = busca_item_hash(pilha->tabela_escopo, item->lexema, item->tipo_token);

	return item_encontrado;
}

comp_dict_item_t* busca_item_pilha(pilha_ids_escopo* pilha, comp_dict_item_t* item)
{
	comp_dict_item_t* item_encontrado = NULL;

	while(pilha != NULL)
	{
		item_encontrado = busca_item_escopo(pilha, item);
		if(item_encontrado != NULL)
			break;
		pilha = pilha->proximo;
	}

	return item_encontrado;
}

pilha_ids_escopo* desempilha_escopo(pilha_ids_escopo* pilha)
{
	pilha_ids_escopo* p = pilha->proximo;

	pilha->tabela_escopo = NULL;

	free(pilha);

	pilha = NULL;

	return p;
}

pilha_ids_escopo* destroi_pilha(pilha_ids_escopo* pilha)
{

	while(pilha != NULL)
	{
		pilha = desempilha_escopo(pilha);
	}

	return NULL;
}

void imprime_pilha(pilha_ids_escopo* pilha)
{
	int i=0;
	printf("**********INICIO DA PILHA DE ESCOPOS***************\n");
	while(pilha != NULL)
	{
		printf("%i - ", i++);
		lista_hash(*pilha->tabela_escopo);
		printf("\n");
		pilha = pilha->proximo;
	}
	printf("**********FIM DA PILHA DE ESCOPOS***************\n");
}
