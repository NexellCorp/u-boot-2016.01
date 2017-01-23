/*
 * (C) Copyright 2016 Nexell
 * Youngbok, Park <ybpark@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */
#include <common.h>
#include <config.h>

#include <asm/byteorder.h>
#include <command.h>
#include <malloc.h>
#include <mmc.h>
#include <fat.h>
#include <fs.h>
#include <pwm.h>

#include <asm/arch/nexell.h>
#include <asm/arch/clk.h>
#include <asm/arch/reset.h>
#include <asm/arch/nx_gpio.h>
//#include <asm/arch/nx_uart.h>


#include <linux/types.h>
#include <asm/io.h>
//#include <asm/arch/nexell.h>
//#include "asm/arch/nx_uart.h"



#define UART_PL01x_RSR_OE		0x08
#define UART_PL01x_RSR_BE		0x04
#define UART_PL01x_RSR_PE		0x02
#define UART_PL01x_RSR_FE		0x01

#define UART_PL01x_FR_TXFE		0x80
#define UART_PL01x_FR_RXFF		0x40
#define UART_PL01x_FR_TXFF		0x20
#define UART_PL01x_FR_RXFE		0x10
#define UART_PL01x_FR_BUSY		0x08
#define UART_PL01x_FR_TMSK	(UART_PL01x_FR_TXFF + UART_PL01x_FR_BUSY)

#define CFG_SYS_DEBUG_UART_CH		2

#define CFG_SYS_DEBUG_UART_BAUDRATE	115200

#define INIT_CON			0x7F

#define GET_CMD				0x00
#define GET_VER_ROPS_CMD		0x01
#define GET_ID_CMD			0x02
#define SET_SPEED_CMD			0x03
#define READ_CMD			0x11
#define GO_CMD				0x21
#define WRITE_CMD			0x31
#define ERASE_CMD			0x43
#define ERASE_EXT_CMD			0x44
#define WRITE_PROTECT_CMD		0x63
#define WRITE_TEMP_UNPROTECT_CMD	0x71
#define WRITE_PERM_UNPROTECT_CMD	0x73
#define READOUT_PROTECT_CMD		0x82
#define READOUT_TEMP_UNPROTECT_CMD	0x91
#define READOUT_PERM_UNPROTECT_CMD	0x92

#define NXCBL_SUCCESS			0x00
#define ERROR_OFFSET			0x00

#define COM_ERROR_OFFSET		ERROR_OFFSET + 0x00
#define NO_CON_AVAILABLE		COM_ERROR_OFFSET + 0x01
#define COM_ALREADY_OPENED		COM_ERROR_OFFSET + 0x02
#define CANT_OPEN_COM			COM_ERROR_OFFSET + 0x03
#define SEND_FAIL			COM_ERROR_OFFSET + 0x04
#define READ_FAIL			COM_ERROR_OFFSET + 0x05

#define SYS_MEM_ERROR_OFFSET		ERROR_OFFSET + 0x10
#define CANT_INIT_BL			SYS_MEM_ERROR_OFFSET + 0x01
#define UNREOGNIZED_DEVICE		SYS_MEM_ERROR_OFFSET + 0x02
#define CMD_NOT_ALLOWED			SYS_MEM_ERROR_OFFSET + 0x03
#define CMD_FAIL			SYS_MEM_ERROR_OFFSET + 0x04

#define PROGRAM_ERROR_OFFSET		ERROR_OFFSET + 0x20
#define INPUT_PARAMS_ERROR		PROGRAM_ERROR_OFFSET + 0x01
#define INPUT_PARAMS_MEMORY_ALLOCATION_ERROR	PROGRAM_ERROR_OFFSET + 0x02

#define CTRUE	1
#define CFALSE	0

#define NX_UART_FIFO_DEPTH	32

#define UTRSR_MASK		0x03
#define UTIIR_MASK		0x1f
#define UTFR_MASK		0xff
#define UTDMACR_MASK		0x3f

#define UARTDR_MASK		0xff
#define UARTECR_MASK		0xff
#define UARTRSR_MASK		0x0f
#define UARTLCR_H_MASK		0x7f
#define UARTLCR_M_MASK		0xff
#define UARTLCR_L_MASK		0xff
#define UARTCR_MASK		0x7f
#define UARTCR_newMASK		0xffff
#define UARTFR_MASK		0xf8
#define UARTILPR_MASK		0xff
#define UARTIBRD_MASK		0xffff
#define UARTFBRD_MASK		0x3f
#define UARTLCR_H_new_MASK	0xff
#define UARTIFLS_MASK		0x3f
#define UARTIMSC_MASK		0x7ff
#define UARTRIS_MASK		0xfff
#define UARTMIS_MASK		0xfff
#define UARTICR_MASK		0x7ff
#define UARTDMACR_MASK		0x07
#define MASK_IDREG		0x0ff

#define UARTTCR_MASK		0x07
#define UARTITIP_MASK		0xff
#define UARTITOP_MASK		0xffff
#define UARTTDR_MASK		0xfff

#define RSTMODE_ENABLE		0x01
#define PCLK_ENABLE		0x02
#define PCLKGEN			0x03
#define REFCLKGEN		0x04
#define REFCLKON		0x05
#define PCLKON			0x06

#define UT_MSINT		0x01
#define UT_RXINT		0x02
#define UT_TXINT		0x04
#define UT_RTISINT		0x08
#define UT_UARTINTR		0x10
#define UT_UARTEINTR		0x20

#define FORCED_PARITY_ERR	0x01
#define FORCED_FRAMING_ERR	0x02
#define RX_JITTER_BITS		0x0c
#define RX_JITTER_SIGN		0x10
#define TX_JITTER_BITS		0x60
#define TX_JITTER_SIGN		0x80

#define UTRXFIFO_EMPTY		0x01
#define UTRXFIFO_HALFFULL	0x02
#define UTRXFIFO_FULL		0x04
#define UTTXFIFO_EMPTY		0x08
#define UTTXFIFO_HALFEMPTY	0x10
#define UTTXFIFO_FULL		0x20
#define UTTXBUSY		0x40
#define UTRXBUSY		0x80

#define UTPE			0x02

#define TXDMASREQ		0x04
#define RXDMASREQ		0x08
#define TXDMABREQ		0x10
#define RXDMABREQ		0x20

#define UART_CTS		0x01
#define UART_DSR		0x02
#define UART_DCD		0x04
#define UART_UBUSY		0x08
#define UART_RXFE		0x10
#define UART_TXFF		0x20
#define UART_RXFF		0x40
#define UART_TXFE		0x80
#define UART_RI			0x100

#define UART_RXFEIGHT		0x00
#define UART_RXFQUART		0x08
#define UART_RXFHALF		0x10
#define UART_RXF3QUART		0x18
#define UART_RXF7EIGHT		0x20
#define UART_TXFEIGHT		0x00
#define UART_TXFQUART		0x01
#define UART_TXFHALF		0x02
#define UART_TXF3QUART		0x03
#define UART_TXF7EIGHT		0x04

#define UART_RIRINT		0x01
#define UART_CTSRINT		0x02
#define UART_DCDRINT		0x04
#define UART_DSRRINT		0x08
#define UART_RXRINT		0x10
#define UART_TXRINT		0x20
#define UART_RTRINT		0x40
#define UART_FERINT		0x80
#define UART_PERINT		0x100
#define UART_BERINT		0x200
#define UART_OERINT		0x400

#define UART_RIRINT		0x01
#define UART_CTSRINT		0x02
#define UART_DCDRINT		0x04
#define UART_DSRRINT		0x08
#define UART_RXRINT		0x10
#define UART_TXRINT		0x20
#define UART_RTRINT		0x40
#define UART_FERINT		0x80
#define UART_PERINT		0x100
#define UART_BERINT		0x200
#define UART_OERINT		0x400

#define UART_RXDMAE		0x01
#define UART_TXDMAE		0x02
#define DMAONERR		0x04

struct nx_uart_register_set {
	u32 dr;
	u32 rsr_ecr;
	u32 __reserved0[(0x18 - 0x08) / 4];
	u32 fr;
	u32 __reserved1;
	u32 ilpr;
	u32 ibrd;
	u32 fbrd;
	u32 lcr_h;
	u32 cr;
	u32 ifls;
	u32 imsc;
	u32 ris;
	u32 mis;
	u32 icr;
	u32 dmacr;
	u32 __reserved2[(0x80 - 0x4C) / 4];

	u32 tcr;
	u32 itip;
	u32 itop;
	u32 tdr;
	u32 __reserved4[(0xFD0 - 0x90) / 4];
	u32 __reserved5[(0xFE0 - 0xFD0) / 4];
	u32 periph_id0;
	u32 periph_id1;
	u32 periph_id2;
	u32 periph_id3;
	u32 pcellid0;
	u32 pcellid1;
	u32 pcellid2;
	u32 pcellid3;
};

enum { nx_uart_int_rim = 0ul,
	nx_uart_int_ctsm = 1ul,
	nx_uart_int_dcdm = 2ul,
	nx_uart_int_dsrm = 3ul,
	nx_uart_int_rx = 4ul,
	nx_uart_int_tx = 5ul,
	nx_uart_int_rt = 6ul,
	nx_uart_int_fe = 7ul,
	nx_uart_int_pe = 8ul,
	nx_uart_int_be = 9ul,
	nx_uart_int_oe = 10ul };

enum { nx_uart_cr_uarten = 0ul,
	nx_uart_cr_siren = 1ul,
	nx_uart_cr_iirlp = 2ul,
	nx_uart_cr_lpe = 7ul,
	nx_uart_cr_txe = 8ul,
	nx_uart_cr_rxe = 9ul,
	nx_uart_cr_dtr = 10ul,
	nx_uart_cr_rts = 11ul,
	nx_uart_cr_out1 = 12ul,
	nx_uart_cr_out2 = 13ul,
	nx_uart_cr_rtsen = 14ul,
	nx_uart_cr_ctsen = 15ul };

typedef enum {
	NX_UART_PARITYMODE_NONE = 0ul,
	NX_UART_PARITYMODE_ODD = 1ul,
	NX_UART_PARITYMODE_EVEN = 2ul,
	NX_UART_PARITYMODE_FONE = 3ul,
	NX_UART_PARITYMODE_FZ = 4ul
} nx_uart_paritymode;

typedef enum {
	nx_uart_errstat_frame = 1ul << 0,
	nx_uart_errstat_parity = 1ul << 1,
	nx_uart_errstat_break = 1ul << 2,
	nx_uart_errstat_overrun = 1ul << 3
} nx_uart_errstat;

typedef enum {
	NX_UART_FLAG_CTS = 1ul << 0,
	NX_UART_FLAG_DSR = 1ul << 1,
	NX_UART_FLAG_DCD = 1ul << 2,
	NX_UART_FLAG_BUSY = 1ul << 3,
	NX_UART_FLAG_RXFE = 1ul << 4,
	NX_UART_FLAG_TXFF = 1ul << 5,
	NX_UART_FLAG_RXFF = 1ul << 6,
	NX_UART_FLAG_TXFE = 1ul << 7,
	NX_UART_FLAG_ri = 1ul << 8
} NX_UART_FLAG;

typedef enum {
	nx_uart_rxdmae = 1ul << 0,
	nx_uart_txdmae = 1ul << 1,
	nx_uart_dmaonerr = 1ul << 2,
} nx_uart_dma;

typedef enum {
	nx_uart_fifolevel1_8 = (NX_UART_FIFO_DEPTH * 1 / 8),
	nx_uart_fifolevel2_8 = (NX_UART_FIFO_DEPTH * 2 / 8),
	nx_uart_fifolevel4_8 = (NX_UART_FIFO_DEPTH * 4 / 8),
	nx_uart_fifolevel6_8 = (NX_UART_FIFO_DEPTH * 6 / 8),
	nx_uart_fifolevel7_8 = (NX_UART_FIFO_DEPTH * 7 / 8),
	nx_uart_fifolevel_err = 0xFFFFFFFFUL
} nx_uart_fifolevel;

typedef enum {
	nx_uart_databit_5 = 0ul,
	nx_uart_databit_6 = 1ul,
	nx_uart_databit_7 = 2ul,
	nx_uart_databit_8 = 4ul,
	nx_uart_databit_err = 0xFFFFFFFFUL
} nx_uart_databit;





enum  ACKS {UNDEFINED = 0x00, NXC75 = 0x75, NXC100 = 0x79};

typedef u8	BYTE;
typedef u8	*LPBYTE;
typedef u16	USHORT;
typedef u16	WORD;
typedef u32	DWORD;

typedef struct {
	BYTE Version;
	BYTE CmdCount;
	BYTE PIDLen;
	BYTE *PID;

	BYTE ROPE;
	BYTE ROPD;
} TARGET_DESCRIPTOR;

typedef struct {
	BYTE	_cmd;
	DWORD	_address;
	WORD	_length;
	BYTE	_nbSectors;
	TARGET_DESCRIPTOR *_target;
	BYTE	*_data;
	WORD	_wbSectors;
} NXCBL_Request;

#define MAX_DATA_SIZE	128  /* Packet size(in byte) */

BYTE ACK		= 0x79;
BYTE NACK		= 0x1F;
BYTE ACK_VALUE	= NXC100;

#define NUMBER_OF_UART_MODULE	5
#define NUMBER_OF_RESET_MODULE_PIN	69
static struct {
	struct nx_uart_register_set *pregister;
} __g_module_variables[NUMBER_OF_UART_MODULE] = {
	{ (struct nx_uart_register_set *)(PHY_BASEADDR_UART0)},
	{ (struct nx_uart_register_set *)(PHY_BASEADDR_UART1)},
	{ (struct nx_uart_register_set *)(PHY_BASEADDR_UART2)},
	{ (struct nx_uart_register_set *)(PHY_BASEADDR_UART3)},
	{ (struct nx_uart_register_set *)(PHY_BASEADDR_UART4)},
};
int nx_uart_initialize(void)
{
	return true;
}
u32 nx_uart_get_number_of_module(void) { return (u32) NUMBER_OF_UART_MODULE; }
#ifdef NUMBER_OF_UART_MODULE
u32 nx_uart_get_physical_address(u32 module_index)
{
	return (u32)__g_module_variables[module_index].pregister;
}
#endif

u32 nx_uart_get_number_of_reset(void)
{
	return (u32)NUMBER_OF_RESET_MODULE_PIN;
}

u32 nx_uart_get_size_of_register_set(void)
{
	return sizeof(struct nx_uart_register_set);
}

void nx_uart_set_base_address(u32 module_index, void *base_address)
{
	__g_module_variables[module_index].pregister =
		(struct nx_uart_register_set *)base_address;
}

void *nx_uart_get_base_address(u32 module_index)
{
	return (void *)__g_module_variables[module_index].pregister;
}
int nx_uart_open_module(u32 module_index)
{
	register struct nx_uart_register_set *pregister;
	pregister = __g_module_variables[module_index].pregister;

	writel(0, &pregister->rsr_ecr);
	writel(0x60, &pregister->lcr_h);
	writel(0, &pregister->cr);
	writel(0, &pregister->dmacr);
	writel(0, &pregister->imsc);
	return true;
}
int nx_uart_close_module(u32 module_index)
{
	register struct nx_uart_register_set *pregister;
	pregister = __g_module_variables[module_index].pregister;
	writel(0, &pregister->rsr_ecr);
	writel(0x60, &pregister->lcr_h);
	writel(0, &pregister->cr);
	writel(0, &pregister->dmacr);
	writel(0, &pregister->imsc);
	return true;
}
int nx_uart_check_busy(u32 module_index) { return false; }
int nx_uart_can_power_down(u32 module_index) { return true; }
u32 nx_uart_get_clock_number(u32 module_index)
{
	const u32 clock_number[] = {
		CLK_ID_UART_0,
		CLK_ID_UART_1,
		CLK_ID_UART_2,
		CLK_ID_UART_3,
		CLK_ID_UART_4,
	};
	return clock_number[module_index];
}
u32 nx_uart_get_reset_number(u32 module_index)
{
	const u32 reset_number[] = {
		RESET_ID_UART0,
		RESET_ID_UART1,
		RESET_ID_UART2,
		RESET_ID_UART3,
		RESET_ID_UART4,
	};
	return reset_number[module_index];
}

u32 nx_uart_get_interrupt_number(u32 module_index)
{
	const u32 uart_interrupt_number[NUMBER_OF_UART_MODULE] = {
		7, 6, 8, 9, 10,
	};
	return uart_interrupt_number[module_index];
}

void nx_uart_set_interrupt_enable(u32 module_index, u32 int_num, int enable)
{
	const u32 pend_pos = 0;
	const u32 pend_mask = 0x7FF << pend_pos;
	register struct nx_uart_register_set *pregister;
	register u32 read_value;
	pregister = __g_module_variables[module_index].pregister;
	read_value = readl(&pregister->imsc) & pend_mask;
	read_value &= (u32)(~(1ul << (int_num)));
	read_value |= (u32)enable << (int_num);
	writel(read_value, &pregister->imsc);
}

int nx_uart_get_interrupt_enable(u32 module_index, u32 int_num)
{
	return (int)((readl(&__g_module_variables[module_index].pregister->imsc)
				>> int_num) & 0x01);
}
void nx_uart_set_interrupt_enable32(u32 module_index, u32 enable_flag)
{
	const u32 enb_pos = 0;

	writel((u16)(enable_flag << enb_pos), &__g_module_variables[module_index].pregister->imsc);
}

u32 nx_uart_get_interrupt_enable32(u32 module_index)
{
	const u32 enb_pos = 0;
	const u32 enb_mask = 0x7FF << enb_pos;
	return (u32)((readl(&__g_module_variables[module_index].pregister->imsc)
				& enb_mask) >> enb_pos);
}
int nx_uart_get_interrupt_pending(u32 module_index, u32 int_num)
{
	return (int)((readl(&__g_module_variables[module_index].pregister->mis)
				>> int_num) & 0x01);
}
u32 nx_uart_get_interrupt_pending32(u32 module_index)
{
	const u32 pend_mask = 0x7FF;
	return (u32)(readl(&__g_module_variables[module_index].pregister->mis)
			& pend_mask);
}
void nx_uart_clear_interrupt_pending(u32 module_index, u32 int_num)
{
	const u32 pend_pos = 0;
	const u32 pend_mask = 0x7FF << pend_pos;
	register struct nx_uart_register_set *pregister;
	pregister = __g_module_variables[module_index].pregister;
	writel((1 << int_num) & pend_mask, &pregister->icr);
}
void nx_uart_clear_interrupt_pending32(u32 module_index, u32 pending_flag)
{
	const u32 pend_pos = 0;
	const u32 pend_mask = 0x7FF << pend_pos;
	register struct nx_uart_register_set *pregister;
	pregister = __g_module_variables[module_index].pregister;
	writel(pending_flag & pend_mask, &pregister->icr);
}
void nx_uart_set_interrupt_enable_all(u32 module_index, int enable)
{
	const u32 int_mask = 0x7FF;
	register u32 set_value;
	set_value = 0;
	if (enable)

	{
		set_value |= int_mask;
	}
	writel(set_value, &__g_module_variables[module_index].pregister->imsc);
}
int nx_uart_get_interrupt_enable_all(u32 module_index)
{
	const u32 int_mask = 0x7FF;
	if (readl(&__g_module_variables[module_index].pregister->imsc)
			& int_mask) {
		return true;
	}
	return false;
}
int nx_uart_get_interrupt_pending_all(u32 module_index)
{
	const u32 pend_mask = 0x7FF;
	if (readl(&__g_module_variables[module_index].pregister->imsc)
			& pend_mask) {
		return true;
	}
	return false;
}
void nx_uart_clear_interrupt_pending_all(u32 module_index)
{
	const u32 pend_pos = 0;
	const u32 pend_mask = 0x7FF << pend_pos;
	register struct nx_uart_register_set *pregister;
	pregister = __g_module_variables[module_index].pregister;
	writel(pend_mask, &pregister->icr);
}
u32 nx_uart_get_interrupt_pending_number(u32 module_index)
{
	const u32 pend_pos = 0;
	const u32 pend_mask = 0x7FF << pend_pos;
	register u32 pending_index = 0;
	register struct nx_uart_register_set *pregister;
	register u32 pend;
	pregister = __g_module_variables[module_index].pregister;
	pend = readl(&pregister->mis) & pend_mask;

	for (pending_index = 0; pending_index <= 10; pending_index++)
		if (pend & ((u32)0x1) << pending_index)
			break;
	if (pending_index > 10)
		return -1;
	else
		return pending_index;
}
u32 nx_uart_get_dmaindex_tx(u32 module_index)
{
	const u32 uart_dma_index_tx[3] = {
		0, 2, 4,
	};
	return uart_dma_index_tx[module_index];
}
u32 nx_uart_get_dmaindex_rx(u32 module_index)
{
	const u32 uart_dma_index_rx[3] = {
		1, 3, 5,
	};
	return uart_dma_index_rx[module_index];
}
u32 nx_uart_get_dmabus_width(u32 module_index) { return 8; }
void nx_uart_set_sirmode(u32 module_index, int benb)
{
	const u32 sir_mode_mask = (1 << 1);
	register u32 set_value;
	set_value = readl(&__g_module_variables[module_index].pregister->cr);
	if (1 == benb)
		set_value = (u16)(set_value | sir_mode_mask);
	else
		set_value &= ~sir_mode_mask;
	writel(set_value, &__g_module_variables[module_index].pregister->cr);
}
int nx_uart_get_sirmode(u32 module_index)
{
	const u32 sir_mode_mask = (1 << 1);
	if (readl(&__g_module_variables[module_index].pregister->cr)
			& sir_mode_mask) {
		return true;
	} else {
		return false;
	}
}
void nx_uart_set_loop_back_mode(u32 module_index, int benb)
{
	const u32 loopback_mode_mask = (1 << 7);
	register u32 set_value;
	set_value = readl(&__g_module_variables[module_index].pregister->cr);
	if (1 == benb)
		set_value = (u16)(set_value | loopback_mode_mask);
	else
		set_value &= ~loopback_mode_mask;
	writel(set_value, &__g_module_variables[module_index].pregister->cr);
}
int nx_uart_get_loop_back_mode(u32 module_index)
{
	const u32 loopback_mode_mask = (1 << 7);
	if (readl(&__g_module_variables[module_index].pregister->cr)
			& loopback_mode_mask) {
		return true;
	} else {
		return false;
	}
}
void nx_uart_set_int_enb_when_exception_occur(u32 module_index, int benb)
{
	const u32 exception_mask = (0xFUL << 7);
	register u32 set_value;
	set_value = readl(&__g_module_variables[module_index].pregister->imsc);
	if (1 == benb)
		set_value |= exception_mask;
	else
		set_value &= exception_mask;
	writel(set_value, &__g_module_variables[module_index].pregister->imsc);
}
int nx_uart_get_int_enb_when_exception_occur(u32 module_index)
{
	const u32 exception_mask = (0xFUL << 7);
	if (readl(&__g_module_variables[module_index].pregister->imsc)
			& exception_mask) {
		return true;
	} else {
		return false;
	}
}
void nx_uart_set_txenable(u32 module_index, int benb)
{
	const u32 txenb = (1 << 8);
	register u32 set_value;
	set_value = readl(&__g_module_variables[module_index].pregister->cr);
	if (1 == benb)
		set_value = (u16)(set_value | txenb);
	else
		set_value &= ~txenb;
	writel(set_value, &__g_module_variables[module_index].pregister->cr);
}

int nx_uart_get_txenable(u32 module_index)
{
	const u32 txenb = (1 << 8);
	if (readl(&__g_module_variables[module_index].pregister->cr) & txenb)
		return true;
	else
		return false;
}

void nx_uart_set_rxenable(u32 module_index, int benb)
{
	const u32 txenb = (1 << 9);
	register u32 set_value;
	set_value = readl(&__g_module_variables[module_index].pregister->cr);
	if (1 == benb)
		set_value = (u16)(set_value | txenb);
	else
		set_value &= ~txenb;
	writel(set_value, &__g_module_variables[module_index].pregister->cr);
}
int nx_uart_get_rxenable(u32 module_index)
{
	const u32 txenb = (1 << 9);
	if (readl(&__g_module_variables[module_index].pregister->cr) & txenb)
		return true;
	else
		return false;
}
void nx_uart_set_frame_configuration(u32 module_index,
		nx_uart_paritymode parity, u32 data_width, u32 stop_bit)
{
	register u16 temp;
	const u32 parity_bitpos = 1;
	const u32 evenp_bitpos = 2;
	const u32 stickp_bitpos = 7;
	const u32 stop_bitpos = 3;
	const u32 data_bitpos = 5;
	temp = (u16)(readl(&__g_module_variables[module_index].pregister->lcr_h));
	temp = (u16)((u16)((stop_bit - 1) << stop_bitpos) |
			(u16)((data_width - 5) << data_bitpos));
	if (parity != 0) {
		temp |= (u16)1 << parity_bitpos;
		if (parity == 2)
			temp |= (u16)1 << evenp_bitpos;
		else if (parity == 3)
			temp |= (u16)1 << stickp_bitpos;
		else if (parity == 4)
			temp |=	((u16)1 << evenp_bitpos | (u16)1 << stickp_bitpos);
	}

	writel(temp, &__g_module_variables[module_index].pregister->lcr_h);
}
void nx_uart_get_frame_configuration(u32 module_index,
		nx_uart_paritymode *pparity, u32 *pdatawidth, u32 *pstopbit)
{
	const u32 parity_bitpos = 1;
	const u32 evenp_bitpos = 2;
	const u32 stickp_bitpos = 7;
	const u32 wlen_bitpos = 5;
	const u32 wlen_mask = (0x03 << wlen_bitpos);
	const u32 stp2_bitpos = 3;
	const u32 stp2_mask = (0x01 << stp2_bitpos);
	register u32 temp;
	temp = readl(&__g_module_variables[module_index].pregister->lcr_h);
	*pparity = (nx_uart_paritymode)0;
	if (temp & 1 << parity_bitpos) {
		*pparity = (nx_uart_paritymode)1;

		if (temp & 1 << stickp_bitpos)
			*pparity = (nx_uart_paritymode)((int)(*pparity) + 2);
		if (temp & 1 << evenp_bitpos)
			*pparity = (nx_uart_paritymode)((int)(*pparity) + 1);
	}
	*pdatawidth = ((temp & wlen_mask) >> wlen_bitpos) + 5;
	*pstopbit = ((temp & stp2_mask) >> stp2_bitpos) + 1;
}
void nx_uart_set_brd(u32 module_index, u16 brd)
{
	writel(brd, &__g_module_variables[module_index].pregister->ibrd);
}
u16 nx_uart_get_brd(u32 module_index)
{
	return (u16)readl(&__g_module_variables[module_index].pregister->ibrd);
}

void nx_uart_set_fbrd(u32 module_index, u16 fbrd)
{
	writel(fbrd, &__g_module_variables[module_index].pregister->fbrd);
}
u16 nx_uart_get_fbrd(u32 module_index)
{
	return (u16)readl(&__g_module_variables[module_index].pregister->fbrd);
}

u16 nx_uart_make_brd(u32 baud_rate, u32 clkinhz)
{
	clkinhz /= baud_rate;
	clkinhz /= 16;
	return (u16)clkinhz;
}
u16 nx_uart_make_fbrd(u32 baud_rate, u32 clkinhz)
{
	clkinhz %= (baud_rate * 16 + 16 / 2);
	clkinhz <<= 6;
	clkinhz += 32;
	clkinhz /= (baud_rate * 16 + 16 / 2);
	return (u16)clkinhz;
}
int nx_uart_get_rx_time_out_enb(u32 module_index)
{
	const u32 rxtime_mask = (0x01 << 6);
	if (readl(&__g_module_variables[module_index].pregister->imsc)
			& rxtime_mask) {
		return true;
	} else {
		return false;
	}
}
void nx_uart_send_break(u32 module_index)
{
	const u32 sendbreak_mask = (1 << 0);
	register u32 set_value;
	set_value =
	    readl(&__g_module_variables[module_index].pregister->lcr_h) |
	    sendbreak_mask;
	writel(set_value, &__g_module_variables[module_index].pregister->lcr_h);
}
void nx_uart_set_tx_dmamode(u32 module_index, int benb)
{
	const u32 txopermode_bitpos = 1;
	const u32 txopermode_mask = (1 << txopermode_bitpos);
	register u32 temp;
	temp = readl(&__g_module_variables[module_index].pregister->dmacr);
	if (benb)
		temp |= ((u32)1ul << txopermode_bitpos);
	else
		temp &= ~txopermode_mask;
	writel((u16)temp, &__g_module_variables[module_index].pregister->dmacr);
}
int nx_uart_get_tx_dmamode(u32 module_index)
{
	const u32 txopermode_bitpos = 1;
	const u32 txopermode_mask = (1 << txopermode_bitpos);
	return (int)((__g_module_variables[module_index].pregister->dmacr &
		      txopermode_mask) >>
		     txopermode_bitpos);
}
void nx_uart_set_rx_dmamode(u32 module_index, int benb)
{
	const u32 rxopermode_mask = (1 << 0);
	register u32 temp;
	temp = readl(&__g_module_variables[module_index].pregister->dmacr);
	if (benb)

		temp |= rxopermode_mask;
	else
		temp &= ~rxopermode_mask;
	writel((u16)temp, &__g_module_variables[module_index].pregister->dmacr);
}
int nx_uart_get_rx_dmamode(u32 module_index)
{
	const u32 rxopermode_mask = (1 << 0);
	const u32 rxopermode_bitpos = 0;
	return (
	    int)((readl(&__g_module_variables[module_index].pregister->dmacr) &
		  rxopermode_mask) >>
		 rxopermode_bitpos);
}
void nx_uart_set_uartenable(u32 module_index, int benb)
{
	const u32 uarten_bitpos = 0;
	const u32 uarten_mask = (1 << uarten_bitpos);
	register u32 temp;
	temp = readl(&__g_module_variables[module_index].pregister->cr);
	if (benb)

		temp |= uarten_mask;
	else
		temp &= ~uarten_mask;
	writel(temp, &__g_module_variables[module_index].pregister->cr);
}
int nx_uart_get_uartenable(u32 module_index)
{
	const u32 uarten_bitpos = 0;
	const u32 uarten_mask = (1 << uarten_bitpos);
	return (int)((readl(&__g_module_variables[module_index].pregister->cr) &
		      uarten_mask) >>
		     uarten_bitpos);
}
void nx_uart_set_tx_fifotrigger_level(u32 module_index, nx_uart_fifolevel level)
{
	const u32 txfifotl_bitpos = 0;
	const u32 txfifotl_mask = (7 << txfifotl_bitpos);
	register u32 temp;
	register u32 set_value = 0;

	switch (level) {
	case nx_uart_fifolevel1_8:
		set_value = 0;
		break;
	case nx_uart_fifolevel2_8:
		set_value = 1;
		break;
	case nx_uart_fifolevel4_8:
		set_value = 2;
		break;
	case nx_uart_fifolevel6_8:
		set_value = 3;
		break;
	case nx_uart_fifolevel7_8:
		set_value = 4;
		break;
	case nx_uart_fifolevel_err:
		return;
	}
	temp = readl(&__g_module_variables[module_index].pregister->ifls);
	temp &= ~txfifotl_mask;
	temp |= ((u32)set_value << txfifotl_bitpos);
	writel((u16)temp, &__g_module_variables[module_index].pregister->ifls);
}

nx_uart_fifolevel nx_uart_get_tx_fifotrigger_level(u32 module_index)
{
	const u32 txfifotl_bitpos = 0;
	const u32 txfifotl_mask = (7 << txfifotl_bitpos);
	register u32 read_value;
	read_value =
	    (readl(&__g_module_variables[module_index].pregister->ifls) &
	     txfifotl_mask) >>
	    txfifotl_bitpos;
	switch (read_value) {
	case 0:
		return nx_uart_fifolevel1_8;
	case 1:
		return nx_uart_fifolevel2_8;
	case 2:
		return nx_uart_fifolevel4_8;
	case 3:
		return nx_uart_fifolevel6_8;
	case 4:
		return nx_uart_fifolevel7_8;
	}
	return nx_uart_fifolevel_err;
}

void nx_uart_set_rx_fifotrigger_level(u32 module_index, nx_uart_fifolevel level)
{
	const u32 rxfifotl_bitpos = 3;
	const u32 rxfifotl_mask = (7 << rxfifotl_bitpos);
	register u32 temp;
	register u32 set_value = 0;
	switch (level) {
	case nx_uart_fifolevel1_8:
		set_value = 0;
		break;
	case nx_uart_fifolevel2_8:
		set_value = 1;
		break;
	case nx_uart_fifolevel4_8:
		set_value = 2;
		break;
	case nx_uart_fifolevel6_8:
		set_value = 3;
		break;
	case nx_uart_fifolevel7_8:
		set_value = 4;
		break;
	case nx_uart_fifolevel_err:
		return;
	}
	temp = readl(&__g_module_variables[module_index].pregister->ifls);
	temp &= ~rxfifotl_mask;
	temp |= ((u32)set_value << rxfifotl_bitpos);
	writel((u16)temp, &__g_module_variables[module_index].pregister->ifls);
}
nx_uart_fifolevel nx_uart_get_rx_fifotrigger_level(u32 module_index)
{
	const u32 rxfifotl_bitpos = 3;
	const u32 rxfifotl_mask = (7 << rxfifotl_bitpos);
	register u32 read_value;
	read_value =
	    ((readl(&__g_module_variables[module_index].pregister->ifls) &
	      rxfifotl_mask) >>
	     rxfifotl_bitpos);
	switch (read_value)

	{
	case 0:
		return nx_uart_fifolevel1_8;
	case 1:
		return nx_uart_fifolevel2_8;
	case 2:
		return nx_uart_fifolevel4_8;
	case 3:
		return nx_uart_fifolevel6_8;
	case 4:
		return nx_uart_fifolevel7_8;
	}
	return nx_uart_fifolevel_err;
}

void nx_uart_set_fifoenb(u32 module_index, int benb)
{
	const u32 fifoen_mask = (1 << 4);
	register u32 set_value;
	set_value = readl(&__g_module_variables[module_index].pregister->lcr_h);
	if (1 == benb)
		set_value = (u16)(set_value | fifoen_mask);
	else
		set_value &= ~fifoen_mask;
	writel(set_value, &__g_module_variables[module_index].pregister->lcr_h);
}
int nx_uart_get_fifoenb(u32 module_index)
{
	const u32 fifoen_mask = (1 << 4);
	if (__g_module_variables[module_index].pregister->lcr_h & fifoen_mask)
		return true;
	else
		return false;
}
u32 nx_uart_get_tx_rx_status(u32 module_index)
{
	const u32 txrxst_bitpos = 4;
	const u32 txrxst_mask = (0x0F << txrxst_bitpos);
	return (u32)(readl(&__g_module_variables[module_index].pregister->fr) &
		     txrxst_mask);
}
u32 nx_uart_get_error_status(u32 module_index)
{
	return (u32)(
	    readl(&__g_module_variables[module_index].pregister->rsr_ecr) &
	    0xFUL);
}
void nx_uart_clear_error_status(u32 module_index)
{
	writel(0xFF, &__g_module_variables[module_index].pregister->rsr_ecr);
}

void nx_uart_set_dtr(u32 module_index, int bactive)
{
	const u32 dtr_mask = (1 << 10);
	register u32 set_value;
	set_value = readl(&__g_module_variables[module_index].pregister->cr);
	if (1 == bactive)
		set_value = (u16)(set_value | dtr_mask);
	else
		set_value &= ~dtr_mask;
	writel(set_value, &__g_module_variables[module_index].pregister->cr);
}
int nx_uart_get_dtr(u32 module_index)
{
	const u32 dtr_mask = (1 << 10);
	if (readl(&__g_module_variables[module_index].pregister->cr) &
	    dtr_mask) {
		return true;
	} else {
		return false;
	}
}
void nx_uart_set_rts(u32 module_index, int bactive)
{
	const u32 rts_mask = (1 << 11);
	register u32 set_value;
	set_value = readl(&__g_module_variables[module_index].pregister->cr);
	if (1 == bactive)
		set_value = (u16)(set_value | rts_mask);
	else
		set_value &= ~rts_mask;
	writel(set_value, &__g_module_variables[module_index].pregister->cr);
}
int nx_uart_get_rts(u32 module_index)
{
	const u32 rts_mask = (1 << 11);
	if (readl(&__g_module_variables[module_index].pregister->cr) &
	    rts_mask)
		return true;
	else
		return false;
}
void nx_uart_set_auto_flow_control(u32 module_index, int enable)
{
	const u32 afc_mask = (0x03 << 14);
	register u32 temp;
	temp = readl(&__g_module_variables[module_index].pregister->cr);
	if (enable == 1)
		temp |= afc_mask;
	else
		temp &= ~afc_mask;
	writel((u16)temp, &__g_module_variables[module_index].pregister->cr);
}
int nx_uart_get_auto_flow_control(u32 module_index)
{
	const u32 afc_mask = (0x03 << 14);
	if (readl(&__g_module_variables[module_index].pregister->cr) &
	    afc_mask)
		return true;
	else
		return false;
}

u32 nx_uart_get_modem_status(u32 module_index)
{
	return readl(&__g_module_variables[module_index].pregister->fr) &
		0x1FF;
}
void nx_uart_send_byte(u32 module_index, u8 data)
{
	writel(data, &__g_module_variables[module_index].pregister->dr);
}
u8 nx_uart_get_byte(u32 module_index)
{
	return (u8)(readl(&__g_module_variables[module_index].pregister->dr) &
		    0xFF);
}
void nx_uart_set_line_config(u32 module_index, int sirmode,
			     nx_uart_paritymode parity, u32 data_width,
			     u32 stop_bit)
{
	nx_uart_set_sirmode(module_index, sirmode);
	nx_uart_set_frame_configuration(module_index, parity, data_width,
					stop_bit);
}
void nx_uart_set_control_config(u32 module_index, int txoper, int rxoper)
{
	nx_uart_set_tx_dmamode(module_index, txoper);
	nx_uart_set_rx_dmamode(module_index, rxoper);
}
void nx_uart_set_fifoconfig(u32 module_index, int fifoenb, u32 txlevel,
			    u32 rxlevel)
{
	nx_uart_set_fifoenb(module_index, fifoenb);
	nx_uart_set_tx_fifotrigger_level(module_index, txlevel);
	nx_uart_set_rx_fifotrigger_level(module_index, rxlevel);
}
void nx_uart_set_baud_rate_config(u32 module_index, u16 baudrate)
{
	nx_uart_set_brd(module_index, baudrate);
}
void nx_uart_set_modem_config(u32 module_index, int autoflow, int dtr, int rts)
{
	nx_uart_set_auto_flow_control(module_index, autoflow);
	nx_uart_set_dtr(module_index, dtr);
	nx_uart_set_rts(module_index, rts);
}
void nx_uart_get_line_config(u32 module_index, u32 *psirmode,
			     nx_uart_paritymode *pparity, u32 *pdatawidth,
			     u32 *pstopbit)
{
	if (1 == nx_uart_get_sirmode(module_index))
		*psirmode = 1;
	else
		*psirmode = 0;

	nx_uart_get_frame_configuration(module_index, pparity, pdatawidth,
					pstopbit);
}
void nx_uart_get_control_config(u32 module_index, int *ptxoper, int *prxoper)
{
	*ptxoper = nx_uart_get_tx_dmamode(module_index);
	*prxoper = nx_uart_get_rx_dmamode(module_index);
}
void nx_uart_get_fifoconfig(u32 module_index, u32 *pfifoenb, u32 *ptxlevel,
			    u32 *prxlevel)
{
	if (1 == nx_uart_get_fifoenb(module_index))
		*pfifoenb = 1;
	else
		*pfifoenb = 0;
	*ptxlevel = nx_uart_get_tx_fifotrigger_level(module_index);
	*prxlevel = nx_uart_get_rx_fifotrigger_level(module_index);
}
void nx_uart_get_baud_rate_config(u32 module_index, u16 *pbaudrate)
{
	*pbaudrate = nx_uart_get_brd(module_index);
}
void nx_uart_get_modem_config(u32 module_index, u32 *pautoflow, u32 *pdtr,
			      u32 *prts)
{
	if (1 == nx_uart_get_auto_flow_control(module_index))
		*pautoflow = 1;
	else
		*pautoflow = 0;
	if (1 == nx_uart_get_dtr(module_index))
		*pdtr = 1;
	else
		*pdtr = 0;
	if (1 == nx_uart_get_rts(module_index))
		*prts = 1;
	else
		*prts = 0;
}
void nx_uart_set_rsrecr(u32 module_index, u16 value)
{
	writel(value, &__g_module_variables[module_index].pregister->rsr_ecr);
}
u16 nx_uart_get_rsrecr(u32 module_index)
{
	return (u16)(
	    readl(&__g_module_variables[module_index].pregister->rsr_ecr));
}
u16 nx_uart_get_fr(u32 module_index)
{
	return (u16)(readl(&__g_module_variables[module_index].pregister->fr));
}
void nx_uart_set_ilpr(u32 module_index, u16 value)
{
	writel(value, &__g_module_variables[module_index].pregister->ilpr);
}
u16 nx_uart_get_ilpr(u32 module_index)
{
	return (u16)(
	    readl(&__g_module_variables[module_index].pregister->ilpr));
}
void nx_uart_set_lcr_h(u32 module_index, u16 value)
{
	writel(value, &__g_module_variables[module_index].pregister->lcr_h);
}
u16 nx_uart_get_lcr_h(u32 module_index)
{
	return (u16)(
	    readl(&__g_module_variables[module_index].pregister->lcr_h));
}
void nx_uart_set_cr(u32 module_index, u16 value)
{
	writel(value, &__g_module_variables[module_index].pregister->cr);
}
u16 nx_uart_get_cr(u32 module_index)
{
	return (u16)readl(&__g_module_variables[module_index].pregister->cr);
}

void nx_uart_set_ifls(u32 module_index, u16 value)
{
	writel(value, &__g_module_variables[module_index].pregister->ifls);
}
u16 nx_uart_get_ifls(u32 module_index)
{
	return (u16)readl(&__g_module_variables[module_index].pregister->ifls);
}
void nx_uart_set_imsc(u32 module_index, u16 value)
{
	writel(value, &__g_module_variables[module_index].pregister->imsc);
}
u16 nx_uart_get_imsc(u32 module_index)
{
	return (u16)(
	    readl(&__g_module_variables[module_index].pregister->imsc));
}
u16 nx_uart_get_ris(u32 module_index)
{
	return (u16)(readl(&__g_module_variables[module_index].pregister->ris));
}
u16 nx_uart_get_mis(u32 module_index)
{
	return (u16)(readl(&__g_module_variables[module_index].pregister->mis));
}
void nx_uart_set_icr(u32 module_index, u16 value)
{
	writel(value, &__g_module_variables[module_index].pregister->icr);
}
void nx_uart_set_dmacr(u32 module_index, u16 value)
{
	writel(value, &__g_module_variables[module_index].pregister->dmacr);
}
u16 nx_uart_get_dmacr(u32 module_index)
{
	return (u16)(
	    readl(&__g_module_variables[module_index].pregister->dmacr));
}
void nx_uart_set_tcr(u32 module_index, u16 value)
{
	writel(value, &__g_module_variables[module_index].pregister->tcr);
}
u16 nx_uart_get_tcr(u32 module_index)
{
	return (u16)(readl(&__g_module_variables[module_index].pregister->tcr));
}
void nx_uart_set_itip(u32 module_index, u16 value)
{
	writel(value, &__g_module_variables[module_index].pregister->itip);
}
u16 nx_uart_get_itip(u32 module_index)
{
	return (u16)(
	    readl(&__g_module_variables[module_index].pregister->itip));
}
void nx_uart_set_itop(u32 module_index, u16 value)
{
	writel(value, &__g_module_variables[module_index].pregister->itop);
}
u16 nx_uart_get_itop(u32 module_index)
{
	return (u16)(
	    readl(&__g_module_variables[module_index].pregister->itop));
}
void nx_uart_set_tdr(u32 module_index, u16 value)
{
	writel(value, &__g_module_variables[module_index].pregister->tdr);
}
u16 nx_uart_get_tdr(u32 module_index)
{
	return (u16)(readl(&__g_module_variables[module_index].pregister->tdr));
}

static void uart_reset_fifo(u32 channel)
{
	while (!(NX_UART_FLAG_TXFE & nx_uart_get_tx_rx_status(channel)))
		;
	while (!(NX_UART_FLAG_RXFE & nx_uart_get_tx_rx_status(channel)))
		nx_uart_get_byte(channel);
	nx_uart_set_fifoenb(channel, CFALSE);
	nx_uart_set_fifoenb(channel, CTRUE);
}

static int uart_byte_write(u32 channel, u8 data)
{
	int ret = 0;

	nx_uart_send_byte(channel, data);

	uart_reset_fifo(channel);

	return ret;
}

static int uart_write(u32 channel, u8 *buffer, u32 size)
{
	int ret = 0;
	int i = 0;

	for (i = 0; i < size; i++)
		ret = uart_byte_write(channel, *(buffer+i));

	if (ret < 0)
		printf("## \e[31m%s() fail! \e[0m\n", __func__);
	else
		ret = size;

	return ret;
}

static u8 uart_read_byte(u32 channel, ulong time_out_us)
{
	u8 ret = 0;
	ulong time_start_us, time_us = 0;

	register struct nx_uart_register_set *puart;
	puart =
	(struct nx_uart_register_set *)nx_uart_get_base_address(channel);

	time_start_us = timer_get_us();

	while (puart->fr & UART_PL01x_FR_RXFE) {
		if (time_out_us > 0) {
			time_us = timer_get_us();
			if ((time_us - time_start_us) > time_out_us) {
				printf("## \e[31m%s()", __func__);
				printf(" time out !\e[0m\n");
				break;
			}
		}
	}

	ret = nx_uart_get_byte(channel);

	return ret;
}


static int uart_read(u32 channel, u8 *buffer, u32 size, ulong time_out_us)
{
	int i = 0;

	for (i = 0; i < size; i++)
		*(buffer+i) = uart_read_byte(channel, time_out_us);

	return size;
}

static void uart2_pad_init(void)
{
	nx_gpio_set_pad_function(3, 16, 1);
	nx_gpio_set_output_enable(3, 16, 0);
	nx_gpio_set_pad_function(3, 20, 1);
	nx_gpio_set_output_enable(3, 20, 1);
}

static void uart_init(u32 channel, u8 parity)
{
	u32 srcclk;
	struct clk *clk = NULL;

	uart2_pad_init();

	clk = clk_get("nx-uart.2");
	clk_set_rate(clk, 100000000);
	clk_enable(clk);
	srcclk = clk_get_rate(clk);
	nx_rstcon_setrst(RESET_ID_UART2, RSTCON_ASSERT);
	nx_rstcon_setrst(RESET_ID_UART2, RSTCON_NEGATE);

	nx_uart_initialize();
	nx_uart_open_module(channel);

	/* UART clock */
	nx_uart_set_brd(channel,
			nx_uart_make_brd(CFG_SYS_DEBUG_UART_BAUDRATE, srcclk));
	nx_uart_set_fbrd(channel,
			 nx_uart_make_fbrd(CFG_SYS_DEBUG_UART_BAUDRATE,
				 srcclk));

	nx_uart_set_fifoconfig(channel, CFALSE, 4, 4);

	/* Frame Configuration : Data 8 - parity  0 - Stop 1 */
	nx_uart_set_frame_configuration(channel, parity , 8, 1);

	/* UART Mode : Tx, Rx Only */
	nx_uart_set_uartenable(channel, CFALSE);
	nx_uart_set_tx_dmamode(channel, CFALSE);
	nx_uart_set_rx_dmamode(channel, CFALSE);
	nx_uart_set_txenable(channel, CFALSE);
	nx_uart_set_rxenable(channel, CFALSE);
	nx_uart_set_sirmode(channel, CFALSE);
	nx_uart_set_loop_back_mode(channel, CFALSE);
	nx_uart_set_interrupt_enable_all(channel, CFALSE);
	nx_uart_set_sirmode(channel, CFALSE);
	nx_uart_set_loop_back_mode(channel, CFALSE);
	nx_uart_set_txenable(channel, CTRUE);
	nx_uart_set_rxenable(channel, CTRUE);
	nx_uart_set_uartenable(channel, CTRUE);

	nx_uart_set_auto_flow_control(channel, CFALSE);
	/* Modem Interrupt Clear */
	nx_uart_clear_interrupt_pending32(channel, 0x7FF);
	uart_reset_fifo(channel);

	/* Tx Rx Operation : Polling */
	nx_uart_set_interrupt_enable_all(channel, CFALSE);
	nx_uart_clear_interrupt_pending_all(channel);
}

static void nxcbl_pwm_disable(int channel)
{
	pwm_disable(channel);
}

static void nxcbl_pwm_init(int channel)
{
	pwm_init(channel, 0, 0);
	pwm_config(channel, TO_DUTY_NS(50, 8000000), TO_PERIOD_NS(8000000));
	pwm_enable(channel);
}

static int sendData(DWORD lg, LPBYTE data)
{
	return uart_write(CFG_SYS_DEBUG_UART_CH, data, lg);
}

static int receiveData(DWORD lg, LPBYTE data)
{
	return uart_read(CFG_SYS_DEBUG_UART_CH, data, lg, 1000000);
}


BYTE Send_RQ(NXCBL_Request *prq)
{
	int i;
	BYTE datasize = 1;
	BYTE Get_ACK;
	LPBYTE rq_buffer;

	/* put command code in the buffer */
	rq_buffer = (LPBYTE) malloc(2);
	rq_buffer[0] = prq->_cmd;

	if (ACK_VALUE == NXC100) { /* put XOR command code in the buffer */
		rq_buffer[1] = ~prq->_cmd;
		datasize = 2;
	}

	/* Send command code (and its XOR value) */
	if (sendData(datasize, rq_buffer) != datasize) {
		printf("## \e[31m[%s():%s:%d\t]\e[0m Send cmd error:0x%x\n",
				__func__, strrchr(__FILE__, '/') + 1, __LINE__,
				rq_buffer[0]);
		free(rq_buffer);
		return SEND_FAIL;
	}

	free(rq_buffer);
	datasize = 1;

	/* Get ACK (verify if the command was accepted) */
	if (!((ACK_VALUE == NXC75) && (prq->_cmd == GET_VER_ROPS_CMD)))	{
		if (receiveData(1, &Get_ACK) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Receive error:0x%x\n", Get_ACK);
			return READ_FAIL;
		}

		if (Get_ACK != ACK) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Receive ACK error:0x%x\n", Get_ACK);
			return CMD_NOT_ALLOWED;
		}
	}

	switch (prq->_cmd) {
	case GET_CMD:
		/* Get the version and the allowed commands supported by
		   the current version of the boot loader */
		printf("## SendCMD:GET_CMD, ACK:0x%x\n", Get_ACK);
		{
		BYTE TMP_Buffer;

		memset(prq->_target, 0x00, sizeof(TARGET_DESCRIPTOR));
		/* Get number of bytes (Version + commands) */
		if (receiveData(1, &TMP_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Receive error:0x%x\n", TMP_Buffer);
			return READ_FAIL;
		}
		prq->_target->CmdCount = TMP_Buffer;

		/* Get boot loader version */
		if (receiveData(1, &TMP_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf("Receive error:0x%x\n", TMP_Buffer);
			return READ_FAIL;
		}
		prq->_target->Version = TMP_Buffer;

		/* Get supported commands */
		rq_buffer = (LPBYTE) malloc(prq->_target->CmdCount);
		if (receiveData(prq->_target->CmdCount, rq_buffer)
				!= prq->_target->CmdCount) {
			free(rq_buffer);
			printf("## \e[31m[%s():%s:%d\t]\e[0m READ_FAIL\n",
					__func__, strrchr(__FILE__, '/')+1,
					__LINE__);
			return READ_FAIL;
		}

		free(rq_buffer);

		/* Get ACK byte */
		if (receiveData(1, &TMP_Buffer) != 1)
			return READ_FAIL;

		if (TMP_Buffer != ACK) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Receive ACK error:0x%x\n", TMP_Buffer);

			return READ_FAIL;
		}
		}
		break;

	case GET_VER_ROPS_CMD:
		/*Get the BL version and the Read Protection status
		  of the NVM */
		printf("## SendCMD:GET_VER_ROPS_CMD, ACK:0x%x\n", Get_ACK);
		{
		BYTE TMP_Buffer;

		/* Get Version */
		if (receiveData(1, &TMP_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Receive error:0x%x\n",TMP_Buffer);
			return READ_FAIL;
		}
		prq->_target->Version = TMP_Buffer;

		/* Get ROPE */
		if (receiveData(1, &TMP_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Receive error:0x%x\n", TMP_Buffer);
			return READ_FAIL;
		}
		prq->_target->ROPE = TMP_Buffer;

		/* Get ROPD */
		if (receiveData(1, &TMP_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Receive error:0x%x\n", TMP_Buffer);
			return READ_FAIL;
		}
		prq->_target->ROPD = TMP_Buffer;

		/* Get ACK */
		if (receiveData(1, &TMP_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Receive error:0x%x\n", TMP_Buffer);
			return READ_FAIL;
		}

		if (TMP_Buffer != ACK) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
				strrchr(__FILE__, '/')+1, __LINE__);
			printf("Receive ACK error:0x%x\n", TMP_Buffer);
			return READ_FAIL;
		}
		}
		break;

	case GET_ID_CMD: /*Get the chip ID */
		printf("## SendCMD:GET_ID_CMD, ACK:0x%x\n", Get_ACK);
		{
		BYTE TMP_Buffer;

		/* Get PID length */
		if (receiveData(1, &TMP_Buffer) != 1)
			return READ_FAIL;
		prq->_target->PIDLen = TMP_Buffer + 1;

		/* Get PID */
		rq_buffer = (LPBYTE) malloc(prq->_target->PIDLen);
		if (receiveData(prq->_target->PIDLen, rq_buffer)
				!= prq->_target->PIDLen) {
			free(rq_buffer);
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Receive error:0x%x\n", TMP_Buffer);
			return READ_FAIL;
		}

		if (prq->_target->PID) {
			prq->_target->PID =
				(BYTE *)malloc(sizeof(prq->_target->PIDLen));
			memcpy(prq->_target->PID, rq_buffer,
					prq->_target->PIDLen);
		}
		free(rq_buffer);

		/* Get ACK */
		if (receiveData(1, &TMP_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf("Receive error:0x%x\n", TMP_Buffer);
			return READ_FAIL;
		}

		if (TMP_Buffer != ACK) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf("Receive ACK error:0x%x\n", TMP_Buffer);
			return READ_FAIL;
		}
		}
		break;

	case READ_CMD:
		/* Read up to 256 bytes of memory starting from
		   an address specified by the user */
		printf("## SendCMD:READ_CMD, ACK:0x%x\n", Get_ACK);
		{
		BYTE checksum = 0x00;

		rq_buffer = (LPBYTE) malloc(5);

		if (ACK_VALUE == NXC100)
			datasize = 5;
		else
			datasize = 4;

		/* Send Read address and checksum */
		rq_buffer[0] = (prq->_address >> 24) & 0x000000FF;
		checksum = checksum ^ rq_buffer[0];
		rq_buffer[1] = (prq->_address >> 16) & 0x000000FF;
		checksum = checksum ^ rq_buffer[1];
		rq_buffer[2] = (prq->_address >> 8) & 0x000000FF;
		checksum = checksum ^ rq_buffer[2];
		rq_buffer[3] = (prq->_address) & 0x000000FF;
		checksum = checksum ^ rq_buffer[3];
		rq_buffer[4] = checksum;

		if (sendData(datasize, rq_buffer) != datasize) {
			free(rq_buffer);
			printf("## \e[31m[%s():%s:%d\t]\e[0m Send error\n",
					__func__, strrchr(__FILE__, '/')+1,
					__LINE__);
			return SEND_FAIL;
		}

		/* Get ACK */
		if (receiveData(1, rq_buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1,__LINE__);
			printf(" Receive error:0x%x\n", rq_buffer[0]);
			free(rq_buffer);
			return READ_FAIL;
		}

		if (rq_buffer[0] != ACK) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Receive error:0x%x\n", rq_buffer[0]);
			free(rq_buffer);
			return CMD_FAIL;
		}

		if (prq->_length == 0) {
			free(rq_buffer);
		} else {
			/* send data size to be read */
			rq_buffer[0] = prq->_length-1;
			rq_buffer[1] = ~(prq->_length-1);

			if (ACK_VALUE == NXC100)
				datasize = 2;
			else
				datasize = 1;

			if (sendData(datasize, rq_buffer) != datasize) {
				printf("## \e[31m[%s():%s:%d\t]\e[0m",
						__func__,
						strrchr(__FILE__, '/')+1,
						__LINE__);
				printf(" Send error:0x%x\n", rq_buffer[0]);
				free(rq_buffer);
				return SEND_FAIL;
			}

			/* Get ACK */
			if (ACK_VALUE == NXC100) {
				if (receiveData(1, rq_buffer) != 1) {
					printf("## \e[31m[%s():%s:%d\t]\e[0m",
						__func__,
						strrchr(__FILE__, '/')+1,
						__LINE__);
					printf(" Receive error:0x%x\n",
						rq_buffer[0]);
					free(rq_buffer);
					return READ_FAIL;
				}

				if (rq_buffer[0] != ACK) {
					printf("## \e[31m[%s():%s:%d\t]\e[0m",
						 __func__,
						 strrchr(__FILE__, '/')+1,
						 __LINE__);
					printf(" Receive error:0x%x\n",
						rq_buffer[0]);
					free(rq_buffer);
					return CMD_FAIL;
				}
			}

			free(rq_buffer);

			/* Get data */
			if (receiveData(prq->_length, prq->_data)
					!= prq->_length) {
				printf("## \e[31m[%s():%s:%d\t]\e[0m",
						__func__,
						strrchr(__FILE__, '/')+1,
						__LINE__);
				printf(" Receive error\n");
			}
			return READ_FAIL;
		}
		}
		break;

	case GO_CMD:
		/* Jump to an address specified by
		   the user to execute (a loaded) code */
		printf("## SendCMD:GO_CMD, ACK:0x%x\n", Get_ACK);
		{
		BYTE checksum = 0x00;

		if (ACK_VALUE == NXC100)
			datasize = 5;
		else
			datasize = 4;

		rq_buffer = (LPBYTE) malloc(5);

		/* Send Go address and checksum */
		rq_buffer[0] = (prq->_address >> 24) & 0x000000FF;
		checksum = checksum ^ rq_buffer[0];
		rq_buffer[1] = (prq->_address >> 16) & 0x000000FF;
		checksum = checksum ^ rq_buffer[1];
		rq_buffer[2] = (prq->_address >> 8) & 0x000000FF;
		checksum = checksum ^ rq_buffer[2];
		rq_buffer[3] = (prq->_address) & 0x000000FF;
		checksum = checksum ^ rq_buffer[3];
		rq_buffer[4] = checksum;

		if (sendData(datasize, rq_buffer) != datasize) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Send error\n",
					__func__, strrchr(__FILE__, '/')+1,
					__LINE__);
			free(rq_buffer);
			return SEND_FAIL;
		}

		/* Get ACK */
		if (receiveData(1, rq_buffer) != 1) {
			printf("##\e[31m[%s():%s:%d\t]\e[0m",
					__func__, strrchr(__FILE__, '/')+1,
					__LINE__);
			printf(" Receive error:0x%x\n", rq_buffer[0]);
			free(rq_buffer);
			return READ_FAIL;
		}

		free(rq_buffer);
		}
		break;

	case WRITE_CMD:
		/* Write maximum 256 bytes to the RAM or the NVM starting
		   from an address specified by the user */
		if ((prq->_address % 0x400) == 0) {
			printf("## SendCMD:WRITE_CMD, ACK:0x%x,",
					Get_ACK);
			printf(" address:0x%08x, _length:%d\n",
					prq->_address, prq->_length);
		}
		{
		/* Send Read address and checksum */
		BYTE checksum = 0x00;

		if (ACK_VALUE == NXC100)
			datasize = 5;
		else
			datasize = 4;

		rq_buffer = (LPBYTE) malloc(5);

		rq_buffer[0] = (prq->_address >> 24) & 0x000000FF;
		rq_buffer[1] = (prq->_address >> 16) & 0x000000FF;
		rq_buffer[2] = (prq->_address >> 8) & 0x000000FF;
		rq_buffer[3] = (prq->_address) & 0x000000FF;

		checksum = checksum ^ rq_buffer[0];
		checksum = checksum ^ rq_buffer[1];
		checksum = checksum ^ rq_buffer[2];
		checksum = checksum ^ rq_buffer[3];
		rq_buffer[4] = checksum;

		if (sendData(datasize, rq_buffer) != datasize) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Send address error(0x%08x)\n", prq->_address);
			free(rq_buffer);
			return SEND_FAIL;
		}


		/* Get ACK */
		if (receiveData(1, rq_buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Receive Data error:0x%x\n", rq_buffer[0]);
			free(rq_buffer);
			return READ_FAIL;
		}

		if (rq_buffer[0] != ACK) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Receive ACK error:0x%x\n", rq_buffer[0]);
			free(rq_buffer);
			return CMD_FAIL;
		}

		mdelay(5);

		checksum = 0x00;

		/* send data size to be writen */
		rq_buffer[0] = prq->_length-1;
		checksum = rq_buffer[0];

		if (sendData(1, rq_buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Send Data errors\n");
			free(rq_buffer);
			return SEND_FAIL;
		}

		mdelay(1);

		/* Send data */
		if (sendData(prq->_length, prq->_data) != prq->_length) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Send Data error\n");
			free(rq_buffer);
			return SEND_FAIL;
		}

		mdelay(1);

		if (ACK_VALUE == NXC100) {
			for (i = 0; i < prq->_length; i++)
				checksum = checksum ^ prq->_data[i];

			/* send checksum */
			rq_buffer[0] = checksum;

			if (sendData(1, rq_buffer) != 1) {
				printf("## \e[31m[%s():%s:%d\t]\e[0m",
						__func__,
						strrchr(__FILE__, '/')+1,
						__LINE__);
				printf(" Send checksum error:\n");
				free(rq_buffer);
				return SEND_FAIL;
			}
		}

		/* Get ACK */
		if (receiveData(1, rq_buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Receive Data error:0x%x\n", rq_buffer[0]);
			free(rq_buffer);
			return READ_FAIL;
		}

		if (rq_buffer[0] != ACK) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Receive ACK error:0x%x\n", rq_buffer[0]);
			free(rq_buffer);
			return CMD_FAIL;
		}

		free(rq_buffer);
		}
		break;

	case ERASE_EXT_CMD:
		/* Erase from one to all the NVM sectors */
		printf("## SendCMD:ERASE_EXT_CMD, ACK:0x%x\n", Get_ACK);
		{
		if ((prq->_wbSectors  & 0xFF00) != 0xFF00) {
			BYTE checksum = ((prq->_wbSectors)-1) >> 8;
			checksum = checksum ^ ((prq->_wbSectors)-1);

			rq_buffer = (LPBYTE) malloc(prq->_length*2 + 3);
			memset(rq_buffer, 0xFF, prq->_length*2 + 3);

			rq_buffer[0] = ((prq->_wbSectors)-1) >> 8;
			rq_buffer[1] = ((prq->_wbSectors)-1);

			for (i = 2; i <= prq->_wbSectors*2; i += 2) {
				rq_buffer[i] = prq->_data[i-1];
				rq_buffer[i+1] = prq->_data[i-2];

				checksum = checksum ^ prq->_data[i-1];
				checksum = checksum ^ prq->_data[i-2];
			}
			rq_buffer[prq->_wbSectors*2 + 2] = checksum;
		} else {
			rq_buffer = (LPBYTE) malloc(3);
			rq_buffer[0] = prq->_wbSectors >> 8;
			rq_buffer[1] = prq->_wbSectors;
			rq_buffer[2] = rq_buffer[0] ^ rq_buffer[1];
		}

		if (ACK_VALUE == NXC100)
			datasize = 3;
		else
			datasize = 1;

		if (sendData(prq->_length*2 + datasize, rq_buffer)
				!= prq->_length*2 + datasize) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Send Data error:\n");
			free(rq_buffer);
			return SEND_FAIL;
		}

		/* Get ACK */
		if (receiveData(1, rq_buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Receive Data error:0x%x\n", rq_buffer[0]);
			free(rq_buffer);
			return READ_FAIL;
		}

		if (rq_buffer[0] != ACK) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Receive ACK error:0x%x\n", rq_buffer[0]);

			free(rq_buffer);
			return CMD_FAIL;
		}

		free(rq_buffer);
		}
		break;

	case ERASE_CMD:
		/* Erase from one to all the NVM sectors */
		printf("## SendCMD:ERASE_CMD, ACK:0x%x\n", Get_ACK);
		{
		if (prq->_nbSectors != 0xFF) {
			BYTE checksum = (prq->_nbSectors-1);

			rq_buffer = (LPBYTE) malloc(prq->_length + 2);
			memset(rq_buffer, 0xFF, prq->_length + 2);

			rq_buffer[0] = prq->_nbSectors - 1;
			for (i = 1; i <= prq->_nbSectors; i++) {
				rq_buffer[i] = prq->_data[i-1];
				checksum = checksum ^ prq->_data[i-1];
			}
			rq_buffer[prq->_nbSectors + 1] = checksum;
		} else {
			rq_buffer = (LPBYTE) malloc(2);

			rq_buffer[0] = prq->_nbSectors;
			rq_buffer[1] = ~prq->_nbSectors;
		}

		if (ACK_VALUE == NXC100)
			datasize = 2;
		else
			datasize = 1;

		if (sendData(prq->_length + datasize, rq_buffer)
				!= prq->_length + datasize) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Send Data error:\n");
			free(rq_buffer);
			return SEND_FAIL;
		}

		/* Get ACK */
		if (receiveData(1, rq_buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Receive Data error:0x%x\n", rq_buffer[0]);
			free(rq_buffer);
			return READ_FAIL;
		}

		if (rq_buffer[0] != ACK) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m", __func__,
					strrchr(__FILE__, '/')+1, __LINE__);
			printf(" Receive ACK error:0x%x\n", rq_buffer[0]);
			free(rq_buffer);
			return CMD_FAIL;
		}

		free(rq_buffer);
		}
		break;

	case WRITE_PROTECT_CMD:
		/* Enable the write protection in a permanent
		   way for some sectors */
		break;

	case WRITE_TEMP_UNPROTECT_CMD:
		/* Disable the write protection in a temporary
		   way for all NVM sectors  */
		break;

	case WRITE_PERM_UNPROTECT_CMD:
		/* Disable the write protection in a
		   permanent way for all NVM sectors */
		break;

	case READOUT_PROTECT_CMD:
		/* Disable the readout protection in a temporary way */
		break;

	case READOUT_TEMP_UNPROTECT_CMD:
		/* Disable the readout protection in a permanent way */
		break;

	case READOUT_PERM_UNPROTECT_CMD:
		break;
	}

	return NXCBL_SUCCESS;
}

static BYTE nxcbl_read(DWORD address, BYTE size, LPBYTE pdata)
{
	BYTE Result = NXCBL_SUCCESS;
	NXCBL_Request *prq;

	if (!pdata)
		return INPUT_PARAMS_MEMORY_ALLOCATION_ERROR;

	prq = (NXCBL_Request *)malloc(sizeof(NXCBL_Request));
	prq->_target = (TARGET_DESCRIPTOR *)malloc(sizeof(TARGET_DESCRIPTOR));
	prq->_cmd = READ_CMD;
	prq->_address = address;

	prq->_length  = size;

	if (pdata) {
		prq->_data = pdata;
		memset(prq->_data, 0xFF, MAX_DATA_SIZE);
	}

	Result = Send_RQ(prq);
	if (Result == NXCBL_SUCCESS) {
		if (pdata)
			memcpy(pdata, prq->_data, size);
	} else {
		printf("## \e[31m%s():%d fail\e[0m : 0x%x\n",
				__func__, __LINE__, Result);
	}

	free(prq->_target);
	free(prq);
	return Result;
}

static BYTE nxcbl_write(DWORD address, BYTE size, LPBYTE pdata)
{
	BYTE Result = NXCBL_SUCCESS;
	NXCBL_Request *prq;

	if (!pdata)
		return INPUT_PARAMS_MEMORY_ALLOCATION_ERROR;

	prq = (NXCBL_Request *)malloc(sizeof(NXCBL_Request));

	prq->_cmd     = WRITE_CMD;
	prq->_address = address;

	prq->_length  = size;

	prq->_data = pdata;

	Result = Send_RQ(prq);
	if (Result != NXCBL_SUCCESS)
		printf("## \e[31m%s():%d fail\e[0m : 0x%x\n",
				__func__, __LINE__, Result);

	free(prq);
	return Result;
}

static BYTE nxcbl_erase(WORD nb_sectors, LPBYTE p_sectors)
{
	BYTE Result;
	NXCBL_Request *prq;

	if ((!p_sectors) && (nb_sectors != 0xFFFF))
		return INPUT_PARAMS_MEMORY_ALLOCATION_ERROR;

	prq = (NXCBL_Request *)malloc(sizeof(NXCBL_Request));
	prq->_target = (TARGET_DESCRIPTOR *)malloc(sizeof(TARGET_DESCRIPTOR));

	prq->_cmd = ERASE_EXT_CMD;

	if (nb_sectors == 0xFFFF) {
		prq->_wbSectors = 0xFFFF;
		prq->_length = 0;

		Result = Send_RQ(prq);
		if (Result != NXCBL_SUCCESS) {
			printf("## \e[31m%s():%d fail\e[0m : 0x%x\n",
					__func__, __LINE__, Result);
			free(prq->_target);
			free(prq);
			return Result;
		}
	} else {
		WORD nerase = nb_sectors / 10;
		WORD remain = nb_sectors % 10;
		int i = 0;
		int j = 0; /*  This is for WORD */

		if (nerase > 0)	{
			for (i = 0; i < nerase; i++) {
				BYTE Convert[0xff];
				prq->_length = 10;
				prq->_wbSectors = 10;
				prq->_data = (LPBYTE)malloc(10*2);

				for (j = 0; j < 10*2; j++)
					Convert[j] =  p_sectors[i*10*2 + j];
				memcpy(prq->_data, Convert, 10*2);

				Result = Send_RQ(prq);
				if (Result != NXCBL_SUCCESS) {
					printf("## \e[31m%s():%d fail\e[0m",
							__func__, __LINE__);
					printf(" : 0x%x\n", Result);
					free(prq->_target);
					free(prq);
					return Result;
				}
			}
		}

		if (remain > 0) {
			BYTE Convert[0xff];
			prq->_length = remain;
			prq->_wbSectors = remain;
			prq->_data = (LPBYTE)malloc(remain*2);

			for (j = 0; j < remain*2; j++)
				Convert[j] = p_sectors[i*10*2+j];
			memcpy(prq->_data, Convert, remain*2);

			Result = Send_RQ(prq);
			if (Result != NXCBL_SUCCESS) {
				printf("## \e[31m%s():%d fail\e[0m : 0x%x\n",
						__func__, __LINE__, Result);
				free(prq->_target);
				free(prq);
				return Result;
			}
		}
	}

	free(prq->_target);
	free(prq);

	return NXCBL_SUCCESS;
}

static BYTE nxcbl_dnload(DWORD address, LPBYTE pdata, DWORD length,
		bool btruncate_lead_ffffordnload)
{
	int i;
	BYTE   Result = NXCBL_SUCCESS;
	LPBYTE holder = pdata;
	LPBYTE buffer = (LPBYTE) malloc(MAX_DATA_SIZE);
	LPBYTE empty = (LPBYTE) malloc(MAX_DATA_SIZE);

	DWORD nbuffer = (DWORD)(length / MAX_DATA_SIZE);
	DWORD ramain  = (DWORD)(length % MAX_DATA_SIZE);
	DWORD newramain = ramain;

	memset(empty, 0xFF, MAX_DATA_SIZE);

	if (nbuffer > 0) {
		for (i = 1; i <= nbuffer; i++) {
			bool all_ffs = false;

			memset(buffer, 0xFF, MAX_DATA_SIZE);
			memcpy(buffer, pdata, MAX_DATA_SIZE);

			if ((memcmp(empty, buffer, MAX_DATA_SIZE) == 0)
					&& btruncate_lead_ffffordnload)
				all_ffs = true;

			if (!all_ffs) {
				Result =
				nxcbl_write(address, MAX_DATA_SIZE, buffer);
				if (Result != NXCBL_SUCCESS) {
					printf("## \e[31m%s():%d fail\e[0m",
							__func__, __LINE__);
					printf(" : 0x%x\n", Result);
					free(buffer);
					free(empty);
					return Result;
				}
			}

			pdata = pdata + MAX_DATA_SIZE;
			address += MAX_DATA_SIZE;
		}
	}

	if (ramain > 0) {
		bool all_ffs = false;

		memset(buffer, 0xFF, MAX_DATA_SIZE);

		Result = nxcbl_read(address,  newramain, buffer);
		/* removed in version 2.8.0 upon customer
		   case @ end of flash area. */
		if (Result != NXCBL_SUCCESS) {
			printf("## \e[31m%s():%d fail\e[0m : 0x%x\n",
					__func__, __LINE__, Result);
			free(buffer);
			free(empty);
			return Result;
		}

		memcpy(buffer, pdata, ramain);

		if ((memcmp(empty, buffer, ramain) == 0)
				&& btruncate_lead_ffffordnload)
			all_ffs = true;

		if (!all_ffs) {
			Result = nxcbl_write(address, newramain, buffer);
			if (Result != NXCBL_SUCCESS) {
				printf("## \e[31m%s():%d fail\e[0m : 0x%x\n",
						__func__, __LINE__, Result);
				free(buffer);
				free(empty);
				return Result;
			}
		}
	}

	pdata = holder;

	free(buffer);
	free(empty);
	return Result;
}

static int nxcbl_init_bl(void)
{
	int ret = READ_FAIL;
	int i = 0;
	BYTE rdata = 0x0;
	BYTE cmd = INIT_CON;

	sendData(1, &cmd);

	for (i = 0; i < 5; i++) {
		receiveData(1, &rdata);

		if (rdata != 0x0) {
			ACK  = rdata;
			ACK_VALUE = rdata;

			ret = NXCBL_SUCCESS;

			switch (rdata) {
			case 0x75:
				printf("NXC75 used : 0x%x\n", rdata);
				NACK = 0x3F;
				break;

			case 0x79:
				printf("NXC100, used : 0x%x\n", rdata);
				NACK = 0x1F;
				break;

			default:
				printf("Undefined device : 0x%x\n", rdata);
				ACK_VALUE = UNDEFINED;
				ret = UNREOGNIZED_DEVICE;
				break;
			}

			break;
		}
		mdelay(100);
	}
	return ret;
}

static int nxcbl_check_version(u32 channel, char *version)
{
	int ret = 0;
	u8 buffer[10];

	memset(buffer, 0x00, sizeof(u8)*10);

	uart_read(channel, buffer, 2, 1000000);
	buffer[9] = 0;

	printf("nxc100 version : %s\n\n", buffer);

	if (strncmp((char *)buffer, version, 2) != 0) {
		printf("## \e[31m%s():%d Version fail\e[0m :",
				__func__, __LINE__);
		printf("bin_version:%s, read_version:%s\n", version, buffer);
		ret = -1;
	}

	return ret;
}

static void nxcbl_reset(void)
{
	u32 grp = 4 , bit = 15, pad = 0;

	nx_gpio_set_pad_function(grp, bit, pad);
	nx_gpio_set_output_enable(grp, bit, CTRUE);

	nx_gpio_set_output_value(grp, bit, CFALSE);

	mdelay(100);

	nx_gpio_set_output_value(grp, bit, CTRUE);

	mdelay(100);
}

static void nxcbl_bootmode(int mode)
{
	u32 grp = 2, bit = 15, pad = 1;

	nx_gpio_set_pad_function(grp, bit, pad);
	nx_gpio_set_output_enable(grp, bit, CTRUE);

	nx_gpio_set_output_value(grp, bit, mode);
}

static int do_nxc100_fusing(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	int i = 0;
	int res = 0;
	unsigned long addr;

	u8 *p, *pos;
	char *bin_version;
	BYTE Result = NXCBL_SUCCESS;
	DWORD address;
	bool optimize = false;

	printf("\n");

	addr = simple_strtoul(argv[1], NULL, 16);
	if (addr == 1)
		goto down_load;
	if (addr == 2)
		goto erase;
	if (addr == 3) {
		nxcbl_pwm_init(1);
		nxcbl_bootmode(0);
		nxcbl_reset();
		goto normal_boot;
	}

	if (argc != 2) {
		printf("## [%s():%d] \e[31m argc(%d) error.\e[0m\n",
				__func__, __LINE__, argc);
		goto ret_error;
	}

	addr = simple_strtoul(argv[1], NULL, 16);
	if (addr < 0x48000000) {
		printf("## [%s():%d] \e[31mload address(0x%lx) error.\e[0m\n",
				__func__, __LINE__, addr);
		goto ret_error;
	}
	p = (u8 *)addr;

	bin_version = (char *)p;

	uart_init(CFG_SYS_DEBUG_UART_CH, NX_UART_PARITYMODE_NONE);
	nxcbl_pwm_init(1);
	nxcbl_bootmode(0);
	nxcbl_reset();
	printf("load address  : 0x%p\n", p);
	printf("bin version   : %s\n", bin_version);

	if (0 == nxcbl_check_version(CFG_SYS_DEBUG_UART_CH, bin_version))
		goto normal_boot;

	uart_init(CFG_SYS_DEBUG_UART_CH, NX_UART_PARITYMODE_EVEN);

	nxcbl_pwm_disable(1);
	nxcbl_bootmode(1);

	nxcbl_reset();

	Result = nxcbl_init_bl();
	if (Result != NXCBL_SUCCESS)
		goto ret_error;

	Result = nxcbl_erase(0xFFFF, NULL);
	if (Result != NXCBL_SUCCESS)
		goto ret_error;

	p += 0x4;
	address = 0x8000000;
	for (i = 0; i < 32768; i += 0x400) {
		pos = (p+i);
		Result = nxcbl_dnload(address, pos, 0x400, optimize);
		if (Result != NXCBL_SUCCESS) {
			printf("## \e[31m%s():%d fail\e[0m : 0x%x\n",
					__func__, __LINE__, Result);
			goto ret_error;
		}
		address += 0x400;
	}

	nxcbl_pwm_init(1);
	nxcbl_bootmode(0);
	nxcbl_reset();

	printf("## NXC100 Normal Booting\n");

	return res;

ret_error:

	nxcbl_pwm_init(1);
	nxcbl_bootmode(0);
	nxcbl_reset();

	printf("## NXC100 ret_error!!!\n");
	return -1;

down_load:
	printf("## NXC100 Download Mode\n");

	nxcbl_pwm_disable(1);
	nxcbl_bootmode(1);
	nxcbl_reset();
	return 0;

erase:
	printf("## NXC100 Erase All\n");

	uart_init(CFG_SYS_DEBUG_UART_CH, NX_UART_PARITYMODE_EVEN);

	nxcbl_pwm_disable(1);
	nxcbl_bootmode(1);
	nxcbl_reset();

	nxcbl_init_bl();
	nxcbl_erase(0xFFFF, NULL);

	nxcbl_pwm_init(1);
	nxcbl_bootmode(0);
	nxcbl_reset();
	return 0;

normal_boot:
	printf("## NXC100 Normal Booting\n");
	return 0;
}


U_BOOT_CMD(
	nxc100,	3,	1,	do_nxc100_fusing,
	"nxc100 fusing from memory",
	" <addr>\n"
	"    - addr : nxc100 image load address\n"
	"  ex> nxc100 0x48000000\n"
	"\n"
	"  etc> nxc100 1  : Download Mode\n"
	"       nxc100 2  : Erase\n"
	"       nxc100 3  : Normal Booting\n"
);
