/*

//------------------------------------------------------------------------------
// estrutura de uma fila genérica, sem conteúdo definido.
// Veja um exemplo de uso desta estrutura em testafila.c

typedef struct queue_t
{
   struct queue_t *prev ;  // aponta para o elemento anterior na fila
   struct queue_t *next ;  // aponta para o elemento seguinte na fila
} queue_t ;
*/

#include <stdio.h>
#include "queue.h"

//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila

void queue_append (queue_t **queue, queue_t *elem){

	queue_t *next,*prev;

	if ((elem->next == NULL) && (elem->prev == NULL)){
			
		if (queue != NULL){

			if (*queue != NULL)
			{
				
			

				next = *queue;
				prev = next->prev;

				elem->next = next;
				elem->prev = prev;
				next->prev = elem;
				prev->next = elem;

			}else {
			
			*queue = elem;

			elem->next = elem;
			elem->prev = elem;

			}	
		} else {
			fprintf(stderr, "FILA NAO EXISTE\n");
		}
	} else {
		fprintf(stderr, "Elemento ja pertence a uma fila\n");
	}
}

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: apontador para o elemento removido, ou NULL se erro

queue_t *queue_remove (queue_t **queue, queue_t *elem){

	queue_t *aux,*next,*prev;

	if (elem == NULL){
		fprintf(stderr, "ELEMENTO NAO\n");
	} else {

		if (queue != NULL){

			if (*queue != NULL){
				
				if (*queue == elem){

					if ((*queue)->next == *queue){

						elem->next = NULL;
						elem->prev = NULL;
						
						*queue = NULL;
						return (elem);

					} else {
						next = (*queue)->next;
						prev = (*queue)->prev;

						elem->next = NULL;
						elem->prev = NULL;

						next->prev = prev;
						prev->next = next;

						*queue = next;

						return (elem);
					}
					
				}else{

					aux = (*queue)->next;

					while((elem != aux) && (aux != *queue)){
						aux = aux->next;
					}

					if (aux == elem){

						next = aux->next;
						prev = aux->prev;

						elem->next = NULL;
						elem->prev = NULL;

						next->prev = prev;
						prev->next = next;
						
						return (elem);
					
					} else{

						fprintf(stderr, "ELEMENTO NAO PERTENCENTE A FILA\n");
					}					
				}
			}	
		} else {
			fprintf(stderr, "FILA NAO EXISTE\n");
		}
	}
	return (elem);
}

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue){

	int tam = 0;
	queue_t *aux;

	if ( queue != NULL){
		tam++;
		aux = queue->next;
		while(queue != aux){
			aux = aux->next;
			tam++;
		}
	}

	return(tam);


}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca.
//
// Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print (char *name, queue_t *queue, void print_elem (void*) ){

	queue_t *aux;

	printf("%s\n",name);

	if ( queue != NULL){
		aux = queue;
		print_elem(aux);
		aux = aux->next;
		while(queue != aux){
			print_elem(aux);
			aux = aux->next;
			
		}
	}

	

}