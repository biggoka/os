#include <inc/vsyscall.h>
#include <inc/lib.h>

bool first = true;

static inline int32_t
vsyscall(int num)
{
	// LAB 12: Your code here.

	if (num == VSYS_gettime)
	{
		if (first)
		{
			sys_gettime();
			first = false;
		}
		return vsys[VSYS_gettime];

	}
	else
	{
		panic("vsyscall() is not implemented yet!");

	}

	return 0;
}

int vsys_gettime(void)
{
	return vsyscall(VSYS_gettime);
}