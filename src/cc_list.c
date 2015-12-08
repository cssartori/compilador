#include "../include/cc_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int extern instrucoes_criadas;

lista_instrucoes* cria_lista_instrucoes(char* instrucao)
{
	lista_instrucoes* lista;
	lista = (lista_instrucoes*)malloc(sizeof(lista_instrucoes));
	lista->instrucao = strdup(instrucao);
	lista->numero_instrucoes_antes = 0;
	lista->anterior = NULL;
	lista->proximo = NULL;

	instrucoes_criadas++;

	return lista;
}

/*Concatena 'lista' a 'instrucoes' tornando 'instrucoes' a nova cabeça da lista*/
lista_instrucoes* concatena_lista_instrucoes(lista_instrucoes* lista, lista_instrucoes* instrucoes)
{
	if(instrucoes != NULL)
	{
		lista_instrucoes* aux = instrucoes;
		
		int modificador_numero_instrucoes = 0;
		if(lista != NULL)
			modificador_numero_instrucoes = lista->numero_instrucoes_antes+1;

		while(aux->proximo != NULL){
			aux->numero_instrucoes_antes+=modificador_numero_instrucoes;
			aux = aux->proximo;
		}

		aux->numero_instrucoes_antes+=modificador_numero_instrucoes;

		aux->proximo = lista;
		if(lista != NULL)
			lista->anterior = aux;

		return instrucoes;
	}

	return lista;
}


lista_instrucoes* concatena_lista_instrucoes_fim(lista_instrucoes* lista, lista_instrucoes* instrucoes)
{
	if(lista != NULL)
	{
		lista_instrucoes* aux = lista;
		
		int modificador_numero_instrucoes = 0;
		if(instrucoes != NULL)
			modificador_numero_instrucoes = instrucoes->numero_instrucoes_antes+1;

		while(aux->proximo != NULL){
			aux->numero_instrucoes_antes+=modificador_numero_instrucoes;
			aux = aux->proximo;
		}

		aux->numero_instrucoes_antes+=modificador_numero_instrucoes;

		aux->proximo = instrucoes;
		if(instrucoes != NULL)
			instrucoes->anterior = aux;

		return lista;
	}

	return instrucoes;
}

lista_instrucoes* destroi_lista_instrucoes(lista_instrucoes* lista)
{
	lista_instrucoes* aux = lista;
	while(aux != NULL)
	{
		lista = aux;
		aux = aux->proximo;

		free(lista->instrucao);
		free(lista);
	}

	return NULL;
}

void imprime_lista_instrucoes(lista_instrucoes* lista)
{
	lista_instrucoes* aux = lista;

	if(aux != NULL)
	{
		while(aux->proximo != NULL)	
			aux = aux->proximo;

		while(aux != NULL)
		{
			printf("%s\n", aux->instrucao);
			aux = aux->anterior;
			
		}

	}
	else
		printf("Programa vazio.\n");
}
