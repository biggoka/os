#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/env.h>
#include <kern/monitor.h>
#include <kern/sched.h>
#include <kern/kclock.h>


struct Taskstate cpu_ts;
void sched_halt(void);

struct Env *queues[MAX_PRIORITY];


// void remove_from_queue(struct Env *env);
// void add_in_head(struct Env *env, int end_time);
// void add_in_tail(struct Env *env, int end_time);
// void sched_yield_by_time(void);

void add_in_tail(struct Env *env, int end_time)
{
	struct Env **cur = &(queues[env->priority]);
	while (*cur)
		cur = &((**cur).next_sched_queue);
	*cur = env;
	env->next_sched_queue = NULL;
	if (end_time != -1)
		env->end_time = end_time;
}

void add_in_head(struct Env *env, int end_time)
{
	env->next_sched_queue = queues[env->priority];
	queues[env->priority] = env;
	if (end_time != -1)
		env->end_time = end_time;
}

void remove_from_queue(struct Env *env)
{
	struct Env **cur = &(queues[env->priority]);
	while (*cur)
	{
		if ((**cur).env_id == env->env_id)
		{
			struct Env *tmp;
			tmp = *cur;
			*cur = (**cur).next_sched_queue;
			tmp->next_sched_queue = NULL;
		}
		else
		{
			cur = &((**cur).next_sched_queue);
		}
	}
}

void find_and_run(void)
{
	cprintf("find and run\n");
	int pr;
	for (pr = MAX_PRIORITY-1; pr >= MIN_PRIORITY; pr--)
	{
		struct Env *tmp;
		while (queues[pr])
		{
			tmp = queues[pr];
			queues[pr] = tmp->next_sched_queue;

			if (tmp->env_status == ENV_RUNNABLE)
			{
				if (tmp->end_time == 0 && tmp->priority != 1 && curenv->sched_policy == SCHED_RR)
				{
					tmp->end_time = TIME_QUANT;	
				}
				tmp->end_time += gettime();
				tmp->next_sched_queue = NULL;
				env_run(tmp);
			}
			else
			{
				tmp->next_sched_queue = NULL;
			}
		}
	}
}

int present_in_queues(struct Env *env)
{
	struct Env **cur = &(queues[env->priority]);
	while (*cur)
	{
		if ((**cur).env_id == env->env_id)
			return 1;
		cur = &((**cur).next_sched_queue);
	}
	return 0;
}

void sched_yield_by_time(void)
{
	cprintf("sched_yield_by_time\n");
	add_in_head(curenv, -1);
	sched_yield();
}

// void sched_yield(void)
// {
// 	cprintf("sched_yield\n");

// 	if (curenv)
// 	{
// 		int time_left = curenv->end_time - gettime();
// 		if (time_left <= 0 && curenv->priority != 1 && curenv->sched_policy == SCHED_RR)
// 		{
// 			remove_from_queue(curenv);
// 			add_in_tail(curenv, 0);
// 			if (curenv->env_status == ENV_RUNNING)
// 				curenv->env_status = ENV_RUNNABLE;
// 		}
// 		else
// 		{
// 			if (!present_in_queues(curenv))
// 			{
// 				add_in_tail(curenv, time_left);
// 			}
// 			else
// 			{
// 				remove_from_queue(curenv);
// 				add_in_head(curenv, time_left);
// 			}

// 			if (curenv->env_status == ENV_RUNNING)
// 			{
// 				curenv->env_status = ENV_RUNNABLE;
// 			}
// 		}
// 	}

// 	find_and_run();

// 	sched_halt();
// 	for(;;)
// 		;
// }


// Choose a user environment to run and run it.
void
sched_yield(void)
{
	cprintf("sched_yield\n");
	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// If there are no runnable environments,
	// simply drop through to the code
	// below to halt the cpu.

	//LAB 3: Your code here.
	struct Env *e;
	if (curenv == NULL)
		curenv = &envs[NENV - 1];
	e = &envs[(curenv - envs + 1) % NENV];
	while ((e != curenv) && (e->env_status != ENV_RUNNABLE))
		e = &envs[(e - envs + 1) % NENV];
	if ((e->env_status == ENV_RUNNABLE) || (e->env_status == ENV_RUNNING))
		env_run(e);
	
	// sched_halt never returns
	sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.

void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on CPU
	curenv = NULL;

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile (
		"movl $0, %%ebp\n"
		"movl %0, %%esp\n"
		"pushl $0\n"
		"pushl $0\n"
		"sti\n"
		"hlt\n"
	: : "a" (cpu_ts.ts_esp0));
}

