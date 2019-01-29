/* 
 * File:   dispatchQueue.c
 * ID:     676683456
 * Name:   Wentao Hao
 * upi:    hwen398
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "dispatchQueue.h"
 
sem_t semaphore;
dispatch_queue_thread_t *pool; 

void *thread_routine (void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&(pool->queue_lock));
        //If there is no task waiting and the pool hasn't been destroyed, keep waiting for the task
        while (pool->cur_queue_size == 0 && !pool->shutdown)
        {
            pthread_cond_wait(&(pool->queue_ready), &(pool->queue_lock));
        }
 
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&(pool->queue_lock));
            pthread_exit (NULL);
        }
        
        if(pool->queue_head->queue_type == SERIAL)
        {
            sem_wait(&semaphore);
        }
        // one task starts to run
        pool->cur_queue_size--;
        dispatch_queue_t *worker = pool->queue_head;
        pool->queue_head = worker->next;
        if(worker->task->type == SYNC)
        {
            worker->task->work(worker->task->params);
            task_destroy(worker->task);
            worker->is_end = 1;
            pthread_mutex_unlock(&(pool->queue_lock));
        }
        else
        {
            pthread_mutex_unlock(&(pool->queue_lock));
            //starts running
            worker->task->work(worker->task->params);
            task_destroy(worker->task);
            worker->is_end = 1;
        }
        if(pool->queue_head->queue_type == SERIAL)
        {
            sem_post(&semaphore);
        }
    }
}

void pool_init(int max_thread_num, dispatch_queue_t *list)
{
    pool = (dispatch_queue_thread_t *)malloc(sizeof(dispatch_queue_thread_t));
    //initialization
    pthread_mutex_init(&(pool->queue_lock), NULL);
    pthread_cond_init(&(pool->queue_ready), NULL);
    pool->queue_head = list;
    pool->max_thread_num = max_thread_num;
    pool->cur_queue_size = 0;
    pool->shutdown = 0;
    pool->threadid = (pthread_t *)malloc(max_thread_num*sizeof(pthread_t));
    for (int i = 0; i < max_thread_num; i++)
    { 
        pthread_create(&(pool->threadid[i]),NULL,thread_routine,NULL);
    }
}


dispatch_queue_t *dispatch_queue_create(queue_type_t queue_type)
{
    //create head node
    dispatch_queue_t *list;
    list = (dispatch_queue_t *)malloc(sizeof(dispatch_queue_t));
    list->queue_type = queue_type;
    list->prev = list;
    list->next = list;
    list->task = NULL;
    int num = sysconf(_SC_NPROCESSORS_ONLN);
    pool_init (num,list); 
    return list;
}

task_t *task_create(void (*work)(void *), void *params, char *name)
{
    task_t *task = (task_t *)malloc(sizeof(task_t));
    task->work = work; 
    task->params = params;
    strcpy(task->name, name);
    return task;
}
 
dispatch_queue_t *add_to_list(dispatch_queue_t *head, task_t *task) 
{
    dispatch_queue_t *new_node;
    new_node = (dispatch_queue_t *)malloc(sizeof(dispatch_queue_t));
    new_node->queue_type = head->queue_type;
    new_node->task = task;
    new_node->next = head;
    new_node->prev = head->prev;
    new_node->is_end = 0;
    head->prev->next = new_node;
    head->prev = new_node;
    return new_node;
}
 
int dispatch_async(dispatch_queue_t *list, task_t *task)
{
    if(list->queue_type == SERIAL)
    {
        sem_init(&semaphore, 0, 1);
    }
    pthread_mutex_lock(&(pool->queue_lock));
    task->type = ASYNC;
    dispatch_queue_t *new_node = add_to_list(list, task);
    if(pool->queue_head == list)
    {
        pool->queue_head = new_node;
    }

    pool->cur_queue_size++;
    pthread_mutex_unlock(&(pool->queue_lock));
    pthread_cond_signal(&(pool->queue_ready));
    return 0;
}

int dispatch_sync(dispatch_queue_t *list, task_t *task)
{
    pthread_mutex_lock(&(pool->queue_lock));
    task->type = SYNC;
    dispatch_queue_t *new_node = add_to_list(list,task);
    if(pool->queue_head == list)
    {
        pool->queue_head = new_node;
    }
    pool->cur_queue_size++;
    pthread_mutex_unlock(&(pool->queue_lock));
    pthread_cond_signal(&(pool->queue_ready));
    dispatch_queue_t *node;
    for (node = list->next; node != list; node = node->next) 
    {
        while(node->is_end == 0){}
    } 
    return 0;
}

 
void dispatch_queue_destroy(dispatch_queue_t *list)
{
    dispatch_queue_t *node, *tmp;
    for (node = list->next; node != list; node = tmp) 
    {
        tmp = node->next;
        free(node);
        node = NULL;
    }
    free(list);
    list = NULL;
    pool_destroy ();
}

int dispatch_queue_wait(dispatch_queue_t *list)
{
    dispatch_queue_t *node;
    for (node = list->next; node != list; node = node->next) 
    {
        while(node->is_end == 0){}
    } 
    pool->shutdown = 1;
    pthread_cond_broadcast(&(pool->queue_ready));
    return 0;
}


void pool_destroy ()
{
    free (pool->threadid);
    pthread_mutex_destroy(&(pool->queue_lock));
    pthread_cond_destroy(&(pool->queue_ready));
    free(pool);
    pool = NULL;
}
 
void task_destroy(task_t *task)
{
    free(task);
    task = NULL;
}