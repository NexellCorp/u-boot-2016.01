/*
 * (C) Copyright 2016 Nexell
 * Youngbok, Park <ybpark@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <config.h>
#include <common.h>
#ifdef CONFIG_PWM_NX
#include <pwm.h>
#endif
#include <asm/io.h>

#ifdef CONFIG_DM_PMIC_NXE2000
#include <dm.h>
#include <dm/uclass-internal.h>
#include <power/pmic.h>
#include <power/nxe2000.h>
#endif

#ifdef CONFIG_DM_REGULATOR_NXE2000
#include <power/regulator.h>
#endif


#include <asm/arch/nexell.h>
#include <asm/arch/nx_gpio.h>

DECLARE_GLOBAL_DATA_PTR;

enum gpio_group {
    gpio_a,
    gpio_b,
    gpio_c,
    gpio_d,
    gpio_e,
};


/*------------------------------------------------------------------------------
 * intialize nexell soc and board status.
 */

/* call from u-boot */
int board_early_init_f(void)
{
	return 0;
}


void board_gpio_init(void)
{
	nx_gpio_initialize();
	nx_gpio_set_base_address(0, (void *)PHY_BASEADDR_GPIOA);
	nx_gpio_set_base_address(1, (void *)PHY_BASEADDR_GPIOB);
	nx_gpio_set_base_address(2, (void *)PHY_BASEADDR_GPIOC);
	nx_gpio_set_base_address(3, (void *)PHY_BASEADDR_GPIOD);
	nx_gpio_set_base_address(4, (void *)PHY_BASEADDR_GPIOE);
}

int mmc_get_env_dev(void)
{
	return 0;
}

int board_init(void)
{
	board_gpio_init();
	return 0;
}

/* u-boot dram initialize  */
int dram_init(void)
{
	gd->ram_size = CONFIG_SYS_SDRAM_SIZE;
	return 0;
}

/* u-boot dram board specific */
void dram_init_banksize(void)
{
	/* set global data memory */
	gd->bd->bi_arch_number = machine_arch_type;
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x00000100;

	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size  = CONFIG_SYS_SDRAM_SIZE;
}

#ifdef CONFIG_DM_PMIC_NXE2000
void power_init_board(void)
{
    struct udevice *pmic;
    struct udevice *dev;
#ifdef CONFIG_DM_REGULATOR
    struct dm_regulator_uclass_platdata *reg_uc_pdata;
    struct udevice *regulator;
#endif
    int ret = -ENODEV;

    ret = pmic_get("nxe2000_gpio@32", &pmic);
    if (ret)
        printf("Can't get PMIC: %s!\n", "nxe2000_gpio@32");

    if (device_has_children(pmic)) {
#ifdef CONFIG_DM_REGULATOR
        for (ret = uclass_find_first_device(UCLASS_REGULATOR, &dev);
            dev;
            ret = uclass_find_next_device(&dev)) {
            if (ret)
                continue;

            reg_uc_pdata = dev_get_uclass_platdata(dev);
            if (!reg_uc_pdata)
                continue;

            uclass_get_device_tail(dev, 0, &regulator);
			//printf("%s------ %d\n",__func__,ret);
        }
#endif
    }
}
#endif


static void uart2_pad_init(void)
{
	nx_gpio_set_pad_function(3, 16, 1);
	nx_gpio_set_output_enable(3, 16, 0);
	nx_gpio_set_pad_function(3, 20, 1);
	nx_gpio_set_output_enable(3, 20, 0);
}

int board_late_init(void)
{
	int i;
	uart2_pad_init();

	/* DOUT */
    nx_gpio_set_pad_function(gpio_d, 9, 1);
    nx_gpio_set_output_enable(gpio_d, 9, 1);

	/* BCLK */
    nx_gpio_set_pad_function(gpio_d, 10, 1);
    nx_gpio_set_output_enable(gpio_d, 10, 1);

	/* LRCLK */
    nx_gpio_set_pad_function(gpio_d, 12, 1);
    nx_gpio_set_output_enable(gpio_d, 12, 1);

	/* MCLK */
	nx_gpio_set_pad_function(gpio_d, 13, 1);
    nx_gpio_set_output_enable(gpio_d, 13, 1);

    /* WIFI/BT */
    nx_gpio_set_pad_function(gpio_a, 9, 0);
    nx_gpio_set_output_enable(gpio_a, 9, 1);
	nx_gpio_set_output_value(gpio_a, 9, 1);

    nx_gpio_set_pad_function(gpio_a, 12, 0);
    nx_gpio_set_output_enable(gpio_a, 12, 1);
	nx_gpio_set_output_value(gpio_a, 12, 1);

    nx_gpio_set_pad_function(gpio_a, 13, 0);
    nx_gpio_set_output_enable(gpio_a, 13, 0);
    //nx_gpio_set_output_value(gpio_a, 13, 1);



	return 0;
}

