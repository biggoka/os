//join test
#include <inc/lib.h>

void *func1(void *a)
{
	cprintf("\ni am alive  %d !!!!!!!!!!!!!!!!!\n", (int)(*((int *)(a))));
	int i = 0;
	for (i = 0;;i++){
		cprintf("I am thread with arg %d, i return 4242\n", (int)(*((int *)(a))));
		if ((int)(*((int *)(a))) == 1)
		{
			sys_print_pthread_info(-1);
		}
		int *res = malloc(4);
		*res = 4242;
		if (i > 10)
			return (void*)res;
			// sys_pthread_exit((void*)res);
		sys_yield();
	}
	return NULL;
};

void *func2(void *a)
{
	cprintf("i want to join thread!\n");
	int *res;
	sys_pthread_join(*(pthread*)a, (void**)&res);
	*res += 2;
	return res;
}

void *func3(void *a)
{
	cprintf("i want to join thread! but no one will join me!\n");
	int *res;

	int i;
	for(i=0;i<5;i++)
		sys_yield();

	sys_pthread_join(*(pthread*)a, (void**)&res);
	// assert(*res == 4242);
	cprintf("!!!!!!!!!i joined, res is %d!!!!!!!!!!!!!!!!!\n", *res);
	return res;
}


void
umain(int argc, char **argv)
{
	cprintf("in umain \n");
	pthread t1, t2, t3,t4;
	int arg1 = 1;

	struct pthread_params params;
	params.priority = 2;
	params.sched_policy = SCHED_RR;
	params.pthread_type = PTHREAD_CREATE_JOINABLE;


	sys_pthread_create(&t1, &params, &func1, (uint32_t)&arg1);

	int *res;
	cprintf("waiting for join\n");
	sys_pthread_join(t1, (void**)&res);
	assert(*res == 4242);
	cprintf("joined successfully, res is %d\n\n\n", *res);

	cprintf("\n\nTesting join from thread\n");
	sys_pthread_create(&t1, &params, &func1, (uint32_t)&arg1);
	sys_pthread_create(&t2, &params, &func2, (uint32_t)&t1);
	sys_pthread_join(t2, (void**)&res);
	assert(*res == 4242 + 2);
	cprintf("joined successfully, res is %d\n\n\n", *res);

	cprintf("\n\nTesting join from thread with no join\n");
	sys_pthread_create(&t1, &params, &func1, (uint32_t)&arg1);
	sys_pthread_create(&t2, &params, &func3, (uint32_t)&t1);
	sys_pthread_create(&t3, &params, &func1, (uint32_t)&arg1);
	sys_pthread_create(&t4, &params, &func1, (uint32_t)&arg1);

	// for(;;);
}
