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
};


void
umain(int argc, char **argv)
{
	pthread t1, t2;
	int arg1 = 1, arg2 = 2;

	sys_pthread_create(&t1, NULL, &func1, (uint32_t)&arg1);
	sys_pthread_create(&t2, NULL, &func1, (uint32_t)&arg1);

	int *res;
	sys_pthread_join(t1, &res);
	sys_pthread_join(t2, &res);

	cprintf("both joins returned\n");
}
