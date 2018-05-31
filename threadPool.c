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


/**
 * write error in system call, and exit the program.
 */
void writeError() {
    int out;
    out = open("myFile.txt", O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
    if (out == FAIL)
        writeError();
    // change the file where the errors will be written.
    dup2(out, 1);
    write(STDERR, ERROR, SIZEERROR);
    dup2(1,out);
    exit(FAIL);

}


/**
 * create the thread pool and all the threads, mutexes, and update all the params
 * @param numOfThreads is the number of threads that will be in the pool
 * @return the thread pool.
 */
ThreadPool *tpCreate(int numOfThreads) {
    ThreadPool *pool;
    int i;

    if(numOfThreads <= 0){
        writeError();
    }
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


/**
 * insert a new task to the queue.
 * @param threadPool is the threadpool that will execute the task
 * @param computeFunc is the function of the task.
 * @param param is the parameters to the function.
 * @return if succeded or not.
 */
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

/**
 * this function calls the excute task of the thread pool
 * @param args is the thread pool
 */
void *func(void *args) {
    ThreadPool *pool = (ThreadPool *) args;
    pool->executeTasks(pool);
}

/**
 * this function is the running function of the queue. it run in all the threads all the time. if there is a task
 * in the queue it take it and commit it. other wise the queue is empty and the thread waits for a signal in order to
 * avoid busy waiting.
 * @param args is the thread pool so we could reach the queue.
 */
void executeTasks(void *args) {
    ThreadPool *pool = (ThreadPool *) args;
    while (pool->state != BEFORE_FREE) {

        // in case the thread knows the queue is empty and we are inside the destroy function
        if (pool->state == SHOULD_DESTROY && osIsQueueEmpty(pool->queue)) {
            break;
            //in case we are not in destroy and the queue is empty the thread wait for signal
        } else if (osIsQueueEmpty(pool->queue) && (pool->state == RUN || pool->state == JOIN_ALL_THREADS)) {
            pthread_mutex_lock(&pool->lock);
            pthread_cond_wait(&pool->condition, &pool->lock);

        } else {
            //any ways the thread lock the queue when it execute task
            pthread_mutex_lock(&pool->lock);
        }

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
                //sleep(1);
            }
            // we are inside the destroy function, and finished the tasks in the queue
            if (pool->state == JOIN_ALL_THREADS) {
                break;
            }
        }
    }
}


/**
 * this function destroy the pool. according to the flag:
 * 0 - when we should join all the threads, until they will finish their current tasks.
 * 1 - if we should also finish all the tasks in the queue and not let insertion of other tasks.
 * @param threadPool
 * @param shouldWaitForTasks is the flag for the type of the destroy.
 */
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