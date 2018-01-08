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

int present_in_queues(struct Env *env);

void add_in_tail(struct Env *env, int end_time)
{
	// return add_in_head(env, end_time);

	if (!env)
		return;

	if (present_in_queues(env))
	{
		remove_from_queue(env);
	}

	struct Env **cur = &(queues[env->priority]);

	if (!(*cur))
	{
		queues[env->priority] = env;
	}
	else
	{
		while ((*cur)->next_sched_queue)
			cur = &((**cur).next_sched_queue);

		(*cur)->next_sched_queue = env;
	}

	// *cur = env;
	env->next_sched_queue = NULL;
	if (end_time != -1)
		env->end_time = end_time;
}

void add_in_head(struct Env *env, int end_time)
{
	if (!env)
		return;

	if (present_in_queues(env))
	{
		remove_from_queue(env);
	}

	env->next_sched_queue = queues[env->priority];
	queues[env->priority] = env;
	if (end_time != -1)
		env->end_time = end_time;
}

void remove_from_queue(struct Env *env)
{
	if (!env)
		return;

	// if (!queues[env->priority])
	// 	return;

	if (!present_in_queues(env))
		return;

	// struct Env **cur = &(queues[env->priority]);
	// while ((**cur).next_sched_queue)
	// {
	// 	if ((**cur).next_sched_queue->env_id == env->env_id)
	// 	{
	// 		(**cur).next_sched_queue = (**cur).next_sched_queue->next_sched_queue;
	// 		return;
	// 	}
	// 	else
	// 	{
	// 		*cur = (**cur).next_sched_queue;
	// 	}
	// }

	struct Env *cur = queues[env->priority];
	if (cur->env_id == env->env_id)
	{
		queues[env->priority] = cur->next_sched_queue;
		cur->next_sched_queue = NULL;
		return;
	}
	while (cur)
	{
		if (cur->next_sched_queue->env_id == env->env_id)
		{
			cur->next_sched_queue = cur->next_sched_queue->next_sched_queue;
			// cur->next_sched_queue->next_sched_queue = NULL;
			env->next_sched_queue = NULL;
			return;
		}
		else
		{
			cur = cur->next_sched_queue;
		}
	}

}

void find_and_run(void)
{
	// cprintf("find and run\n");
	// int pr;
	// for (pr = MAX_PRIORITY-1; pr >= MIN_PRIORITY; pr--)
	// {
	// 	struct Env *tmp;
	// 	while (queues[pr])
	// 	{
	// 		tmp = queues[pr];
	// 		queues[pr] = tmp->next_sched_queue;

	// 		if (tmp->env_status == ENV_RUNNABLE)
	// 		{
	// 			// if (tmp->end_time == 0 && curenv->sched_policy == SCHED_RR)
	// 			// {
	// 			// 	tmp->end_time = TIME_QUANT;	
	// 			// }
	// 			// tmp->end_time += gettime();
	// 			// tmp->next_sched_queue = NULL;
	// 			if (!curenv)
	// 				curenv = tmp;
	// 			env_run(tmp);
	// 		}
	// 		else
	// 		{
	// 			// tmp->next_sched_queue = NULL;
	// 		}
	// 	}
	// }

	// cprintf("no env to run\n");


	int pr;
	for (pr = MAX_PRIORITY-1; pr >= MIN_PRIORITY; pr--)
	{
		struct Env *tmp = queues[pr];
		while (tmp)
		{
			// tmp = queues[pr];
			// queues[pr] = tmp->next_sched_queue;

			if (tmp->env_status == ENV_RUNNABLE)
			{
				// if (tmp->end_time == 0 && curenv->sched_policy == SCHED_RR)
				// {
				// 	tmp->end_time = TIME_QUANT;	
				// }
				// tmp->end_time += gettime();
				// tmp->next_sched_queue = NULL;
				if (!curenv)
					curenv = tmp;
				env_run(tmp);
			}
			else
			{
				tmp = tmp->next_sched_queue;
			}
		}
	}
}

int present_in_queues(struct Env *env)
{

	// struct Env **cur = &(queues[env->priority]);
	// while (*cur)
	// {
	// 	if ((**cur).env_id == env->env_id)
	// 		return 1;
	// 	cur = &((**cur).next_sched_queue);
	// }
	// return 0;
	struct Env *cur = queues[env->priority];
	while (cur)
	{
		if (cur->env_id == env->env_id)
			return 1;
		cur = cur->next_sched_queue;
	}
	return 0;
}

void sched_yield_by_time(void)
{
	cprintf("sched_yield_by_time\n");
	add_in_head(curenv, -1);
	sched_yield();
}

void chech_priorities(void)
{
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
				
			}
			else
			{
				// tmp->next_sched_queue = NULL;
			}
		}
	}
}

void sched_yield(void)
{
	// cprintf("sched_yield\n");

	// if (curenv && !present_in_queues(curenv))
	// 	add_in_tail(curenv, 0);


	if (curenv)
	{

		if (curenv->sched_policy == SCHED_FIFO &&
			(curenv->env_status == ENV_RUNNING || curenv->env_status == ENV_RUNNABLE))
		{
			env_run(curenv);
		}

		add_in_head(curenv, 0);

		if (curenv->env_status == ENV_RUNNING)
			curenv->env_status = ENV_RUNNABLE;

		int time_left = curenv->end_time - gettime();
		if (time_left <= 0 && curenv->sched_policy == SCHED_RR)
		{
			add_in_tail(curenv, gettime + TIME_QUANT);
		}
		else
		{
			// add_in_head(curenv, gettime() + time_left);
		}

		find_and_run();
	}

	size_t i;
	for (i = 0; i < NENV; i++)
	{
		// if ((envs[i].waiting_for_children == 0) &&
		// 	envs[i].env_status == ENV_NOT_RUNNABLE &&
		// 	envs[i].is_pthread == 0)
		// {
		// 	env_free(&envs[i]);
		// }

		if (envs[i].env_status != ENV_FREE &&
			!present_in_queues(&envs[i]))
		{
			add_in_tail(&envs[i], 0);
		}
	}

	// 	size_t i;
	// 	for (i = 0; i < NENV; i++)
	// 	{
	// 		if ((envs[i].waiting_for_children == 0) &&
	// 			envs[i].env_status == ENV_NOT_RUNNABLE &&
	// 			envs[i].is_pthread == 0)
	// 		{
	// 			env_free(&envs[i]);
	// 		}
	// 	}

	find_and_run();

	sched_halt();
	for(;;)
		;
}


// Choose a user environment to run and run it.
// void
// sched_yield(void)
// {
// 	cprintf("sched_yield\n");
// 	// Implement simple round-robin scheduling.
// 	//
// 	// Search through 'envs' for an ENV_RUNNABLE environment in
// 	// circular fashion starting just after the env was
// 	// last running.  Switch to the first such environment found.
// 	//
// 	// If no envs are runnable, but the environment previously
// 	// running is still ENV_RUNNING, it's okay to
// 	// choose that environment.
// 	//
// 	// If there are no runnable environments,
// 	// simply drop through to the code
// 	// below to halt the cpu.

// 	//LAB 3: Your code here.
// 	struct Env *e;
// 	if (curenv == NULL)
// 		curenv = &envs[NENV - 1];
// 	e = &envs[(curenv - envs + 1) % NENV];
// 	while ((e != curenv) && (e->env_status != ENV_RUNNABLE))
// 		e = &envs[(e - envs + 1) % NENV];

// 	size_t i;
// 	for (i = 0; i < NENV; i++)
// 	{
// 		if ((envs[i].waiting_for_children == 0) &&
// 			envs[i].env_status == ENV_NOT_RUNNABLE &&
// 			envs[i].is_pthread == 0)
// 		{
// 			env_free(&envs[i]);
// 		}
// 	}

// 	if ((e->env_status == ENV_RUNNABLE) || (e->env_status == ENV_RUNNING))
// 		env_run(e);
	
// 	// sched_halt never returns
// 	sched_halt();
// }

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.

void
sched_halt(void)
{
	cprintf("halt\n");

	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		// cprintf("%d, %08x\n", i, envs[i].env_id);
		// if (envs[i].env_status == ENV_RUNNABLE)
		// 	cprintf("runnable\n");
		// if (envs[i].env_status == ENV_RUNNING)
		// 	cprintf("running\n");
		// if (envs[i].env_status == ENV_DYING)
		// 	cprintf("dying\n");
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
		{
			// cprintf("present in queue: %d\n", present_in_queues(&envs[i]) ? 1 : 0);
			break;
			
		}
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	cprintf("not all done\n");

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

