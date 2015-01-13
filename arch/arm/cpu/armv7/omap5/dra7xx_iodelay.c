/*
 * (C) Copyright 2015
 * Texas Instruments Incorporated, <www.ti.com>
 *
 * Lokesh Vutla <lokeshvutla@ti.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/utils.h>
#include <asm/arch/dra7xx_iodelay.h>
#include <asm/arch/omap.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/clock.h>
#include <asm/omap_common.h>

static void isolate_io(u32 isolate)
{
	/* Override control on ISOCLKIN signal to IO pad ring. */
	clrsetbits_le32((*prcm)->prm_io_pmctrl, PMCTRL_ISOCLK_OVERRIDE_MASK,
			PMCTRL_ISOCLK_OVERRIDE_CTRL);
	wait_on_value(PMCTRL_ISOCLK_STATUS_MASK, PMCTRL_ISOCLK_STATUS_MASK,
		      (u32 *)(*prcm)->prm_io_pmctrl, LDELAY);

	/* Isolate/Deisolate IO */
	clrsetbits_le32((*ctrl)->ctrl_core_sma_sw_0, CTRL_ISOLATE_MASK,
			isolate << CTRL_ISOLATE_SHIFT);
	/* Dummy read to add delay */
	readl((*ctrl)->ctrl_core_sma_sw_0);

	/* Return control on ISOCLKIN to hardware */
	clrsetbits_le32((*prcm)->prm_io_pmctrl, PMCTRL_ISOCLK_OVERRIDE_MASK,
			PMCTRL_ISOCLK_NOT_OVERRIDE_CTRL);
	wait_on_value(PMCTRL_ISOCLK_STATUS_MASK,
		      0 << PMCTRL_ISOCLK_STATUS_SHIFT,
		      (u32 *)(*prcm)->prm_io_pmctrl, LDELAY);
}

static void calibrate_iodelay(void)
{
	u32 reg;

	/* Configure REFCLK period */
	reg = readl((*ctrl)->iodelay_config_reg_2);
	reg &= ~CFG_REG_REFCLK_PERIOD_MASK;
	reg |= CFG_REG_REFCLK_PERIOD;
	writel(reg, (*ctrl)->iodelay_config_reg_2);

	/* Initiate Calibration */
	clrsetbits_le32((*ctrl)->iodelay_config_reg_0, CFG_REG_CALIB_STRT_MASK,
			CFG_REG_CALIB_STRT << CFG_REG_CALIB_STRT_SHIFT);
	wait_on_value(CFG_REG_CALIB_STRT_MASK, CFG_REG_CALIB_END,
		      (u32 *)(*ctrl)->iodelay_config_reg_0, LDELAY);
}

static void update_delay(void)
{
	/* Initiate the reload of calibrated values. */
	clrsetbits_le32((*ctrl)->iodelay_config_reg_0, CFG_REG_ROM_READ_MASK,
			CFG_REG_ROM_READ_START);
	wait_on_value(CFG_REG_ROM_READ_MASK, CFG_REG_ROM_READ_END,
		      (u32 *)(*ctrl)->iodelay_config_reg_0, LDELAY);
}

static void configure_pad_mux(struct pad_conf_entry const *array, int size)
{
	int i;
	struct pad_conf_entry *pad = (struct pad_conf_entry *)array;

	for (i = 0; i < size; i++, pad++)
		writel(pad->val,
		       (*ctrl)->control_padconf_core_base + pad->offset);
}

void recalibrate_iodelay(struct pad_conf_entry const *array, int size)
{
	/* IO recalibration should be done only from SRAM */
	if (OMAP_INIT_CONTEXT_SPL != omap_hw_init_context())
		return;

	/* unlock IODELAY CONFIG registers */
	writel(CFG_IO_DELAY_UNLOCK_KEY, (*ctrl)->iodelay_config_reg_8);

	calibrate_iodelay();

	isolate_io(ISOLATE_IO);

	update_delay();

	configure_pad_mux(array, size);

	isolate_io(DEISOLATE_IO);

	/* lock IODELAY CONFIG registers */
	writel(CFG_IO_DELAY_LOCK_KEY, (*ctrl)->iodelay_config_reg_8);
}
