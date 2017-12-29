//join test
#include <inc/lib.h>

void *func1(void *a)
{
	int i;
	for(i = 0; i < 10; i++)
	{
		sys_print_pthread_info(-1);
		sys_yield();
	}

	// cprintf("\ni am alive  %d !!!!!!!!!!!!!!!!!\n", (int)(*((int *)(a))));
	// int i = 0;
	// for (i = 0;;i++){
	// 	cprintf("I am thread with arg %d, i return 4242\n", (int)(*((int *)(a))));
	// 	if ((int)(*((int *)(a))) == 1)
	// 	{
	// 		sys_print_pthread_info(-1);
	// 	}
	// 	int *res = malloc(4);
	// 	*res = 4242;
	// 	if (i > 3)
	// 		return (void*)res;
	// 		sys_pthread_exit((void*)res);
	// 	sys_yield();
	// }
	// return NULL;
};


void
umain(int argc, char **argv)
{
	pthread t1, t2;
	int arg1 = 1, arg2 = 2;

	struct pthread_params params;
	params.priority = 1;
	params.sched_policy = SCHED_RR;
	params.pthread_type = PTHREAD_CREATE_JOINABLE;

	sys_pthread_create(&t1, &params, &func1, (uint32_t)&arg1);

	params.priority = 2;	
	sys_pthread_create(&t2, &params, &func1, (uint32_t)&arg1);

	int *res;
	sys_pthread_join(t1, &res);
	sys_pthread_join(t2, &res);

	cprintf("bot joins returned\n");
	// int *res;
	// cprintf("waiting for join\n");
	// sys_pthread_join(t1, (void**)&res);
	// assert(*res == 4242);
	// cprintf("joined successfully, res is %d\n\n\n", *res);

	// for(;;);
}
