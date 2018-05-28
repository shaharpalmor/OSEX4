/*
 * shahar palmor
 * 307029927
*/
#include <pthread.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "threadPool.h"


//TODO unlink myfile.txt

ThreadPool *tpCreate(int numOfThreads) {
    ThreadPool *pool;
    int i, out;
    out = open("myFile.txt", O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
    if ((out == FAIL) || (numOfThreads <= 0))
        handleFail();
    // change the file where the errors will be written.dup2(out, 1);

    //initialize the pointer to the threadPool
    if ((pool = (ThreadPool *) malloc(sizeof(ThreadPool))) == NULL)
        handleFail();
    pool->threadCount = numOfThreads;
    pool->queue = osCreateQueue();
    pool->stopped = 0;
    pool->state = RUN;
    pool->executeTasks = executeTasks;
    //initialize the threads in the pool, and the tasks in the queue
    if ((pool->threads = (pthread_t *) malloc(sizeof(pthread_t) * numOfThreads)) == NULL)
        handleFail();

    //initialize  the mutex for the critical sections.
    if (pthread_mutex_init(&pool->lock, NULL) != 0)
         handleFail();
    // create the theards in the pool
    for (i = 0; i < numOfThreads; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, func, pool) != 0)
            handleFail();
    }
    return pool;

}
//
void *func(void *args) {
    ThreadPool *pool = (ThreadPool *) args;
    pool->executeTasks(pool);
}

void executeTasks(void *args) {
    ThreadPool *pool = (ThreadPool *) args;
    while (pool->state != AFTER_JOIN) {
        pthread_mutex_lock(&pool->lock);
        if (pool->state != AFTER_JOIN) {
            if (!osIsQueueEmpty(pool->queue)) {
                task *task = osDequeue(pool->queue);
                pthread_mutex_unlock(&pool->lock);
                //activating the function inside the thread.
                task->function(task->argument);
                free(task);//free the allocation for the task
            } else {
                pthread_mutex_unlock(&pool->lock);
                sleep(1);
            }
            if (pool->state == BEFORE_JOIN) {
                break;
            }
        }

    }
}


int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param) {


    if (threadPool->stopped)
        return FAIL;
    task *newTask = (task *) malloc(1 * sizeof(task));
    if (newTask == NULL)
        handleFail();
    newTask->argument = param;
    newTask->function = computeFunc;
    pthread_mutex_lock(&threadPool->lock);
    osEnqueue(threadPool->queue, newTask);
    pthread_mutex_unlock(&threadPool->lock);
    return SUCCESS;
}

void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {
    int i, j;
    if (shouldWaitForTasks) {
        // Not allow insert more task to queue
        //todo:busy waiting!!
        while (1) {
            if (osIsQueueEmpty(threadPool->queue)) {
                break;
            } else {
                sleep(1);
            }
        }
    }
    threadPool->state = BEFORE_JOIN;
    threadPool->stopped = 1;

    for (i = 0; i < threadPool->threadCount; ++i)
        pthread_join(threadPool->threads[i], NULL);

    threadPool->state = AFTER_JOIN;
    /*for (j = 0; j < threadPool->threadCount; j++)
        free((void *) threadPool->threads[j]);
*/
    free(threadPool->threads);
    osDestroyQueue(threadPool->queue);

    free(threadPool);
    pthread_mutex_destroy(&threadPool->lock);

}

void handleFail() {
    write(STDERR, ERROR, SIZEERROR);
    exit(FAIL);
}