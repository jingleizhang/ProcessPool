/*
 * processpool_pipe_shm.c
 *      Author: jingleizhang
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "common.h"

static int nNumChild = 0;

//shm
static shared_shm *shms;

void sig_int(int signo){
	fprintf(stderr, "sig_int() parent: start clean up\n");
	int i;
	for(i=0; i<nNumChild; i++){
		shms[i].shared_mem->is_running = 0;
	}

	//wait child
	wait(NULL);

	for(i=0; i<nNumChild; i++){
		//dettach memmory
		if( shmdt(shms[i].shared_mem) == -1){
			fprintf(stderr, "shmdt fail\n");
		}
		//remove shm
		if(shmctl(shms[i].shmid, IPC_RMID, 0) == -1){
			fprintf(stderr, "shmctl fail\n");
		}
	}
	exit(0);
}

int child_process(shared_shm *p_shm){
	int res = fork();
	if(res == -1){
		fprintf(stderr, "Fork fail\n");
		return -1;
	}
	if(res > 0){
		//parent
		return 0;
	}

	p_shm->pid = getpid();
	p_shm->shmid = shmget(p_shm->shm_key, sizeof(shared_st), 0666|IPC_CREAT);
	if(p_shm->shmid == -1){
		fprintf(stderr, "child shmget fail\n");
		return -1;
	}

	p_shm->shared_mem = (shared_st *)shmat(p_shm->shmid, NULL, 0);
	if(p_shm->shared_mem == NULL){
		fprintf(stderr, "shmat fail\n");
		return -1;
	}

	fprintf(stdout, "child[pid:%d]: memory attached at %p\n", p_shm->pid, p_shm->shared_mem);

	shared_st *p_shared_st = p_shm->shared_mem;
	p_shared_st->is_running = 1;
	p_shared_st->buffer_finish = 0;
	char pipe_buff[PIPE_BUFF_LEN];
	while(p_shared_st && p_shared_st->is_running){
		memset(pipe_buff, '\0', sizeof(pipe_buff));
		int n = read(*p_shm->pipe, pipe_buff, sizeof(pipe_buff));//receive command
		if(n > 0 && p_shared_st->buffer_finish == 0){
			fprintf(stderr, "child[pid:%d] got: |%s|\n", p_shm->pid, pipe_buff);

			int num = atoi(pipe_buff);
			memset(p_shared_st->text, '\0', SHM_BUFF_LEN);
			sprintf(p_shared_st->text, "shm: %d", num+1);

			p_shared_st->buffer_finish = 1;
		}
	}
	if(p_shared_st->is_running == 0){
		fprintf(stdout, "child[pid:%d] quit\n", p_shm->pid);
		//dettach memmory
		if(shmdt(p_shm->shared_mem) == -1){
			fprintf(stderr, "shmdt fail\n");
		}
	}
	return 0;
}

int start_test(const int num_child){
	int i;

	srand(time(NULL));

	//start shm
	shms = (shared_shm *)calloc(num_child, sizeof(shared_shm));
	if(shms == NULL){
		printf("Error: shms calloc fail\n");
		return -1;
	}
	for(i=0; i<num_child; i++){
		shms[i].shm_key = SHM_KEY + i;
		int res = pipe(shms[i].pipe);
		if(res != 0){
			printf("Error: pipe fail\n");
			return -2;
		}
	}

	//建立共享内存
	for(i=0; i<num_child; i++){
		shms[i].shmid = shmget(shms[i].shm_key, sizeof(shared_st), 0666|IPC_CREAT);
		if(shms[i].shmid == -1){
			fprintf(stderr, "parent shmget fail\n");
			return -1;
		}

		//attach memory
		shms[i].shared_mem = (shared_st *)shmat(shms[i].shmid, NULL, 0);
		if(shms[i].shared_mem == (void *)-1){
			fprintf(stderr, "shmat fail\n");
			return -1;
		}

		shms[i].shared_mem->is_running = 1;
		shms[i].shared_mem->buffer_finish = 0;

		fprintf(stdout, "parent: memory attached at %p\n", shms[i].shared_mem);
	}

	//start fork
	for(i=0; i<num_child; i++){
		child_process(&shms[i]);
	}

	signal(SIGINT, sig_int);

	char pipe_buff[PIPE_BUFF_LEN];
	for(;;){
		//随机选择一个
		i = rand() % num_child;

		shared_st *p_shared_st = shms[i].shared_mem;
		if(p_shared_st && p_shared_st->is_running){
			int r = rand() % 100;
			memset(pipe_buff, '\0', sizeof(pipe_buff));
			sprintf(pipe_buff, "%d", r);

			fprintf(stdout, "parent: write some random num: %d, to %d\n", r, i);
			write(shms[i].pipe[1], pipe_buff, strlen(pipe_buff));
		}

		while(p_shared_st && p_shared_st->buffer_finish == 0){
			sleep(1);
			fprintf(stdout, "parent: wait for reader\n");
			p_shared_st = shms[i].shared_mem;
		}

		if(p_shared_st!=NULL && p_shared_st->buffer_finish == 1){
			fprintf(stdout, "parent: got shm data |%s|\n", p_shared_st->text);
			p_shared_st->buffer_finish = 0;
		}
	}

	return 0;
}

int main(int argc, char *argv[]){
	if(argc != 2){
		printf("Usage: %s num_children\n", argv[0]);
		return -1;
	}
	nNumChild = atoi(argv[1]);

	return start_test(nNumChild);
}
