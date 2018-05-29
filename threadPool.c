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

void writeError() {
    write(STDERR, ERROR, SIZEERROR);
    exit(FAIL);
}

ThreadPool *tpCreate(int numOfThreads) {
    ThreadPool *pool;
    int i, out;
    out = open("myFile.txt", O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
    if ((out == FAIL) || (numOfThreads <= 0))
        writeError();
    // change the file where the errors will be written.
    dup2(out, 1);

    //initialize the pointer to the threadPool
    if ((pool = (ThreadPool *) malloc(sizeof(ThreadPool))) == NULL)
        writeError();
    pool->threadCount = numOfThreads;
    pool->queue = osCreateQueue();
    pool->stopped = 0;
    pool->state = RUN;
    pool->executeTasks = executeTasks;
    //initialize the threads in the pool
    if ((pool->threads = (pthread_t *) malloc(sizeof(pthread_t) * numOfThreads)) == NULL)
        writeError();
    if (pthread_cond_init(&pool->condition, NULL) != 0)
        writeError();

    //initialize  the mutex for the critical sections.
    if (pthread_mutex_init(&pool->lock, NULL) != 0)
        writeError();
    // create the theards in the pool
    for (i = 0; i < numOfThreads; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, func, pool) != 0)
            writeError();
    }
    return pool;

}



int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param) {
    if (threadPool->stopped)
        return FAIL;

    task *newTask = (task *) malloc(1 * sizeof(task));
    if (newTask == NULL)
        writeError();
    newTask->argument = param;
    newTask->function = computeFunc;

    pthread_mutex_lock(&threadPool->lock);
    osEnqueue(threadPool->queue, newTask);
    //after inserting task we want to stop the thread from wait, wake up call that the queue is not empty any more
    if (pthread_cond_signal(&threadPool->condition) != 0) {
        writeError();
    }
    pthread_mutex_unlock(&threadPool->lock);
    return SUCCESS;
}

void *func(void *args) {
    ThreadPool *pool = (ThreadPool *) args;
    pool->executeTasks(pool);
}

void executeTasks(void *args) {
    ThreadPool *pool = (ThreadPool *) args;
    while (pool->state != BEFORE_FREE) {

        // in case the thread knows the queue is empty and we are inside the destroy function
        if (pool->state == SHOULD_DESTROY && osIsQueueEmpty(pool->queue)) {
            printf("checkkkk\n ");
            break;
            //in case we are not in destroy and the queue is empty the thread wait for signal
        } else if (osIsQueueEmpty(pool->queue) && (pool->state == RUN || pool->state == JOIN_ALL_THREADS)) {
            pthread_mutex_lock(&pool->lock);
            pthread_cond_wait(&pool->condition, &pool->lock);

        } else {
            //any ways the thread lock the queue when it execute task
            pthread_mutex_lock(&pool->lock);
        }

       /* printf("busy\n");
        if (osIsQueueEmpty(pool->queue) && (pool->state == RUN || pool->state == JOIN_ALL_THREADS)) {
            pthread_mutex_lock(&pool->lock);
            pthread_cond_wait(&pool->condition, &pool->lock);
        } else if (pool->state == SHOULD_DESTROY && osIsQueueEmpty(pool->queue)) {
            printf("checkkkk\n ");
            break;
        } else {
            pthread_mutex_lock(&pool->lock);
        }*/

        //there is a task to commit
        if (pool->state != BEFORE_FREE) {
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
            // we are inside the destroy function, and finished the tasks in the queue
            if (pool->state == JOIN_ALL_THREADS) {
                break;
            }
        }
        /*else{
            pthread_mutex_unlock(&pool->lock);
        }*/

    }
}



void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {
    int i;

    pthread_mutex_lock(&threadPool->lock);
    if (pthread_cond_broadcast(&threadPool->condition) > 0 || pthread_mutex_unlock(&threadPool->lock) > 0) {
        writeError();
    }
    // should finish the tasks in the queue, and not let insert more
    if (shouldWaitForTasks)
        threadPool->state = SHOULD_DESTROY;
        //should finish the threads and finish
    else
        threadPool->state = JOIN_ALL_THREADS;


    threadPool->stopped = 1;

    for (i = 0; i < threadPool->threadCount; ++i)
        pthread_join(threadPool->threads[i], NULL);

    threadPool->state = BEFORE_FREE;

    //free all remaind tasks
     while (!osIsQueueEmpty(threadPool->queue)) {
        task *task1 = osDequeue(threadPool->queue);
        free(task1);
    }

    free(threadPool->threads);
    osDestroyQueue(threadPool->queue);
    free(threadPool);
    pthread_mutex_destroy(&threadPool->lock);

}