/*
 * Copyright (C) 2007-2008 Michal Simek <monstr@monstr.eu>
 * Copyright (C) 2006 Atmark Techno, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/sched.h>
#include <asm/page.h>
#include <asm/io.h>
#include <asm/setup.h>

void heartbeat(void)
{
	static unsigned int cnt, period, dist;

	if (cnt == 0 || cnt == dist)
		iowrite32(1, CONFIG_HEART_BEAT_ADDRESS);
	else if (cnt == 7 || cnt == dist + 7)
		iowrite32(0, CONFIG_HEART_BEAT_ADDRESS);

	if (++cnt > period) {
		cnt = 0;
		/*
		 * The hyperbolic function below modifies the heartbeat period
		 * length in dependency of the current (5min) load. It goes
		 * through the points f(0)=126, f(1)=86, f(5)=51, f(inf)->30.
		 */
		period = ((672 << FSHIFT) / (5 * avenrun[0] +
					(7 << FSHIFT))) + 30;
		dist = period / 4;
	}
}
