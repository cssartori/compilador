#include "../include/lista_dicionarios.h"

lista_dicionarios* insere_dicionario_lista(lista_dicionarios* lista, comp_dict_t* dict)
{
	lista_dicionarios* aux = lista;
	lista = (lista_dicionarios*)malloc(sizeof(lista_dicionarios));
	lista->dict = dict;
	lista->proximo = aux;

	return lista;
}

lista_dicionarios* destroi_lista_dicionarios(lista_dicionarios* lista)
{
	lista_dicionarios* aux = lista;

	while(aux != NULL)
	{
		lista = aux;
		lista->dict = destroi_hash(lista->dict);
		aux = aux->proximo;
		free(lista);
	}

	return NULL;
}
