/*
 * (C) Copyright 2016 Nexell
 * Youngbok, Park <park@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <linux/types.h>
#include <asm/io.h>
#include <asm/arch/nexell.h>
#include <linux/linkage.h>


#define NEXELL_SMC_BASE		0x82000000

#define NEXELL_SMC_FN(n)	(NEXELL_SMC_BASE + (n))

#define NEXELL_SMC_SEC_REG_WRITE	NEXELL_SMC_FN(0)
#define NEXELL_SMC_SEC_REG_READ		NEXELL_SMC_FN(1)

asmlinkage int __invoke_nexell_fn_smc(u32, u32, u32, u32);

int write_sec_reg(void __iomem *reg, int val)
{
	int ret = 0;
	ret = __invoke_nexell_fn_smc(NEXELL_SMC_SEC_REG_WRITE,
			(u32)reg, val, 0);
	return ret;
}
EXPORT_SYMBOL_GPL(write_sec_reg);

int read_sec_reg(void __iomem *reg)
{
int ret = 0;
ret = __invoke_nexell_fn_smc(NEXELL_SMC_SEC_REG_READ,
		(u32)reg, 0, 0);
return ret;
}
EXPORT_SYMBOL_GPL(read_sec_reg);
