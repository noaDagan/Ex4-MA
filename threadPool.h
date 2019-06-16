// 311302137
// Noa Dagan

#include <sys/types.h>
#include "osqueue.h"
#include <stdbool.h>
#ifndef EX4_THREADPOOL_H
#define EX4_THREADPOOL_H

// struct save all the data of a task : function and parameters
typedef struct threadFunction
{
    void(*param);
    void(*func)(void*);
}ThreadFunction;

// struct save all the data of thread pool
typedef struct thread_pool {
    pthread_t* thread;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    OSQueue* osQueueFuncs;
    bool toDestroy;
    int threadCounter;
    int waitToTask;
    int numOfThread;
}ThreadPool;


ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif //EX4_THREADPOOL_H
