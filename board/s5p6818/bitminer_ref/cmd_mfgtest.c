/*
 * Copyright (C) 2010 Texas Instruments
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>

#include <net.h>

#include <asm/io.h>

#include <asm/arch/nexell.h>
#include <asm/arch/nx_gpio.h>

#ifdef CONFIG_CMD_MFGTEST

#define set_rled_on()  nx_gpio_set_output_value  (gpio_a,  4, 0)
#define set_rled_off() nx_gpio_set_output_value  (gpio_a,  4, 1)
#define set_gled_on()  nx_gpio_set_output_value  (gpio_a,  5, 0)
#define set_gled_off() nx_gpio_set_output_value  (gpio_a,  5, 1)
#define	get_key()      (nx_gpio_get_input_value(gpio_a, 3)?0:1)

static void test_led_key(void)
{
	printf("press key if led blink\n");
	while(1) {
		if(get_key()) break;
		set_gled_on();
		set_rled_off();
		udelay(300000);
		if(get_key()) break;
		set_gled_off();
		set_rled_on();
		udelay(300000);
	}
}

#if 1
#define error_led_no_return() return 0
#else
static void error_led_no_return(void)
{
	set_gled_off();
	while(1) {
		if(get_key()) break;
		set_rled_on();
		udelay(300000);
		if(get_key()) break;
		set_rled_off();
		udelay(300000);
	}
}
#endif
static void ok_led_no_return(void)
{
	set_rled_off();
	while(1) {
		if(get_key()) break;
		set_gled_on();
		udelay(300000);
		if(get_key()) break;
		set_gled_off();
		udelay(300000);
	}
}
// AP_SPI0_CLK		: GPIOC29
// AP_SPI0_CS 		: GPIOC30
// AP_SPI0_TX 		: GPIOC31
// AP_SPI0_RX 		: GPIOD0
// AP_HASH0_VOLCTRL	: GPIOA20
// AP_HASH0_PLUG	: GPIOA24
// AP_HASH0_PWREN	: GPIOA0
// AP_HASH0_OON		: GPIOD29
// AP_HASH0_GLD		: GPIOD30
// AP_HASH0_RST		: GPIOD31

// AP_SPI2_CLK		: GPIOC9
// AP_SPI2_CS 		: GPIOC10
// AP_SPI2_TX 		: GPIOC12
// AP_SPI2_RX 		: GPIOC11
// AP_HASH1_VOLCTRL	: GPIOA9
// AP_HASH1_PLUG	: GPIOA11
// AP_HASH1_PWREN	: GPIOA16
// AP_HASH1_OON		: GPIOE2
// AP_HASH1_GLD		: GPIOE3
// AP_HASH1_RST		: GPIOE4
static int test_hash_con(void)
{
	int ret = 0;
	// set HASH 0 pins to out
	nx_gpio_set_pad_function  (gpio_c, 29, 0);	// AP_SPI0_CLK
	nx_gpio_set_pull_mode     (gpio_c, 29, 2);
	nx_gpio_set_drive_strength(gpio_c, 29, 3);
	nx_gpio_set_output_enable (gpio_c, 29, 1);
	nx_gpio_set_pad_function  (gpio_c, 30, 0);	// AP_SPI0_CS
	nx_gpio_set_pull_mode     (gpio_c, 30, 2);
	nx_gpio_set_drive_strength(gpio_c, 30, 3);
	nx_gpio_set_output_enable (gpio_c, 30, 1);
	nx_gpio_set_pad_function  (gpio_c, 31, 0);	// AP_SPI0_TX
	nx_gpio_set_pull_mode     (gpio_c, 31, 2);
	nx_gpio_set_drive_strength(gpio_c, 31, 3);
	nx_gpio_set_output_enable (gpio_c, 31, 1);
	nx_gpio_set_pad_function  (gpio_d,  0, 0);	// AP_SPI0_RX
	nx_gpio_set_pull_mode     (gpio_d,  0, 2);
	nx_gpio_set_drive_strength(gpio_d,  0, 3);
	nx_gpio_set_output_enable (gpio_d,  0, 1);
	nx_gpio_set_pad_function  (gpio_a, 20, 0);	// AP_HASH0_VOLCTRL
	nx_gpio_set_pull_mode     (gpio_a, 20, 2);
	nx_gpio_set_drive_strength(gpio_a, 20, 3);
	nx_gpio_set_output_enable (gpio_a, 20, 1);
	nx_gpio_set_pad_function  (gpio_a, 24, 0);	// AP_HASH0_PLUG
	nx_gpio_set_pull_mode     (gpio_a, 24, 2);
	nx_gpio_set_drive_strength(gpio_a, 24, 3);
	nx_gpio_set_output_enable (gpio_a, 24, 1);
	nx_gpio_set_pad_function  (gpio_a,  0, 0);	// AP_HASH0_PWREN
	nx_gpio_set_pull_mode     (gpio_a,  0, 2);
	nx_gpio_set_drive_strength(gpio_a,  0, 3);
	nx_gpio_set_output_enable (gpio_a,  0, 1);
	nx_gpio_set_pad_function  (gpio_d, 29, 0);	// AP_HASH0_OON
	nx_gpio_set_pull_mode     (gpio_d, 29, 2);
	nx_gpio_set_drive_strength(gpio_d, 29, 3);
	nx_gpio_set_output_enable (gpio_d, 29, 1);
	nx_gpio_set_pad_function  (gpio_d, 30, 0);	// AP_HASH0_GLD
	nx_gpio_set_pull_mode     (gpio_d, 30, 2);
	nx_gpio_set_drive_strength(gpio_d, 30, 3);
	nx_gpio_set_output_enable (gpio_d, 30, 1);
	nx_gpio_set_pad_function  (gpio_d, 31, 0);	// AP_HASH0_RST
	nx_gpio_set_pull_mode     (gpio_d, 31, 2);
	nx_gpio_set_drive_strength(gpio_d, 31, 3);
	nx_gpio_set_output_enable (gpio_d, 31, 1);

	// set HASH 1 pins to in
	nx_gpio_set_pad_function  (gpio_c,  9, 1);	// AP_SPI2_CLK
	nx_gpio_set_pull_mode     (gpio_c,  9, 2);
	nx_gpio_set_drive_strength(gpio_c,  9, 3);
	nx_gpio_set_output_enable (gpio_c,  9, 0);
	nx_gpio_set_pad_function  (gpio_c, 10, 1);	// AP_SPI2_CS
	nx_gpio_set_pull_mode     (gpio_c, 10, 2);
	nx_gpio_set_drive_strength(gpio_c, 10, 3);
	nx_gpio_set_output_enable (gpio_c, 10, 0);
	nx_gpio_set_pad_function  (gpio_c, 12, 1);	// AP_SPI2_TX
	nx_gpio_set_pull_mode     (gpio_c, 12, 2);
	nx_gpio_set_drive_strength(gpio_c, 12, 3);
	nx_gpio_set_output_enable (gpio_c, 12, 0);
	nx_gpio_set_pad_function  (gpio_c, 11, 1);	// AP_SPI2_RX
	nx_gpio_set_pull_mode     (gpio_c, 11, 2);
	nx_gpio_set_drive_strength(gpio_c, 11, 3);
	nx_gpio_set_output_enable (gpio_c, 11, 0);
	nx_gpio_set_pad_function  (gpio_a,  9, 0);	// AP_HASH1_VOLCTRL
	nx_gpio_set_pull_mode     (gpio_a,  9, 2);
	nx_gpio_set_drive_strength(gpio_a,  9, 3);
	nx_gpio_set_output_enable (gpio_a,  9, 1);
	nx_gpio_set_pad_function  (gpio_a, 11, 0);	// AP_HASH1_PLUG
	nx_gpio_set_pull_mode     (gpio_a, 11, 2);
	nx_gpio_set_drive_strength(gpio_a, 11, 3);
	nx_gpio_set_output_enable (gpio_a, 11, 1);
	nx_gpio_set_pad_function  (gpio_a, 16, 0);	// AP_HASH1_PWREN
	nx_gpio_set_pull_mode     (gpio_a, 16, 2);
	nx_gpio_set_drive_strength(gpio_a, 16, 3);
	nx_gpio_set_output_enable (gpio_a, 16, 1);
	nx_gpio_set_pad_function  (gpio_e,  2, 0);	// AP_HASH1_OON
	nx_gpio_set_pull_mode     (gpio_e,  2, 2);
	nx_gpio_set_drive_strength(gpio_e,  2, 3);
	nx_gpio_set_output_enable (gpio_e,  2, 1);
	nx_gpio_set_pad_function  (gpio_e,  3, 0);	// AP_HASH1_GLD
	nx_gpio_set_pull_mode     (gpio_e,  3, 2);
	nx_gpio_set_drive_strength(gpio_e,  3, 3);
	nx_gpio_set_output_enable (gpio_e,  3, 1);
	nx_gpio_set_pad_function  (gpio_e,  4, 0);	// AP_HASH1_RST
	nx_gpio_set_pull_mode     (gpio_e,  4, 2);
	nx_gpio_set_drive_strength(gpio_e,  4, 3);
	nx_gpio_set_output_enable (gpio_e,  4, 1);

	nx_gpio_set_output_value  (gpio_c, 29, 0);	// AP_SPI0_CLK
	nx_gpio_set_output_value  (gpio_c, 30, 0);	// AP_SPI0_CS
	nx_gpio_set_output_value  (gpio_c, 31, 0);	// AP_SPI0_TX
	nx_gpio_set_output_value  (gpio_d,  0, 0);	// AP_SPI0_RX
	nx_gpio_set_output_value  (gpio_a, 20, 0);	// AP_HASH0_VOLCTRL
	nx_gpio_set_output_value  (gpio_a, 24, 0);	// AP_HASH0_PLUG
	nx_gpio_set_output_value  (gpio_a,  0, 0);	// AP_HASH0_PWREN
	nx_gpio_set_output_value  (gpio_d, 29, 0);	// AP_HASH0_OON
	nx_gpio_set_output_value  (gpio_d, 30, 0);	// AP_HASH0_GLD
	nx_gpio_set_output_value  (gpio_d, 31, 0);	// AP_HASH0_RST

	if(
		nx_gpio_get_input_value  (gpio_c,  9)	// AP_SPI2_CLK
	  != 0) {
		printf("SPI0_CLK, SPI2_CLK are not connected(LOW)\n");
		ret++;
	}
	if(
		nx_gpio_get_input_value  (gpio_c, 10)	// AP_SPI2_CS
	  != 0) {
		printf("SPI0_CS, SPI2_CS are not connected(LOW)\n");
		ret++;
	}
	if(
		nx_gpio_get_input_value  (gpio_c, 12)	// AP_SPI2_TX
	  != 0) {
		printf("SPI0_TX, SPI2_TX are not connected(LOW)\n");
		ret++;
	}
	if(
		nx_gpio_get_input_value  (gpio_c, 11)	// AP_SPI2_RX
	  != 0) {
		printf("SPI0_RX, SPI2_RX are not connected(LOW)\n");
		ret++;
	}
	if(
		nx_gpio_get_input_value  (gpio_a,  9)	// AP_HASH1_VOLCTRL
	  != 0) {
		printf("AP_HASH0_VOLCTRL, AP_HASH1_VOLCTRL are not connected(LOW)\n");
		ret++;
	}
	if(
		nx_gpio_get_input_value  (gpio_a, 11)	// AP_HASH1_PLUG
	  != 0) {
		printf("AP_HASH0_PLUG, AP_HASH1_PLUG are not connected(LOW)\n");
		ret++;
	}
	if(
		nx_gpio_get_input_value  (gpio_a, 16)	// AP_HASH1_PWREN
	  != 0) {
		printf("AP_HASH0_PWREN, AP_HASH1_PWREN are not connected(LOW)\n");
		ret++;
	}
	if(
		nx_gpio_get_input_value  (gpio_e,  2)	// AP_HASH1_OON
	  != 0) {
		printf("AP_HASH0_OON, AP_HASH1_OON are not connected(LOW)\n");
		ret++;
	}
	if(
		nx_gpio_get_input_value  (gpio_e,  3)	// AP_HASH1_GLD
	  != 0) {
		printf("AP_HASH0_GLD, AP_HASH1_GLD are not connected(LOW)\n");
		ret++;
	}
	if(
		nx_gpio_get_input_value  (gpio_e,  4)	// AP_HASH1_RST
	  != 0) {
		printf("AP_HASH0_RST, AP_HASH1_RST are not connected(LOW)\n");
		ret++;
	}

	nx_gpio_set_output_value  (gpio_c, 29, 1);	// AP_SPI0_CLK
	nx_gpio_set_output_value  (gpio_c, 30, 1);	// AP_SPI0_CS
	nx_gpio_set_output_value  (gpio_c, 31, 1);	// AP_SPI0_TX
	nx_gpio_set_output_value  (gpio_d,  0, 1);	// AP_SPI0_RX
	nx_gpio_set_output_value  (gpio_a, 20, 1);	// AP_HASH0_VOLCTRL
	nx_gpio_set_output_value  (gpio_a, 24, 1);	// AP_HASH0_PLUG
	nx_gpio_set_output_value  (gpio_a,  0, 1);	// AP_HASH0_PWREN
	nx_gpio_set_output_value  (gpio_d, 29, 1);	// AP_HASH0_OON
	nx_gpio_set_output_value  (gpio_d, 30, 1);	// AP_HASH0_GLD
	nx_gpio_set_output_value  (gpio_d, 31, 1);	// AP_HASH0_RST

	if(
		nx_gpio_get_input_value  (gpio_c,  9)	// AP_SPI2_CLK
	  != 1) {
		printf("SPI0_CLK, SPI2_CLK are not connected(HIGH)\n");
		ret++;
	}
	if(
		nx_gpio_get_input_value  (gpio_c, 10)	// AP_SPI2_CS
	  != 1) {
		printf("SPI0_CS, SPI2_CS are not connected(HIGH)\n");
		ret++;
	}
	if(
		nx_gpio_get_input_value  (gpio_c, 12)	// AP_SPI2_TX
	  != 1) {
		printf("SPI0_TX, SPI2_TX are not connected(HIGH)\n");
		ret++;
	}
	if(
		nx_gpio_get_input_value  (gpio_c, 11)	// AP_SPI2_RX
	  != 1) {
		printf("SPI0_RX, SPI2_RX are not connected(HIGH)\n");
		ret++;
	}
	if(
		nx_gpio_get_input_value  (gpio_a,  9)	// AP_HASH1_VOLCTRL
	  != 1) {
		printf("AP_HASH0_VOLCTRL, AP_HASH1_VOLCTRL are not connected(HIGH)\n");
		ret++;
	}
	if(
		nx_gpio_get_input_value  (gpio_a, 11)	// AP_HASH1_PLUG
	  != 1) {
		printf("AP_HASH0_PLUG, AP_HASH1_PLUG are not connected(HIGH)\n");
		ret++;
	}
	if(
		nx_gpio_get_input_value  (gpio_a, 16)	// AP_HASH1_PWREN
	  != 1) {
		printf("AP_HASH0_PWREN, AP_HASH1_PWREN are not connected(HIGH)\n");
		ret++;
	}
	if(
		nx_gpio_get_input_value  (gpio_e,  2)	// AP_HASH1_OON
	  != 1) {
		printf("AP_HASH0_OON, AP_HASH1_OON are not connected(HIGH)\n");
		ret++;
	}
	if(
		nx_gpio_get_input_value  (gpio_e,  3)	// AP_HASH1_GLD
	  != 1) {
		printf("AP_HASH0_GLD, AP_HASH1_GLD are not connected(HIGH)\n");
		ret++;
	}
	if(
		nx_gpio_get_input_value  (gpio_e,  4)	// AP_HASH1_RST
	  != 1) {
		printf("AP_HASH0_RST, AP_HASH1_RST are not connected(HIGH)\n");
		ret++;
	}

	/* TODO: run AP_HASH0_ADC(ADC0) for detecting V3P3_HASH1 */

	/* TODO: run AP_HASH1_ADC(ADC1) for detecting V3P3_HASH0 */

	return ret;
}

static int do_mfgtest(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
//	test_led_key();

	set_gled_off();
	set_rled_on();
//	if(test_hash_con()) error_led_no_return();

	set_gled_on();
	set_rled_off();
	return 0;

bat_cmd_usage:
	return cmd_usage(cmdtp);
}

U_BOOT_CMD(
	mfgtest, 2, 1, do_mfgtest,
	"Product test, if errors, RED led blink and no return\n",
	""
);

static int do_okled(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ok_led_no_return();

	return 0;

bat_cmd_usage:
	return cmd_usage(cmdtp);
}

U_BOOT_CMD(
	okled, 2, 1, do_okled,
	"blink green led, turn off red, no return\n",
	""
);

static char *mac_str = (char *)0x40000000;

static char atoh(char p)
{
	char ret = -1;
	if((p>='0')&&(p<='9'))
		ret = p-'0';
	else if((p>='a')&&(p<='f'))
		ret = p-'a'+10;
	else if((p>='A')&&(p<='F'))
		ret = p-'A'+10;

	return ret;
}

static int str2mac(char *str, char *mac)
{
	int ii;
	char tt;
	for(ii=0; ii<6; ii++) {
		tt = (char)atoh(str[2*ii]);
		if(tt == -1) return -1;
		mac[ii] = tt<<4;
		tt = (char)atoh(str[2*ii+1]);
		if(tt == -1) return -1;
		mac[ii] |= tt;
	}
	return 0;
}

static int do_mfgmac(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char vars[256];
	char mac[6], *s;
	int ii, ret = 0;
	s = getenv("ethaddr");

	if(strcmp(s, "00:19:D3:FF:FF:FF")) return 0;

	run_command("dhcp", 0);

	if(!run_command("tftpboot macshort.txt", 0)) error_led_no_return();

	if(run_command("tftpput 0x40000000 4 req_$ipaddr.txt", 0)) error_led_no_return();

	ret = 0;
	udelay(3000000);
	for(ii=0; ii<4; ii++) {
		ret = run_command("tftpboot 0x40000000 mac_$ipaddr.txt", 0);
		if(ret == 0) break;
		udelay(5000000);
	}
	if(ret) ret = run_command("tftpboot 0x40000000 mac_$ipaddr.txt", 0);
	if(ret) error_led_no_return();

	if(run_command("tftpput 0x40000000 4 ack_$ipaddr.txt", 0)) error_led_no_return();

	if(str2mac(0x40000000, mac)) error_led_no_return();
	sprintf(vars, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	if(setenv("ethaddr", vars)) error_led_no_return();
	if(saveenv()) error_led_no_return();

	if(run_command("fdisk 0 3: 0x400000:0x4000000 0x4400000:0x2000000 0x6400000:0x9000000", 0)) error_led_no_return();
	if(run_command("tftpboot 40000000 boot.img; ext4_img_write 0 0000000040000000 1 $filesize", 0)) error_led_no_return();
//	if(run_command("tftpboot 40000000 modules.img; mmc write 40000000 22000 10000", 0)) error_led_no_return();
	if(run_command("tftpboot 40000000 rootfs.img; ext4_img_write 0 0000000040000000 3 $filesize", 0)) error_led_no_return();

	ok_led_no_return();

	return 0;

bat_cmd_usage:
	return cmd_usage(cmdtp);
}

U_BOOT_CMD(
	mfgmac, 2, 1, do_mfgmac,
	"get mac from tftp server and write it to env area\n",
	""
);
static int do_mfgflash(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char vars[256];
	char mac[6];
	int ii, ret = 0;
	run_command("dhcp", 0);

	if(run_command("tftpboot 40000000 bl1-emmcboot.img; mmc write 40000000 1 80", 0)) error_led_no_return();
	if(run_command("tftpboot 40000000 fip-loader-emmc.img; mmc write 40000000 81 280", 0)) error_led_no_return();
	if(run_command("tftpboot 40000000 fip-secure.img; mmc write 40000000 301 c00", 0)) error_led_no_return();
	if(run_command("tftpboot 40000000 fip-nonsecure.img; mmc write 40000000 f01 800", 0)) error_led_no_return();
	if(run_command("tftpboot 40000000 params.bin; mmc write 40000000 1701 20", 0)) error_led_no_return();

	if(run_command("reset", 0)) error_led_no_return();

	return 0;

bat_cmd_usage:
	return cmd_usage(cmdtp);
}

U_BOOT_CMD(
	mfgflash, 2, 1, do_mfgflash,
	"burn files to mmc for mmc booting\n",
	""
);
#endif /* CONFIG_CMD_MFGTEST */
