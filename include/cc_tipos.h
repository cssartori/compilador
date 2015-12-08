
/*Definição dos tipos para cada nó da AST*/
#define IKS_INT		1
#define IKS_FLOAT	2
#define IKS_CHAR 	3
#define IKS_STRING 	4
#define IKS_BOOL 	5

/*Definição dos tamanhos (em bytes) de cada tipo da linguagem*/
#define IKS_TAM_INT	4
#define IKS_TAM_FLOAT	8
#define IKS_TAM_CHAR	1
#define IKS_TAM_BOOL	1
#define IKS_TAM_STRING  1

/*Definição dos tipos de coerções que precisam ser anotadas*/
#define COER_INT_BOOL	1
#define COER_INT_FLOAT	2
#define COER_BOOL_INT	3
#define COER_BOOL_FLOAT	4
#define COER_FLOAT_BOOL	5
#define COER_FLOAT_INT	6
