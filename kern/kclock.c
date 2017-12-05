/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <kern/kclock.h>
#include <inc/time.h>
#include <inc/vsyscall.h>
#include <kern/vsyscall.h>

int read_time(void)
{
	struct tm time;

	int sec = BCD2BIN(mc146818_read(RTC_SEC));
	int min = BCD2BIN(mc146818_read(RTC_MIN));
	int hour = BCD2BIN(mc146818_read(RTC_HOUR));

	int day = BCD2BIN(mc146818_read(RTC_DAY));
	int mon = BCD2BIN(mc146818_read(RTC_MON)) - 1;
	int year = BCD2BIN(mc146818_read(RTC_YEAR));

	time.tm_sec = sec;
	time.tm_min = min;
	time.tm_hour = hour;
	time.tm_mday = day;
	time.tm_mon = mon;
	time.tm_year = year;

	int t = timestamp(&time);
	return t;
}

int gettime(void)
{
	nmi_disable();
	// LAB 12: your code here
	while (mc146818_read(RTC_UPDATE_IN_PROGRESS) == 1) 
		;

	int t1 = read_time();
	int t2 = read_time();

	while (t1 != t2)
	{
		t1 = read_time();
		t2 = read_time();
	}
	
	vsys[VSYS_gettime] = t1;


	nmi_enable();
	return t1;
}

void
rtc_init(void)
{
	nmi_disable();
	// LAB 4: your code here

	outb(IO_RTC_CMND, RTC_BREG);
	uint8_t regB = inb(IO_RTC_DATA);
	regB = regB | RTC_PIE;
	outb(IO_RTC_CMND, RTC_BREG);
	outb(IO_RTC_DATA, regB);
	
	outb(IO_RTC_CMND, RTC_AREG);
	uint8_t regA = inb(IO_RTC_DATA);
	regA = regA & 0xF0;
	regA = regA | 0x0F;
	outb(IO_RTC_CMND, RTC_AREG);
	outb(IO_RTC_DATA, regA);


	nmi_enable();
}

uint8_t
rtc_check_status(void)
{
	uint8_t status = 0;
	// LAB 4: your code here

	outb(IO_RTC_CMND, RTC_CREG);
	status = inb(IO_RTC_DATA);

	return status;
}

unsigned
mc146818_read(unsigned reg)
{
	outb(IO_RTC_CMND, reg);
	return inb(IO_RTC_DATA);
}

void
mc146818_write(unsigned reg, unsigned datum)
{
	outb(IO_RTC_CMND, reg);
	outb(IO_RTC_DATA, datum);
}

