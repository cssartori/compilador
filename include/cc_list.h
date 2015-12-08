#ifndef __CC_LIST_H
#define __CC_LIST_H

typedef struct lista_instrucoes {
	char* instrucao;
	int numero_instrucoes_antes;
	struct lista_instrucoes* anterior;
	struct lista_instrucoes* proximo;
}lista_instrucoes;

lista_instrucoes* cria_lista_instrucoes(char* instrucao);
/*Concatena 'lista' a 'instrucoes' tornando 'instrucoes' a nova cabeça da lista*/
lista_instrucoes* concatena_lista_instrucoes(lista_instrucoes* lista, lista_instrucoes* instrucoes);
/*Concatena 'instrucoes' ao final de lista*/
lista_instrucoes* concatena_lista_instrucoes_fim(lista_instrucoes* lista, lista_instrucoes* instrucoes);
lista_instrucoes* destroi_lista_instrucoes(lista_instrucoes* lista);
void imprime_lista_instrucoes(lista_instrucoes* lista);

#endif
