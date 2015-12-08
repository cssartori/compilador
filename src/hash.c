 /* 
 Tibuga
 Carlo Sartori
 Marcelo Magno Rodrigues 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/cc_dict.h"
#include "../include/main.h"
#include "../include/cc_tipos.h"
/*#define SIMBOLO_LITERAL_INT    1
#define SIMBOLO_LITERAL_FLOAT  2
#define SIMBOLO_LITERAL_CHAR   3
#define SIMBOLO_LITERAL_STRING 4
#define SIMBOLO_LITERAL_BOOL   5
#define SIMBOLO_IDENTIFICADOR  6*/

/* Estrutura da tabela HASH:   

        	        __________________________________________________________________________
	comp_dict_t -> |__entrada_hash__|__entrada_hash__|__entrada_hash__|__entrada_hash__|__...

 	Acesso à chave:		comp_dict_t.tabela[]->chave
	Acesso aos dados: 	comp_dict_t.tabela[]->conteudo."campo buscado"

	ex.: hash.tabela[i]->conteudo.lexema

*/

/*Função que calcula um valor inteiro de hash baseada no lexema (a chave).*/
unsigned int funcao_hash(char* lexema, int tipo_token);

/*Função que busca a posição de determinado lexema na tabela. Retorna o índice da entrada se esta existir, ou o índice da primeira posição livre
caso não exista  e -1 para tabela cheia.*/
int busca_posicao(comp_dict_t tabela_hash, char* lexema, int tipo_token);

/*Função que extrai o valor de um dado lexema dado o seu tipo*/
valor_token extrai_valor(char* lexema, int tipo_token);

/*Função que cria uma tabela hash de TAM_HASH posições.*/
comp_dict_t* cria_hash(int deslocamento_base)
{
	comp_dict_t* tabela = NULL;

	/*Aloca espaço para a estrutura de tabela hash: comp_dict_t*/
	tabela = (comp_dict_t*) malloc(sizeof(comp_dict_t));

	int i = 0;

	/*Se a alocação ter tido sucesso (isto é, não resultou em NULL), 
	inicializamos o vetor dicionario com ponteiros para NULL, pois estão todas as posições desocupadas*/
	if(tabela != NULL)	
		for (i = 0; i < TAM_HASH; ++i)
			tabela->dicionario[i] = NULL;
	
	tabela->deslocamento = deslocamento_base;

	return tabela;
}

/*Função que calcula um valor inteiro de hash baseada no lexema (a chave) e o seu tipo.*/
unsigned int funcao_hash(char* lexema, int tipo_token)
{
	unsigned int produto = 1;

	/*Percorre todo o lexema*/
	while(*lexema != '\0')
	{
		/*A cada iteração multiplicamos o valor de cada caractere do lexema pelo produtor anterior*/
		produto *= *lexema;
		/*Incrementa a posição do ponteiro em um para apontar para o próximo caractere na string*/
		lexema++;
	}
	
	/*Por fim, multiplica o produto pelo valor do tipo do lexema*/
	produto *= tipo_token;

	/*hash é um número entre 0 e TAM_HASH-1 : um indice do vetor dicionario*/
	unsigned int hash = produto % TAM_HASH;

	return hash;
}

/*Função que busca a posição de determinado lexema na tabela. Retorna o índice da entrada se esta existir, ou o índice da primeira posição livre
caso não exista  e -1 para tabela cheia.*/
int busca_posicao(comp_dict_t tabela_hash, char* lexema, int tipo_token)
{
	/*Calcula o valor hash para o lexema dado*/
	unsigned int posicao = funcao_hash(lexema, tipo_token);
	unsigned int posicao_atual = posicao;
	int retorno;
	

	/*Se a posição no dicionário apontada pelo valor hash (posicao) estiver vazia, então devemos retornar ela*/
	if(tabela_hash.dicionario[posicao] == NULL)
		retorno = posicao;
	else
		/*Caso a posição esteja ocupada E o lexema que a ocupa é exatamente o mesmo que queremos inserir, devemos retornar esta posição também*/
		if((strcmp(lexema, tabela_hash.dicionario[posicao]->chave) == 0) && (tabela_hash.dicionario[posicao]->chave_tipo == tipo_token))
			retorno = posicao;
		else
		{
			/*Caso contrário, devemos percorrer toda a tabela hash em busca de um lexema igual ao que queremos inserir, 
			ou de um espaço vazio, onde então iremos inserir o lexema*/

			/*incrementamos a variável de acordo com um valor PASSO_HASH para evitar o número de colisões na tabela*/
			posicao_atual += PASSO_HASH;
			posicao_atual = posicao_atual%TAM_HASH;
			/*Enquanto não voltar para o ponto de partida*/
			while(posicao_atual != posicao)
			{
				/*Se a posição estiver livre OU a posição estiver ocupada com um lexema exatamente igual ao que queremos inserir, retornamos a posição atual*/
				if((tabela_hash.dicionario[posicao_atual] == NULL) || ((strcmp(lexema, tabela_hash.dicionario[posicao_atual]->chave) == 0) && tabela_hash.dicionario[posicao_atual]->chave_tipo == tipo_token))
 				{
					retorno = posicao_atual;
					break;
				}
				
				posicao_atual += PASSO_HASH;
				posicao_atual = posicao_atual%TAM_HASH;
			}
			/*Se deu a volta completa e não encontrou qualquer espaço vazio ou lexema igual a este, significa que a tabela está cheia*/
			if(posicao_atual == posicao)
				retorno = TABELA_CHEIA;
		}
	return retorno;
}

/*Função que insere um lexema na tabela hash. Utiliza busca_posição para calcular a posição de inserção.
Se a entrada já existir, atualiza a linha da mesma com o novo valor.*/
comp_dict_item_t* insere_entrada(comp_dict_t *tabela_hash, char* lexema, int tipo_token, int linha_lexema)
{
	int retorno = RETORNO_OK;
	int posicao;

		posicao = busca_posicao(*tabela_hash, lexema, tipo_token);
		//printf("Posicao de %s  :  %i\n", lexema, posicao);
		if(posicao != TABELA_CHEIA)
		{
			/*Se a posição estiver livre, alocamos espaço para o item*/
			if(tabela_hash->dicionario[posicao] == NULL)
			{
				tabela_hash->dicionario[posicao] = (entrada_hash_t*) malloc(sizeof(entrada_hash_t));
				/*Se não houve erro na alocação, podemos atualizar os campos do item*/
				if(tabela_hash->dicionario[posicao] != NULL)
				{
					tabela_hash->dicionario[posicao]->chave = strdup(lexema);
					tabela_hash->dicionario[posicao]->chave_tipo = tipo_token;

					tabela_hash->dicionario[posicao]->conteudo.linha_lexema = linha_lexema;
					tabela_hash->dicionario[posicao]->conteudo.tipo_token = tipo_token;
					tabela_hash->dicionario[posicao]->conteudo.valor = extrai_valor(lexema, tipo_token);
					tabela_hash->dicionario[posicao]->conteudo.lexema = tabela_hash->dicionario[posicao]->chave;
					tabela_hash->dicionario[posicao]->conteudo.def_id = 0;
					tabela_hash->dicionario[posicao]->conteudo.tipo_id = 0;
					tabela_hash->dicionario[posicao]->conteudo.vid._funcao = NULL;
					tabela_hash->dicionario[posicao]->conteudo.vid._variavel = NULL;
				}
				else
					retorno = RETORNO_ERRO;

			}
			/*Caso o lexema já esteja na tabela, só precisamos atualizar sua linha com a última linha onde foi encontrado e o tipo da última ocorrência*/
			else{
				tabela_hash->dicionario[posicao]->conteudo.linha_lexema = linha_lexema;
				tabela_hash->dicionario[posicao]->conteudo.tipo_token = tipo_token;
			}
		}
		else
			retorno = RETORNO_ERRO;
	
	
	if(retorno == RETORNO_OK)
		return &(tabela_hash->dicionario[posicao]->conteudo);
	else
		return NULL;

}

comp_dict_item_t* busca_item_hash(comp_dict_t* tabela_hash, char* lexema, int tipo_token)
{
	int posicao = busca_posicao(*tabela_hash, lexema, tipo_token);

	if(tabela_hash->dicionario[posicao] != NULL)
		return &(tabela_hash->dicionario[posicao]->conteudo);

	return NULL;
}

/*Função que destroi a tabela hash, liberando memória.*/
comp_dict_t* destroi_hash(comp_dict_t *tabela_hash)
{
	
	if(tabela_hash == NULL)
		return tabela_hash;

	int i=0;
	/*Para cada item na tabela*/
	for(i=0;i<TAM_HASH;i++)
	{
		/*Se ele ter sido alocado*/
		if(tabela_hash->dicionario[i] != NULL)
		{
			/*Liberamos o espaço ocupado pela chave - que é resultado de um strdup*/
			free(tabela_hash->dicionario[i]->chave);
			
			if(tabela_hash->dicionario[i]->conteudo.def_id == FUNCAO)
			{
					if(tabela_hash->dicionario[i]->conteudo.vid._funcao != NULL)
						tabela_hash->dicionario[i]->conteudo.vid._funcao->parametros =  destroi_lista_parametros(tabela_hash->dicionario[i]->conteudo.vid._funcao->parametros);
					free(tabela_hash->dicionario[i]->conteudo.vid._funcao->rotulo_funcao);
					free(tabela_hash->dicionario[i]->conteudo.vid._funcao);
			}
			else if(tabela_hash->dicionario[i]->conteudo.def_id == VARIAVEL_COMUM || tabela_hash->dicionario[i]->conteudo.def_id == VARIAVEL_VETOR)
			{
					destroi_lista_int(tabela_hash->dicionario[i]->conteudo.vid._variavel->dimensoes);
					free(tabela_hash->dicionario[i]->conteudo.vid._variavel);				
			}

			/*Se o valor do token for um LITERAL_STRING ou um IDENTIFICADOR devemos liberar a memória ocupada pelo valor, pois ele é resultado de um strdup nestes casos*/
			if(tabela_hash->dicionario[i]->conteudo.tipo_token == SIMBOLO_LITERAL_STRING 
				|| tabela_hash->dicionario[i]->conteudo.tipo_token == SIMBOLO_IDENTIFICADOR)
			{
				free(tabela_hash->dicionario[i]->conteudo.valor._string);
				tabela_hash->dicionario[i]->conteudo.valor._string = NULL;
				
				
			} 
			/*Por fim liberamos todo o item e o igualamos a NULL*/
			free(tabela_hash->dicionario[i]);
			tabela_hash->dicionario[i] = NULL;
		}
	}

	/*Ao final, liberamos toda a tabela*/
	free(tabela_hash);
	tabela_hash = NULL;
	
	/*Retornamos a tabela já NULA*/
	return tabela_hash;
}


/*Função que lista as entradas da tabela em ordem de posição.*/
void lista_hash(comp_dict_t tabela_hash) 
{
	int i=0;
	for(i=0;i<TAM_HASH;i++)
	{
		if(tabela_hash.dicionario[i] != NULL) {
			char *tipo[] = {"int", "float", "char", "string", "bool", "id"};
			printf("%i - \"%s\" ( %s ) :: ", i, tabela_hash.dicionario[i]->chave, tipo[tabela_hash.dicionario[i]->chave_tipo-1]);
			switch(tabela_hash.dicionario[i]->chave_tipo)
			{
			case SIMBOLO_LITERAL_INT:
				printf("%i @ ", tabela_hash.dicionario[i]->conteudo.valor._int);
				break;
			case SIMBOLO_LITERAL_FLOAT:
				printf("%f @ ", tabela_hash.dicionario[i]->conteudo.valor._float);
				break;
			case SIMBOLO_LITERAL_CHAR:
				printf("%c @ ", tabela_hash.dicionario[i]->conteudo.valor._char);
				break;
			case SIMBOLO_LITERAL_STRING:
				printf("%s @ ", tabela_hash.dicionario[i]->conteudo.valor._string);
				break;
			case SIMBOLO_LITERAL_BOOL:
				printf("%i @ ", tabela_hash.dicionario[i]->conteudo.valor._bool);
				break;
			case SIMBOLO_IDENTIFICADOR:
				printf("%s @ ", tabela_hash.dicionario[i]->conteudo.valor._id);
				break;
			}

 			printf("%i   -  def:  %i\n",  tabela_hash.dicionario[i]->conteudo.linha_lexema, tabela_hash.dicionario[i]->conteudo.def_id);
		}
	}

}

/*Função que extrai o valor de um dado lexema dado o seu tipo*/
valor_token extrai_valor(char* lexema, int tipo_token)
{
	valor_token valor;
	
	/*Dependendo do tipo do token deve extrair o valor de uma forma diferente*/
	switch(tipo_token){
	case SIMBOLO_LITERAL_INT:
		valor._int = atoi(lexema);
		break;
	case SIMBOLO_LITERAL_FLOAT:
		valor._float = atof(lexema);
		break;
	case SIMBOLO_LITERAL_CHAR:
		valor._char = lexema[0];
		break;
	case SIMBOLO_LITERAL_STRING:
	case SIMBOLO_IDENTIFICADOR:
		valor._string = strdup(lexema);
		break;
	case SIMBOLO_LITERAL_BOOL:
		if(strcmp(lexema, "false") == 0)
			valor._bool = FALSE;
		else if(strcmp(lexema, "true") == 0)
			valor._bool = TRUE;
		break;

	}

	return valor;
}

void atualiza_item(comp_dict_item_t* item, int def_id, int tipo_id, bool e_static)
{
	item->def_id = def_id;
	item->tipo_id = tipo_id;
	item->e_static = e_static;
}

void atualiza_item_funcao(comp_dict_item_t* item, int num_param, fun_param_lista* parametros)
{
	item->vid._funcao = (function*)malloc(sizeof(function));
	item->vid._funcao->num_param = num_param;
	item->vid._funcao->parametros = parametros;
	item->vid._funcao->bytes_declarados = 0;

	fun_param_lista* aux = parametros;
	int bytes_correcao = 16 + num_param*4; /*(End. retorno + sp + fp + retorno = 16) + numero de parametros*4*/
	while(aux != NULL)
	{
		aux->var->deslocamento+=bytes_correcao;
		aux = aux->proximo;
	}
}


void atualiza_item_variavel(comp_dict_item_t* item, bool e_const, bool e_global, lista_int* dimensoes, int deslocamento_base)
{
	item->vid._variavel = (variable*)malloc(sizeof(variable));
	item->vid._variavel->e_const = e_const;
	item->vid._variavel->dimensoes = dimensoes;	
	item->vid._variavel->e_global = e_global;
	item->vid._variavel->deslocamento = deslocamento_base;

	switch(item->tipo_id)
	{
		case IKS_INT:
			item->vid._variavel->tamanho = IKS_TAM_INT;
			break;
		case IKS_FLOAT:
			item->vid._variavel->tamanho = IKS_TAM_FLOAT;
			break;
		case IKS_CHAR:
			item->vid._variavel->tamanho = IKS_TAM_CHAR;
			break;
		case IKS_BOOL:
			item->vid._variavel->tamanho = IKS_TAM_BOOL;
			break;
		case IKS_STRING:
			item->vid._variavel->tamanho = IKS_TAM_STRING;
			break;
	}

	if(item->def_id == VARIAVEL_VETOR)
	{
		int mult = 1;
		while(dimensoes != NULL)
		{
			mult = (dimensoes->valor) * (mult);
			dimensoes = dimensoes->proximo;
		}

		item->vid._variavel->tamanho_total = mult*item->vid._variavel->tamanho;
	}
	else
	{
		item->vid._variavel->tamanho_total = item->vid._variavel->tamanho;
	}

}

void atualiza_deslocamento_dicionario(comp_dict_t* dicionario, int adicao)
{
	dicionario->deslocamento+=adicao;
}

void atualiza_deslocamento_base(comp_dict_t* dict, int novo_deslocamento_base)
{
	dict->deslocamento = novo_deslocamento_base;
}

void atualiza_numero_bytes_declarados(comp_dict_item_t* funcao, int bytes_declarados)
{
	funcao->vid._funcao->bytes_declarados = bytes_declarados;
}

int numero_declaracoes(comp_dict_item_t* funcao)
{
	return funcao->vid._funcao->bytes_declarados;
}

void definir_rotulo_funcao(comp_dict_item_t* item, char* rotulo)
{
	if(item->def_id == FUNCAO)
		item->vid._funcao->rotulo_funcao = rotulo;
}

/*void definir_reg_retorno_funcao(comp_dict_item_t* funcao, char* reg_retorno)
{
	if(funcao->def_id == FUNCAO)
		funcao->vid._funcao->reg_retorno = reg_retorno;
}*/

/*
int main() {

	comp_dict_t* d = cria_hash();

	char* lex1 = "meuID";
	char* lex2 = "58795";
	char* lex3 = "___STRING_MUITO_LONGA______________________________________________________________________________________AAA";
	char* lex4 = "6.978";
	char* lex5 = "aiaiOUtroID";
	
	printf("Inicio de insercao\n");
	if(insere_entrada(d, lex1, 6, 25) != RETORNO_OK)
	{
		printf("ERRO inserindo %s\n", lex1);
		return 0;
	}
	printf("Inseriu a primeira\n");
	if(insere_entrada(d, lex2, 1, 85) != RETORNO_OK)
	{
		printf("ERRO inserindo %s\n", lex2);
		return 0;
	}
	printf("Inseriu a segunda\n");
	if(insere_entrada(d, lex3, 6, 102) != RETORNO_OK)
	{
		printf("ERRO inserindo %s\n", lex3);
		return 0;
	}
	printf("Inseriu a terceira\n");
	if(insere_entrada(d, lex4, 2, 28) != RETORNO_OK)
	{
		printf("ERRO inserindo %s\n", lex4);
		return 0;
	}
	printf("Inseriu a quarta\n");
	if(insere_entrada(d, lex5, 6, 785) != RETORNO_OK)
	{
		printf("ERRO inserindo %s\n", lex5);
		return 0;
	}
	printf("Inseriu a quinta\n");
	
	insere_entrada(d, "ab", 6, 8);
	insere_entrada(d, "ba", 4, 18);

	//while(insere_entrada(d, "a", 9) != RETORNO_ERRO);

	printf("Inseriu todas!\n");

	lista_hash(*d);

}
*/
