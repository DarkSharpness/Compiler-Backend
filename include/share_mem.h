#pragma once
#include <cstdlib>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

struct shm_data {
	unsigned long long cycle_start;
	unsigned long long cycle_end;
};

static const void *shm_addr_expect = (void *) 0x0000'003f'0000'0000;
static void *shm_addr = nullptr;
static volatile shm_data *run_data = nullptr;

static int create_shm() {
	auto shmid = shmget(1, 1024 * 4, IPC_CREAT | 0666);
	if (shmid == -1)
		return -1;
	shm_addr = shmat(shmid, shm_addr_expect, 0);
	if (shm_addr == (void *) -1)
		return -2;
	run_data = (shm_data *) shm_addr;
	return 0;
}
