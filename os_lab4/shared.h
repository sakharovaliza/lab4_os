#pragma once
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static const char* SEM_FULL  = "/saharova_sem_full";
static const char* SEM_EMPTY = "/saharova_sem_empty";
static const char* SEM_MUTEX = "/saharova_sem_mutex";
static const char* SHM_IDX   = "/saharova_shm_idx";

struct QueueIdx {
    int head;
    int tail;
};

inline void cleanupIpc() {
    sem_unlink(SEM_FULL);
    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_MUTEX);
    shm_unlink(SHM_IDX);
}