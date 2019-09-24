// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"		// biblioteca de filas genéricas
#include "hard_disk.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
   struct task_t *prev, *next ;		// ponteiros para usar em filas
   int id ;				// identificador da tarefa
   ucontext_t context ;			// contexto armazenado da tarefa
   void *stack ;			// aponta para a pilha da tarefa
   int prioD;
   int prioC;
   int status;

   int isUser;

   unsigned int nActivations;
   unsigned long nascimentoTime, inicioTurnoTime;
   unsigned long execTime, processorTime;

   struct task_t *waitlist;
   int exitcode_waitlist;

   unsigned int sleepingtime;
   // ... (outros campos serão adicionados mais tarde)
} task_t ;

//NAO FAÇO IDEIA DE QUANDO ESSA LINHA DA BUG OU NAO /ERRO
extern task_t taskMain, taskDispatcher, *taskCurrent, taskProntas, taskTerminadas , taskAdormecidas, taskDisk, taskDiskSuspend;

// estrutura que define um semáforo
typedef struct
{
	int lock;
	int contador;
	int status;
	struct task_t *suspensas;
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct mqueue_t
{
	struct mqueue_t *prev, *next ;		// ponteiros para usar em filas
	void *content;
	int max_msg;
	int msg_size;
	int status;
	semaphore_t *s_vaga;
    semaphore_t *s_buffer;
    semaphore_t *s_item;
  // preencher quando necessário
} mqueue_t ;

#endif

