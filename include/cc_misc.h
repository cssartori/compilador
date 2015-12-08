#ifndef __MISC_H
#define __MISC_H
#include <stdio.h>
#include "cc_dict.h"
#include "cc_tree.h"
#include "../include/dict_misc.h"

comp_dict_t* dicionario_global;
comp_tree_t* ast_programa;
lista_int* lista_auxiliar_inteiros;

int getLineNumber (void);
void yyerror (char const *mensagem);
void main_init (int argc, char **argv);
void main_finalize (void);
char* remove_aspa(char* str);

#endif
