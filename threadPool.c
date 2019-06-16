// 311302137
// Noa Dagan

#include <malloc.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include "threadPool.h"

// const msg
const char MSG_ERROR[150] = "Error in system call\n";
const int LEN_MSG = sizeof(MSG_ERROR);

const CANT_INSERT = -1;
const INSERT_OK = 0;

void freeAll(ThreadPool* threadPool) ;
int printErr();
ThreadFunction* createFunction( void (*computeFunc)(void *), void *param);

// The function print error in system call
int printErr() {
    write(2, MSG_ERROR, LEN_MSG);
    return -1;
}

// The function free all the memory and destroy mutex of thread pool
void freeAll(ThreadPool* threadPool) {
    // run over all the task in the queue and free them
    while(!osIsQueueEmpty(threadPool->osQueueFuncs)) {
        ThreadFunction *function = osDequeue(threadPool->osQueueFuncs);
        free(function);
    }
    // destroy mutex
    osDestroyQueue(threadPool->osQueueFuncs);
    pthread_cond_destroy(&threadPool->cond);
    pthread_mutex_destroy(&threadPool->mutex);
    free(threadPool->thread);
    free(threadPool);
}

// The function create and run all the threads
static void *createThread(void *param) {
    ThreadPool *threadPool = (ThreadPool *) param;
    // if empty write error
    if(threadPool == NULL) {
        printErr();
        return NULL;
    }
    // loop endless
    for (;;) {
        // lock for another thread
        if (pthread_mutex_lock(&(threadPool->mutex)) != 0) {
            printErr();
            return NULL;
        }
        // waiting while the queue is empty and not destroy yet
        while ((osIsQueueEmpty(threadPool->osQueueFuncs) == true) && (threadPool->toDestroy == false)) {
            if (pthread_cond_wait(&threadPool->cond, &threadPool->mutex) != 0) {
                printErr();
                return NULL;
            }
        }
        // check if the queue is destroy and need to wait to task
        if ((threadPool->toDestroy) == true) {
            if (((threadPool->waitToTask != 0) && ( osIsQueueEmpty(threadPool->osQueueFuncs) == true)) ||
                (threadPool->waitToTask == 0)) {
                // cancel the lock and break
                if (pthread_mutex_unlock(&(threadPool->mutex)) != 0) {
                    printErr();
                    return NULL;
                }
                break;
            }
        }
        // push the function to queue
        ThreadFunction* func = osDequeue(threadPool->osQueueFuncs);
        // cancel the lock
        if (pthread_mutex_unlock(&(threadPool->mutex)) != 0) {
            printErr();
            free(func);
            return NULL;
        }
        // run the function on thread and free after
        ((*func->func))(func->param);
        free(func);
    }
    pthread_exit(NULL);
}

// The function create thread createThread with numOfThreads threads
ThreadPool *tpCreate(int numOfThreads) {
    int i;
    // create thread createThread and allocate a memory
    ThreadPool *threadPool = (ThreadPool *) malloc(sizeof(ThreadPool));
    // check if threadPool failed and print error
    if (threadPool == NULL) {
        printErr();
        return NULL;
    }
    // initialize the thread pool and createThread
    threadPool->numOfThread = numOfThreads;
    threadPool->threadCounter = 0;
    threadPool->waitToTask = 0;
    threadPool->toDestroy = false;
    threadPool->osQueueFuncs = osCreateQueue();
    // create threads and allocate memory
    threadPool->thread = (pthread_t *) malloc(sizeof(pthread_t) * numOfThreads);
    // check if failed and print error
    if (threadPool->thread == NULL) {
        printErr();
        return NULL;
    }
    // initialize mutex and condition
    if ((pthread_mutex_init(&(threadPool->mutex), NULL) != 0) ||
        (pthread_cond_init(&(threadPool->cond), NULL) != 0)) {
        printErr();
        return NULL;
    }
    // run over all the threads in thread createThread and create them
    for (i = 0; i < threadPool->numOfThread; i++) {
        if (pthread_create(&(threadPool->thread[i]), NULL, createThread,
                           (void *) threadPool) != 0) {
            printErr();
            return NULL;
        }
        // update threadCounter
        threadPool->threadCounter++;
    }
    return threadPool;
}

// The function create new task with parameters
ThreadFunction* createFunction( void (*computeFunc)(void *), void *param) {
    // not destroy yet allocate memory for struct task and initialize
    ThreadFunction *function = (ThreadFunction *) malloc(
            sizeof(ThreadFunction));
    if (function == NULL) {
         printErr();
         return NULL;
    }
    function->func = computeFunc;
    function->param = param;
    return function;
}

// The function create and insert a task to the queue
int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param) {
    // check if failed and print error
    if ((threadPool == NULL) || (computeFunc == NULL)) {
        return printErr();
    }
    // return -1
    if (threadPool->toDestroy == true) {
        return CANT_INSERT;
    } else {
        // lock for another thread and create task
        ThreadFunction* function = createFunction(computeFunc,param);
        if (pthread_mutex_lock(&(threadPool->mutex)) != 0) {
            return printErr();
        }
        osEnqueue(threadPool->osQueueFuncs, function);
        // notify for thread that have a function to run
        if (pthread_cond_signal(&(threadPool->cond)) != 0) {
            return printErr();
        }
        // unlock for another thread
        if (pthread_mutex_unlock(&(threadPool->mutex)) != 0) {
            return printErr();
        }
    }
    // return 0
    return INSERT_OK;
}

// The function destroy the thread pool
void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {

    // check if failed and print error
    if (threadPool == NULL) {
        printErr();
    }
    // if destroy start return
    if (threadPool->toDestroy == true) {
        return ;
    } else {
        if (pthread_mutex_lock(&(threadPool->mutex)) != 0) {
            printErr();
        }
        // initialize values
        int i;
        threadPool->waitToTask = shouldWaitForTasks;
        threadPool->toDestroy = true;
        // wake up all the thread
        if ((pthread_cond_broadcast(&(threadPool->cond)) != 0) ){
            printErr();
        }
        // unlock the threads
        if (pthread_mutex_unlock(&(threadPool->mutex)) != 0) {
            printErr();
        }
        // run over all the threads and make join to pthread
        // that thread waiting for another
        for (i = 0; i < threadPool->threadCounter; i++) {
            pthread_t pthreadTemp = threadPool->thread[i];
            if (pthread_join(pthreadTemp, NULL) != 0) {
                printErr();
            }
        }
        // pop task ,destroy mutex and free all the memory of thread pool
        freeAll(threadPool);
    }
}

