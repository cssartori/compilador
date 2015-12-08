#include <stdlib.h>
#include "../include/gera_codigo.h"
#include "../include/cc_dict.h"

#define BOOL char
#define TRUE 1
#define FALSE 0

/*
	Modelo RA
fp-> ---------------  --
	|	end_ret		|	|
	 ---------------	|
	|	sp 			|	|-- 12 bytes
	 ---------------	|
	|	fp 			|	|
	 ---------------  --
	|	parametros	| ---- n*4 bytes
	 ---------------
	|	val_ret		| ---- 4 bytes
	 ---------------
	|	var. locais	| ---- m*4 bytes
sp-> ---------------
*/

char* rotulo_main = NULL;

int instrucoes_criadas = 0;

/*Função que gera o nome de um rótulo (label) do tipo L#*/
char* gera_rotulo();

/*Função que gera o nome de um registrador do tipo r#*/
char* gera_registrador();

/*Função que concatena o rótulo dado à lista de instruções dada, seguido de um nop*/
lista_instrucoes* concatena_rotulo(char* rotulo, lista_instrucoes* lista);

char sp[5] = "rsp\0";
int desloc_valor_retorno = 0;


lista_instrucoes* cod_programa(comp_tree_t* no_ast)
{
	comp_tree_t* prox = no_ast->filho[0];

	while(prox != NULL)
	{
		prox->codigo = gera_codigo(prox);

		no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, prox->codigo);

		prox = prox->proximo;
	}

	char* cod = (char*) malloc(TAM_INSTR);

	sprintf(cod, "jumpI -> %s", rotulo_main);
	no_ast->codigo = concatena_lista_instrucoes_fim(no_ast->codigo, cria_lista_instrucoes(cod));
	sprintf(cod, "loadI %i => rbss", no_ast->codigo->numero_instrucoes_antes+4);
	no_ast->codigo = concatena_lista_instrucoes_fim(no_ast->codigo, cria_lista_instrucoes(cod));
	sprintf(cod, "loadI 1024 => %s", sp);
	no_ast->codigo = concatena_lista_instrucoes_fim(no_ast->codigo, cria_lista_instrucoes(cod));
	sprintf(cod, "loadI 1024 => rarp");
	no_ast->codigo = concatena_lista_instrucoes_fim(no_ast->codigo, cria_lista_instrucoes(cod));

	free(cod);

	return no_ast->codigo;
}

lista_instrucoes* cod_funcao(comp_tree_t* no_ast)
{
	comp_tree_t* prox = no_ast->filho[0];
	
	/*Gera o rotulo da função*/
	definir_rotulo_funcao(no_ast->conteudo, gera_rotulo());
	
	BOOL e_main = FALSE;	
	if(!strcmp(no_ast->conteudo->lexema, "main")){
		rotulo_main = no_ast->conteudo->vid._funcao->rotulo_funcao;
		e_main = TRUE;
	}

	/*Concatena a label da função para chamadas de função*/
	no_ast->codigo = concatena_rotulo(no_ast->conteudo->vid._funcao->rotulo_funcao, no_ast->codigo);
	
	char* cod = (char*) malloc(TAM_INSTR);
	
	if(e_main){
		sprintf(cod, "addI %s, %i => %s", sp, no_ast->conteudo->vid._funcao->bytes_declarados, sp);
		no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
	}
	else{
		
		sprintf(cod, "i2i %s => rarp", sp);
		no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

		int num_param = no_ast->conteudo->vid._funcao->num_param;
		int bytes_retorno = 4;
		int fim_RA = no_ast->conteudo->vid._funcao->bytes_declarados + bytes_retorno + 12 + num_param*4; // 12 = fp + sp + end_ret empilhados
		desloc_valor_retorno = 12 + num_param*4;

		sprintf(cod, "addI %s, %i => %s", sp, fim_RA, sp);
		no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
			

		/*Código para carregar parâmetros*/
		if( num_param > 0)
		{
			int end_variaveis = 16 + num_param * 4; //aponta para a área de variaveis locais
			char* reg = gera_registrador();

			int i;

			for(i = 0;i < num_param; i++)
			{
				int end_parametro = i*4 + 12; //4 bytes por parâmetro + sp, fp e end_retorno empilhados
				/*Carrega os parametros da área de argumentos para um registrador*/
				sprintf(cod, "loadAI rarp, %i => %s", end_parametro, reg);
				no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
				/*Salva o parâmetro carregado para registrador na área respectiva de memória*/
				sprintf(cod, "storeAI %s => rarp, %i", reg, i*4 + end_variaveis);
				no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
			}

			free(reg);
		}
	}

	/*Gera o código da função*/
	while(prox != NULL)
	{
		if(!(e_main && prox->tipo_ast == AST_RETURN)){
			prox->codigo = gera_codigo(prox);
			no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, prox->codigo);
		}

		prox = prox->proximo;
	}
	
	free(cod);

	return no_ast->codigo;
}

lista_instrucoes* cod_retorno(comp_tree_t* no_ast)
{
	no_ast->codigo = gera_codigo(no_ast->filho[0]);
	
	char* cod = (char*) malloc(TAM_INSTR);

	/*Armazena o valor de retorno na área da pilha apropriada*/
	sprintf(cod, "storeAI %s => rarp, %i", no_ast->filho[0]->reg_temp, desloc_valor_retorno);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
	
	char* reg_sp = gera_registrador();
	char* reg_fp = gera_registrador();
	char* reg_retorno = gera_registrador();	

	/*Carrega sp, fp e endereço de retorno*/
	sprintf(cod, "loadAI rarp, 0 => %s", reg_retorno);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
	sprintf(cod, "loadAI rarp, 4 => %s", reg_sp);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
	sprintf(cod, "loadAI rarp, 8 => %s", reg_fp);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
	sprintf(cod, "i2i %s => %s", reg_sp, sp);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
	sprintf(cod, "i2i %s => rarp", reg_fp);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
		
	/*Volta para o endereço de retorno carregado*/
	sprintf(cod, "jump -> %s", reg_retorno);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

	free(reg_sp);
	free(reg_fp);	

	free(cod);

	return no_ast->codigo;
}

lista_instrucoes* cod_chamada_funcao(comp_tree_t* no_ast)
{
	char* cod = (char*) malloc(TAM_INSTR);

	/*Salva fp e sp*/
	sprintf(cod, "storeAI %s => %s, 4", sp, sp);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
	sprintf(cod, "storeAI rarp => %s, 8", sp);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
	
	int deslocamento_argumentos = 12;
	comp_tree_t* argumentos = no_ast->filho[1];

	/*Slava argumentos na pilha*/
	while(argumentos != NULL)
	{
		argumentos->codigo = gera_codigo(argumentos);
		no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, argumentos->codigo);
		sprintf(cod, "storeAI %s => %s, %i", argumentos->reg_temp, sp, deslocamento_argumentos);
		no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

		deslocamento_argumentos+=4;
		argumentos = argumentos->proximo;
	}

	/*Executa o Jump para a função*/
	sprintf(cod, "jumpI -> %s", no_ast->filho[0]->conteudo->vid._funcao->rotulo_funcao);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

	char* reg_x = gera_registrador();

	/*Concatena, ao FINAL da lista (ou seja, seriam os códigos iniciais) o código para armazenar o endereço de retorno.*/
	/******************CÓDIGO DE ENDEREÇO DE RETORNO******************/
	sprintf(cod, "storeAI %s => %s, 0", reg_x, sp);
	no_ast->codigo = concatena_lista_instrucoes_fim(no_ast->codigo, cria_lista_instrucoes(cod));
	sprintf(cod, "loadI %i => %s", instrucoes_criadas+5, reg_x);
	no_ast->codigo = concatena_lista_instrucoes_fim(no_ast->codigo, cria_lista_instrucoes(cod));
	/*****************************************************************/

	/*Código para obter o valor retornado pela função, vem após o jump*/
	sprintf(cod, "loadAI %s, %i => %s", sp, deslocamento_argumentos, reg_x);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

	no_ast->reg_temp = strdup(reg_x);

	free(cod);
	free(reg_x);

	return no_ast->codigo;
}

lista_instrucoes* cod_if_else(comp_tree_t* no_ast)
{
	comp_tree_t *S = no_ast;
	comp_tree_t *B = no_ast->filho[0];
	comp_tree_t *S1 = no_ast->filho[1];

	S->next = gera_rotulo();
	S->t = gera_rotulo(); //rotulo para o corpo do if

	if(no_ast->filho[2] != NULL)
		S->f = gera_rotulo(); //rotulo para corpo do else, se houver	
	else
		S->f = strdup(S->next);

	B->t = strdup(S->t);
	B->f = strdup(S->f);

	S1->next = strdup(S->next);	
	

	B->codigo = gera_codigo(B);
	S->codigo = concatena_rotulo(B->t, S->codigo);
	S1->codigo = gera_codigo(S1);

	//rotulo e codigo para true
	S->codigo = concatena_lista_instrucoes(S->codigo, S1->codigo);	

	//Codigo teste
	S->codigo = concatena_lista_instrucoes_fim(S->codigo, B->codigo);

	char* cod = (char*) malloc(TAM_INSTR);	
	//sprintf(cod, "cbr %s -> %s, %s", B->reg_temp, B->t, B->f);
	//S->codigo = concatena_lista_instrucoes(S->codigo, cria_lista_instrucoes(cod));
	

	//caso else
	if(no_ast->filho[2] != NULL)
	{
		//jump para proximo comando
		sprintf(cod, "jumpI -> %s ", S1->next);
		S->codigo = concatena_lista_instrucoes(S->codigo, cria_lista_instrucoes(cod));
		// Rótulo para teste falso
		S->codigo = concatena_rotulo(B->f, S->codigo);
		comp_tree_t *S2 = no_ast->filho[2];
		S2->next = strdup(S->next);
		S2->codigo = gera_codigo(S2);
		
		//codigo para false (else)
		S->codigo = concatena_lista_instrucoes(S->codigo, S2->codigo);
		//jump para proximo comando
		sprintf(cod, "jumpI -> %s ", S2->next);
		S->codigo = concatena_lista_instrucoes(S->codigo, cria_lista_instrucoes(cod));
	}

	//rotulo para proximo comando
	S->codigo = concatena_rotulo(S->next, S->codigo);

	free(cod);
	return no_ast->codigo;
}

lista_instrucoes* cod_do_while(comp_tree_t* no_ast)
{
	comp_tree_t *S = no_ast;
	comp_tree_t *S1 = no_ast->filho[0];
	comp_tree_t *B = no_ast->filho[1];
	
	S->next = gera_rotulo();

	//rotulos do teste
	B->t = gera_rotulo();
	B->f = strdup(S->next);

	//gera codigo dos filhos
	B->codigo = gera_codigo(B);
	//Rotulo do início e comandos do laço
	S->codigo = concatena_rotulo(B->t, S->codigo);
	S1->codigo = gera_codigo(S1);
	S->codigo = concatena_lista_instrucoes(S->codigo, S1->codigo);

	//Rótulo e comando do teste
	S->codigo = concatena_lista_instrucoes(S->codigo, B->codigo);

	//Rótulo para fora do laço
	S->codigo = concatena_rotulo(S->next, S->codigo);

	return no_ast->codigo;
}

lista_instrucoes* cod_while_do(comp_tree_t* no_ast)
{
	comp_tree_t *S = no_ast;
	comp_tree_t *B = no_ast->filho[0];
	comp_tree_t *S1 = no_ast->filho[1];

	S->next = gera_rotulo();
	S1->next = gera_rotulo();

	//rotulos do teste
	B->t = gera_rotulo();
	B->f = strdup(S->next);

	//gera codigo dos filhos
	B->codigo = gera_codigo(B);
	//Rotulo e comandos do teste
	S->codigo = concatena_rotulo(S1->next, S->codigo);
	S->codigo = concatena_lista_instrucoes(S->codigo, B->codigo);
	S->codigo = concatena_rotulo(B->t, S->codigo);
	S1->codigo = gera_codigo(S1);
	//Rótulo e comando do corpo
	S->codigo = concatena_lista_instrucoes(S->codigo, S1->codigo);

	char* cod = (char*) malloc(TAM_INSTR);

	//Jump incondicional do laço
	sprintf(cod, "jumpI -> %s", S1->next);
	S->codigo = concatena_lista_instrucoes(S->codigo, cria_lista_instrucoes(cod));

	//Rótulo para fora do laço
	S->codigo = concatena_rotulo(S->next, S->codigo);

	free(cod);
	return no_ast->codigo;
}

lista_instrucoes* cod_atribuicao(comp_tree_t* no_ast)
{
	no_ast->filho[0]->codigo = gera_codigo(no_ast->filho[0]);
	no_ast->filho[1]->codigo = gera_codigo(no_ast->filho[1]);

	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, no_ast->filho[0]->codigo);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, no_ast->filho[1]->codigo);

	char* cod = (char*) malloc(TAM_INSTR);
	char fp[10] = "rarp\0";
	char rbss[10] = "rbss\0";
	char* r;

	comp_dict_item_t* id = NULL;
	
	if(no_ast->filho[0]->tipo_ast == AST_VETOR_INDEXADO)
		id = no_ast->filho[0]->filho[0]->conteudo;
	else
		id = no_ast->filho[0]->conteudo;

	if(id->vid._variavel->e_global)
		r = rbss;
	else
		r = fp;

	if(id->def_id == VARIAVEL_VETOR)
	//Se variavel for um vetor
		sprintf(cod, "storeAO %s => %s, %s", no_ast->filho[1]->reg_temp, no_ast->filho[0]->vec_reg_temp, r);
	else
		sprintf(cod, "storeAI %s => %s, %d", no_ast->filho[1]->reg_temp, r, id->vid._variavel->deslocamento);

	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));


	free(cod);

	return no_ast->codigo;
}

lista_instrucoes* cod_bloco(comp_tree_t* no_ast)
{
	comp_tree_t* prox = no_ast->filho[0];

	while(prox != NULL)
	{
		prox->codigo = gera_codigo(prox);

		no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, prox->codigo);

		prox = prox->proximo;
	}

	return no_ast->codigo;
}

lista_instrucoes* cod_identificador(comp_tree_t* no_ast)
{
	no_ast->reg_temp = gera_registrador();
	char* reg_temp = no_ast->reg_temp;

	char* cod = (char*) malloc(TAM_INSTR);

	if(no_ast->conteudo->vid._variavel->e_global)//se a variável é global
		sprintf(cod, "loadAI rbss, %d => %s", no_ast->conteudo->vid._variavel->deslocamento, reg_temp);
	else
		sprintf(cod, "loadAI rarp, %d => %s", no_ast->conteudo->vid._variavel->deslocamento, reg_temp);

	no_ast->codigo = cria_lista_instrucoes(cod);

	free(cod);

	return no_ast->codigo;
}

lista_instrucoes* cod_literal(comp_tree_t* no_ast)
{
	no_ast->reg_temp = gera_registrador();
	char* reg_temp = no_ast->reg_temp;

	char* cod = (char*) malloc(TAM_INSTR);

	sprintf(cod, "loadI %d => %s", no_ast->conteudo->valor._int, reg_temp);

	no_ast->codigo = cria_lista_instrucoes(cod);
	free(cod);

	return no_ast->codigo;
}

lista_instrucoes* cod_soma(comp_tree_t* no_ast)
{
	no_ast->reg_temp = gera_registrador();
	char* reg_temp = no_ast->reg_temp;

	char* cod = (char*) malloc(TAM_INSTR);

	lista_instrucoes* cod_filho_0 = gera_codigo(no_ast->filho[0]);	//gera codigo dos filhos
	lista_instrucoes* cod_filho_1 = gera_codigo(no_ast->filho[1]);
	//gera o proprio codigo
	sprintf(cod, "add %s, %s => %s", no_ast->filho[0]->reg_temp, no_ast->filho[1]->reg_temp, reg_temp);

	//concatena o codigo dos filhos e o proprio
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_0);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_1);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
	free(cod);
	return no_ast->codigo;
}

lista_instrucoes* cod_subtracao(comp_tree_t* no_ast)
{
	no_ast->reg_temp = gera_registrador();
	char* reg_temp = no_ast->reg_temp;

	char* cod = (char*) malloc(TAM_INSTR);

	lista_instrucoes* cod_filho_0 = gera_codigo(no_ast->filho[0]);	//gera codigo dos filhos
	lista_instrucoes* cod_filho_1 = gera_codigo(no_ast->filho[1]);
	//gera o proprio codigo
	sprintf(cod, "sub %s, %s => %s", no_ast->filho[0]->reg_temp, no_ast->filho[1]->reg_temp, reg_temp);

	//concatena o codigo dos filhos e o proprio
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_0);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_1);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
	free(cod);
	return no_ast->codigo;
}

lista_instrucoes* cod_multiplicacao(comp_tree_t* no_ast)
{
	no_ast->reg_temp = gera_registrador();
	char* reg_temp = no_ast->reg_temp;

	char* cod = (char*) malloc(TAM_INSTR);

	lista_instrucoes* cod_filho_0 = gera_codigo(no_ast->filho[0]);	//gera codigo dos filhos
	lista_instrucoes* cod_filho_1 = gera_codigo(no_ast->filho[1]);
	//gera o proprio codigo
	sprintf(cod, "mult %s, %s => %s", no_ast->filho[0]->reg_temp, no_ast->filho[1]->reg_temp, reg_temp);

	//concatena o codigo dos filhos e o proprio
	if(no_ast->filho[0]->tipo_ast == AST_CHAMADA_DE_FUNCAO){
		no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_0);
		no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_1);
	} else {
		no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_1);
		no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_0);
	}

	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
	free(cod);
	return no_ast->codigo;
}

lista_instrucoes* cod_divisao(comp_tree_t* no_ast)
{
	no_ast->reg_temp = gera_registrador();
	char* reg_temp = no_ast->reg_temp;

	char* cod = (char*) malloc(TAM_INSTR);

	lista_instrucoes* cod_filho_0 = gera_codigo(no_ast->filho[0]);	//gera codigo dos filhos
	lista_instrucoes* cod_filho_1 = gera_codigo(no_ast->filho[1]);
	//gera o proprio codigo
	sprintf(cod, "div %s, %s => %s", no_ast->filho[0]->reg_temp, no_ast->filho[1]->reg_temp, reg_temp);

	//concatena o codigo dos filhos e o proprio
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_0);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_1);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
	free(cod);
	return no_ast->codigo;
}

lista_instrucoes* cod_inversao(comp_tree_t* no_ast)
{
	no_ast->reg_temp = gera_registrador();
	char* reg_temp = no_ast->reg_temp;

	char* cod = (char*) malloc(TAM_INSTR);

	lista_instrucoes* cod_filho_0 = gera_codigo(no_ast->filho[0]);	//gera codigo do filho
	//gera o proprio codigo
	sprintf(cod, "rsubI %s, 0 => %s", no_ast->filho[0]->reg_temp, reg_temp);

	//concatena o codigo do filho e o proprio
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_0);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
	free(cod);
	return no_ast->codigo;
}

lista_instrucoes* cod_and(comp_tree_t* no_ast)
{
	comp_tree_t *B = no_ast;
	comp_tree_t *B1 = no_ast->filho[0];
	comp_tree_t *B2 = no_ast->filho[1];
	
	B1->t = gera_rotulo();
	char *rot_temp_f =  gera_rotulo();
	B1->f = B->f;

	B2->t = gera_rotulo();
	B2->f = strdup(B1->f);

	B1->codigo = gera_codigo(B1);	//gera codigo dos filhos
	B2->codigo = gera_codigo(B2);

	B->reg_temp = gera_registrador();


	char* cod = (char*) malloc(TAM_INSTR);

	//codigo B1 e teste B1
	B->codigo = concatena_lista_instrucoes(B->codigo, B1->codigo);
	sprintf(cod, "cbr %s -> %s, %s", B1->reg_temp, B1->t, rot_temp_f);
	B->codigo = concatena_lista_instrucoes(B->codigo, cria_lista_instrucoes(cod));
	//rotulo B1.t
	B->codigo = concatena_rotulo(B1->t, B->codigo);
	//codigo B2 e teste B2
	B->codigo = concatena_lista_instrucoes(B->codigo, B2->codigo);

	if(B2->tipo_ast != AST_LOGICO_E && B2->tipo_ast != AST_LOGICO_OU)
	{
		sprintf(cod, "cbr %s -> %s, %s", B2->reg_temp, B2->t, rot_temp_f);
		B->codigo = concatena_lista_instrucoes(B->codigo, cria_lista_instrucoes(cod));
	}
	
	//rotulo B2.t
	B->codigo = concatena_rotulo(B2->t, B->codigo);
	sprintf(cod, "loadI 1 => %s", B->reg_temp); //load true no B.temp
	B->codigo = concatena_lista_instrucoes(B->codigo, cria_lista_instrucoes(cod));
	sprintf(cod, "jumpI -> %s", B->t);
	B->codigo = concatena_lista_instrucoes(B->codigo, cria_lista_instrucoes(cod));
	//rotulo B2.f
	B->codigo = concatena_rotulo(rot_temp_f, B->codigo);
	sprintf(cod, "loadI 0 => %s", B->reg_temp);
	B->codigo = concatena_lista_instrucoes(B->codigo, cria_lista_instrucoes(cod));
	sprintf(cod, "jumpI -> %s", B->f);
	B->codigo = concatena_lista_instrucoes(B->codigo, cria_lista_instrucoes(cod));
	
	free(cod);
	free(rot_temp_f);
	return no_ast->codigo;
}

lista_instrucoes* cod_or(comp_tree_t* no_ast)
{
	comp_tree_t *B = no_ast;
	comp_tree_t *B1 = no_ast->filho[0];
	comp_tree_t *B2 = no_ast->filho[1];

	B1->f = gera_rotulo();
	B1->t = B->t;

	char *rot_temp_t =  gera_rotulo();

	B2->f = gera_rotulo();
	B2->t = strdup(B1->t);

	B1->codigo = gera_codigo(B1);	//gera codigo dos filhos
	B2->codigo = gera_codigo(B2);

	B->reg_temp = gera_registrador();

	char* cod = (char*) malloc(TAM_INSTR);

	//codigo B1 e teste B1
	B->codigo = concatena_lista_instrucoes(B->codigo, B1->codigo);
	sprintf(cod, "cbr %s -> %s, %s", B1->reg_temp, rot_temp_t, B1->f);
	B->codigo = concatena_lista_instrucoes(B->codigo, cria_lista_instrucoes(cod));
	//rotulo B1.f
	B->codigo = concatena_rotulo(B1->f, B->codigo);
	//codigo B2 e teste B2
	B->codigo = concatena_lista_instrucoes(B->codigo, B2->codigo);
	
	if(B2->tipo_ast != AST_LOGICO_E && B2->tipo_ast != AST_LOGICO_OU)
	{
		sprintf(cod, "cbr %s -> %s, %s", B2->reg_temp, B2->t, B2->f);
		B->codigo = concatena_lista_instrucoes(B->codigo, cria_lista_instrucoes(cod));
	}

	//rotulo B2.t
	B->codigo = concatena_rotulo(rot_temp_t, B->codigo);
	sprintf(cod, "loadI 1 => %s", B->reg_temp); //load true no B.temp
	B->codigo = concatena_lista_instrucoes(B->codigo, cria_lista_instrucoes(cod));
	sprintf(cod, "jumpI -> %s", B->t);
	B->codigo = concatena_lista_instrucoes(B->codigo, cria_lista_instrucoes(cod));
	//rotulo B2.f
	B->codigo = concatena_rotulo(B2->f, B->codigo);
	sprintf(cod, "loadI 0 => %s", B->reg_temp);
	B->codigo = concatena_lista_instrucoes(B->codigo, cria_lista_instrucoes(cod));
	sprintf(cod, "jumpI -> %s", B->f);
	B->codigo = concatena_lista_instrucoes(B->codigo, cria_lista_instrucoes(cod));

	//B->codigo = concatena_rotulo(B->next, B->codigo);

	free(cod);
	free(rot_temp_t);
	return no_ast->codigo;
}

lista_instrucoes* cod_diferente(comp_tree_t* no_ast)
{
	no_ast->reg_temp = gera_registrador();
	char* reg_temp = no_ast->reg_temp;

	char* cod = (char*) malloc(TAM_INSTR);

	lista_instrucoes* cod_filho_0 = gera_codigo(no_ast->filho[0]);	//gera codigo dos filhos
	lista_instrucoes* cod_filho_1 = gera_codigo(no_ast->filho[1]);
	//gera o proprio codigo
	sprintf(cod, "cmp_NE %s, %s => %s", no_ast->filho[0]->reg_temp, no_ast->filho[1]->reg_temp, reg_temp);

	//concatena o codigo dos filhos e o proprio
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_0);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_1);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

	sprintf(cod, "cbr %s -> %s, %s", no_ast->reg_temp, no_ast->t, no_ast->f);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

	free(cod);
	return no_ast->codigo;
}

lista_instrucoes* cod_igual(comp_tree_t* no_ast)
{
	no_ast->reg_temp = gera_registrador();
	char* reg_temp = no_ast->reg_temp;

	char* cod = (char*) malloc(TAM_INSTR);

	lista_instrucoes* cod_filho_0 = gera_codigo(no_ast->filho[0]);	//gera codigo dos filhos
	lista_instrucoes* cod_filho_1 = gera_codigo(no_ast->filho[1]);
	//gera o proprio codigo
	sprintf(cod, "cmp_EQ %s, %s => %s", no_ast->filho[0]->reg_temp, no_ast->filho[1]->reg_temp, reg_temp);

	//concatena o codigo dos filhos e o proprio
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_0);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_1);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

	sprintf(cod, "cbr %s -> %s, %s", no_ast->reg_temp, no_ast->t, no_ast->f);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

	free(cod);
	return no_ast->codigo;
}

lista_instrucoes* cod_menor_igual(comp_tree_t* no_ast)
{
	no_ast->reg_temp = gera_registrador();
	char* reg_temp = no_ast->reg_temp;

	char* cod = (char*) malloc(TAM_INSTR);

	lista_instrucoes* cod_filho_0 = gera_codigo(no_ast->filho[0]);	//gera codigo dos filhos
	lista_instrucoes* cod_filho_1 = gera_codigo(no_ast->filho[1]);
	//gera o proprio codigo
	sprintf(cod, "cmp_LE %s, %s => %s", no_ast->filho[0]->reg_temp, no_ast->filho[1]->reg_temp, reg_temp);

	//concatena o codigo dos filhos e o proprio
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_0);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_1);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

	sprintf(cod, "cbr %s -> %s, %s", no_ast->reg_temp, no_ast->t, no_ast->f);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

	free(cod);
	return no_ast->codigo;
}

lista_instrucoes* cod_maior_igual(comp_tree_t* no_ast)
{
	no_ast->reg_temp = gera_registrador();
	char* reg_temp = no_ast->reg_temp;

	char* cod = (char*) malloc(TAM_INSTR);

	lista_instrucoes* cod_filho_0 = gera_codigo(no_ast->filho[0]);	//gera codigo dos filhos
	lista_instrucoes* cod_filho_1 = gera_codigo(no_ast->filho[1]);
	//gera o proprio codigo
	sprintf(cod, "cmp_GE %s, %s => %s", no_ast->filho[0]->reg_temp, no_ast->filho[1]->reg_temp, reg_temp);

	//concatena o codigo dos filhos e o proprio
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_0);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_1);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

	sprintf(cod, "cbr %s -> %s, %s", no_ast->reg_temp, no_ast->t, no_ast->f);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

	free(cod);
	return no_ast->codigo;
}

lista_instrucoes* cod_menor(comp_tree_t* no_ast)
{
	no_ast->reg_temp = gera_registrador();
	char* reg_temp = no_ast->reg_temp;

	char* cod = (char*) malloc(TAM_INSTR);

	lista_instrucoes* cod_filho_0 = gera_codigo(no_ast->filho[0]);	//gera codigo dos filhos
	lista_instrucoes* cod_filho_1 = gera_codigo(no_ast->filho[1]);
	//gera o proprio codigo
	sprintf(cod, "cmp_LT %s, %s => %s", no_ast->filho[0]->reg_temp, no_ast->filho[1]->reg_temp, reg_temp);

	//concatena o codigo dos filhos e o proprio
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_0);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_1);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
	
	sprintf(cod, "cbr %s -> %s, %s", no_ast->reg_temp, no_ast->t, no_ast->f);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

	free(cod);
	return no_ast->codigo;
}

lista_instrucoes* cod_maior(comp_tree_t* no_ast)
{
	no_ast->reg_temp = gera_registrador();
	char* reg_temp = no_ast->reg_temp;

	char* cod = (char*) malloc(TAM_INSTR);

	lista_instrucoes* cod_filho_0 = gera_codigo(no_ast->filho[0]);	//gera codigo dos filhos
	lista_instrucoes* cod_filho_1 = gera_codigo(no_ast->filho[1]);
	//gera o proprio codigo
	sprintf(cod, "cmp_GT %s, %s => %s", no_ast->filho[0]->reg_temp, no_ast->filho[1]->reg_temp, reg_temp);

	//concatena o codigo dos filhos e o proprio
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_0);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_1);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

	sprintf(cod, "cbr %s -> %s, %s", no_ast->reg_temp, no_ast->t, no_ast->f);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

	free(cod);
	return no_ast->codigo;
}

lista_instrucoes* cod_negacao(comp_tree_t* no_ast)
{
	no_ast->reg_temp = gera_registrador();
	char* reg_zero = gera_registrador();

	char* reg_temp = no_ast->reg_temp;

	char* cod = (char*) malloc(TAM_INSTR);

	//carrega zero em um registrador
	sprintf(cod, "loadI 0 => %s", reg_zero);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

	//compara o valor da expressao com zero (false), e poe o resultado booleano no temporário da negação
	sprintf(cod, "cmp_EQ %s, %s => %s", reg_zero, no_ast->filho[0]->reg_temp, no_ast->reg_temp);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

	free(cod);
	return no_ast->codigo;
}

lista_instrucoes* cod_vetor(comp_tree_t* no_ast)
{
	no_ast->reg_temp = gera_registrador();
	char* reg_temp = no_ast->reg_temp;

	no_ast->filho[1]->codigo = gera_codigo(no_ast->filho[1]);

	
	char* dk = gera_registrador();

	comp_tree_t* aux_ast = no_ast->filho[1];
	lista_int* dimensoes = no_ast->filho[0]->conteudo->vid._variavel->dimensoes;

	lista_instrucoes* expr_cods = NULL;//Lista dos in's para acesso ao vetor
	
	lista_instrucoes* end_cods = NULL;//Lista das instruções referentes ao cálculo do endereço (iniciam com nop)
	expr_cods = concatena_lista_instrucoes(expr_cods, aux_ast->codigo);

	char* cod = (char*) malloc(TAM_INSTR);
	/*Em dk (um registrador) está o valor da primeira expressão do acesso ao vetor v[i0, i1, i2,..., in], ou seja, dk = i0*/
	//sprintf(cod, "load %s => %s", no_ast->filho[1]->reg_temp, dk);
	//end_cods = concatena_lista_instrucoes(end_cods, cria_lista_instrucoes(cod));
	sprintf(cod, "multI %s, 0 => %s", no_ast->filho[1]->reg_temp, dk);
	end_cods = concatena_lista_instrucoes(end_cods, cria_lista_instrucoes(cod));
	sprintf(cod, "add %s, %s => %s", dk, aux_ast->reg_temp, dk);
	end_cods = concatena_lista_instrucoes(end_cods, cria_lista_instrucoes(cod));
	aux_ast = aux_ast->proximo;
	
	dimensoes = dimensoes->proximo;

	while(dimensoes != NULL)
	{
		aux_ast->codigo = gera_codigo(aux_ast);
		expr_cods = concatena_lista_instrucoes(expr_cods, aux_ast->codigo);
		
		char* dkm1 = gera_registrador();
		
		//dk-1*Highk + ik
		sprintf(cod, "multI %s, %d => %s", dk, dimensoes->valor, dkm1);
		end_cods = concatena_lista_instrucoes(end_cods, cria_lista_instrucoes(cod));
		sprintf(cod, "add %s, %s => %s", dkm1, aux_ast->reg_temp, dkm1);
		end_cods = concatena_lista_instrucoes(end_cods, cria_lista_instrucoes(cod));
		free(dk);
		dk = dkm1;

		dimensoes = dimensoes->proximo;

		aux_ast = aux_ast->proximo;		
	}

	char* end = gera_registrador();

	sprintf(cod, "multI %s, %d => %s", dk, no_ast->filho[0]->conteudo->vid._variavel->tamanho, dk);
	end_cods = concatena_lista_instrucoes(end_cods, cria_lista_instrucoes(cod));
	sprintf(cod, "addI %s, %d => %s", dk, no_ast->filho[0]->conteudo->vid._variavel->deslocamento, end);
	end_cods = concatena_lista_instrucoes(end_cods, cria_lista_instrucoes(cod));
	
	if(no_ast->filho[0]->conteudo->vid._variavel->e_global)//se a variável é global
		sprintf(cod, "loadAO rbss, %s => %s", end, reg_temp);
	else
		sprintf(cod, "loadAO rarp, %s => %s", end, reg_temp);
	
	no_ast->vec_reg_temp = end;

	end_cods = concatena_lista_instrucoes(end_cods, cria_lista_instrucoes(cod));

	end_cods = concatena_lista_instrucoes(expr_cods, end_cods);

	no_ast->codigo = end_cods;
	
	free(cod);
	free(dk);

	return end_cods;
}

lista_instrucoes* cod_sr(comp_tree_t* no_ast)
{
	char* cod = (char*) malloc(TAM_INSTR);

	lista_instrucoes* cod_filho_0 = gera_codigo(no_ast->filho[0]);	//gera codigo dos filhos
	lista_instrucoes* cod_filho_1 = gera_codigo(no_ast->filho[1]);
	
	no_ast->reg_temp = strdup(no_ast->filho[0]->reg_temp);
	char* reg_temp = no_ast->reg_temp;

	//gera o proprio codigo
	sprintf(cod, "rshift %s, %s => %s", no_ast->filho[0]->reg_temp, no_ast->filho[1]->reg_temp, reg_temp);

	//concatena o codigo do filho e o proprio
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_0);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_1);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
	
	sprintf(cod, "storeAI %s => rbss, %d", reg_temp, no_ast->filho[0]->conteudo->vid._variavel->deslocamento);

	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

	free(cod);
	return no_ast->codigo;
}

lista_instrucoes* cod_sl(comp_tree_t* no_ast)
{
	char* cod = (char*) malloc(TAM_INSTR);

	lista_instrucoes* cod_filho_0 = gera_codigo(no_ast->filho[0]);	//gera codigo dos filhos
	lista_instrucoes* cod_filho_1 = gera_codigo(no_ast->filho[1]);
	
	no_ast->reg_temp = strdup(no_ast->filho[0]->reg_temp);
	char* reg_temp = no_ast->reg_temp;

	//gera o proprio codigo
	sprintf(cod, "lshift %s, %s => %s", no_ast->filho[0]->reg_temp, no_ast->filho[1]->reg_temp, reg_temp);

	//concatena o codigo do filho e o proprio
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_0);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cod_filho_1);
	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));
	
	sprintf(cod, "storeAI %s => rbss, %d", reg_temp, no_ast->filho[0]->conteudo->vid._variavel->deslocamento);

	no_ast->codigo = concatena_lista_instrucoes(no_ast->codigo, cria_lista_instrucoes(cod));

	free(cod);
	return no_ast->codigo;
}




lista_instrucoes* gera_codigo(comp_tree_t* no_ast)
{
	if(no_ast == NULL)
		return NULL;
	else
		switch(no_ast->tipo_ast) 
		{
			case AST_PROGRAMA:				return cod_programa(no_ast);
			case AST_FUNCAO:				return cod_funcao(no_ast);
			case AST_IF_ELSE:				return cod_if_else(no_ast);
			case AST_DO_WHILE:				return cod_do_while(no_ast);
			case AST_WHILE_DO:				return cod_while_do(no_ast);
			case AST_ATRIBUICAO:			return cod_atribuicao(no_ast);
			case AST_BLOCO:					return cod_bloco(no_ast);
			case AST_IDENTIFICADOR:			return cod_identificador(no_ast);
			case AST_LITERAL:				return cod_literal(no_ast);
			case AST_ARIM_SOMA:				return cod_soma(no_ast);
			case AST_ARIM_SUBTRACAO:		return cod_subtracao(no_ast);
			case AST_ARIM_MULTIPLICACAO:	return cod_multiplicacao(no_ast);
			case AST_ARIM_DIVISAO:			return cod_divisao(no_ast);
			case AST_ARIM_INVERSAO:			return cod_inversao(no_ast);
			case AST_LOGICO_E:				return cod_and(no_ast);
			case AST_LOGICO_OU:				return cod_or(no_ast);
			case AST_LOGICO_COMP_DIF:		return cod_diferente(no_ast);
			case AST_LOGICO_COMP_IGUAL:		return cod_igual(no_ast);
			case AST_LOGICO_COMP_LE:		return cod_menor_igual(no_ast);
			case AST_LOGICO_COMP_GE:		return cod_maior_igual(no_ast);
			case AST_LOGICO_COMP_L:			return cod_menor(no_ast);
			case AST_LOGICO_COMP_G:			return cod_maior(no_ast);
			case AST_LOGICO_COMP_NEGACAO:	return cod_negacao(no_ast);
			case AST_VETOR_INDEXADO:		return cod_vetor(no_ast);
			case AST_SHIFT_RIGHT:			return cod_sr(no_ast);
			case AST_SHIFT_LEFT:			return cod_sl(no_ast);
			case AST_RETURN:				return cod_retorno(no_ast);
			case AST_CHAMADA_DE_FUNCAO:		return cod_chamada_funcao(no_ast);
			default:						return NULL;
		}
}




/*Função que gera o nome de um rótulo (label) do tipo L#*/
char* gera_rotulo()
{
	static int num_rot = 0;
	char *nome_rot = (char*)malloc(50);
	sprintf(nome_rot, "L%i", num_rot);
	num_rot++;		
	return nome_rot;
}



/*Função que gera o nome de um registrador do tipo r#*/
char* gera_registrador()
{
	static int num_reg = 1;
	char *nome_reg = (char*)malloc(50);
	sprintf(nome_reg, "r%i", num_reg);
	num_reg++;		
	return nome_reg;
}


lista_instrucoes* concatena_rotulo(char* rotulo, lista_instrucoes* lista)
{
	char* cod = (char*) malloc(TAM_INSTR);
	sprintf(cod, "%s: nop", rotulo);
	lista = concatena_lista_instrucoes(lista, cria_lista_instrucoes(cod));
	free (cod);

	return lista;
}
