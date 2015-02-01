/*
 * common.h
 *      Author: jingleizhang
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

//shm
#define SHM_BUFF_LEN 2*1024*1024 //2M
#define SHM_KEY 0x1234

//pipe
#define PIPE_BUFF_LEN 512

typedef struct s_shared_st{
	int is_running;
	int buffer_finish;
	char text[SHM_BUFF_LEN + 1];
}shared_st;

typedef struct s_shm{
	shared_st *shared_mem;//store shared_st
	key_t shm_key;
	pid_t pid;
	int shmid;
	int pipe[2];
}shared_shm;

#endif /* COMMON_H_ */
