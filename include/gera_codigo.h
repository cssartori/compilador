#ifndef __GERA_CODIGO_H
#define __GERA_CODIGO_H
//#include "cc_list.h"
#include "cc_tree.h"

#define TAM_INSTR 50 //tamanho do buffer de caracteres que suporta uma instrução


lista_instrucoes* gera_codigo(comp_tree_t* no_ast);


#endif
