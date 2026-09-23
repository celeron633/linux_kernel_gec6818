/*
 * Nexell S5P6818 running in AArch32 (CONFIG_ARCH_S5P6818 on arch/arm).
 *
 * Based on mach-s5p4418/cpu.c. The Cortex-A53 has no PL310, SCU or
 * TWD, so all that is left is the machine descriptor and reset.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */
#include <linux/of_platform.h>
#include <linux/reboot.h>
#include <asm/mach/arch.h>
#include <linux/io.h>

#define PHYS_BASE_CLKPWR	0xC0010000
#define ALIVE_GATE		0x800
#define PWR_CONT		0x224
#define PWR_MODE		0x228
#define SW_RESET_EN		(1 << 3)
#define SW_RESET		(1 << 12)

static void s5p6818_reboot(enum reboot_mode mode, const char *cmd)
{
	void __iomem *base = ioremap(PHYS_BASE_CLKPWR, 0x1000);

	__raw_writel(0, base + ALIVE_GATE);
	__raw_writel(SW_RESET_EN, base + PWR_CONT);
	__raw_writel(SW_RESET, base + PWR_MODE);
}

static const char * const s5p6818_dt_compat[] = {
	"nexell,s5p6818",
	NULL
};

DT_MACHINE_START(S5P6818, "s5p6818")
	.dt_compat	= s5p6818_dt_compat,
	.restart	= s5p6818_reboot,
MACHINE_END
