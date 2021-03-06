#include "cc_misc.h"


extern int getyylineno();

int getLineNumber (void)
{
  return getyylineno();
}

void yyerror (char const *mensagem)
{
  fprintf (stderr, "%d - %s\n", getLineNumber(), mensagem);
}

void main_init (int argc, char **argv)
{
	dicionario_global = NULL;
	dicionario_global = cria_hash(0); //Inicialização do dicionário global
	lista_auxiliar_inteiros = NULL;

	ast_programa = NULL; //Inicializa a AST do programa como NULL
}

void main_finalize (void)
{
	destroi_lista_int(lista_auxiliar_inteiros);
	dicionario_global =  destroi_hash(dicionario_global); //Libera a memória ocupada pelo dicionário global
	ast_programa = destroi_tree(ast_programa); //Libera a memória ocupada pela AST do programa
}

/*Função que remove tanto aspas simples, quanto duplas.*/
char* remove_aspa(char* str)
{
	char* sem_aspa = NULL;
	if(str != NULL)
	{
		sem_aspa = strdup(str);
		int ptr = 0;
		//avança todos os caracteres uma posição
		while(sem_aspa[ptr] != '\0')
		{
			sem_aspa[ptr] = sem_aspa[ptr+1];
			ptr++;
		}
		//coloca o '\0' sobre a última aspa
		sem_aspa[ptr-2] = sem_aspa[ptr-1];
	}

	return sem_aspa;
}
