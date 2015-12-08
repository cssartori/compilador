/*
Grupo Tibuga:
	-Carlo Sulzbach Sartori
	-Marcelo Magno Rodrigues
*/
%{
#include <stdio.h>
#include "../include/cc_misc.h"
#include "../include/cc_dict.h"
#include "../include/cc_tree.h"
#include "../include/pilha_escopo.h"
#include "../include/cc_errors.h"
#include "../include/cc_tipos.h"
#include "../include/dict_misc.h"
#include "../include/gera_codigo.h"
#include "../include/lista_dicionarios.h"

pilha_ids_escopo* pilha_escopo = NULL;
lista_dicionarios* ld = NULL;

int tipo_funcao_atual = 0;
int contador_declaracoes = 0; //contador do número de variáveis declaradas por escopo

/*Função que cria um nó de expressão a partir de duas expressões (filhas) e um operador (que diferencia o tipo de nó gerado).*/
comp_tree_t* cria_no_expressao(comp_tree_t* exp1, comp_tree_t* exp2, int op);

/*Função que verifica se um id existe em algum escopo válido. Se existir, retorna o primeiro item (mais próximo) referente aquele id. Caso contrário, libera a memória e encerra o programa.*/
comp_dict_item_t* id_existe(comp_dict_item_t* item);

/*Função que "declara" o identificador na pilha de escopos (no escopo sendo verificado atualmente) e retorna o item criado. Caso o identificador já existe, libera a memória e encerra o programa.*/
comp_dict_item_t* declara_id(comp_dict_item_t* item);

/*Função que mapeia de um token de tipo (palavra reservada) para o respectivo tipo (valor inteiro)*/
int mapeia_token_tipos(int token_tipo);

/*Função que finaliza o parser e emite código de erro*/
void parser_finalize(int error);

/*Função que, dadas duas expressões e o tipo de nó AST ao qual elas estão ligadas, infere o tipo final do nó*/
int infere_tipos(comp_tree_t* exp1, comp_tree_t* exp2, int tipo_ast);

/*Função que compara uma lista de argumentos com os parâmetros esperados pela função*/
void compara_parametros(comp_dict_item_t* id, comp_tree_t* argumentos);

/*Função que compara a lsita de expressões passadas para acesso a vetor*/
void compara_lista_exp_vetores(comp_dict_item_t* id, comp_tree_t* lista_exp);

%}

/*Faz com que o parser imprima mensagens de erro com maior número de informações*/
%define parse.error verbose


/*Configura a variável yylval para ter os campos definidos dentro da %union*/
%union {
	struct comp_dict_item_t* valor_simbolo_lexico;
	struct comp_tree_t* _ast;
	int _i;
	struct fun_param_lista* _plst;
	struct lista_int* _ilst;
}



/*Com as definições abaixo o "else" terá precedência sobre o "then". Isso elimita a ambiguidade de shift/reduce na gramática nos casos do if-else. Assim ele sempre executa um shift e nunca um reduce quando reconhecendo um "if" e após existindo um "else", isso quer dizer que ele realiza o "casamento" do "else" sempre com o "if" mais próximo a ele.*/
%nonassoc TK_PR_THEN
%nonassoc TK_PR_ELSE

/*Associatividade dos operadores lógico e aritméticos*/
/* menor precedência*/
%left TK_OC_OR
%left TK_OC_AND 
%left '!'
%left '>' '<' TK_OC_LE TK_OC_GE TK_OC_EQ TK_OC_NE
%left '+' '-'
%left '/' '*'
/* maior precedência*/

%right '='


/* Declaração dos tokens da linguagem */
%token TK_PR_INT
%token TK_PR_FLOAT
%token TK_PR_BOOL
%token TK_PR_CHAR
%token TK_PR_STRING
%token TK_PR_IF
%token TK_PR_THEN
%token TK_PR_ELSE
%token TK_PR_WHILE
%token TK_PR_DO
%token TK_PR_INPUT
%token TK_PR_OUTPUT
%token TK_PR_RETURN
%token TK_PR_STATIC
%token TK_PR_CONST
%token TK_OC_LE
%token TK_OC_GE
%token TK_OC_EQstr
%token TK_OC_NE
%token TK_OC_AND
%token TK_OC_OR
%token TK_OC_SL
%token TK_OC_SR
%token TK_OC_IN
%token TK_LIT_INT
%token TK_LIT_FLOAT
%token TK_LIT_FALSE
%token TK_LIT_TRUE
%token TK_LIT_CHAR
%token TK_LIT_STRING
%token TK_IDENTIFICADOR
%token TOKEN_ERRO

%type <_i> tipo TK_PR_INT
%type <_ilst> lista_int
%type <_plst> lista_parametros
%type <_ast> vetor_indexado literal literal_bool literal_aritmetico expressao exp comando_chamada_funcao comando_if comando_if_else comando_simples comando_atribuicao comando_entrada comando_saida comando_retorno proximo_comando corpo_bloco comando_while comando_do_while comando_shift bloco_de_comando lista_argumentos lista_expressao declaracao_funcao base programa  declaracao_global corpo_funcao literal_int
%type <valor_simbolo_lexico> id
%%
/* Regras (e ações) da gramática */

programa:
{
	/*Insere o escopo global*/
	pilha_escopo = empilha_novo_escopo(pilha_escopo, 0);
} 
base 
{
	ld = insere_dicionario_lista(ld, pilha_escopo->tabela_escopo);
	$$ = cria_no(AST_PROGRAMA, 0,NULL);
	$$ = insere_no($$, $2, 0);
	ast_programa = $$;
	
	pilha_escopo = destroi_pilha(pilha_escopo);
	lista_instrucoes* codigo = gera_codigo(ast_programa);
	imprime_lista_instrucoes(codigo);
	codigo = destroi_lista_instrucoes(codigo);

	ld = destroi_lista_dicionarios(ld);
}
;

base:
declaracao_global base
{
	if($1 == NULL)
	{
		if($2 == NULL)
		{
			$$ = NULL;
		}
		else
		{
			$$ = $2;
		}
	}
	else
	{
		$$ = insere_proximo($1, $2);
	}
}
|%empty {$$ = NULL;}/*Define o programa vazio*/
;


/*===========================Declarações globais===========================*/
declaracao_global:
declaracao_variavel_global 
{
	$$ = NULL;
}
| declaracao_vetor_global 
{
	$$ = NULL;
}
| declaracao_funcao 
{
	$$ = $1;
}
;

/*Declarações de variáveis globais DEVEM ser terminadas por ';' e NÃO podem ser inicializadas com qualquer valor. Permitem a opção "static".*/
declaracao_variavel_global:
tipo id';'
{
	comp_dict_item_t* id = declara_id($2);
	atualiza_item(id, VARIAVEL_COMUM, mapeia_token_tipos($1), FALSE);
	atualiza_item_variavel(id, FALSE, TRUE, NULL, pilha_escopo->tabela_escopo->deslocamento);
	atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, id->vid._variavel->tamanho_total);
}
| TK_PR_STATIC tipo id';'
{
	comp_dict_item_t* id = declara_id($3);
	atualiza_item(id, VARIAVEL_COMUM, mapeia_token_tipos($2), TRUE);
	atualiza_item_variavel(id, FALSE, TRUE, NULL, pilha_escopo->tabela_escopo->deslocamento);
	atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, id->vid._variavel->tamanho_total);
}
;

/*Só existem, por definição, vetores globais, os quais seguem as mesmas regras de definição que as variáveis globais. O seu tamanho é um literal inteiro positivo entre colchetes.*/
declaracao_vetor_global:
tipo id'['lista_int']'';'
{
	comp_dict_item_t* id = declara_id($2);
	atualiza_item(id, VARIAVEL_VETOR, mapeia_token_tipos($1), FALSE);
	atualiza_item_variavel(id, FALSE, TRUE, $4, pilha_escopo->tabela_escopo->deslocamento);
	atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, id->vid._variavel->tamanho_total);
}
| TK_PR_STATIC tipo id'['lista_int']'';'
{
	comp_dict_item_t* id = declara_id($3);
	atualiza_item(id, VARIAVEL_VETOR, mapeia_token_tipos($2), TRUE);
	atualiza_item_variavel(id, FALSE, TRUE, $5, pilha_escopo->tabela_escopo->deslocamento);
	atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, id->vid._variavel->tamanho_total);
}
;

/*Uma declaração de função deve ter um tipo, um identificador e uma lista de parâmetros, possivelmente vazia, entre parenteses. Após isso uma sequência de comandos entre '{''}'. A declaração pode ser, opcionalmente, precedida pela opção "static".*/
declaracao_funcao:
tipo id '('
{
	contador_declaracoes = 0;
	tipo_funcao_atual = mapeia_token_tipos($1);
	comp_dict_item_t* id = declara_id($2);
	$2 = id;
	atualiza_item(id, FUNCAO, mapeia_token_tipos($1), FALSE);
	/*Empilha o escopo da função*/
	pilha_escopo = empilha_novo_escopo(pilha_escopo, 0);
}
lista_parametros')' 
{
	int num_parametros = conta_parametros($5);
	contador_declaracoes = num_parametros;
	atualiza_item_funcao($2, num_parametros, $5);
	if(strcmp($2->lexema, "main"))	//Se não é main
		atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, 16 + num_parametros*4);
	else
		atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, num_parametros*4);
}
corpo_funcao
{	
	ld = insere_dicionario_lista(ld, pilha_escopo->tabela_escopo);
	atualiza_numero_bytes_declarados($2, contador_declaracoes*4);
	contador_declaracoes = 0;
	/*Desempilha o escopo da função*/
	pilha_escopo = desempilha_escopo(pilha_escopo);
	$$ = cria_no(AST_FUNCAO, $2->tipo_id, $2);
	$$ = insere_no($$, $8, 0);
}
|tipo id '('')' 
{
	contador_declaracoes = 0;
	tipo_funcao_atual = mapeia_token_tipos($1);
	comp_dict_item_t* id = declara_id($2);
	$2 = id;
	atualiza_item(id, FUNCAO, mapeia_token_tipos($1), FALSE);
	atualiza_item_funcao($2, 0, NULL);
		
	/*Empilha o escopo da função*/
	pilha_escopo = empilha_novo_escopo(pilha_escopo, 0);
	if(strcmp($2->lexema, "main"))	//Se não é main
		atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, 16);
}
corpo_funcao
{
	ld = insere_dicionario_lista(ld, pilha_escopo->tabela_escopo);
	atualiza_numero_bytes_declarados($2, contador_declaracoes*4);
	contador_declaracoes = 0;

	/*Desempilha o escopo da função*/
	pilha_escopo = desempilha_escopo(pilha_escopo);
	$$ = cria_no(AST_FUNCAO, $2->tipo_id, $2);
	$$ = insere_no($$, $6, 0);
}
|TK_PR_STATIC tipo id'('
{
	contador_declaracoes = 0;
	tipo_funcao_atual = mapeia_token_tipos($2);
	comp_dict_item_t* id = declara_id($3);
	$3 = id;
	atualiza_item(id, FUNCAO, mapeia_token_tipos($2), TRUE);
	/*Empilha o escopo da função*/
	pilha_escopo = empilha_novo_escopo(pilha_escopo, 0);
}
lista_parametros')'
{
	int num_parametros = conta_parametros($6);
	contador_declaracoes = num_parametros;
	atualiza_item_funcao($3, num_parametros, $6);
	if(strcmp($3->lexema, "main"))	//Se não é main
		atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, 16 + num_parametros*4);
	else
		atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, num_parametros*4);
}
corpo_funcao
{
	ld = insere_dicionario_lista(ld, pilha_escopo->tabela_escopo);
	atualiza_numero_bytes_declarados($3, contador_declaracoes*4);
	contador_declaracoes = 0;

	/*Desempilha o escopo da função*/
	pilha_escopo = desempilha_escopo(pilha_escopo);
	$$ = cria_no(AST_FUNCAO, $3->tipo_id, $3);
	$$ = insere_no($$, $9, 0);
}
|TK_PR_STATIC tipo id'('')'
{
	contador_declaracoes = 0;
	tipo_funcao_atual = mapeia_token_tipos($2);
	comp_dict_item_t* id = declara_id($3);
	$3 = id;
	atualiza_item(id, FUNCAO, mapeia_token_tipos($2), TRUE);
	atualiza_item_funcao($3, 0, NULL);
	/*Empilha o escopo da função*/
	pilha_escopo = empilha_novo_escopo(pilha_escopo, 0);
	if(strcmp($3->lexema, "main"))	//Se não é main
		atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, 16);
}
corpo_funcao
{
	ld = insere_dicionario_lista(ld, pilha_escopo->tabela_escopo);
	atualiza_numero_bytes_declarados($3, contador_declaracoes*4);
	contador_declaracoes = 0;

	/*Desempilha o escopo da função*/
	pilha_escopo = desempilha_escopo(pilha_escopo);
	$$ = cria_no(AST_FUNCAO, $3->tipo_id, $3);
	$$ = insere_no($$, $7, 0);
}
;

corpo_funcao:
'{'corpo_bloco'}' {$$ = $2;}
|'{''}' {$$ = NULL;}
;

/*Uma lista de parametros (usada na declaração de uma função) é uma lista de identificadores e seu tipo, opcionalmente precedidos pela opção "const" e cara parametro é separado do outro por uma ','*/
lista_parametros:
tipo id
{
	comp_dict_item_t* id = declara_id($2);
	atualiza_item(id, VARIAVEL_COMUM, mapeia_token_tipos($1), FALSE);
	atualiza_item_variavel(id, FALSE, FALSE, NULL, pilha_escopo->tabela_escopo->deslocamento);
	atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, id->vid._variavel->tamanho_total);
	$$ = NULL;
	$$ = insere_parametro($$, id->vid._variavel, $1, FALSE);
}
| TK_PR_CONST tipo id
{
	comp_dict_item_t* id = declara_id($3);
	atualiza_item(id, VARIAVEL_COMUM, mapeia_token_tipos($2), FALSE);
	atualiza_item_variavel(id, TRUE, FALSE, NULL, pilha_escopo->tabela_escopo->deslocamento);
	atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, id->vid._variavel->tamanho_total);
	$$ = NULL;
	$$ = insere_parametro($$, id->vid._variavel, $2, TRUE);
}
| tipo id','lista_parametros
{
	comp_dict_item_t* id = declara_id($2);
	atualiza_item(id, VARIAVEL_COMUM, mapeia_token_tipos($1), FALSE);
	atualiza_item_variavel(id, FALSE, FALSE, NULL, pilha_escopo->tabela_escopo->deslocamento);
	atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, id->vid._variavel->tamanho_total);
	$$ = $4;
	$$ = insere_parametro($$, id->vid._variavel, $1, FALSE);
}
| TK_PR_CONST tipo id','lista_parametros
{
	comp_dict_item_t* id = declara_id($3);
	atualiza_item(id, VARIAVEL_COMUM, mapeia_token_tipos($2), FALSE);
	atualiza_item_variavel(id, TRUE, FALSE, NULL, pilha_escopo->tabela_escopo->deslocamento);
	atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, id->vid._variavel->tamanho_total);
	$$ = $5;
	$$ = insere_parametro($$, id->vid._variavel, $2, TRUE);
}
;
/*===========================================================================*/


/*==============================Comandos locais==============================*/

/*Comandos simples são comandos que dentro de um bloco de comando devem ser separados por ';'. COmandos de fluxo são comandos simples, assim como o próprio bloco de comando também o é.*/
comando_simples:
comando_declaracao_local{$$ = NULL;}
|comando_declaracao_local_vetor{$$ = NULL;}
|comando_declaracao_local_inicializada{$$ = NULL;}
|comando_atribuicao {$$ = $1;}
|comando_entrada {$$ = $1;}
|comando_saida {$$ = $1;}
|comando_retorno {$$ = $1;}
|comando_chamada_funcao {$$ = $1;}
|comando_while {$$ = $1;}
|comando_do_while {$$ = $1;}
|comando_if {$$ = $1;}
|comando_if_else {$$ = $1;}
|comando_shift {$$ = $1;}
|bloco_de_comando {$$ = $1;}
|';'/*Define o comando vazio*/ {$$ = NULL;}
;

/*Um bloco de comando é iniciado e terminado por chaves. Seu corpo é uma série de comandos simples. O bloco pode ser um bloco vazio.*/
bloco_de_comando:
'{'
{
	/*Empilha escopo do bloco*/
	pilha_escopo = empilha_novo_escopo(pilha_escopo, deslocamento_base_escopo(pilha_escopo));
}
corpo_bloco'}' 
{
	ld = insere_dicionario_lista(ld, pilha_escopo->tabela_escopo);
	/*Desempilha o escopo da função*/
	int deslocamento_bloco = deslocamento_base_escopo(pilha_escopo);
	pilha_escopo = desempilha_escopo(pilha_escopo);
	atualiza_deslocamento_escopo(pilha_escopo, deslocamento_bloco);
	$$ = cria_no(AST_BLOCO, 0, NULL);
	$$ =  insere_no($$, $3, 0);
}
|'{''}' { $$ = cria_no(AST_BLOCO, 0, NULL);}
;

/*O corpo de um bloco de comando é um comando simples seguido de um "próximo comando"*/
corpo_bloco:
comando_simples proximo_comando 
{
	if($1 == NULL)
	{ //<comando_simples> é o comando vazio ';'	
		$$ = $2;
	}
	else
	{
		$$ = insere_proximo($1, $2);
	}
}
;

/*O próximo comando inicia com o terminal ';', pois desta forma é possível fazer uma "lista" de comandos simples separados por ';' sem que o último comando tenha, obrigatoriamente, ';' após ele. Desta forma o ';' NÃO é um terminador, mas sim um separador de comandos.*/
proximo_comando:
';'comando_simples proximo_comando
{
	if($2 == NULL)
	{ //<comando_simples> é o comando vazio ';'
		$$ = $3;
	}
	else
	{
		$$ = insere_proximo($2, $3);
	}
}
|';' {$$ = NULL;}
|%empty {$$ = NULL;}
;

/*A atribuição é definida como um identificador (indexado - vetor -, ou não) seguido de '=' e de uma expressão.*/
comando_atribuicao:
id '=' expressao 
{
	comp_dict_item_t* id = 	id_existe($1);
	
	if(id->def_id == FUNCAO)
	{
		parser_finalize(IKS_ERROR_FUNCTION);
	}
	else if(id->def_id == VARIAVEL_VETOR)
	{
		parser_finalize(IKS_ERROR_VECTOR);
	}

	comp_tree_t* no_id = cria_no(AST_IDENTIFICADOR, id->tipo_id, id);

	$$ = cria_no(AST_ATRIBUICAO, infere_tipos(no_id, $3, AST_ATRIBUICAO), NULL);
	$$ = insere_no($$, no_id, 0);
	$$ = insere_no($$, $3, 1);
}
|vetor_indexado '=' expressao
{
	$$ = cria_no(AST_ATRIBUICAO, infere_tipos($1, $3, AST_ATRIBUICAO), NULL);
	$$ = insere_no($$, $1, 0);
	$$ = insere_no($$, $3, 1);
}
;

/*O comando de entrada é definido como "input" seguido de uma expressão seguido de "=>" seguido de outra expressão.*/
comando_entrada:
TK_PR_INPUT expressao TK_OC_IN expressao
{
	if($4->tipo_ast == AST_IDENTIFICADOR)
	{
		comp_dict_item_t *id = id_existe($4->conteudo);
		if(id->def_id == FUNCAO)
			parser_finalize(IKS_ERROR_FUNCTION);
	}
	else
		parser_finalize(IKS_ERROR_WRONG_PAR_INPUT);

	$$ = cria_no(AST_INPUT, 0, NULL);
	$$ = insere_no($$, $2, 0);
	$$ = insere_no($$, $4, 1);
}
;

/*Comando de saída é definido como "output" seguido por uma lista de expressões.*/
comando_saida:
TK_PR_OUTPUT lista_expressao 
{
	comp_tree_t* lista_expressao = $2;
	while(lista_expressao != NULL)
	{
		if(lista_expressao->tipo == IKS_CHAR || lista_expressao->tipo == IKS_BOOL)
			parser_finalize(IKS_ERROR_WRONG_PAR_OUTPUT);
		lista_expressao = lista_expressao->proximo;
	}

	$$ = cria_no(AST_OUTPUT, 0, NULL);
	$$ = insere_no($$, $2, 1);
}
;

/*Uma lista de expressões (usada no comando de saída) é uma série de expressões separadas por ',', semelhante a lista de parametros.*/
lista_expressao:
expressao {$$ = $1;}
|expressao ',' lista_expressao {$$ = insere_proximo($1, $3);}
;

/*Comando retorno é definido como "return" seguido de uma expressão.*/
comando_retorno:
TK_PR_RETURN expressao
{
	if(tipo_funcao_atual != $2->tipo)
	{
		if($2->tipo == IKS_CHAR || $2->tipo == IKS_STRING || tipo_funcao_atual== IKS_CHAR || tipo_funcao_atual== IKS_STRING)
			parser_finalize(IKS_ERROR_WRONG_PAR_RETURN);
		if(tipo_funcao_atual == IKS_INT)
		{
			if($2->tipo == IKS_BOOL)
				define_coercao($2, COER_BOOL_INT);
			else if($2->tipo == IKS_FLOAT)
				define_coercao($2, COER_FLOAT_INT);
		}
		else if(tipo_funcao_atual == IKS_FLOAT)
		{
			if($2->tipo == IKS_BOOL)
				define_coercao($2, COER_BOOL_FLOAT);
			else if($2->tipo == IKS_INT)
				define_coercao($2, COER_INT_FLOAT);
		}
		else if(tipo_funcao_atual == IKS_BOOL)
		{
			if($2->tipo == IKS_INT)
				define_coercao($2, COER_INT_BOOL);
			if($2->tipo == IKS_FLOAT)
				define_coercao($2, COER_FLOAT_BOOL);
		}
	}
	$$ = cria_no(AST_RETURN, 0, NULL);
	$$ = insere_no($$, $2, 0);
}
;

/*Declaração local de variável pode ser, ao contrário de global, "const". Não pode ser um vetor. Pode ser "static" também, vindo antes da palavra "const". É definida como um as opções anteriores seguidas de um tipo e de um identificador.*/
comando_declaracao_local:
tipo id
{
	contador_declaracoes++;
	comp_dict_item_t* id = declara_id($2);
	atualiza_item(id, VARIAVEL_COMUM, mapeia_token_tipos($1), FALSE);
	atualiza_item_variavel(id, FALSE, FALSE, NULL, pilha_escopo->tabela_escopo->deslocamento);
	atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, id->vid._variavel->tamanho_total);
}
| TK_PR_STATIC tipo id
{
	contador_declaracoes++;
	comp_dict_item_t* id = declara_id($3);
	atualiza_item(id, VARIAVEL_COMUM, mapeia_token_tipos($2), TRUE);
	atualiza_item_variavel(id, FALSE, FALSE, NULL, pilha_escopo->tabela_escopo->deslocamento);
	atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, id->vid._variavel->tamanho_total);	
}
| TK_PR_STATIC TK_PR_CONST tipo id
{
	contador_declaracoes++;
	comp_dict_item_t* id = declara_id($4);	
	atualiza_item(id, VARIAVEL_COMUM, mapeia_token_tipos($3), TRUE);
	atualiza_item_variavel(id, TRUE, FALSE, NULL, pilha_escopo->tabela_escopo->deslocamento);
	atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, id->vid._variavel->tamanho_total);
}
| TK_PR_CONST tipo id
{
	contador_declaracoes++;
	comp_dict_item_t* id = declara_id($3);
	atualiza_item(id, VARIAVEL_COMUM, mapeia_token_tipos($2), FALSE);
	atualiza_item_variavel(id, TRUE, FALSE, NULL, pilha_escopo->tabela_escopo->deslocamento);
	atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, id->vid._variavel->tamanho_total);	
}
;

comando_declaracao_local_vetor:
tipo id'['lista_int']'
{
	comp_dict_item_t* id = declara_id($2);
	atualiza_item(id, VARIAVEL_VETOR, mapeia_token_tipos($1), FALSE);
	atualiza_item_variavel(id, FALSE, FALSE, $4, pilha_escopo->tabela_escopo->deslocamento);
	atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, id->vid._variavel->tamanho_total);
	contador_declaracoes+=id->vid._variavel->tamanho_total;
}
| TK_PR_STATIC tipo id'['lista_int']'
{
	comp_dict_item_t* id = declara_id($3);
	atualiza_item(id, VARIAVEL_VETOR, mapeia_token_tipos($2), TRUE);
	atualiza_item_variavel(id, FALSE, FALSE, $5, pilha_escopo->tabela_escopo->deslocamento);
	atualiza_deslocamento_dicionario(pilha_escopo->tabela_escopo, id->vid._variavel->tamanho_total);
	contador_declaracoes+=id->vid._variavel->tamanho_total;
}
;

/*Variáveis locais, ao contrário de globais, podem ser inicializadas. Deve-se realizar uma declaração local comum, porém o identificador deve ser seguido de "<=" seguido por uma expressão.*/
comando_declaracao_local_inicializada:
comando_declaracao_local TK_OC_LE literal_aritmetico {}
|comando_declaracao_local TK_OC_LE literal_bool{}
|comando_declaracao_local TK_OC_LE TK_LIT_STRING{}
;

/*Uma chamada de função é definida como o identificador daquela função seguido por uma lista de argumentos da chamada entre parenteses.*/
comando_chamada_funcao:
id '('lista_argumentos')' 
{
	comp_dict_item_t* id = 	id_existe($1);
	
	if(id->def_id == VARIAVEL_COMUM)
	{
		parser_finalize(IKS_ERROR_VARIABLE);
	}
	else if(id->def_id == VARIAVEL_VETOR)
	{
		parser_finalize(IKS_ERROR_VECTOR);
	}
	
	compara_parametros(id, $3);

	comp_tree_t* no_id = cria_no(AST_IDENTIFICADOR, 0, id);
	$$ = cria_no(AST_CHAMADA_DE_FUNCAO, id->tipo_id, NULL);
	$$ = insere_no($$, no_id, 0);
	$$ = insere_no($$, $3, 1);
}
;

/*A lista de argumentos (usada na chamada de uma função) é uma lista de expressões, ou pode ser vazia.*/
lista_argumentos:
lista_expressao {$$ = $1;}
|%empty {$$ = NULL;}
;

/*Comando shift pode ser tanto para a esquerda quanto para a direita. É definido como um identificador, seguido do operador "<<" ou ">>" e de um literal inteiro.*/
comando_shift:
id TK_OC_SL literal_int 
{
	comp_dict_item_t* id = 	id_existe($1);
	if(id->def_id == FUNCAO)
	{
		parser_finalize(IKS_ERROR_FUNCTION);
	}

	$$ = cria_no(AST_SHIFT_LEFT, id->tipo_id, NULL);
	comp_tree_t* no_id = cria_no(AST_IDENTIFICADOR, 0, id);
	$$ = insere_no($$, no_id, 0);
	$$ = insere_no($$, $3, 1);
}
|id TK_OC_SR literal_int
{
	comp_dict_item_t* id = 	id_existe($1);
	if(id->def_id == FUNCAO)
	{
		parser_finalize(IKS_ERROR_FUNCTION);
	}

	$$ = cria_no(AST_SHIFT_RIGHT, id->tipo_id, NULL);
	comp_tree_t* no_id = cria_no(AST_IDENTIFICADOR, 0, id);
	$$ = insere_no($$, no_id, 0);
	$$ = insere_no($$, $3, 1);
}
;

/*O comando while é definido como "while" seguido de uma expressão entre parenteses seguido de "do" e de um comando simples (que pode ser um bloco de comandos).*/
comando_while:
TK_PR_WHILE '('expressao')' TK_PR_DO comando_simples
{
	$$ = cria_no(AST_WHILE_DO, 0, NULL);
	$$ = insere_no($$, $3, 0);
	$$ = insere_no($$, $6, 1);
}
;

/*O comando do while é formado como o while, porém iniciado com um "do" seguido de um comando simples de "while" e da expressao entre parenteses.*/
comando_do_while:
TK_PR_DO comando_simples TK_PR_WHILE '('expressao')'
{
	$$ = cria_no(AST_DO_WHILE, 0, NULL);
	$$ = insere_no($$, $2, 0);
	$$ = insere_no($$, $5, 1);
}
;

/*O comando if é formado por "if" seguido de expressão entre parenteses seguido de "then" e por um comando simples (que pode ser um bloco de comandos).*/
comando_if:
TK_PR_IF '('expressao')' TK_PR_THEN comando_simples 
{
	$$ = cria_no(AST_IF_ELSE, 0, NULL);
	$$ = insere_no($$, $3, 0);
	$$ = insere_no($$, $6, 1);
}
;

/*O comando if-else é formado como o if, inicialmente, mas seguido de um "else" e de mais um comando simples. As duas regras foram divididas para melhor organização.*/
comando_if_else:
TK_PR_IF '('expressao')' TK_PR_THEN comando_simples TK_PR_ELSE comando_simples 
{
	$$ = cria_no(AST_IF_ELSE, 0, NULL);
	$$ = insere_no($$, $3, 0);
	$$ = insere_no($$, $6, 1);
	$$ = insere_no($$, $8, 2);
}
;

/*===========================================================================*/


/*=================================Expressões================================*/

/*Expressão é formada por uma regra eCOER_BOOL_INTspecial exp juntamente com um operador e recursivamente expressao.*/
expressao:
exp
|expressao TK_OC_AND expressao {$$ = cria_no_expressao($1, $3, TK_OC_AND);}
|expressao TK_OC_OR expressao {$$ = cria_no_expressao($1, $3, TK_OC_OR);}
|expressao TK_OC_GE expressao {$$ = cria_no_expressao($1, $3, TK_OC_GE);}
|expressao TK_OC_LE expressao {$$ = cria_no_expressao($1, $3, TK_OC_LE);}
|expressao TK_OC_EQ expressao {$$ = cria_no_expressao($1, $3, TK_OC_EQ);}
|expressao TK_OC_NE expressao {$$ = cria_no_expressao($1, $3, TK_OC_NE);}
|expressao '<' expressao {$$ = cria_no_expressao($1, $3, '<');}
|expressao '>' expressao {$$ = cria_no_expressao($1, $3, '>');}
|expressao '+' expressao {$$ = cria_no_expressao($1, $3, '+');}
|expressao '-' expressao {$$ = cria_no_expressao($1, $3, '-');}
|expressao '*' expressao {$$ = cria_no_expressao($1, $3, '*');}
|expressao '/' expressao {$$ = cria_no_expressao($1, $3, '/');}
;

/*Exp é uma regra especial criada para uso em expressao. Conta com todas as regras possíveis que iniciam com literal e que são usadas em expressao. Foi notado que tal técnica evitava o shitf/reduce antes encontrado.*/
exp:
literal {$$ = $1;}
|comando_chamada_funcao {$$ = $1;}
|'('expressao')' {$$ = $2;}
|'-' exp 
{		
	$$ = cria_no(AST_ARIM_INVERSAO, infere_tipos($2, NULL, AST_ARIM_INVERSAO), NULL);
 	$$ = insere_no($$, $2, 0);
}
|'!' exp 
{
	$$ = cria_no(AST_LOGICO_COMP_NEGACAO, infere_tipos($2, NULL, AST_LOGICO_COMP_NEGACAO), NULL);
 	$$ = insere_no($$, $2, 0);
}
;

/*===========================================================================*/


/*===================================Literais================================*/

tipo: TK_PR_INT {$$ = TK_PR_INT;} | TK_PR_FLOAT {$$ = TK_PR_FLOAT;}| TK_PR_CHAR {$$ = TK_PR_CHAR;}| TK_PR_STRING {$$ = TK_PR_STRING;}| TK_PR_BOOL{$$ = TK_PR_BOOL;};

literal_aritmetico: 
id 
{
	comp_dict_item_t* id = 	id_existe($1);
	$$ = cria_no(AST_IDENTIFICADOR, id->tipo_id, id);
}
| vetor_indexado {$$ = $1;}
| literal_int {$$ = $1;}
| TK_LIT_FLOAT {$$ = cria_no(AST_LITERAL, IKS_FLOAT, yylval.valor_simbolo_lexico);}
| TK_LIT_CHAR {$$ = cria_no(AST_LITERAL, IKS_CHAR, yylval.valor_simbolo_lexico);}
;

literal_int:
TK_LIT_INT {$$ = cria_no(AST_LITERAL, IKS_INT, yylval.valor_simbolo_lexico);}
;

lista_int:
literal_int
{
	$$ = NULL;
	$$ = insere_lista_int($$, $1->conteudo->valor._int);
	lista_auxiliar_inteiros = $$;
	destroi_tree($1);
}
|literal_int','lista_int
{
	$$ = insere_lista_int($3, $1->conteudo->valor._int);
	lista_auxiliar_inteiros = $$;
	destroi_tree($1);	
}
;

literal_bool: 
TK_LIT_TRUE {$$ = cria_no(AST_LITERAL, IKS_BOOL, yylval.valor_simbolo_lexico);}
| TK_LIT_FALSE {$$ = cria_no(AST_LITERAL, IKS_BOOL, yylval.valor_simbolo_lexico);}
;

literal: 
literal_aritmetico {$$ = $1;}
| literal_bool {$$ = $1;}
| TK_LIT_STRING {$$ = cria_no(AST_LITERAL, IKS_STRING, yylval.valor_simbolo_lexico);};


/*Vetor indexado serve para definir a regra de como fazer a indexação de um vetor e acessar uma posição arbitrária dele.*/
vetor_indexado:
id'['lista_expressao']' 
{
	comp_dict_item_t* id = 	id_existe($1);
	
	if(id->def_id == VARIAVEL_COMUM)
	{
		parser_finalize(IKS_ERROR_VARIABLE);
	}
	else if(id->def_id == FUNCAO)
	{
		parser_finalize(IKS_ERROR_FUNCTION);
	}

	compara_lista_exp_vetores(id, $3);

	comp_tree_t* no_id = cria_no(AST_IDENTIFICADOR, id->tipo_id, id);
	$$ = cria_no(AST_VETOR_INDEXADO, id->tipo_id, NULL);
	$$ = insere_no($$, no_id, 0);
	$$ = insere_no($$, $3, 1);
};


id: TK_IDENTIFICADOR {$$ = yylval.valor_simbolo_lexico;};

%%

/*Função que cria um nó de expressão a partir de duas expressões (filhas) e um operador (que diferencia o tipo de nó gerado).*/
comp_tree_t* cria_no_expressao(comp_tree_t* exp1, comp_tree_t* exp2, int op)
{
	comp_tree_t* no_exp = NULL;

	switch(op){
		case '+':
			no_exp = cria_no(AST_ARIM_SOMA, infere_tipos(exp1, exp2, AST_ARIM_SOMA), NULL);
		break;
		case '-':
			no_exp = cria_no(AST_ARIM_SUBTRACAO, infere_tipos(exp1, exp2, AST_ARIM_SUBTRACAO), NULL);
		break;
		case '*':
			no_exp = cria_no(AST_ARIM_MULTIPLICACAO, infere_tipos(exp1, exp2, AST_ARIM_MULTIPLICACAO), NULL);
		break;
		case '/':
			no_exp = cria_no(AST_ARIM_DIVISAO, infere_tipos(exp1, exp2, AST_ARIM_DIVISAO), NULL);
		break;
		case TK_OC_AND: //&&
			no_exp = cria_no(AST_LOGICO_E, infere_tipos(exp1, exp2, AST_LOGICO_E), NULL);
		break;
		case TK_OC_OR://||
			no_exp = cria_no(AST_LOGICO_OU, infere_tipos(exp1, exp2, AST_LOGICO_OU), NULL);
		break;
		case TK_OC_NE://!=
			no_exp = cria_no(AST_LOGICO_COMP_DIF, infere_tipos(exp1, exp2, AST_LOGICO_COMP_DIF), NULL);
		break;
		case TK_OC_EQ://==
			no_exp = cria_no(AST_LOGICO_COMP_IGUAL, infere_tipos(exp1, exp2, AST_LOGICO_COMP_IGUAL), NULL);
		break;
		case TK_OC_LE://<=
			no_exp = cria_no(AST_LOGICO_COMP_LE, infere_tipos(exp1, exp2, AST_LOGICO_COMP_LE), NULL);
		break;
		case TK_OC_GE://>=
			no_exp = cria_no(AST_LOGICO_COMP_GE, infere_tipos(exp1, exp2, AST_LOGICO_COMP_GE), NULL);
		break;
		case '>':
			no_exp = cria_no(AST_LOGICO_COMP_G, infere_tipos(exp1, exp2, AST_LOGICO_COMP_G), NULL);
		break;
		case '<':
			no_exp = cria_no(AST_LOGICO_COMP_L, infere_tipos(exp1, exp2, AST_LOGICO_COMP_L), NULL);
		break;
	}
	
	if(no_exp == NULL)
		return NULL;

	no_exp = insere_no(no_exp, exp1, 0);
	no_exp = insere_no(no_exp, exp2, 1);

	
	return no_exp;
}

/*Função que, dadas duas expressões e o tipo de nó AST ao qual elas estão ligadas, infere o tipo final do nó*/
int infere_tipos(comp_tree_t* exp1, comp_tree_t* exp2, int tipo_ast)
{
	int tipo;
	int tipo1 = IKS_INT;
	int tipo2 = IKS_INT;
	switch(tipo_ast){
		case AST_ARIM_SOMA:
		case AST_ARIM_SUBTRACAO:
		case AST_ARIM_MULTIPLICACAO:
		case AST_ARIM_DIVISAO:
			if(exp1->tipo == IKS_BOOL)
				exp1 = define_coercao(exp1, COER_BOOL_INT);
			else if(exp1->tipo == IKS_FLOAT)
				tipo1 = IKS_FLOAT;
			else if(exp1->tipo == IKS_CHAR)
				parser_finalize(IKS_ERROR_CHAR_TO_X);
			else if(exp1->tipo == IKS_STRING)
				parser_finalize(IKS_ERROR_STRING_TO_X);
			
			if(exp2->tipo == IKS_BOOL)
				exp2 = define_coercao(exp2, COER_BOOL_INT);
			else if(exp2->tipo == IKS_FLOAT)
				tipo2 = IKS_FLOAT;
			else if(exp2->tipo == IKS_CHAR)
				parser_finalize(IKS_ERROR_CHAR_TO_X);
			else if(exp2->tipo == IKS_STRING)
				parser_finalize(IKS_ERROR_STRING_TO_X);

			if(tipo1 == tipo2)
				tipo = tipo1;
			else if(tipo1 == IKS_FLOAT || tipo2 == IKS_FLOAT)
				tipo = IKS_FLOAT;
			else
				tipo = IKS_INT;
		break;
		case AST_LOGICO_COMP_G:
		case AST_LOGICO_COMP_L:
		case AST_LOGICO_COMP_LE://<=
		case AST_LOGICO_COMP_GE://>=
		case AST_LOGICO_COMP_DIF://!=
		case AST_LOGICO_COMP_IGUAL://==
			if(exp1->tipo == IKS_BOOL)
				exp1 = define_coercao(exp1, COER_BOOL_INT);
			else if(exp1->tipo == IKS_CHAR)
				parser_finalize(IKS_ERROR_CHAR_TO_X);
			else if(exp1->tipo == IKS_STRING)
				parser_finalize(IKS_ERROR_STRING_TO_X);
			
			if(exp2->tipo == IKS_BOOL)
				exp2 = define_coercao(exp2, COER_BOOL_INT);
			else if(exp2->tipo == IKS_CHAR)
				parser_finalize(IKS_ERROR_CHAR_TO_X);
			else if(exp2->tipo == IKS_STRING)
				parser_finalize(IKS_ERROR_STRING_TO_X);

			tipo = IKS_BOOL;
		break;
		case AST_LOGICO_E: //&&
		case AST_LOGICO_OU://||
			if(exp1->tipo == IKS_INT)
				exp1 = define_coercao(exp1, COER_INT_BOOL);
			else if(exp1->tipo == IKS_FLOAT)
				exp1 = define_coercao(exp1, COER_FLOAT_BOOL);
			else if(exp1->tipo == IKS_CHAR)
				parser_finalize(IKS_ERROR_CHAR_TO_X);
			else if(exp1->tipo == IKS_STRING)
				parser_finalize(IKS_ERROR_STRING_TO_X);
			
			if(exp2->tipo == IKS_INT)
				exp2 = define_coercao(exp2, COER_INT_BOOL);
			else if(exp2->tipo == IKS_FLOAT)
				exp2 = define_coercao(exp2, COER_FLOAT_BOOL);
			else if(exp2->tipo == IKS_CHAR)
				parser_finalize(IKS_ERROR_CHAR_TO_X);
			else if(exp2->tipo == IKS_STRING)
				parser_finalize(IKS_ERROR_STRING_TO_X);

			tipo = IKS_BOOL;
		break;
		case AST_ARIM_INVERSAO:
			if(exp1->tipo == IKS_BOOL)
			{
				exp1 = define_coercao(exp1, COER_BOOL_INT);
				tipo = IKS_INT;
			}
			else if(exp1->tipo == IKS_CHAR)
				parser_finalize(IKS_ERROR_CHAR_TO_X);
			else if(exp1->tipo == IKS_STRING)
				parser_finalize(IKS_ERROR_STRING_TO_X);
			else
				tipo = exp1->tipo;
		break;
		case AST_LOGICO_COMP_NEGACAO:
			if(exp1->tipo == IKS_INT)
				exp1 = define_coercao(exp1, COER_INT_BOOL);
			else if(exp1->tipo == IKS_FLOAT)
				exp1 = define_coercao(exp1, COER_FLOAT_BOOL);
			else if(exp1->tipo == IKS_CHAR)
				parser_finalize(IKS_ERROR_CHAR_TO_X);
			else if(exp1->tipo == IKS_STRING)
				parser_finalize(IKS_ERROR_STRING_TO_X);
			
			tipo = IKS_BOOL;
		break;
		case AST_ATRIBUICAO:
			if(exp1->tipo == exp2->tipo)
				tipo = exp1->tipo;
			else if(exp2->tipo == IKS_STRING)
				parser_finalize(IKS_ERROR_STRING_TO_X);
			else if(exp2->tipo == IKS_CHAR)
				parser_finalize(IKS_ERROR_CHAR_TO_X);
			else if(exp1->tipo == IKS_CHAR || exp1->tipo == IKS_STRING)
				parser_finalize(IKS_ERROR_WRONG_TYPE);
			else if(exp1->tipo == IKS_FLOAT)
			{
				if(exp2->tipo == IKS_INT)
					exp2 = define_coercao(exp2, COER_INT_FLOAT);
				else if(exp2->tipo == IKS_BOOL)
					exp2 = define_coercao(exp2, COER_BOOL_FLOAT);
				tipo = IKS_FLOAT;
			}
			else if(exp1->tipo == IKS_INT)
			{
				if(exp2->tipo == IKS_FLOAT)
					exp2 = define_coercao(exp2, COER_FLOAT_INT);
				else if(exp2->tipo == IKS_BOOL)
					exp2 = define_coercao(exp2, COER_BOOL_INT);
				tipo = IKS_INT;
			}
			else if(exp1->tipo == IKS_BOOL)
			{
				if(exp2->tipo == IKS_INT)
					exp2 = define_coercao(exp2, COER_INT_BOOL);
				else if(exp2->tipo == IKS_FLOAT)
					exp2 = define_coercao(exp2, COER_FLOAT_BOOL);
				tipo = IKS_BOOL;
			}
		break;
	}

	return tipo;
}


/*Função que verifica se um id existe em algum escopo válido. Se existir, retorna o primeiro item (mais próximo) referente aquele id. Caso contrário, libera a memória e encerra o programa.*/
comp_dict_item_t* id_existe(comp_dict_item_t* item)
{
	comp_dict_item_t* id = busca_item_pilha(pilha_escopo, item);

	if(id == NULL)
	{/*Se o identificador não for encontrado em nenhum escopo, ele não foi declarado*/
		pilha_escopo = destroi_pilha(pilha_escopo);
		main_finalize();
		ld = destroi_lista_dicionarios(ld);
		exit(IKS_ERROR_UNDECLARED);
	}
	return id;		
}

/*Função que "declara" o identificador na pilha de escopos (no escopo sendo verificado atualmente) e retorna o item criado. Caso o identificador já existe, libera a memória e encerra o programa.*/
comp_dict_item_t* declara_id(comp_dict_item_t* item)
{
	if(busca_item_escopo(pilha_escopo, item) != NULL)
	{/*Se o nome de variável já existe, deve-se emitir um erro.*/
		pilha_escopo = destroi_pilha(pilha_escopo);
		main_finalize();
		ld = destroi_lista_dicionarios(ld);
		exit(IKS_ERROR_DECLARED);
	}
	/*Caso contrário, devemos inserir o identificador (item) na pilha de escopos*/
	return insere_item_escopo(pilha_escopo, item);	
}

/*Função que mapeia de um token de tipo (palavra reservada) para o respectivo tipo (valor inteiro)*/
int mapeia_token_tipos(int token_tipo)
{
	switch(token_tipo)
	{
	case TK_PR_INT:
		return IKS_INT;
	case TK_PR_FLOAT:
		return IKS_FLOAT;
	case TK_PR_BOOL:
		return IKS_BOOL;
	case TK_PR_CHAR:
		return IKS_CHAR;
	case TK_PR_STRING:
		return IKS_STRING;
	default:
		return -1;
	}
}

/*Função que compara uma lista de argumentos com os parâmetros esperados pela função*/
void compara_parametros(comp_dict_item_t* id, comp_tree_t* argumentos)
{
	fun_param_lista* parametros = id->vid._funcao->parametros;
	comp_tree_t* arg = argumentos;

	while(arg != NULL && parametros != NULL)
	{
		if(parametros->tipo_param == IKS_STRING && arg->tipo != IKS_STRING)
			parser_finalize(IKS_ERROR_WRONG_TYPE_ARGS);
		else if(parametros->tipo_param == IKS_CHAR && arg->tipo != IKS_CHAR)
			parser_finalize(IKS_ERROR_WRONG_TYPE_ARGS);
		else if(parametros->tipo_param != IKS_STRING && arg->tipo == IKS_STRING)
			parser_finalize(IKS_ERROR_WRONG_TYPE_ARGS);
		else if(parametros->tipo_param != IKS_CHAR && arg->tipo == IKS_CHAR)
			parser_finalize(IKS_ERROR_WRONG_TYPE_ARGS);
		else {
			int tipo1 = parametros->tipo_param;
			int tipo2 = arg->tipo;

			if(tipo1 == IKS_INT && tipo2 == IKS_FLOAT)
				arg = define_coercao(arg, COER_FLOAT_INT);
			else if(tipo1 == IKS_FLOAT && tipo2 == IKS_INT)
				arg = define_coercao(arg, COER_INT_FLOAT);
			else if(tipo1 == IKS_INT && tipo2 == IKS_BOOL)
				arg = define_coercao(arg, COER_BOOL_INT);
			else if(tipo1 == IKS_FLOAT && tipo2 == IKS_BOOL)
				arg = define_coercao(arg, COER_BOOL_FLOAT);
			else if(tipo1 == IKS_BOOL && tipo2 == IKS_FLOAT)
				arg = define_coercao(arg, COER_FLOAT_BOOL);
			else if(tipo1 == IKS_BOOL && tipo2 == IKS_INT)
				arg = define_coercao(arg, COER_INT_BOOL);
		}

		arg = arg->proximo;
		parametros = parametros->proximo;
	}

	if(arg == NULL && parametros != NULL)
		parser_finalize(IKS_ERROR_MISSING_ARGS);
	else if(arg != NULL && parametros == NULL)
		parser_finalize(IKS_ERROR_EXCESS_ARGS);
	
}

/*Função que compara a lsita de expressões passadas para acesso a vetor*/
void compara_lista_exp_vetores(comp_dict_item_t* id, comp_tree_t* lista_exp)
{
	lista_int* dimensoes = id->vid._variavel->dimensoes;
	comp_tree_t* lxp = lista_exp;

	while(lxp != NULL && dimensoes != NULL)
	{
		if(lxp->tipo == IKS_CHAR)
			parser_finalize(IKS_ERROR_CHAR_TO_X);
		else if(lxp->tipo == IKS_STRING)
			parser_finalize(IKS_ERROR_STRING_TO_X);
		else if(lxp->tipo == IKS_FLOAT)
			define_coercao(lxp, COER_FLOAT_INT);
		else if(lxp->tipo == IKS_BOOL)
			define_coercao(lxp, COER_BOOL_INT);	

		lxp = lxp->proximo;
		dimensoes = dimensoes->proximo;
	}

	if(lxp == NULL && dimensoes != NULL)
		parser_finalize(IKS_ERROR_MISSING_ARGS);
	else if(lxp != NULL && dimensoes == NULL)
		parser_finalize(IKS_ERROR_EXCESS_ARGS);
}

/*Função que finaliza o parser e emite código de erro*/
void parser_finalize(int error)
{
	pilha_escopo = destroi_pilha(pilha_escopo);
	main_finalize();
	ld = destroi_lista_dicionarios(ld);
	exit(error);
}




