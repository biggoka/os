//Create 1 pthread and print it
#include <inc/lib.h>

void *func1(void *a)
{
	cprintf("\ni am alive !!!!!!!!!!!!!!!!!\n");
	// sys_print_pthread_state(0);
	// return NULL;
	// sched_yield();
}


// ret = sys_pthread_create(a1, (pthread_t*)a2, (const struct pthread_attr_t*)a3, (void*(*)(void*)) a4, a5);


void
umain(int argc, char **argv)
{
	// pthread_t t1;
	// struct pthread_attr_t attr;
	// attr.priority = 2;
	// attr.sched_policy = SCHED_FIFO;
	// attr.pthread_type = PTHREAD_CREATE_DETACHED;
	if (1)
		cprintf("1\n");

	if (!0)
		cprintf("!0\n");

	if (!!1)
		cprintf("!!1\n");

	pthread *t1;

	// cprintf("!!!!! creating thread !!!!!!\n");
	sys_pthread_create(t1, NULL, &func1, NULL);
	// sys_yield();
}
