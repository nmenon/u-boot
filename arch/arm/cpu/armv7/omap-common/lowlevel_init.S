/*
 * Board specific setup info
 *
 * (C) Copyright 2010
 * Texas Instruments, <www.ti.com>
 *
 * Author :
 *	Aneesh V	<aneesh@ti.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <asm/arch/omap.h>
#include <asm/omap_common.h>
#include <asm/arch/spl.h>
#include <linux/linkage.h>

#ifndef CONFIG_OMAP34XX
ENTRY(save_boot_params)
	ldr	r1, =OMAP_SRAM_SCRATCH_BOOT_PARAMS
	str	r0, [r1]
	bx	lr
ENDPROC(save_boot_params)
#endif

ENTRY(omap_smc1)
	PUSH	{r4-r12, lr}	@ save registers - ROM code may pollute
				@ our registers
	MOV	r12, r0		@ Service
	MOV	r0, r1		@ Argument
	DSB
	DMB
	SMC	#0		@ Call the secure monitor for the service

	POP	{r4-r12, pc}
ENDPROC(omap_smc1)
