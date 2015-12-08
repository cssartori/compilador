#include "cc_tree.h"
#include "cc_gv.h"


/*Função que cria um nó da árvore AST. Retorna um ponteiro para esse novo nó criado.*/
comp_tree_t* cria_no(int tipo_ast, int tipo, comp_dict_item_t* item)
{
	comp_tree_t* novo_no = (comp_tree_t*)malloc(sizeof(comp_tree_t));

	if(novo_no == NULL)
		return NULL;

	novo_no->tipo_ast =  tipo_ast;
	novo_no->tipo = tipo;
	novo_no->conteudo = item;
	novo_no->coercao = 0;

	novo_no->codigo = NULL;
	novo_no->t = NULL;
	novo_no->f = NULL;
	novo_no->reg_temp = NULL;
	novo_no->vec_reg_temp = NULL;
	novo_no->next = NULL;

	/*Inicializa os filhos com NULL para ter certeza de que todos serão NULL*/
	novo_no->filho[0] = NULL;
	novo_no->filho[1] = NULL;
	novo_no->filho[2] = NULL;

	novo_no->proximo = NULL;

	return novo_no;
}

/*Função que insere um nó filho na posição (filho_pos) do nó pai.*/
comp_tree_t* insere_no(comp_tree_t* no_pai, comp_tree_t* no_filho, int filho_pos)
{
	if(no_pai == NULL)
		return NULL;

	no_pai->filho[filho_pos] = no_filho;

	return no_pai;
}

comp_tree_t* insere_proximo(comp_tree_t* no, comp_tree_t* no_proximo)
{
	if(no == NULL)
		return NULL;

	no->proximo = no_proximo;

	return no;
}

/*Função que destrói toda a árvore, liberando espaço em memória.*/
comp_tree_t* destroi_tree(comp_tree_t* raiz)
{
	if(raiz == NULL)
		return NULL;

	raiz->filho[0] = destroi_tree(raiz->filho[0]);
	raiz->filho[1] = destroi_tree(raiz->filho[1]);
	raiz->filho[2] = destroi_tree(raiz->filho[2]);
	raiz->proximo = destroi_tree(raiz->proximo);
	
	free(raiz->t);
	free(raiz->f);
	free(raiz->reg_temp);
	free(raiz->vec_reg_temp);
	free(raiz->next);
	free(raiz);
	raiz = NULL;

	return raiz;
}

/*Função que seta um tipo de coercao para um determinado no*/
comp_tree_t* define_coercao(comp_tree_t* no, int coercao)
{
	no->coercao = coercao;
	return no;
}
