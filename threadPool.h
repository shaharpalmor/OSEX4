/*
 * shahar palmor
 * 307029927
*/

#ifndef __THREAD_POOL__
#define __THREAD_POOL__
#include "osqueue.h"
#include "pthread.h"
#define ThreadPoolSize 5
#define FAIL -1
#define SUCCESS 1
#define STDERR 2
#define ERROR "Error in system call\n"
#define SIZEERROR 21


enum stateDestroy{RUN,SHOULD_DESTROY,BEFORE_JOIN,AFTER_JOIN};

typedef struct task{
    void (*function)(void *);
    void *argument;
} task;

typedef struct thread_pool
{
    int threadCount;
    int stopped;
    enum stateDestroy state;
    pthread_mutex_t lock, queueIsEmpty;
    pthread_t *threads;
    OSQueue *queue; //pointer to the queue
    void (*executeTasks)(void *);
}ThreadPool;

void* func(void *args);

void handleFail();

void executeTasks(void *args);

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
