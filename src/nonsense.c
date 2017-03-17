#include "boot.h"

extern void real_mode_jump(void);
extern int a20_test(int);

void protected_mode(void) {
	unsigned int tmp;

	puts("In protected mode!\n");

	asm volatile(
		"mov %%cr0, %[tmp]\n\t"
		"and %[mask], %[tmp]\n\t"
		"mov %[tmp], %%cr0\n\t"
		: [tmp] "=r" (tmp)
		: [mask] "dN" (~X86_CR0_PE));

	real_mode_jump();
}

void real_mode(void) {
	u8 port_a;

	unsigned short int cs;
	asm volatile("\t"
		"mov %%cs, %0\n\t"
		"mov %0, %%ds\n\t"
		"mov %0, %%es\n"
		"mov %0, %%ss\n"
		: "=r"(cs));

	port_a = inb(0x92);     /* Configuration port A */
	port_a &= ~0x02;	/* Disable A20 */
	port_a &= ~0x01;	/* Do not reset machine */
	outb(port_a, 0x92);

	printf("A20 is %s, in real mode again!\n",
		a20_test(1)? "enabled": "disabled");

	die();
}
