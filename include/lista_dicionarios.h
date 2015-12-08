#ifndef __LISTA_DICIONARIOS_H
#define __LISTA_DICIONARIOS_H

#include "cc_dict.h"

typedef struct lista_dicionarios{
	comp_dict_t* dict;
	struct lista_dicionarios* proximo;
}lista_dicionarios;

lista_dicionarios* insere_dicionario_lista(lista_dicionarios* lista, comp_dict_t* dict);
lista_dicionarios* destroi_lista_dicionarios(lista_dicionarios* lista);


#endif
