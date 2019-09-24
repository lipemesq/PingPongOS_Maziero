// Felipe de Lima mesquita GRR20174479
// Victor Rocha de Abreu   GRR20171623

#include "ppos.h"
#include "ppos_disk.h"


#define STACKSIZE 32768		/* tamanho de pilha das threads */

#define PRIO 0
#define NUMQUANTUM 20

// STATUS == 0 -> pronta / 1 -> terminadas / 2 -> suspensas

task_t taskMain, taskDispatcher, *taskCurrent, taskProntas, taskTerminadas , taskAdormecidas, taskDisk, taskDiskSuspend;

int id = 0, userTasks = 0;
int preemp = 0; // 0 quando pode ter preempção

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action,disk ;

// estrutura de inicialização to timer
struct itimerval timer ;

// Quatidade de quantums da tarefa corrente
unsigned int quantumRestante ; 

// "Relógio": número de Ticks
unsigned int relogio;

void diskDriverBody (void * args)
{

	disk_t *Pedido;
    while (1) 
    {
	    // obtém o semáforo de acesso ao disco
	   	sem_down(disco->s_disk);
	 
	    // se foi acordado devido a um sinal do disco
	    if (disco->tratador){

	    	printf("Veio do tratador\n");

	    	Pedido = disco->prev;

	    	Pedido->OP = 3;

	    	queue_remove((queue_t**)&disco->prev,(queue_t*)Pedido);

	    	queue_remove((queue_t**)&taskDiskSuspend.next, (queue_t*) Pedido->taskReq);
			queue_append((queue_t**)&taskProntas.next, (queue_t*) Pedido->taskReq);

	        // acorda a tarefa cujo pedido foi atendido

	        // 	lipemesq: pega a cabeça da lista de tarefas que estão aguardando o resultado,
	        //		muda o status e coloca na fila de prontos.
	        //		Também tem que colocar algum status de retorno.
	        //	? Como status de retorno podemos usar a própria struc de pedidos que o read/write 
	        //		criou, já que o temos armazenado. Colocar mais uma variável nessa struct não 
	        //		seria o fim do mundo.

	        disco->tratador = 0;
	    }
	 
	    // se o disco estiver livre e houver pedidos de E/S na fila
	    if ((disk_cmd(DISK_CMD_STATUS, 0, 0) == 1 )&& (queue_size((queue_t*)disco->next) > 0) ){

	    	Pedido = disco->next;

	    	switch(Pedido->OP){

	    		case(1):
	    			if (disk_cmd(DISK_CMD_READ, Pedido->block, Pedido->buffer) < 0){
	    				fprintf(stderr, "Erro na leitura do bloco: %d\n", Pedido->block );
	    				//	lipemesq: Aqui setamos aquele status de erro também e já podemos colocar
	    				//		a tarefa de volta na fila de prontas sem mais nem menos.
	    				Pedido->OP = 5;
	    			} else {
		    			queue_remove((queue_t**)&disco->next,(queue_t*)Pedido);
		    			queue_append((queue_t**)&disco->prev,(queue_t*)Pedido);
		    			Pedido->OP = 4;

		    			printf("Agendada leitura no bloco %d\n",Pedido->block );
	    			}

	    		break;

	    		case(2):
	    			if (disk_cmd(DISK_CMD_WRITE, Pedido->block, Pedido->buffer) < 0){
	    				fprintf(stderr, "Erro na escrita do bloco: %d\n", Pedido->block );
	    				//	lipemesq: o mesmo que ali em cima
	    				Pedido->OP = 5;
	    			} else {
		    			queue_remove((queue_t**)&disco->next,(queue_t*)Pedido);
		    			queue_append((queue_t**)&disco->prev,(queue_t*)Pedido);
		    			Pedido->OP = 4;

		    			printf("Agendada escrita no bloco %d\n",Pedido->block );
	    			}
	    		break;

	    		default:
	    			fprintf(stderr, "ERRO na leitura do primeiro elemento da fila do disco. O valor da operacao nao era esperado.\n"); 
	    			Pedido->OP = 5;
	    		break;
	    	} 

	        // escolhe na fila o pedido a ser atendido, usando FCFS
	        // solicita ao disco a operação de E/S, usando disk_cmd()
	    }
	 
	    // libera o semáforo de acesso ao disco
	 	sem_up(disco->s_disk);
	    // suspende a tarefa corrente (retorna ao dispatcher)

	    task_yield();

    }
}

// cria um semáforo com valor inicial "value"
int sem_create (semaphore_t *s, int value){

	if (s == NULL)
	{
		return(-1);

	} else {
		

		s->contador = value;
		s->suspensas = NULL;
		s->status = 0;
		return(0);
	}


}

// requisita o semáforo
int sem_down (semaphore_t *s){

	if (s == NULL){

		return(-1);

	} else {

		if (s->status == 1)
		{
			return(-1);
		} 
	
		preemp = 1;

		s->contador--;
		if (s->contador < 0){

			queue_remove((queue_t**) &taskProntas.next, (queue_t*) taskCurrent);
			queue_append((queue_t**) &s->suspensas, (queue_t*) taskCurrent);
			taskCurrent->status = 2;
	
			preemp = 0;
			task_yield();
		} else {
	
			preemp = 0;
		}
	
		if ((s == NULL) || (s->status == 1)){

			return(-1);

		} else {
		
			return (0);
		}

	
	}

}

// libera o semáforo
int sem_up (semaphore_t *s){

	if (s == NULL){

		return(-1);

	} else {

		if (s->status == 1)
		{
			return(-1);
		}
	
		preemp = 1;

		s->contador++;
		
		task_t *aux;
		
		if (s->contador <= 0)
		{

			aux = s->suspensas;

			aux->status = 0;
			queue_remove((queue_t**) &s->suspensas, (queue_t*) aux);
			queue_append((queue_t**) &taskProntas.next, (queue_t*) aux);

		}


		preemp = 0;

		return(0);
	
	}
}

// destroi o semáforo, liberando as tarefas bloqueadas
int sem_destroy (semaphore_t *s){

	if (s == NULL)
	{
		return(-1);

	} else {
	
		task_t *aux;
		
		preemp = 1;

		while(s->suspensas){ //verifica se existe alguma tarefa suspensa


	      	aux = s->suspensas;
			aux->status = 0;

	        queue_remove((queue_t**) &s->suspensas, (queue_t*) aux);
			queue_append((queue_t**) &taskProntas.next, (queue_t*) aux);

	   	}
	   	
	   	s->status = 1;
	   	
		preemp = 0;

		return(0);
	}
}

// cria uma fila para até max mensagens de size bytes cada
int mqueue_create (mqueue_t *queue, int max, int size){

	if (queue == NULL)
	{
		return(-1);
	}
	
	queue->max_msg = max;
	queue->msg_size = size;

	queue->prev = NULL;
	queue->next = NULL;

	queue->status = 0;

	queue->s_vaga = malloc(sizeof(semaphore_t*));
   	queue->s_buffer = malloc(sizeof(semaphore_t*));
   	queue->s_item = malloc(sizeof(semaphore_t*));

	sem_create (queue->s_vaga, max);
   	sem_create (queue->s_buffer, 1);
   	sem_create (queue->s_item, 0);

   	return(0);
}

// envia uma mensagem para a fila
int mqueue_send (mqueue_t *queue, void *msg){


	if (queue == NULL)
	{
		return(-1);
	}

	if (queue->status == 1)
	{
		return(-1);
	}

	mqueue_t *auxM = malloc(sizeof(*auxM));

	auxM->prev = NULL;
	auxM->next = NULL;

	auxM->content = malloc(queue->msg_size);

	memcpy(auxM->content,msg,queue->msg_size);

	sem_down(queue->s_vaga);

    sem_down(queue->s_buffer);

    if (queue->status == 1)
	{
		return(-1);
	}

	queue_append((queue_t**) &queue->next, (queue_t*) auxM);

    sem_up(queue->s_buffer);

    sem_up(queue->s_item);

    return(0);

}

// recebe uma mensagem da fila
int mqueue_recv (mqueue_t *queue, void *msg){

	if (queue == NULL)
	{
		return(-1);
	}

	if (queue->status == 1)
	{
		return(-1);
	}

	mqueue_t *auxM;

	sem_down(queue->s_item);

    sem_down(queue->s_buffer);	

    if (queue->status == 1)
	{
		return(-1);
	}

    memcpy(msg,queue->next->content,queue->msg_size);

    auxM = queue->next;
	   	  
	queue_remove((queue_t**) &queue->next, (queue_t*) queue->next);

	free(auxM);

	sem_up(queue->s_buffer);

    sem_up(queue->s_vaga);

    return(0);

}

// destroi a fila, liberando as tarefas bloqueadas
int mqueue_destroy (mqueue_t *queue){

	queue->status = 1;
	sem_destroy(queue->s_buffer);
	sem_destroy(queue->s_vaga);
	sem_destroy(queue->s_item);

	return(0);
}

// informa o número de mensagens atualmente na fila
int mqueue_msgs (mqueue_t *queue){

	if (queue->status == 0)
	{
		return(-1);
	}

	return(queue_size((queue_t*)queue->next));

}

// suspende a tarefa corrente por t milissegundos
void task_sleep (int t){

	// Remove a tarefa da lista de prontas e adiciona na lista de adormecidas (lista especial para quem esta no sleep)

	queue_remove((queue_t**) &taskProntas.next, (queue_t*) taskCurrent);
	queue_append((queue_t**) &taskAdormecidas.next, (queue_t*) taskCurrent);
	taskCurrent->sleepingtime = systime() + t; 	//task sleeping tem o clock onde a tarefa deve acordar
	taskCurrent->status = 2;					// status 2 = suspensas

	task_yield();								// faz a troca da tarefa 
}

// a tarefa corrente aguarda o encerramento de outra task
int task_join (task_t *task){

	//Remove a tarefa da lista de prontas e adiciona na lista interna da tarefa q ela vai esperar

	if (task->next != NULL){ // Se a tarefa ainda existir faz o processo, caso contrario retorna -1

		queue_remove((queue_t**) &taskProntas.next, (queue_t*) taskCurrent);
		queue_append((queue_t**) &task->waitlist, (queue_t*) taskCurrent);

		task_yield();

		return (taskCurrent->exitcode_waitlist); //retorna o codigo de exit da tarefa q estava esperando, ele fica guardado em um campo da struct da tarefa q adormeceu
	}

	return(-1);
}

// tratador do sinal
void tratadorTick (int signum) {

	// a cada tick do clock (1ms) verifica se a tarefa ainda ja esta a 20 quantuns 
	// se tiver faz task yield e muda a tarefa, ou diminui o quantum restante

  	relogio++; // atualiza o relogio do sistema para nao precisar ficar chamando o kernel
	if (taskCurrent->isUser == 1){
		quantumRestante-- ;
		if (quantumRestante == 0){
			if (preemp == 0){

				task_yield();
			} else {

				quantumRestante++;
			}
		}
	}

}



task_t* scheduler(){

	//escolhe qual a proxima função que vai assumir o controle do processador
	
	if(taskProntas.next != NULL){  // Verifica se tem alguma tarefa pronta

		task_t *aux,*men;	

		men = taskProntas.next; // contem a tarefa com menor prioridade para controlar o envelecimento e sistema de prioridades
		aux = men->next;

		while(aux != taskProntas.next){ //percorre toda a fila de prontas verificando se tem alguma prioridade menor (no caso mais prioritario)

			if (aux->prioC < men->prioC){
		
				men = aux;
		
			}

			aux = aux->next;
		
		}

		//men contem a tarefa com menor prioridade que deve assumir a CPU

		aux = taskProntas.next; 
		
		do{

			if (aux->prioC > -20){
		
				aux->prioC--;
		
			}

			aux = aux->next;

		}while(aux != taskProntas.next); //percorre toda a fila de prontas fazendo o envelhecimento 


		men->prioC = men->prioD; //reinicia a prioridade da tarefa que vai assumir o controle

		return(men);
	}

	return(NULL); //caso nao tenha nenhuma tarefa na fila de prontas retorna NULL
}

void trataFilaAdormecidas(unsigned int timeagora){

	if (taskAdormecidas.next){ //se existe algum elemento na fila de adormecidas

	   	task_t *aux, *ant;	   	

		aux = taskAdormecidas.next;

		if (aux->sleepingtime <= timeagora){ // se a cabeça da fila precisa acordar chama recursão para tratar a cabeça

			//remove da fila de adormecidas e coloca novamente na de prontas

			queue_remove((queue_t**) &taskAdormecidas.next, (queue_t*) aux);
			queue_append((queue_t**) &taskProntas.next, (queue_t*) aux);

			aux->sleepingtime = 0;		//sleeping time volta a 0 para evitar bugs
			aux->status = 0;			//status 0 = pronta

			trataFilaAdormecidas(timeagora);

		} else { //caso a cabeça nao precise acordar trata o resto da fila sem recursão

			//salva a anterior para caso remova da fila de adormecidas e adicione na de prontas nao perca o avanço

			ant = aux;
			aux = aux->next;			

			while(aux != taskAdormecidas.next){ //percorre todo o resto da fila

				if (aux->sleepingtime <= timeagora){

					queue_remove((queue_t**) &taskAdormecidas.next, (queue_t*) aux);
					queue_append((queue_t**) &taskProntas.next, (queue_t*) aux);

					aux->sleepingtime = 0;
					aux->status = 0;

					aux = ant->next;

				} else { 

					ant = aux;
					aux = aux->next;

				}
			}

		}
	
	}

}

void dispatcher_body (){ // dispatcher é uma tarefa

	task_t *next;

	while ( userTasks > 0 ){ // enquanto existir tarefas de usuarios

		while(taskTerminadas.next){ //verifica se existe alguma terminada

	      	next = taskTerminadas.next;
	        queue_remove((queue_t**)&taskTerminadas.next, (queue_t*)taskTerminadas.next);
	      	free(next->stack);
	    	userTasks--;

	   	}	

	   	unsigned int timeagora = systime(); //OLHAR ISSO PARA POSSIVEL MELHORA NO CODIGO 
		trataFilaAdormecidas(timeagora); //chama o tratador de filas de adormecidas com parametro do tempo atual

    	next = scheduler() ;  // scheduler é uma função que retorna NULL ou a proxima tarefa a possuir o CPU
      
      	if (next)
      	{
      	 
	        // ações antes de lançar a tarefa "next", se houverem
	        task_switch (next) ; // transfere controle para a tarefa "next"
	        // ações após retornar da tarefa "next", se houverem
      	}
   }
   task_exit(0) ; // encerra a tarefa dispatcher
}

void tratadorDisk (int signum) {

	disco->tratador = 1;
	taskDisk.status = 0;
	task_yield();


}

// funções gerais ==============================================================

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init () {


	setvbuf(stdout, 0, _IONBF, 0);

	taskMain.id = id;
	id++;
	getcontext(&(taskMain.context));
	taskCurrent = &taskMain;
	taskMain.isUser = 1;
	userTasks++;

	queue_append((queue_t**) &taskProntas.next, (queue_t*) &taskMain);  //NOVALINHA
 
	task_create(&taskDispatcher,dispatcher_body,NULL);
	taskDispatcher.isUser = 0;
	userTasks--;

	task_create(&taskDisk,diskDriverBody,NULL);
	taskDisk.isUser = 0;
	userTasks--;

	// registra a a��o para o sinal de timer SIGALRM
	action.sa_handler = tratadorTick ;
	sigemptyset (&action.sa_mask) ;
	action.sa_flags = 0 ;
	if (sigaction (SIGALRM, &action, 0) < 0) {
		perror ("Erro em sigaction: sinal do clock") ;
		exit (1) ;
	}

	disk.sa_handler = tratadorDisk ;
	sigemptyset (&disk.sa_mask) ;
	disk.sa_flags = 0 ;
	if (sigaction (SIGUSR1, &disk, 0) < 0) {
		perror ("Erro em sigaction: sina do disco") ;
		exit (1) ;
	}
	// ajusta valores do temporizador
	timer.it_value.tv_usec = 1000 ;      // primeiro disparo, em micro-segundos
	timer.it_value.tv_sec  = 0 ;      // primeiro disparo, em segundos
	timer.it_interval.tv_usec = 1000 ;   // disparos subsequentes, em micro-segundos
	timer.it_interval.tv_sec  = 0 ;   // disparos subsequentes, em segundos

	// arma o temporizador ITIMER_REAL (vide man setitimer)
	if (setitimer (ITIMER_REAL, &timer, 0) < 0) {
		perror ("Erro em setitimer: ") ;
		exit (1) ;
	}

	relogio = 0;
}

// gerência de tarefas =========================================================

// Cria uma nova tarefa. Retorna um id> 0 ou erro.
int task_create (task_t *task,			// descritor da nova tarefa
                 void (*start_func)(void *),	// funcao corpo da tarefa
                 void *arg) {			// argumentos para a tarefa



   getcontext (&(task->context)) ;

   task->stack = malloc (STACKSIZE) ;
   if (task->stack)
   {
      task->context.uc_stack.ss_sp = task->stack ;
      task->context.uc_stack.ss_size = STACKSIZE ;
      task->context.uc_stack.ss_flags = 0 ;
      task->context.uc_link = 0 ;
   }
   else
   {
      perror ("Erro na criação da pilha: ") ;
      return(-1) ;
   }

   makecontext ((&(task->context)), (void *)(*start_func), 1, arg) ;

   task->id = id;
   id++;

   task->prioD = PRIO;
   task->prioC = PRIO;

   userTasks++;

   task->status = 0;
   task->isUser = 1;

   task->nActivations = 0;
   task->execTime = 0;
   task->processorTime = 0;
   task->nascimentoTime = relogio;
   task->inicioTurnoTime = 0;

   if (task->id != 1){

   	queue_append((queue_t**) &taskProntas.next, (queue_t*) task);  //NOVALINHA
   

   }


   	#ifdef DEBUG
	printf ("task_create: criou tarefa %d\n", task->id) ;
	#endif

   return(task->id);

}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode) {

   	#ifdef DEBUG
	printf ("task_create: criou tarefa %d\n", task->id) ;
	#endif

	taskCurrent->status = 1;
	queue_remove((queue_t**) &taskProntas.next, (queue_t*) taskCurrent);	//NOVALINHA
	queue_append((queue_t**) &taskTerminadas.next, (queue_t*) taskCurrent); //NOVALINHA

	task_t *aux;

	while(taskCurrent->waitlist){											//NOVALINHA

		aux = taskCurrent->waitlist;										//NOVALINHA

		queue_remove((queue_t**) &taskCurrent->waitlist, (queue_t*) aux);	//NOVALINHA
		aux->exitcode_waitlist = exitCode;
		queue_append((queue_t**) &taskProntas.next, (queue_t*) aux); 		//NOVALINHA
		aux->status = 0;													//NOVALINHA

	}

	taskCurrent->processorTime += (relogio - taskCurrent->inicioTurnoTime);
	taskCurrent->execTime = relogio - taskCurrent->nascimentoTime;

	printf("Task %d exit: execution time %lu ms, processor time %lu ms, %u activations\n", 
		taskCurrent->id, taskCurrent->execTime, taskCurrent->processorTime, taskCurrent->nActivations);

	if (userTasks == 0){

		exit (0);

	} else {

		task_switch(&taskDispatcher);

	}
	
	exitCode++;
	
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task) {

	taskCurrent->processorTime += (relogio - taskCurrent->inicioTurnoTime);

	task_t *aux;
	aux = taskCurrent;
	taskCurrent = task;

	quantumRestante = NUMQUANTUM; // Reseta a quantidade de quantums

	taskCurrent->nActivations++;
	taskCurrent->inicioTurnoTime = relogio;

	swapcontext (&(aux->context), &(taskCurrent->context));

	return(0);

}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id () {

	return(taskCurrent->id);

}

// libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue")
void task_yield (){

	task_switch(&taskDispatcher);

}

// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio){

if (task != NULL){

		task->prioD = prio;
		task->prioC = prio;
		
	} else {

		taskCurrent->prioD = prio;
		taskCurrent->prioC = prio;

	}
	

}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task){

	if (task != NULL){

		return(task->prioD);
		
	} else {

		return(taskCurrent->prioD);

	}

	
}

unsigned int systime () {
	return relogio;
}
