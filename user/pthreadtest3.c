//join test
#include <inc/lib.h>

void *func1(void *a)
{
	int i;
	for(i = 0; i < 5; i++)
	{
		sys_print_pthread_info(-1);
		sys_yield();
	}
};

void *spawn_threads(void *arg)
{
	struct pthread_params params;

	params.priority = 1;
	params.sched_policy = SCHED_RR;
	params.pthread_type = PTHREAD_CREATE_JOINABLE;

	pthread t1;
	int arg1 = 1;

	params.priority = 2;
	arg1 = 2;
	sys_pthread_create(&t1, &params, &func1, (uint32_t)&arg1);

	params.priority = 3;
	arg1 = 3;
	sys_pthread_create(&t1, &params, &func1, (uint32_t)&arg1);

	params.priority = 4;
	arg1 = 4;
	sys_pthread_create(&t1, &params, &func1, (uint32_t)&arg1);

	return NULL;
}


void
umain(int argc, char **argv)
{
	pthread t1, t2;
	int arg1 = 1, arg2 = 2;

	struct pthread_params params;

	params.priority = 1;
	params.sched_policy = SCHED_FIFO;
	params.pthread_type = PTHREAD_CREATE_JOINABLE;

	sys_pthread_create(&t1, &params, &spawn_threads, (uint32_t)&arg1);
	// sys_pthread_join(t1, NULL);


	// int i;
	// for (i = 0; i < 15; i++)
	// 	sys_yield();

	// int *res;

	// cprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	// cprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	// cprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	// cprintf("waiting for joins\n");
	// cprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	// cprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	// cprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

	cprintf("both joins returned\n");
}
