//Create test functions that change pthread params
#include <inc/lib.h>

void *func1(void *a)
{
	int i;
	for (i = 0; i < 10; i++)
	{
		sys_print_pthread_info(-1);
		sys_yield();
	}
	return NULL;
};

void *func2(void *a)
{
	cprintf("i will try to change my priority!\n");
	pthread t;
	struct pthread_params params;
	params.priority = 2;
	params.sched_policy = SCHED_RR;
	params.pthread_type = PTHREAD_CREATE_JOINABLE;
	sys_pthread_create(&t, &params, &func1, NULL);

	int i;
	for (i = 0; i < 10; i++)
	{
		if (i == 5)
		{
			params.priority = 1;
			params.pthread_type = JOINABLE;
			sys_sched_setparam(t, &params);
		}

		sys_print_pthread_info(-1);
		sys_yield();
	}

	return NULL;
};


void
umain(int argc, char **argv)
{
	pthread t1, t2;
	int arg1 = 1, arg2 = 2;

	cprintf("creating pthread from main\n");
	struct pthread_params params;
	params.priority = 2;
	params.sched_policy = SCHED_RR;
	params.pthread_type = PTHREAD_CREATE_JOINABLE;
	sys_pthread_create(&t1, &params, &func2, (uint32_t)(&arg1));
}
