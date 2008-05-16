/*
 * Early printk support for Microblaze.
 *
 * Copyright (C) 2007-2008 Michal Simek <monstr@monstr.eu>
 * Copyright (C) 2003-2006 Yasushi SHOJI <yashi@atmark-techno.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */

/* FIXME Once we got some system without uart light, we need to refactor */

#include <linux/console.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/tty.h>
#include <asm/io.h>
#include <asm/processor.h>
#include <asm/fcntl.h>
#include <asm/setup.h>

#ifdef CONFIG_EARLY_PRINTK
#define BASE_ADDR ((unsigned char *)CONFIG_EARLY_PRINTK_UARTLITE_ADDRESS)

#define RX_FIFO	BASE_ADDR
#define TX_FIFO	((unsigned long *)(BASE_ADDR + 4))
#define STATUS	((unsigned long *)(BASE_ADDR + 8))
#define CONTROL	((unsigned long *)(BASE_ADDR + 12))

static void early_printk_putc(char c)
{
	/*
	 * Limit how many times we'll spin waiting for TX FIFO status.
	 * This will prevent lockups if the base address is incorrectly
	 * set, or any other issue on the UARTLITE.
	 * This limit is pretty arbitrary, unless we are at about 10 baud
	 * we'll never timeout on a working UART. 
	 */

	unsigned retries = 10000;
	while (retries-- && (ioread32(STATUS) & (1<<3)))
		;

	/* Only attempt the iowrite if we didn't timeout */
	if(retries)
		iowrite32((c & 0xff), TX_FIFO);
}

static void early_printk_write(struct console *unused,
					const char *s, unsigned n)
{
	while (*s && n-- > 0) {
		early_printk_putc(*s);
		if (*s == '\n')
			early_printk_putc('\r');
		s++;
	}
}

static struct console early_serial_console = {
	.name = "earlyser",
	.write = early_printk_write,
	.flags = CON_PRINTBUFFER,
	.index = -1,
};

/* Direct interface for emergencies */
static struct console *early_console = &early_serial_console;
static int early_console_initialized;

void early_printk(const char *fmt, ...)
{
	char buf[512];
	int n;
	va_list ap;

	if (early_console_initialized) {
		va_start(ap, fmt);
		n = vscnprintf(buf, 512, fmt, ap);
		early_console->write(early_console, buf, n);
		va_end(ap);
	}
}

static int __initdata keep_early;

int __init setup_early_printk(char *opt)
{
	char *space;
	char buf[256];

	if (early_console_initialized)
		return 1;

	strlcpy(buf, opt, sizeof(buf));
	space = strchr(buf, ' ');
	if (space)
		*space = 0;

	if (strstr(buf, "keep"))
		keep_early = 1;

	early_console = &early_serial_console;
	early_console_initialized = 1;
	register_console(early_console);
	return 0;
}
#if 0
static void __init disable_early_printk(void)
{
	if (!early_console_initialized || !early_console)
		return;
	if (!keep_early) {
		printk(KERN_INFO "disabling early console\n");
		unregister_console(early_console);
		early_console_initialized = 0;
	} else
		printk(KERN_INFO "keeping early console\n");
}
#endif

__setup("earlyprintk=", setup_early_printk);

#else
void early_printk(const char *fmt, ...)
{
}
#endif
