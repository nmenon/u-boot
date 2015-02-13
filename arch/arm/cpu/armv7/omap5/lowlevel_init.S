/*
 * Board specific misc setup
 *
 * (C) Copyright 2015
 * Texas Instruments, <www.ti.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <asm/arch/omap.h>
#include <asm/omap_common.h>
#include <asm/arch/spl.h>
#include <linux/linkage.h>

#define OMAP5_SERVICE_L2ACTLR_SET    0x104

#ifdef CONFIG_ARM_ARCH_CP15_ERRATA
	.globl	arch_cp15_errata_workaround
/*
 * R0 has MIDR
 * R1 has CPU Variant (bits 20-23)
 * R2 has CPU Revision (bits 0-3)
 * R3 is compbined CPU variant << 4 + CPU revision
 */
ENTRY(arch_cp15_errata_workaround)
	push	{r4-r11, lr}	@ save registers - ROM code may pollute

#ifdef CONFIG_ARM_ERRATA_798870
	/* We are r2p2, but anyways for completeness.. */
	cmp	r3, #0x30		@ Applies to lower than R3p0
	bge     skip_errata_798870      @ skip if not affected rev
	cmp	r3, #0x20		@ Applies to including and above R2p0
	blt     skip_errata_798870      @ skip if not affected rev

	mrc     p15, 1, r1, c15, c0, 0  @ read l2 aux ctrl reg
	orr     r1, r1, #1 << 7         @ set bit #7
	ldr     r0, =OMAP5_SERVICE_L2ACTLR_SET    @ Set L2 Cache Auxiliary control register - value in R0
	b omap_smc1

skip_errata_798870:
#endif
	pop	{r4-r11, pc}
ENDPROC(arch_cp15_errata_workaround)

#endif
