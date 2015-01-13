/*
 * (C) Copyright 2015
 * Texas Instruments Incorporated
 *
 * Lokesh Vutla <lokeshvutla@ti.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _DRA7_IODELAY_H_
#define _DRA7_IODELAY_H_

#include <common.h>
#include <asm/arch/sys_proto.h>

/* CONFIG_REG_0 */
#define CFG_REG_ROM_READ_SHIFT		1
#define CFG_REG_ROM_READ_MASK		(1 << 1)
#define CFG_REG_CALIB_STRT_SHIFT	0
#define CFG_REG_CALIB_STRT_MASK		(1 << 0)
#define CFG_REG_CALIB_STRT		1
#define CFG_REG_CALIB_END		0
#define CFG_REG_ROM_READ_START		(1 << 1)
#define CFG_REG_ROM_READ_END		(0 << 1)

/* CONFIG_REG_2 */
#define CFG_REG_REFCLK_PERIOD_SHIFT	0
#define CFG_REG_REFCLK_PERIOD_MASK	(0xFFFF << 0)
#define CFG_REG_REFCLK_PERIOD		0x2EF

/* CTRL_CORE_SMA_SW_0 */
#define CTRL_ISOLATE_SHIFT		2
#define CTRL_ISOLATE_MASK		(1 << 2)
#define ISOLATE_IO			1
#define DEISOLATE_IO			0

/* PRM_IO_PMCTRL */
#define PMCTRL_ISOCLK_OVERRIDE_SHIFT	0
#define PMCTRL_ISOCLK_OVERRIDE_MASK	(1 << 0)
#define PMCTRL_ISOCLK_STATUS_SHIFT	1
#define PMCTRL_ISOCLK_STATUS_MASK	(1 << 1)
#define PMCTRL_ISOCLK_OVERRIDE_CTRL	1
#define PMCTRL_ISOCLK_NOT_OVERRIDE_CTRL	0

void recalibrate_iodelay(struct pad_conf_entry const *array, int size);

#endif
