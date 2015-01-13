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
#include <asm/arch/mux_dra7xx.h>
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

static u32 calculate_delay(u32 base, u32 den)
{
	u32 refclk_period, dly_cnt, ref_cnt, reg;

	refclk_period = readl((*ctrl)->iodelay_config_reg_2) &&
			CFG_REG_REFCLK_PERIOD_MASK;

	reg = readl(base);
	dly_cnt = (reg && CFG_REG_DLY_CNT_MASK) >> CFG_REG_DLY_CNT_SHIFT;
	ref_cnt = (reg && CFG_REG_REF_CNT_MASK) >> CFG_REG_REF_CNT_SHIFT;

	reg = DIV_ROUND_UP(10 * ref_cnt * refclk_period, 2 * dly_cnt * den);

	return reg;
}

static u32 get_cfg_reg(u32 a_delay, u32 g_delay, u32 cpde, u32 fpde)
{
	u32 g_delay_coarse, g_delay_fine;
	u32 a_delay_coarse, a_delay_fine;
	u32 c_elements, f_elements;
	u32 total_delay, reg = 0;

	g_delay_coarse = g_delay / 920;
	g_delay_fine = DIV_ROUND_UP((g_delay % 920) * 10, 60);

	a_delay_coarse = a_delay / cpde;
	a_delay_fine = DIV_ROUND_UP((a_delay % cpde) * 10, fpde);

	c_elements = g_delay_coarse + a_delay_coarse;
	f_elements = DIV_ROUND_UP(g_delay_fine + a_delay_fine, 10);

	if (f_elements > 22) {
		total_delay = c_elements * cpde + f_elements * fpde;

		c_elements = DIV_ROUND_UP(total_delay, cpde);
		f_elements = DIV_ROUND_UP(total_delay % cpde, fpde);
	}

	reg = (c_elements << CFG_X_COARSE_DLY_SHIFT) & CFG_X_COARSE_DLY_MASK;
	reg |= (f_elements << CFG_X_FINE_DLY_SHIFT) & CFG_X_FINE_DLY_MASK;
	reg |= CFG_X_SIGNATURE << CFG_X_SIGNATURE_SHIFT;

	return reg;
}

static void configure_pad_mux(struct pad_conf_entry const *array, int size)
{
	int i;
	struct pad_conf_entry *pad = (struct pad_conf_entry *)array;
	u32 cpde, fpde, reg, mode_select;

	cpde = calculate_delay((*ctrl)->iodelay_config_reg_3, 88);
	fpde = calculate_delay((*ctrl)->iodelay_config_reg_4, 264);

	for (i = 0; i < size; i++, pad++) {
		writel(pad->val,
		       (*ctrl)->control_padconf_core_base + pad->offset);

		mode_select = pad->val & MODE_SELECT;
		/* Configure CFG_X_IN register */
		if (mode_select && pad->cfg_in.offset) {
			reg = get_cfg_reg(pad->cfg_in.a_delay,
					  pad->cfg_in.g_delay, cpde, fpde);
			writel(reg, (*ctrl)->iodelay_config_base +
			       pad->cfg_in.offset);
		}

		/* Configure CFG_X_OEN register */
		if (mode_select && pad->cfg_oen.offset) {
			reg = get_cfg_reg(pad->cfg_oen.a_delay,
					  pad->cfg_oen.g_delay, cpde, fpde);
			writel(reg, (*ctrl)->iodelay_config_base +
			       pad->cfg_oen.offset);
		}

		/* Configure CFG_X_OUT register */
		if (mode_select && pad->cfg_out.offset) {
			reg = get_cfg_reg(pad->cfg_out.a_delay,
					  pad->cfg_out.g_delay, cpde, fpde);
			writel(reg, (*ctrl)->iodelay_config_base +
			       pad->cfg_out.offset);
		}
	}
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
