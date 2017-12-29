/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/sched.h>
#include <kern/kclock.h>

#include <inc/pthread.h>

static struct Env *join_wait_list = NULL;

void remove_from_wait_queue(pthread id)
{
	struct Env **cur;
	cur = &join_wait_list;
	while (*cur)
	{
		if ((**cur).env_id == id)
		{
			struct Env *tmp = *cur;
			(*(**cur).putres) = NULL;
			*cur = (**cur).next_waiting_join;
			tmp->next_waiting_join = NULL;
		}
		else
		{
			cur = &((**cur).next_waiting_join);
		}
	}
}

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.

	// LAB 8: Your code here.
	user_mem_assert(curenv, s, len, PTE_U);

	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//  -E_BAD_ENV if environment envid doesn't currently exist,
//    or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	if (e == curenv)
		cprintf("[%08x] exiting gracefully\n", curenv->env_id);
	else
		cprintf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
	sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//  -E_NO_FREE_ENV if no free environment is available.
//  -E_NO_MEM on memory exhaustion.
static envid_t
sys_exofork(void)
{
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.


	// LAB 9: Your code here.
	struct Env *child;

	if (env_alloc(&child, curenv->env_id, 0, NULL) < 0)
		return -E_NO_FREE_ENV;

	child->env_status = ENV_NOT_RUNNABLE;
	child->env_tf = curenv->env_tf;
	// install the pgfault upcall to the child
	child->env_pgfault_upcall = curenv->env_pgfault_upcall;
	// tweak the register eax of the child,
	// thus, the child will look like the return value
	// of the the system call is zero.
	child->env_tf.tf_regs.reg_eax = 0;
	// but notice that the return value of the parent
	// is the env id of the child
	return child->env_id;
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//  -E_BAD_ENV if environment envid doesn't currently exist,
//    or the caller doesn't have permission to change envid.
//  -E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
	// Hint: Use the 'envid2env' function from kern/env.c to translate an
	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.

	// LAB 9: Your code here.
	int r;
	struct Env *task;

	if ((r = envid2env(envid, &task, 1)) < 0)
		return -E_BAD_ENV;

	if (status != ENV_FREE &&
		status != ENV_RUNNABLE &&
		status != ENV_NOT_RUNNABLE)
		return -E_INVAL;

	task->env_status = status;

	return 0;
}

// Set envid's trap frame to 'tf'.
// tf is modified to make sure that user environments always run at code
// protection level 3 (CPL 3) with interrupts enabled.
//
// Returns 0 on success, < 0 on error.  Errors are:
//  -E_BAD_ENV if environment envid doesn't currently exist,
//    or the caller doesn't have permission to change envid.
static int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
	// LAB 11: Your code here.
	// Remember to check whether the user has supplied us with a good
	// address!
	struct Env *e; 
	int ret = envid2env(envid, &e, 1);
	if (ret) return ret;
	user_mem_assert(e, tf, sizeof(struct Trapframe), PTE_U);
	e->env_tf = *tf;
	e->env_tf.tf_eflags |= FL_IF;
	e->env_tf.tf_cs = GD_UT | 3;
	e->env_tf.tf_es = GD_UD | 3;
	e->env_tf.tf_ds = GD_UD | 3;
	e->env_tf.tf_ss = GD_UD | 3;


	return 0;
}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//  -E_BAD_ENV if environment envid doesn't currently exist,
//    or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
	// LAB 9: Your code here.
	//panic("sys_env_set_pgfault_upcall not implemented");

	struct Env *task;

	if (envid2env(envid, &task, 1) < 0)
		return -E_BAD_ENV;

	task->env_pgfault_upcall = func;

	return 0;
}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_SYSCALL in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//  -E_BAD_ENV if environment envid doesn't currently exist,
//    or the caller doesn't have permission to change envid.
//  -E_INVAL if va >= UTOP, or va is not page-aligned.
//  -E_INVAL if perm is inappropriate (see above).
//  -E_NO_MEM if there's no memory to allocate the new page,
//    or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	// LAB 9: Your code here.
	int err;
	if (((~PTE_SYSCALL) & perm) ||  //-E_INVAL if va >= UTOP
		((uintptr_t)va >= UTOP) ||  //va is not page-aligned
		((uintptr_t)va % PGSIZE)) //-E_INVAL if perm is inappropriate
	{
		err = -E_INVAL;
		return err;
	}

	struct Env *new_proc = NULL;
	err = envid2env(envid, &new_proc, 1);
	if (err) 
	{
		err = -E_BAD_ENV;
		return err;
	}

	struct PageInfo *new_page = NULL;
	new_page = page_alloc(ALLOC_ZERO);
	if (new_page == NULL) 
	{
		err = -E_NO_MEM;
		return err;
	}

	err = page_insert(new_proc->env_pgdir, new_page, va, perm);
	if (err != 0)
	{
		page_free(new_page);
		err = -E_NO_MEM;
		return err;
	}
	return 0;
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//  -E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//    or the caller doesn't have permission to change one of them.
//  -E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//    or dstva >= UTOP or dstva is not page-aligned.
//  -E_INVAL is srcva is not mapped in srcenvid's address space.
//  -E_INVAL if perm is inappropriate (see sys_page_alloc).
//  -E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//    address space.
//  -E_NO_MEM if there's no memory to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva,
		 envid_t dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.

	// LAB 9: Your code here.
	int err;
	if (((~PTE_SYSCALL) & perm)    || 
		((uintptr_t)srcva >= UTOP) || 
		((uintptr_t)srcva % PGSIZE)||
		((uintptr_t)dstva >= UTOP) || 
		((uintptr_t)dstva % PGSIZE))  
	{
		err = -E_INVAL;
		return err;
	}
	
	struct Env *src_env = NULL, *dst_env = NULL;
	err = envid2env(srcenvid, &src_env, 1);
	if (err != 0)
	{
		err = -E_BAD_ENV;
		return err;
	}
	err = envid2env(dstenvid, &dst_env, 1);
	if (err != 0)
	{
		err = -E_BAD_ENV;
		return err;
	}

	struct PageInfo *new_page = NULL;
	pte_t *pte_store;
	new_page = page_lookup(src_env->env_pgdir, srcva, &pte_store);
	if (new_page == NULL) 
	{
		err = -E_INVAL;
		return err;
	}

	if ((*pte_store & PTE_W) == 0)
	{
		if (perm & PTE_W)
		{
			err = -E_INVAL;
			return err;

		}
	}

	err = page_insert(dst_env->env_pgdir, new_page, dstva, perm);
	if (err != 0)
	{
		err = -E_NO_MEM;
		return err;
	}

	return 0;
}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//  -E_BAD_ENV if environment envid doesn't currently exist,
//    or the caller doesn't have permission to change envid.
//  -E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().

	// LAB 9: Your code here.
	struct Env *task;

	if (envid2env(envid, &task, 1) < 0)
		return -E_BAD_ENV;

	if ((unsigned int)va >= UTOP || va != ROUNDDOWN(va, PGSIZE))
		return -E_INVAL;

	page_remove(task->env_pgdir, va);

	return 0;
}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//  -E_BAD_ENV if environment envid doesn't currently exist.
//    (No need to check permissions.)
//  -E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//    or another environment managed to send first.
//  -E_INVAL if srcva < UTOP but srcva is not page-aligned.
//  -E_INVAL if srcva < UTOP and perm is inappropriate
//    (see sys_page_alloc).
//  -E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//    address space.
//  -E_INVAL if (perm & PTE_W), but srcva is read-only in the
//    current environment's address space.
//  -E_NO_MEM if there's not enough memory to map srcva in envid's
//    address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	// LAB 9: Your code here.
	struct Env *env;
	int err;
	err = envid2env(envid, &env, 0);
	if (err) 
	{ 
		return -E_BAD_ENV;
	}
	if (!env->env_ipc_recving) 
	{
		return -E_IPC_NOT_RECV;
	}
	if ((uintptr_t)srcva < UTOP) 
	{
		if ((uintptr_t)srcva % PGSIZE) 
		{
				return -E_INVAL;
		}
		//  -E_INVAL if srcva < UTOP and perm is inappropriate
		if ((perm & ~PTE_SYSCALL)) 
		{
			return -E_INVAL;
		}
		
		pte_t *pte;
		struct PageInfo *page = page_lookup(curenv->env_pgdir, srcva, &pte);
		//  -E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
		//    address space.
		if (!page) 
		{
			return -E_INVAL;
		}
		//  -E_INVAL if (perm & PTE_W), but srcva is read-only in the
		//    current environment's address space.
		if ((perm & PTE_W) && !(*pte & PTE_W))
		{
			return -E_INVAL;
		}
		if ((uintptr_t)env->env_ipc_dstva < UTOP && 
			page_insert(env->env_pgdir, page, env->env_ipc_dstva, perm)) 
		{
			return -E_NO_MEM;
		}
	//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
		env->env_ipc_perm = perm;
	}
	else 
	{
		env->env_ipc_perm = 0;
	}
	
	// Otherwise, the send succeeds, and the target's ipc fields are
	// updated as follows:
	//    env_ipc_recving is set to 0 to block future sends;
	//    env_ipc_from is set to the sending envid;
	//    env_ipc_value is set to the 'value' parameter;
	env->env_ipc_recving = 0;
	env->env_ipc_from = curenv->env_id;
	env->env_ipc_value = value;
	env->env_status = ENV_RUNNABLE;
	return 0;
}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//  -E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
	// LAB 9: Your code here.
	if ((uintptr_t)dstva < UTOP) 
	{
		if ((uintptr_t)dstva % PGSIZE)  
		{
			return -E_INVAL;
		}
		curenv->env_ipc_dstva = dstva;
	}
	else 
	{
		curenv->env_ipc_dstva = (void *)UTOP;
	}
	
	curenv->env_ipc_recving = 1;
	curenv->env_status = ENV_NOT_RUNNABLE;
	curenv->env_tf.tf_regs.reg_eax = 0;
	sys_yield();
	return 0;
}

// Return date and time in UNIX timestamp format: seconds passed
// from 1970-01-01 00:00:00 UTC.
static int
sys_gettime(void)
{
	// LAB 12: Your code here.
	return gettime();
}

//threads

static int
sys_pthread_exit(void *res)
{
	cprintf("sys_pthread_exit\n");
	curenv->res = res;
	if (res)
		// cprintf("res of exit is %d\n", *(int*)res);

	if (curenv->pthread_type != JOINABLE) 
	{
		env_free(curenv);
		sys_yield();
		return 0;
	}

	struct Env **cur;
	int was_found = 0;
	cur = &join_wait_list;
	while (*cur) {
		if ((**cur).wait_for == curenv->env_id) {
			struct Env *tmp = *cur;
			(*(**cur).putres) = res;
			// cprintf("putres = %d\n", *(int *)res);
			(**cur).env_status = ENV_RUNNABLE;
			*cur = (**cur).next_waiting_join;
			tmp->next_waiting_join = NULL;
			// delete_from_queue(tmp);
			// add_in_tail(tmp, 0);
			was_found = 1;
		} else {
			cur = &((**cur).next_waiting_join);
		}
	}
	if (was_found) {
		env_free(curenv);
		env_free(curenv);
		sys_yield();
		return 0;
	} else {
		env_free(curenv);
		sys_yield();
		return 0;
	}
	
}

static int
sys_pthread_create(uint32_t exit_adress, pthread *thread, const struct pthread_params *attr, void *(*start_routine)(void*), uint32_t arg)
{
	cprintf("pthread_create\n");

	if (attr)
	{
		if (!((attr->pthread_type == PTHREAD_CREATE_JOINABLE) || (attr->pthread_type == PTHREAD_CREATE_DETACHED)))
			return -1;
		// if (!((attr->priority >= MIN_PRIORITY) && (attr->priority <= MAX_PRIORITY)))
		// 	return -1;
		// if (!((attr->sched_policy == SCHED_RR) || (attr->sched_policy == SCHED_FIFO)))
		// 	return -1;
	}

	struct Env *newenv;

	env_alloc(&newenv, curenv->env_id, 1, curenv);

	newenv->env_tf.tf_eip = (uintptr_t) start_routine;

	if (!attr)
	{
		newenv->priority = 1;
		newenv->sched_policy = SCHED_RR;
		newenv->pthread_type = JOINABLE;
	}
	else 
	{
		newenv->priority = attr->priority;
		newenv->sched_policy = attr->sched_policy;

		if (attr->pthread_type == PTHREAD_CREATE_JOINABLE)
		{
			newenv->pthread_type = JOINABLE;
		}
		else if (attr->pthread_type == PTHREAD_CREATE_DETACHED)
		{
			newenv->pthread_type = DETACHED;
		}
	}

	cprintf("%u\n", newenv->env_id);
	(*thread) = newenv->env_id;
	uint32_t *curframe;

	// cprintf("ustacktop %u\n", USTACKTOP);
	// cprintf("esp %u\n", (uint32_t*)newenv->env_tf.tf_esp);
	curframe = (uint32_t*)newenv->env_tf.tf_esp - 4;
	curframe[0] = exit_adress;
	// cprintf("pthread create yield\n");
	curframe[1] = arg;
	curframe[2] = 0;
	curframe[3] = 1;
	newenv->env_tf.tf_esp = (uintptr_t)((uint32_t*)(newenv->env_tf.tf_esp) - 4);
	newenv->env_status = ENV_RUNNABLE;

	remove_from_queue(newenv);
	add_in_tail(newenv, 0);


	sched_yield();
	return 0;
}
static int
sys_pthread_join(pthread pthread, void ** res_ptr)
{
	cprintf("pthread_join\n");

	size_t i;
	struct Env *t = NULL;
	for (i = 0; i < NENV; i++)
	{
		// if (envs[i].env_id == pthread &&
		// 	envs[i].env_status != ENV_FREE &&
		// 	envs[i].is_pthread &&
		// 	envs[i].parent_env == curenv->parent_env &&
		// 	envs[i].pthread_type == DETACHED)
		// {
		// 	t = &(envs[i]);
		// }
		if (envs[i].env_id == pthread)
		{
			t = &(envs[i]);
		}
	}


	if (!t)
		return -1;
	// cprintf("i am here !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1\n");

	if (t->pthread_type = DETACHED)
		return -1;

	if (t->pthread_type == JOINABLE_FINISHED)
	{
		*res_ptr = t->res;
		env_free(t);
		return 0;
	}
	else
	{
		if (join_wait_list == NULL)
		{
			join_wait_list = curenv;
		}
		else
		{
			struct Env *tmp = join_wait_list;
			join_wait_list = curenv;
			curenv->next_waiting_join = tmp;
		}

		curenv->env_status = ENV_NOT_RUNNABLE;
		curenv->wait_for = t->env_id;
		curenv->putres = res_ptr;
		sys_yield();
	}

	return 0;
}
static int
sys_sched_setparam(pthread pthread, struct pthread_params *params)
{
	cprintf("sys_sched_setparam\n");
	size_t i;
	for (i = 0; i < NENV; i++)
	{
		if ((envs[i].env_id == pthread) && (envs[i].env_status != ENV_FREE))
		{
			envs[i].pthread_type = params->pthread_type;
			envs[i].priority = params->priority;
			envs[i].sched_policy = params->sched_policy;
			return 0;
		}
	}
	return 0;
}
static int
sys_sched_setscheduler(pthread pthread, int policy)
{
	cprintf("sys_sched_setscheduler\n");
	size_t i;
	for (i = 0; i < NENV; i++)
	{
		if ((envs[i].env_id == pthread) && (envs[i].env_status != ENV_FREE))
		{
			envs[i].sched_policy = policy;
			return 0;
		}
	}
	return 0;
}
static int
sys_print_pthread_info(pthread pthread)
{
	if (pthread == -1)
		pthread = curenv->env_id;

	size_t i;
	for (i = 0; i < NENV; i++)
	{
		if (envs[i].env_id == pthread)
		{
			cprintf("\nPrinting info on %s %08x\n",
				envs[i].is_pthread ? "pthread" : "process", envs[i].env_id);
			if (envs[i].is_pthread)
			{
				cprintf("Parent id is %08x\n", envs[i].parent_env->env_id);
				cprintf("Thread type is: ");

				if (envs[i].pthread_type == JOINABLE)
				{
					cprintf("JOINBABLE\n");
				}
				else if (envs[i].pthread_type == DETACHED)
				{
					cprintf("DETACHED\n");
				}
				else if (envs[i].pthread_type == JOINABLE_FINISHED)
				{
					cprintf("JOINABLE_FINISHED\n");
				}
			}
			else
			{
				cprintf("This process created %d threads\n", envs[i].pthreads_created);
			}

			cprintf("Priority is %d; Sched policy is: %s\n\n",
				envs[i].priority, envs[i].sched_policy == SCHED_RR ? "RR" : "FIFO");


			return 0;
		}
	}

	cprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!env not found%d!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", pthread);
	return 0;
}

// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 8: Your code here.

	int ret = 0;

	switch (syscallno) {
	case SYS_cputs:
		sys_cputs((const char *)a1, (size_t)a2);
		break;
	case SYS_cgetc:
		ret = sys_cgetc();
		break;
	case SYS_getenvid:
		ret = sys_getenvid();
		break;
	case SYS_env_destroy:
		ret = sys_env_destroy((envid_t)a1);
		break;
	case SYS_page_alloc:
		ret = sys_page_alloc((envid_t)a1, (void *)a2, (int)a3);
		break;
	case SYS_page_map:
		ret = sys_page_map((envid_t)a1, (void *)a2,
							 (envid_t)a3, (void *)a4, (int)a5);
		break;
	case SYS_page_unmap:
		ret = sys_page_unmap((envid_t)a1, (void *)a2);
		break;
	case SYS_exofork:
		ret = sys_exofork();
		break;
	case SYS_env_set_status:
		ret = sys_env_set_status((envid_t)a1, (int)a2);
		break;
	case SYS_env_set_trapframe:
		ret = sys_env_set_trapframe((envid_t)a1, (struct Trapframe *)a2);
		break;
	case SYS_env_set_pgfault_upcall:
		ret = sys_env_set_pgfault_upcall((envid_t)a1, (void *)a2);
		break;
	case SYS_yield:
		sys_yield();
		break;
	// case SYS_phy_page:
	//  ret = sys_phy_page((envid_t)a1, (void *)a2);
	//  break;
	case SYS_ipc_try_send:
		ret = sys_ipc_try_send((envid_t)a1, (uint32_t)a2,
								 (void *)a3, (int)a4);
		break;
	case SYS_ipc_recv:
		ret = sys_ipc_recv((void *)a1);
		break;
	case SYS_gettime:
		ret = sys_gettime();
		break;
	case SYS_pthread_create:
		ret = sys_pthread_create(a1, (pthread*)a2, (const struct pthread_params*)a3, (void*(*)(void*)) a4, a5);
		break;
	case SYS_pthread_join:
		ret = sys_pthread_join((pthread)a1, (void **)a2);
		break;
	case SYS_pthread_exit:
		ret = sys_pthread_exit((void*)a1);
		break;
	case SYS_sched_setparam:
		ret = sys_sched_setparam((pthread)a1, (struct pthread_params *)a2);
		break;
	case SYS_sched_setscheduler:
		ret = sys_sched_setscheduler((pthread)a1, (int)a2);
		break;
	case SYS_print_pthread_info:
		ret = sys_print_pthread_info((pthread)a1);
		break;
	default:
		panic("syscall not implemented");
		ret = -E_INVAL;
		break;
	}

	return ret;
}

