/* -*- linux-c -*- ------------------------------------------------------- *
 *
 *   Copyright (C) 1991, 1992 Linus Torvalds
 *   Copyright 2007 rPath, Inc. - All Rights Reserved
 *   Copyright 2009 Intel Corporation; author H. Peter Anvin
 *
 *   This file is part of the Linux kernel, and is made available under
 *   the terms of the GNU General Public License version 2.
 *
 * ----------------------------------------------------------------------- */

/*
 * Very simple screen and serial I/O
 */

#include "boot.h"
#include "string.h"

int early_serial_base;

#define XMTRDY          0x20

#define TXR             0       /*  Transmit register (WRITE) */
#define LSR             5       /*  Line Status               */

/*
 * These functions are in .inittext so they can be used to signal
 * error during initialization.
 */

static int __attribute__((section(".inittext"))) in_protected_mode(void) {
	unsigned int cr0;

	asm volatile("mov %%cr0, %0": "=r" (cr0));
	return X86_CR0_PE & cr0;
}

static void __attribute__((section(".inittext"))) serial_putchar(int ch)
{
	unsigned timeout = 0xffff;

	while ((inb(early_serial_base + LSR) & XMTRDY) == 0 && --timeout)
		cpu_relax();

	outb(ch, early_serial_base + TXR);
}

static void __attribute__((section(".inittext"))) bios_putchar(int ch)
{
	struct biosregs ireg;

	initregs(&ireg);
	ireg.bx = 0x0007;
	ireg.cx = 0x0001;
	ireg.ah = 0x0e;
	ireg.al = ch;
	intcall(0x10, &ireg, NULL);
}

void __attribute__((section(".inittext"))) putchar(int ch)
{
	if (ch == '\n')
		putchar('\r');	/* \n -> \r\n */

	if (!in_protected_mode()) {
		bios_putchar(ch);
	}

	if (early_serial_base != 0)
		serial_putchar(ch);
}

void *memcpy32(void *dest, const void *src, size_t n)
{
	void *dst = dest;

	for (int i = 0; i < n >> 2; ++i, dst += sizeof(int), src += sizeof(int)) {
		*(int*)dst = *(int*)src;
	}

	for (int i = 0; i < (n & 3); ++i) {
		*(char*)dst++ = *(char*)src++;
	}

	return dest; 
}

#define ADDR_ADJUSTMENT 0x10000

static char *vidmem;
static int vidport;
static int lines, cols;

static void __attribute__((section(".inittext"))) scroll(void) {
        int i;                  

	#undef memcpy
                        
        memcpy32(vidmem, vidmem + cols * 2, (lines - 1) * cols * 2);
        for (i = (lines - 1) * cols * 2; i < lines * cols * 2; i += 2)
                vidmem[i] = ' ';
}

static void __attribute__((section(".inittext"))) vid_puts(const char *str) {
	int x, y, pos;
	char c;

	if (NULL == vidmem) {
		if (boot_params.screen_info.orig_video_mode == 7) {
			vidmem = (char *) 0xb0000;
			vidport = 0x3b4;
		} else {
			vidmem = (char *) 0xb8000;
			vidport = 0x3d4;
		}

		vidmem -= ADDR_ADJUSTMENT;

		lines = boot_params.screen_info.orig_video_lines;
		cols = boot_params.screen_info.orig_video_cols;
	}

        if (boot_params.screen_info.orig_video_mode == 0 &&
            lines == 0 && cols == 0)
                return;

        x = boot_params.screen_info.orig_x;
        y = boot_params.screen_info.orig_y;

        while ((c = *str++) != '\0') {
                if (c == '\n') {
                        x = 0;
                        if (++y >= lines) {
                                scroll();
                                y--;
                        }
                } else {
                        vidmem[(x + cols * y) * 2] = c;
                        if (++x >= cols) {
                                x = 0;
                                if (++y >= lines) {
                                        scroll(); 
                                        y--;
                                }       
                        }       
                }       
        }       
        
        boot_params.screen_info.orig_x = x;
        boot_params.screen_info.orig_y = y;

        pos = (x + cols * y) * 2;       /* Update cursor position */
        outb(14, vidport);
        outb(0xff & (pos >> 9), vidport+1);
        outb(15, vidport);
        outb(0xff & (pos >> 1), vidport+1);
}

void __attribute__((section(".inittext"))) puts(const char *str)
{
	const char *s = str;

	while (*s)
		putchar(*s++);

	if (in_protected_mode()) {
		vid_puts(str);
	}
}

/*
 * Read the CMOS clock through the BIOS, and return the
 * seconds in BCD.
 */

static u8 gettime(void)
{
	struct biosregs ireg, oreg;

	initregs(&ireg);
	ireg.ah = 0x02;
	intcall(0x1a, &ireg, &oreg);

	return oreg.dh;
}

/*
 * Read from the keyboard
 */
int getchar(void)
{
	struct biosregs ireg, oreg;

	initregs(&ireg);
	/* ireg.ah = 0x00; */
	intcall(0x16, &ireg, &oreg);

	return oreg.al;
}

static int kbd_pending(void)
{
	struct biosregs ireg, oreg;

	initregs(&ireg);
	ireg.ah = 0x01;
	intcall(0x16, &ireg, &oreg);

	return !(oreg.eflags & X86_EFLAGS_ZF);
}

void kbd_flush(void)
{
	for (;;) {
		if (!kbd_pending())
			break;
		getchar();
	}
}

int getchar_timeout(void)
{
	int cnt = 30;
	int t0, t1;

	t0 = gettime();

	while (cnt) {
		if (kbd_pending())
			return getchar();

		t1 = gettime();
		if (t0 != t1) {
			cnt--;
			t0 = t1;
		}
	}

	return 0;		/* Timeout! */
}

