#include <config.h>
#include <common.h>

#define msleep(a) udelay(a * 1000)

/* SOI */
#define PL_ARM_SOI_0         (0xFF)  /* 0 */
#define PL_ARM_SOI_1         (0xAA)  /* 1 */

/* CID: COMMAND  VERSION  MARK */
/* PL_ARM_COMMAND */                /* 2 */
#define PL_ARM_VERSION       (0x00)  /* 3 */
#define PL_ARM_MASK          (0x00)  /* 4 */

/* LENGTH : LCHKSUM  LENID */
/* LCHKSUM:LENGTH[D15-D12] */        /* 5 */
/* LENID:LENGTH[D11-D0] */            /* 6 */
#define LCHKBITS(LENID,SFBIT) ((u8) ((LENID >>SFBIT)&0x0F))
#define LCHKSUM(LENID) \
	((u8) ((~(LCHKBITS(LENID,8)+LCHKBITS(LENID,4)+LCHKBITS(LENID,0))+1)&0x0F))
#define LENGTH_ID_MASK         (0x0FFF)
#define LENGTH_CHKSUM_MASK     (0xF000)
#define LENGTH_CHKSUM_SHIFT    (12)

/* INFO */

/* CHKSUM: except SOI  CHKSUM  EOI */  /* 7 */

/* EOI */
#define PL_ARM_EOI           (0x0A)  /* 8 */

#define PL_ARM_MIN_SIZE      (0x09)
#define PL_ARM_MAX_SIZE      (32)

/* offset */
#define PL_INDEX_ARM_SOI_0   (0x00) /* */
#define PL_INDEX_ARM_SOI_1   (0x01) /* */
#define PL_INDEX_ARM_COMMAND (0x02) /* */
#define PL_INDEX_ARM_VERSION (0x03) /* */
#define PL_INDEX_ARM_MASK    (0x04) /* */
#define PL_INDEX_ARM_LEN     (0x05) /* LENGTH[D15-D0] */
#define PL_INDEX_ARM_INFO    (0x07) /* */
#define PL_INDEX_ARM_EOI     (0x08) /* */

static u8 Protocol_Check (u8 *p_src, u32 len)
{
	u32 i;
	u8 sum = 0;

	for (i = 0; i < len; i++)
	{
		sum += *(p_src + i);
	}
	return sum;
}

static void mcu_serial_txs(unsigned char *s, int len)
{
	int i;

	for (i = 0; i < len; i++)
		serial_mcu_putc(*(s + i));
}

static int read_uart_fifo(unsigned char *buffer, int index)
{
	int count = index;

	while (serial_mcu_tstc() && count < PL_ARM_MAX_SIZE)
		buffer[count++] = serial_mcu_getc();

	return count;
}

static void clean_buffer(unsigned char *buffer, int index)
{
	int i;

	for (i = 0; i < index; i++)
		buffer[i] = 0;
}

void mcu_serial_tx_data(unsigned char cmd,
	unsigned char *cdata, int clen)
{
	u8 i;
	unsigned char send_buf[PL_ARM_MAX_SIZE] = {0x00};
	int index;
	int len = PL_ARM_MIN_SIZE;

	if (clen > PL_ARM_MAX_SIZE - PL_ARM_MIN_SIZE) {
        clen = (int)(PL_ARM_MAX_SIZE - PL_ARM_MIN_SIZE);
	}

	/* SOI */
	send_buf[PL_INDEX_ARM_SOI_0] = PL_ARM_SOI_0;
	send_buf[PL_INDEX_ARM_SOI_1] = PL_ARM_SOI_1;

	/* CID */
	send_buf[PL_INDEX_ARM_COMMAND] = cmd;
	send_buf[PL_INDEX_ARM_VERSION] = PL_ARM_VERSION;
	send_buf[PL_INDEX_ARM_MASK] = PL_ARM_MASK;

	/* LENGTH */
	len += clen;
	len &= LENGTH_ID_MASK;
	//LCHKSUM
	send_buf[PL_INDEX_ARM_LEN] = LCHKSUM(len);
	send_buf[PL_INDEX_ARM_LEN] <<= 4;
	//LENID
	send_buf[PL_INDEX_ARM_LEN] |= LCHKBITS(len, 8);
	send_buf[PL_INDEX_ARM_LEN + 1] = (u8)(len & 0x00FF);

	/* INFO */
	index = PL_INDEX_ARM_INFO;
	if (cdata)
		for (i = 0; i < clen; i++)
			send_buf[index++] = cdata[i];

	/* CHKSUM : except SOI  CHKSUM  EOI*/
	send_buf[index++] =
	Protocol_Check(&send_buf[PL_INDEX_ARM_COMMAND], len - 4);

    /* EOI */
	send_buf[index] = PL_ARM_EOI;

	/* send data to mcu */
	mcu_serial_txs(send_buf, len);

}

int mcu_serial_rx_data(unsigned char* cmd, unsigned char* cdata, int clen)
{
	int time = 50;
	int index = 0;
	int current =0;
	int count = 3;
	int lchecksum = 0, len = 0;
	int i = 0;
	unsigned char receive_buf[PL_ARM_MAX_SIZE] = {0x00};

#ifndef QUICKBOOT
	printf("[\e[31m%s\e[0m()]\n",  __func__);
#endif

	while(count--){
		/* mcu_uart_repeat(); */
		len = 0;
		time = 100;
		current = 0;
		while (time--) {
			msleep(1);
			current = read_uart_fifo(receive_buf, current);
#ifndef QUICKBOOT
			printf("Receive Data : ");
			for (i = 0; i < current; i++)
				printf("0x%02x ",  receive_buf[i]);
			printf("\n");
#endif

			if ((current < PL_ARM_MIN_SIZE) || (current < len))
				continue;

			/* SOI Checkout */
			if((receive_buf[PL_INDEX_ARM_SOI_0] != PL_ARM_SOI_0) ||
			   (receive_buf[PL_INDEX_ARM_SOI_1] != PL_ARM_SOI_1))
			   goto checkerr;

			/* LENGTH Checkout */
			len	= receive_buf[PL_INDEX_ARM_LEN];
			len <<= 8;
			len	|= receive_buf[PL_INDEX_ARM_LEN + 1];
			lchecksum = LCHKBITS(len, LENGTH_CHKSUM_SHIFT);
			len	&= LENGTH_ID_MASK;

			//printf("aron:lchecksum=0x%x,len=0x%x,LCHKSUM(len)=0x%x\n",lchecksum,len,LCHKSUM(len));
			if ((len < PL_ARM_MIN_SIZE)
				|| (lchecksum != LCHKSUM(len)))
				goto checkerr;

			if (current < len)
				continue;

			/* sum Checkout: except SOI  CHKSUM  EOI */
			index = len - 2; /* CHKSUM index */
			if (receive_buf[index]
				!= Protocol_Check(
				&receive_buf[PL_INDEX_ARM_COMMAND],
				len - 4))
				goto checkerr;

			/* EOI Checkout */
			index = len - 1; /* EOI index */
			if (receive_buf[index] != PL_ARM_EOI)
				goto checkerr;

			if (clen < len - PL_ARM_MIN_SIZE) {
#ifndef QUICKBOOT
				printf("cdata length is not enough! len:%d\n",len - PL_ARM_MIN_SIZE);
#endif
				goto user_err;
			}

			if (!cmd || (!cdata && clen)) {
#ifndef QUICKBOOT
				printf("cdata or cmd point is NULL!\n");
#endif
				goto user_err;
			}

			/* clen = len - PL_ARM_MIN_SIZE; */
			*cmd = receive_buf[PL_INDEX_ARM_COMMAND];
			for (index = 0;
				index < (len - PL_ARM_MIN_SIZE); index++)
					cdata[index] =
						receive_buf[PL_INDEX_ARM_INFO + index];

			return index;

		checkerr:
			clean_buffer(receive_buf, current);
			len = 0;
			current = 0;
			/* mcu_uart_repeat(); */
		}
	}

user_err:
	return -1;
}

static void mcu_uart_repeat(void)
{
	mcu_serial_tx_data(0xA9, NULL, 0);
}

void mcu_serial_backlight_open(void)
{
	unsigned char cmd = 0xE5;
	unsigned char cdata[] = {0x01};

#ifndef QUICKBOOT
	printf("mcu_serial_backlight_open\n");
#endif

	mcu_serial_tx_data(cmd, cdata, sizeof(cdata));
}

void mcu_informed_uart_arm_start(void)
{
#ifndef QUICKBOOT
	printf("%s:inform mcu that arm early started\n", __func__);
#endif
	mcu_uart_repeat();
	//mcu_serial_backlight_open();
}

void mcu_informed_uart_arm_late_start(void)
{
	printf("%s:inform mcu that arm late started\n", __func__);
    mcu_serial_backlight_open();
}

int mcu_serial_is_recovery(void)
{
	unsigned char cmd ;
	unsigned char cdata[32] = {0x00};
	int ret = -1;

	ret = mcu_serial_rx_data(&cmd, cdata, sizeof(cdata));
	mcu_serial_backlight_open();
	if (ret < 0) {
#ifndef QUICKBOOT
		printf("%s: mcu serial rx error!\n", __func__);
#endif
		return ret;
	}

	if (cdata[0] == 0x8a) {
#ifndef QUICKBOOT
		printf("%s: ARM will recovery!\n", __func__);
#endif
		return cdata[0];
	}

	return 0;
}

void mcu_serial_do_recovery(void)
{
	printf("%s: recovery!\n", __func__);
	run_command("misc_w", 0);
	run_command("ghost", 0);
}

void mcu_get_max9286(void)
{
	unsigned char cmd = 0xB3;
	unsigned char s_cdata[] = {0x01};
	unsigned char r_cdata[32] = {0x00};
	int ret = 0;
#ifndef QUICKBOOT
	int i = 0;

	printf("%s: Send ARM to MCU: cmd:0x%02x, data:0x%02x, size:%d\n",
		__func__, cmd, s_cdata[0], sizeof(s_cdata));
#endif

	mcu_serial_tx_data(cmd, s_cdata, sizeof(s_cdata));

	ret = mcu_serial_rx_data(&cmd, r_cdata, sizeof(r_cdata));

#ifndef QUICKBOOT
	printf("%s: Receive MCU to ARM: cmd:0x%02x, size:%d\n",
		__func__, cmd, ret);
	printf("%s: data :", __func__);
	for (i = 0; i < ret; i++)
		printf(" 0x%02x", r_cdata[i]);
	printf("\n");
#endif

}

