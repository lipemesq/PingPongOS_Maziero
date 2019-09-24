#include "ppos_disk.h"

disk_t *disco;
// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize){

	if (disk_cmd(DISK_CMD_INIT, 0, 0) < 0){
	
		fprintf(stderr, "ERRO na criação do disco\n");
		return(-1);
	
	}

	*numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);

	if(*numBlocks < 0){

		fprintf(stderr, "ERRO na consultado do numero de blocos do disco\n");
		return(-1);

	}

	*blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);

	if(*blockSize < 0){

		fprintf(stderr, "ERRO na consultado do tamanho do blocos do disco\n");
		return(-1);

	}

	disco = malloc(sizeof(disk_t*));
	disco->s_disk = malloc(sizeof(semaphore_t*));

	sem_create(disco->s_disk, 1);

	disco->tratador = 0;

	taskDisk.status = 0;

	disco->next = NULL;
	disco->prev = NULL;

	return(0);
}

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer){

	disk_t *aux = malloc(sizeof(disk_t*));

	aux->next = NULL;
	aux->prev = NULL;

	aux->block = block;
	aux->buffer = buffer;
	aux->OP = 1;

	aux->taskReq = taskCurrent;


	sem_down(disco->s_disk);
	
	queue_remove((queue_t**)&taskProntas.next, (queue_t*) aux->taskReq);
	queue_append((queue_t**)&taskDiskSuspend.next, (queue_t*) aux->taskReq);

	
	queue_append((queue_t**)&disco->next, (queue_t*)aux);

	if(taskDisk.status == 2){
		taskDisk.status = 0;
	}

	sem_up(disco->s_disk);

	task_yield();

	return(0);

}

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer){

	disk_t *aux = malloc(sizeof(disk_t*));

	aux->next = NULL;
	aux->prev = NULL;

	aux->block = block;
	aux->buffer = buffer;
	aux->OP = 2;

	aux->taskReq = taskCurrent;


	sem_down(disco->s_disk);
	
	queue_remove((queue_t**)&taskProntas.next, (queue_t*) aux->taskReq);
	queue_append((queue_t**)&taskDiskSuspend.next, (queue_t*) aux->taskReq);

	queue_append((queue_t**)&disco->next, (queue_t*)aux);
	
	if(taskDisk.status == 2){
		taskDisk.status = 0;
	}

	sem_up(disco->s_disk);


	task_yield();

	return(0);

	//	lipemesq: aqui verificamos se a operação foi ocncluída com sucesso
	//		para retornar o valor certo


 }