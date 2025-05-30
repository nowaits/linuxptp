/**
 * @file clockadj.c
 * @note Copyright (C) 2013 Richard Cochran <richardcochran@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "clock.h"
#include "clockadj.h"
#include "missing.h"
#include "print.h"
#if USE_KTIME
#include <ktime.h>
#define ADJTIMEX(a) ktime_adjtimex(a)
#else
#define ADJTIMEX(a) adjtimex(a)
#endif

#define NS_PER_SEC 1000000000LL

static int realtime_leap_bit;
static long realtime_hz;
static long realtime_nominal_tick;

static int do_clock_adjtime(clockid_t clkid, struct timex* tx)
{
	if (clkid != CLOCK_REALTIME) {
		return clock_adjtime(clkid, tx);
	}

	return ADJTIMEX(tx);
}


void clockadj_init(clockid_t clkid)
{
#if USE_KTIME
	realtime_hz = USER_HZ;
	realtime_nominal_tick = (USEC_PER_SEC + USER_HZ / 2) / USER_HZ;
	return;
#endif
#ifdef _SC_CLK_TCK
	if (clkid == CLOCK_REALTIME) {
		/* This is USER_HZ in the kernel. */
		realtime_hz = sysconf(_SC_CLK_TCK);
		if (realtime_hz > 0) {
			/* This is TICK_USEC in the kernel. */
			realtime_nominal_tick =
				(1000000 + realtime_hz / 2) / realtime_hz;
		}
	}
#endif
}

int clockadj_set_freq(clockid_t clkid, double freq)
{
	struct timex tx;
	memset(&tx, 0, sizeof(tx));

	/* With system clock set also the tick length. */
	if (clkid == CLOCK_REALTIME && realtime_nominal_tick) {
		tx.modes |= ADJ_TICK;
		tx.tick = round(freq / 1e3 / realtime_hz) + realtime_nominal_tick;
		freq -= 1e3 * realtime_hz * (tx.tick - realtime_nominal_tick);
	}

	tx.modes |= ADJ_FREQUENCY;
	tx.freq = (long) (freq * 65.536);
	if (do_clock_adjtime(clkid, &tx) < 0) {
		pr_err("failed to adjust the clock: %m");
		return -1;
	}
	return 0;
}

double clockadj_get_freq(clockid_t clkid)
{
	double f = 0.0;
	struct timex tx = {0};

	if (do_clock_adjtime(clkid, &tx) < 0) {
		pr_err("failed to read out the clock frequency adjustment: %m");
		exit(1);
	} else {
		f = tx.freq / 65.536;
		if (clkid == CLOCK_REALTIME && realtime_nominal_tick && tx.tick)
			f += 1e3 * realtime_hz * (tx.tick - realtime_nominal_tick);
	}
	return f;
}

int clockadj_set_phase(clockid_t clkid, long offset)
{
	struct timex tx;
	memset(&tx, 0, sizeof(tx));

	tx.modes = ADJ_OFFSET | ADJ_NANO;
	tx.offset = offset;
	if (do_clock_adjtime(clkid, &tx) < 0) {
		pr_err("failed to set the clock offset: %m");
		return -1;
	}
	return 0;
}

int clockadj_step(clockid_t clkid, int64_t step)
{
	struct timex tx;
	int sign = 1;
	if (step < 0) {
		sign = -1;
		step *= -1;
	}
	memset(&tx, 0, sizeof(tx));
	tx.modes = ADJ_SETOFFSET | ADJ_NANO;
	tx.time.tv_sec  = sign * (step / NS_PER_SEC);
	tx.time.tv_usec = sign * (step % NS_PER_SEC);
	/*
	 * The value of a timeval is the sum of its fields, but the
	 * field tv_usec must always be non-negative.
	 */
	if (tx.time.tv_usec < 0) {
		tx.time.tv_sec  -= 1;
		tx.time.tv_usec += 1000000000;
	}
	if (do_clock_adjtime(clkid, &tx) < 0) {
		pr_err("failed to step clock: %m");
		return -1;
	}

	return 0;
}

int clockadj_max_freq(clockid_t clkid)
{
	int f = 0;
	struct timex tx;

	memset(&tx, 0, sizeof(tx));
	if (do_clock_adjtime(clkid, &tx) < 0)
		pr_err("failed to read out the clock maximum adjustment: %m");
	else
		f = tx.tolerance / 65.536;
	if (!f)
		f = 500000;

	/* The kernel allows the tick length to be adjusted up to 10%. But use
	 * it only if the overall frequency of the clock can be adjusted
	 * continuously with the tick and freq fields (i.e. hz <= 1000).
	 */
	if (clkid == CLOCK_REALTIME && (realtime_nominal_tick && 2 * f >=
					1000 * realtime_hz))
		f = realtime_nominal_tick / 10 * 1000 * realtime_hz;

	return f;
}

int clockadj_compare(clockid_t clkid, clockid_t sysclk, int readings,
		     int64_t *offset, uint64_t *ts, int64_t *delay)
{
	struct timespec tdst1, tdst2, tsrc;
	int i;
	int64_t interval, best_interval = INT64_MAX;

	/* Pick the quickest clkid reading. */
	for (i = 0; i < readings; i++) {
		if (do_clock_gettime(sysclk, &tdst1) ||
			do_clock_gettime(clkid, &tsrc) ||
			do_clock_gettime(sysclk, &tdst2)) {
			pr_err("failed to read clock: %m");
			return -errno;
		}

		interval = (tdst2.tv_sec - tdst1.tv_sec) * NS_PER_SEC +
			tdst2.tv_nsec - tdst1.tv_nsec;

		if (best_interval > interval) {
			best_interval = interval;
			*offset = (tdst1.tv_sec - tsrc.tv_sec) * NS_PER_SEC +
				tdst1.tv_nsec - tsrc.tv_nsec + interval / 2;
			*ts = tdst2.tv_sec * NS_PER_SEC + tdst2.tv_nsec;
		}
	}
	*delay = best_interval;

	return 0;
}

void sysclk_set_leap(int leap)
{
	clockid_t clkid = CLOCK_REALTIME;
	struct timex tx;
	const char *m = NULL;
	memset(&tx, 0, sizeof(tx));
	tx.modes = ADJ_STATUS;
	switch (leap) {
	case -1:
		tx.status = STA_DEL;
		m = "clock set to delete leap second at midnight (UTC)";
		break;
	case 1:
		tx.status = STA_INS;
		m = "clock set to insert leap second at midnight (UTC)";
		break;
	default:
		tx.status = 0;
	}
	if (do_clock_adjtime(clkid, &tx) < 0)
		pr_err("failed to set the clock status: %m");
	else if (m)
		pr_notice("%s", m);
	realtime_leap_bit = tx.status;
}

void sysclk_set_tai_offset(int offset)
{
	clockid_t clkid = CLOCK_REALTIME;
	struct timex tx;
	memset(&tx, 0, sizeof(tx));
	tx.modes = ADJ_TAI;
	tx.constant = offset;
	if (do_clock_adjtime(clkid, &tx) < 0)
		pr_err("failed to set TAI offset: %m");
}

int sysclk_max_freq(void)
{
	return clockadj_max_freq(CLOCK_REALTIME);
}

void sysclk_set_sync(void)
{
	clockid_t clkid = CLOCK_REALTIME;
	struct timex tx;
	memset(&tx, 0, sizeof(tx));
	/* Clear the STA_UNSYNC flag from the status and keep the maxerror
	   value (which is increased automatically by 500 ppm) below 16 seconds
	   to avoid getting the STA_UNSYNC flag back. */
	tx.modes = ADJ_STATUS | ADJ_MAXERROR;
	tx.status = realtime_leap_bit;
	if (do_clock_adjtime(clkid, &tx) < 0)
		pr_err("failed to set clock status and maximum error: %m");
}
