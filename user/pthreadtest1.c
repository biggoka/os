//Create 1 pthread and print it
#include <inc/lib.h>

void *func1(void *a)
{
	cprintf("\ni am alive  %d !!!!!!!!!!!!!!!!!\n", (int)(*((int *)(a))));
	sys_print_pthread_info(-1);
	return NULL;
};

void *func2(void *a)
{
	cprintf("i will try to create pthread, and i am pthread myself\n");
	pthread t;
	int arg = 4242;
	sys_pthread_create(&t, NULL, &func1, (uint32_t)(&arg));
	cprintf("end of thread 2\n");

	// for (;;);
	return NULL;
};


void
umain(int argc, char **argv)
{
	cprintf("in umain \n");
	pthread t1, t2;
	int arg1 = 1, arg2 = 2;

	cprintf("creating pthread from main\n");
	sys_pthread_create(&t1, NULL, &func1, (uint32_t)(&arg1));

	cprintf("try to create pthread from pthread\n");
	sys_pthread_create(&t2, NULL, &func2, (uint32_t)(&arg2));
}
