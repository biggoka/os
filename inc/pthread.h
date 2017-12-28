#ifndef PTHREAD_H
#define PTHREAD_H

#define PTHREADS_MAX 10
#define ERR_MAX_PTHREADS -123

typedef uint32_t pthread;
typedef struct pthread_params
{
    int priority;
    int sched_policy;
};

enum
{
    PTHREAD_CREATE_JOINABLE = 0,
    PTHREAD_CREATE_DETACHED,
    SCHED_RR,
    SCHED_FIFO,
    JOINABLE,
    DETACHED,
    JOINABLE_FINISHED,
};





#endif