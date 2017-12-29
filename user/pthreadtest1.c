//Create 1 pthread and print it
#include <inc/lib.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>

void *func1(void *a)
{
	cprintf("\ni am alive  %d !!!!!!!!!!!!!!!!!\n", (int)(*((int *)(a))));

	for (;;){
		cprintf("I am thread with arg %d\n", (int)(*((int *)(a))));
		if ((int)(*((int *)(a))) == 1)
		{
			sys_print_pthread_info(-1);
		}
		int *res = malloc(4);
		*res = 42;
		sys_pthread_exit(res);
		sys_yield();
	}
};


void
umain(int argc, char **argv)
{
	cprintf("in umain \n");
	pthread t1, t2;
	int arg1 = 1, arg2 = 2;

	struct pthread_params params;
	params.priority = 2;
	params.sched_policy = SCHED_RR;
	params.pthread_type = PTHREAD_CREATE_JOINABLE;

	sys_pthread_create(&t1, &params, &func1, &arg1);

	int *res;
	cprintf("waiting for join\n");
	sys_pthread_join(t1, (void**)&res);
	assert(*res == 42);
	cprintf("!!!joined successfully!!!, res is %d\n", *res);
}
