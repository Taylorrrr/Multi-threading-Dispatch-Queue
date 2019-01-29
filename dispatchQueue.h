/* 
 * File:        dispatchQueue.h
 * Author:      robert
 * Name:        Wentao Hao
 * Modified by: hwen398
 * ID:          676683456
 */

#ifndef DISPATCHQUEUE_H
#define	DISPATCHQUEUE_H

#include <semaphore.h>
#include <pthread.h>
    
#define error_exit(MESSAGE)     perror(MESSAGE), exit(EXIT_FAILURE)

    typedef enum {                  // whether dispatching a task synchronously or asynchronously
        ASYNC, SYNC
    } task_dispatch_type_t;
    
    typedef enum {                    // The type of dispatch queue.
        CONCURRENT, SERIAL
    } queue_type_t;

    typedef struct task {
        char name[64];                // to identify it when debugging
        void (*work)(void *);         // the function to perform
        void *params;                 // parameters to pass to the function
        task_dispatch_type_t type;    // asynchronous/synchronous
    } task_t;
    
    typedef struct dispatch_queue_t dispatch_queue_t;                  // dispatch queue type
    typedef struct dispatch_queue_thread_t dispatch_queue_thread_t;    // thread pool for the queue

    struct dispatch_queue_thread_t{
        pthread_mutex_t queue_lock;   // mutex
        pthread_cond_t queue_ready;   // signal
        dispatch_queue_t *queue_head; // pointer of the dispatch queue
        int shutdown;                 // destroy the thread pool?
        pthread_t *threadid;          // pointer of ids of the threads in the thread pool
        int max_thread_num;           // maximun number of the thread
        int cur_queue_size;           // number of the threads in the waiting queue
    };

    struct dispatch_queue_t {
        queue_type_t queue_type;      // queue type: serial/concurrent 
        dispatch_queue_t *prev;      
        dispatch_queue_t *next;       
        task_t *task;                 
        int is_end;
    };
    
    task_t *task_create(void (*)(void *), void *, char*);
    
    void task_destroy(task_t *);

    dispatch_queue_t *dispatch_queue_create(queue_type_t);
    
    void dispatch_queue_destroy(dispatch_queue_t *);
    
    int dispatch_async(dispatch_queue_t *, task_t *);
    
    int dispatch_sync(dispatch_queue_t *, task_t *);
        
    int dispatch_queue_wait(dispatch_queue_t *);

    void *thread_routine(void *);

    void pool_init(int ,dispatch_queue_t *);

    void pool_destroy();

    dispatch_queue_t *add_to_list(dispatch_queue_t *, task_t *);

#endif	/* DISPATCHQUEUE_H */