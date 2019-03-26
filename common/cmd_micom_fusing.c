/*
 * (C) Copyright 2016 Nexell Co.,
 * jong sin park<pjsin865@nexell.co.kr>
 *
 * Configuation settings for the Nexell board.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
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

#define TRUE    true
#define FALSE   false
#define BOOL    bool

#define INIT_CON					0x7F
#define GET_CMD						0x00 //Get the version and the allowed commands supported by the current version of the boot loader
#define GET_VER_ROPS_CMD			0x01 //Get the BL version and the Read Protection status of the NVM
#define GET_ID_CMD					0x02 //Get the chip ID
#define SET_SPEED_CMD				0x03 //set the new baudrate
#define READ_CMD					0x11 //Read up to 256 bytes of memory starting from an address specified by the user
#define GO_CMD						0x21 //Jump to an address specified by the user to execute (a loaded) code
#define WRITE_CMD					0x31 //Write maximum 256 bytes to the RAM or the NVM starting from an address specified by the user
#define ERASE_CMD					0x43 //Erase from one to all the NVM sectors
#define ERASE_EXT_CMD				0x44 //Erase from one to all the NVM sectors
#define WRITE_PROTECT_CMD			0x63 //Enable the write protection in a permanent way for some sectors
#define WRITE_TEMP_UNPROTECT_CMD	0x71 //Disable the write protection in a temporary way for all NVM sectors
#define WRITE_PERM_UNPROTECT_CMD	0x73 //Disable the write protection in a permanent way for all NVM sectors
#define READOUT_PROTECT_CMD			0x82 //Enable the readout protection in a permanent way
#define READOUT_TEMP_UNPROTECT_CMD	0x91 //Disable the readout protection in a temporary way
#define READOUT_PERM_UNPROTECT_CMD	0x92 //Disable the readout protection in a permanent way

#define NXCBL_SUCCESS				0x00 // No error
#define ERROR_OFFSET				0x00 //error offset

#define COM_ERROR_OFFSET			ERROR_OFFSET + 0x00
#define NO_CON_AVAILABLE			COM_ERROR_OFFSET + 0x01  // No serial port opened
#define COM_ALREADY_OPENED			COM_ERROR_OFFSET + 0x02  // Serial port already opened
#define CANT_OPEN_COM				COM_ERROR_OFFSET + 0x03  // Fail to open serial port
#define SEND_FAIL					COM_ERROR_OFFSET + 0x04  // send over serial port fail
#define READ_FAIL					COM_ERROR_OFFSET + 0x05  // Read from serial port fail

#define SYS_MEM_ERROR_OFFSET		ERROR_OFFSET + 0x10
#define CANT_INIT_BL				SYS_MEM_ERROR_OFFSET + 0x01 // Fail to start system memory BL
#define UNREOGNIZED_DEVICE			SYS_MEM_ERROR_OFFSET + 0x02 // Unreconized device
#define CMD_NOT_ALLOWED				SYS_MEM_ERROR_OFFSET + 0x03 // Command not allowed
#define CMD_FAIL					SYS_MEM_ERROR_OFFSET + 0x04 // command failed

#define PROGRAM_ERROR_OFFSET		ERROR_OFFSET + 0x20
#define INPUT_PARAMS_ERROR			PROGRAM_ERROR_OFFSET + 0x01
#define INPUT_PARAMS_MEMORY_ALLOCATION_ERROR	PROGRAM_ERROR_OFFSET + 0x02

enum  ACKS {UNDEFINED=0x00, STM32_NACK=0x1F, STM32_ACK=0x79};

typedef u8	BYTE;
typedef u8	*LPBYTE;
typedef u16	USHORT;
typedef u16	WORD;
typedef u32	DWORD;

typedef struct {
	BYTE Version;
	BYTE CmdCount;
	BYTE PIDLen;
	BYTE* PID;

	BYTE ROPE;
	BYTE ROPD;
} TARGET_DESCRIPTOR;

typedef struct {
	BYTE	_cmd;
	DWORD	_address;
	WORD	_length;
	BYTE	_nbSectors;
	TARGET_DESCRIPTOR *_target;
	BYTE 	*_data;
	WORD	_wbSectors;
} NXCBL_Request;



typedef enum {
	STM32_ERR_OK = 0,
	STM32_ERR_UNKNOWN,	/* Generic error */
	STM32_ERR_NACK,
	STM32_ERR_NO_CMD,	/* Command not available in bootloader */
} stm32_err_t;

#define MAX_DATA_SIZE	128  // Packet size(in byte)

BYTE ACK		= 0x79;
BYTE NACK		= 0x1F;
BYTE ACK_VALUE	= STM32_ACK;

static int UART_Write_Byte(unsigned char data)
{
	int ret = 0;

    while (serial_mcu_tstc())
        serial_mcu_getc();
    serial_mcu_putc(data);

	return ret;
}

static int UART_Write(u8 *buffer, u32 size)
{
	int ret = 0;
	int i = 0;

	for (i=0; i<size; i++)
		ret = UART_Write_Byte(*(buffer+i));

	if (ret < 0)
		printf("## \e[31m%s() fail! \e[0m \n", __FUNCTION__);
	else
		ret = size;

	return ret;
}

static u8 UART_Read_Byte(ulong time_out_us)
{
	u8 ret = 0;
	ulong time_start_us, time_us = 0;

	time_start_us = timer_get_us();

	while (serial_mcu_tstc()) {
		if (time_out_us > 0) {
			time_us = timer_get_us();
			if ((time_us - time_start_us) > time_out_us) {
				printf("## \e[31m%s() time out ! \e[0m \n", __FUNCTION__);
				break;
			}
		}
	}
	ret = serial_mcu_getc();
	return ret;
}

static int UART_Read(u8 *buffer, u32 size, ulong time_out_us)
{
	int i = 0;
	for (i= 0; i<size; i++)
		*(buffer+i) = UART_Read_Byte(time_out_us);
	return size;
}

static void PWM_Disable(int channel)
{
	/*
	pwm_disable(channel);
	*/
}

static void PWM_init(int channel)
{
	/*
	pwm_init(channel, 0, 0);
	pwm_config(channel,TO_DUTY_NS(50, 8000000),TO_PERIOD_NS(8000000));
	pwm_enable(channel);
	*/
}
static int sendData(DWORD lg, LPBYTE data)
{
	return UART_Write(data, lg);
}

static int receiveData(DWORD lg, LPBYTE data)
{
	return UART_Read(data, lg, 1000000);
}

BYTE Send_RQ(NXCBL_Request *pRQ)
{
	int i;
	BYTE DataSize = 1;
	BYTE Get_ACK;
	LPBYTE RQ_Buffer;

	// put command code in the buffer
	RQ_Buffer = (LPBYTE) malloc(2);
	RQ_Buffer[0] = pRQ->_cmd;

	if (ACK_VALUE == STM32_ACK) {		// put XOR command code in the buffer
		RQ_Buffer[1] = ~pRQ->_cmd;
		DataSize = 2;
	}

	// Send command code (and its XOR value)
	if (sendData(DataSize, RQ_Buffer) != DataSize) {
		printf("## \e[31m[%s():%s:%d\t]\e[0m Send cmd error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, RQ_Buffer[0]);

		free(RQ_Buffer);
		return SEND_FAIL;
	}

	free(RQ_Buffer);
	DataSize = 1;

	// Get ACK (verify if the command was accepted)
	if (/*ACK_VALUE == STM32_ACK) */!((ACK_VALUE == STM32_NACK) && (pRQ->_cmd == GET_VER_ROPS_CMD)))
	{
		if (receiveData(1, &Get_ACK) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, Get_ACK);
			return READ_FAIL;
		}
		if(Get_ACK != ACK) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive ACK error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, Get_ACK);
			return CMD_NOT_ALLOWED;
		}
	}

	switch (pRQ->_cmd)
	{
	case GET_CMD					: //Get the version and the allowed commands supported by the current version of the boot loader
		printf("## SendCMD:GET_CMD, ACK:0x%x \n", Get_ACK);
		{
		BYTE TMP_Buffer;

		memset(pRQ->_target, 0x00, sizeof(TARGET_DESCRIPTOR));
		// Get number of bytes (Version + commands)
		if (receiveData(1, &TMP_Buffer) != 1){
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, TMP_Buffer);
			return READ_FAIL;
		}
		pRQ->_target->CmdCount = TMP_Buffer;

		// Get boot loader version
		if (receiveData(1, &TMP_Buffer) != 1){
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, TMP_Buffer);
			return READ_FAIL;
		}
		pRQ->_target->Version = TMP_Buffer;

		// Get supported commands
		RQ_Buffer = (LPBYTE) malloc(pRQ->_target->CmdCount);
		if (receiveData(pRQ->_target->CmdCount, RQ_Buffer) != pRQ->_target->CmdCount) {
			free(RQ_Buffer);
			printf("## \e[31m[%s():%s:%d\t]\e[0m READ_FAIL \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__);
			return READ_FAIL;
		}
		free(RQ_Buffer);

		// Get ACK byte
		if (receiveData(1, &TMP_Buffer) != 1)
			return READ_FAIL;

		if(TMP_Buffer != ACK) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive ACK error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, TMP_Buffer);
			return READ_FAIL;
		}
		}
		break;

	case GET_VER_ROPS_CMD			: //Get the BL version and the Read Protection status of the NVM
		printf("## SendCMD:GET_VER_ROPS_CMD, ACK:0x%x \n", Get_ACK);
		{
		BYTE TMP_Buffer;

		// Get Version
		if (receiveData(1, &TMP_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, TMP_Buffer);
			return READ_FAIL;
		}
		pRQ->_target->Version = TMP_Buffer;

		// Get ROPE
		if (receiveData(1, &TMP_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, TMP_Buffer);
			return READ_FAIL;
		}
		pRQ->_target->ROPE = TMP_Buffer;

		// Get ROPD
		if (receiveData(1, &TMP_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, TMP_Buffer);
			return READ_FAIL;
		}
		pRQ->_target->ROPD = TMP_Buffer;

		// Get ACK
		if (receiveData(1, &TMP_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, TMP_Buffer);
			return READ_FAIL;
		}

		if (TMP_Buffer != ACK ) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive ACK error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, TMP_Buffer);
			return READ_FAIL;
		}
		}
		break;

	case GET_ID_CMD					: //Get the chip ID
		printf("## SendCMD:GET_ID_CMD, ACK:0x%x \n", Get_ACK);
		{
		BYTE TMP_Buffer;

		// Get PID Length
		if (receiveData(1, &TMP_Buffer) != 1)
			return READ_FAIL;
		pRQ->_target->PIDLen = TMP_Buffer + 1;

		// Get PID
		RQ_Buffer = (LPBYTE) malloc(pRQ->_target->PIDLen);
		if (receiveData(pRQ->_target->PIDLen, RQ_Buffer) != pRQ->_target->PIDLen) {
			free(RQ_Buffer);
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, TMP_Buffer);
			return READ_FAIL;
		}

		if (pRQ->_target->PID) {
			pRQ->_target->PID = (BYTE*)malloc(sizeof(pRQ->_target->PIDLen));
			memcpy(pRQ->_target->PID, RQ_Buffer, pRQ->_target->PIDLen);
		}
		free(RQ_Buffer);

		// Get ACK
		if (receiveData(1, &TMP_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, TMP_Buffer);
			return READ_FAIL;
		}

		if (TMP_Buffer != ACK ){
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive ACK error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, TMP_Buffer);
			return READ_FAIL;
		}

		}
		break;

	case READ_CMD					: //Read up to 256 bytes of memory starting from an address specified by the user
		printf("## SendCMD:READ_CMD, ACK:0x%x \n", Get_ACK);
		{
		BYTE Checksum = 0x00;

		RQ_Buffer = (LPBYTE) malloc(5);

		if (ACK_VALUE == STM32_ACK) DataSize = 5;
		else DataSize = 4;

		// Send Read address and checksum
		RQ_Buffer[0] = (pRQ->_address >> 24) & 0x000000FF; Checksum= Checksum ^ RQ_Buffer[0];
		RQ_Buffer[1] = (pRQ->_address >> 16) & 0x000000FF; Checksum= Checksum ^ RQ_Buffer[1];
		RQ_Buffer[2] = (pRQ->_address >> 8 ) & 0x000000FF; Checksum= Checksum ^ RQ_Buffer[2];
		RQ_Buffer[3] = (pRQ->_address      ) & 0x000000FF; Checksum= Checksum ^ RQ_Buffer[3];
		RQ_Buffer[4] = Checksum;

		if (sendData(DataSize, RQ_Buffer) != DataSize) {
			free(RQ_Buffer);
			printf("## \e[31m[%s():%s:%d\t]\e[0m Send error \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__);
			return SEND_FAIL;
		}

		// Get ACK
		if (receiveData(1, RQ_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, RQ_Buffer[0]);
			free(RQ_Buffer);
			return READ_FAIL;
		}

		if (RQ_Buffer[0] != ACK ) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, RQ_Buffer[0]);
			free(RQ_Buffer);
			return CMD_FAIL;
		}

		if(pRQ->_length == 0) {
			free(RQ_Buffer);
		}
		else {
			// send data size to be read
			RQ_Buffer[0] = pRQ->_length-1;
			RQ_Buffer[1] = ~(pRQ->_length-1);

			if (ACK_VALUE == STM32_ACK) DataSize = 2;
			else DataSize = 1;

			if (sendData(DataSize, RQ_Buffer) != DataSize) {
				printf("## \e[31m[%s():%s:%d\t]\e[0m Send error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, RQ_Buffer[0]);
				free(RQ_Buffer);
				return SEND_FAIL;
			}

			// Get ACK
			if (ACK_VALUE == STM32_ACK)
			{
				if (receiveData(1, RQ_Buffer) != 1) {
					printf("## \e[31m[%s():%s:%d\t]\e[0m Receive error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, RQ_Buffer[0]);
					free(RQ_Buffer);
					return READ_FAIL;
				}

				if (RQ_Buffer[0] != ACK ) {
					printf("## \e[31m[%s():%s:%d\t]\e[0m Receive error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, RQ_Buffer[0]);
					free(RQ_Buffer);
					return CMD_FAIL;
				}
			}

			free(RQ_Buffer);

			// Get data
			if (receiveData(pRQ->_length, pRQ->_data)!= pRQ->_length )
				printf("## \e[31m[%s():%s:%d\t]\e[0m Receive error\n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__);
				return READ_FAIL;
			}
		}
		printf("## SendCMD:READ_CMD:COMPLETED, ACK:0x%x \n", Get_ACK);
		break;

	case GO_CMD						: //Jump to an address specified by the user to execute (a loaded) code
		printf("## SendCMD:GO_CMD, ACK:0x%x \n", Get_ACK);
		{
		BYTE Checksum = 0x00;

		if (ACK_VALUE == STM32_ACK) DataSize = 5;
		else DataSize = 4;

		RQ_Buffer = (LPBYTE) malloc(5);

		// Send Go address and checksum
		RQ_Buffer[0] = (pRQ->_address >> 24) & 0x000000FF; Checksum= Checksum ^ RQ_Buffer[0];
		RQ_Buffer[1] = (pRQ->_address >> 16) & 0x000000FF; Checksum= Checksum ^ RQ_Buffer[1];
		RQ_Buffer[2] = (pRQ->_address >> 8 ) & 0x000000FF; Checksum= Checksum ^ RQ_Buffer[2];
		RQ_Buffer[3] = (pRQ->_address      ) & 0x000000FF; Checksum= Checksum ^ RQ_Buffer[3];
		RQ_Buffer[4] = Checksum;

		if (sendData(DataSize, RQ_Buffer) != DataSize) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Send error\n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__);
			free(RQ_Buffer);
			return SEND_FAIL;
		}

		// Get ACK
		if (receiveData(1, RQ_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, RQ_Buffer[0]);
			free(RQ_Buffer);
			return READ_FAIL;
		}

		// to be added when Go command can return ACK when performed
		//if (RQ_Buffer[0] != ACK )
		//return CMD_FAIL;

		free(RQ_Buffer);
		}
		break;

	case WRITE_CMD					: //Write maximum 256 bytes to the RAM or the NVM starting from an address specified by the user
		if( (pRQ->_address % 0x400) == 0)
			printf("## SendCMD:WRITE_CMD, ACK:0x%x, Address:0x%08x, _length:%d \n", Get_ACK, pRQ->_address, pRQ->_length);

		{
		// Send Read address and checksum
		BYTE Checksum = 0x00;

		if (ACK_VALUE == STM32_ACK) DataSize = 5;
		else DataSize = 4;
		DataSize = 5;

		RQ_Buffer = (LPBYTE) malloc(5);

		RQ_Buffer[0] = (pRQ->_address >> 24) & 0x000000FF;
		RQ_Buffer[1] = (pRQ->_address >> 16) & 0x000000FF;
		RQ_Buffer[2] = (pRQ->_address >> 8 ) & 0x000000FF;
		RQ_Buffer[3] = (pRQ->_address      ) & 0x000000FF;

		Checksum= Checksum ^ RQ_Buffer[0];
		Checksum= Checksum ^ RQ_Buffer[1];
		Checksum= Checksum ^ RQ_Buffer[2];
		Checksum= Checksum ^ RQ_Buffer[3];
		RQ_Buffer[4] = Checksum;

		if (sendData(DataSize, RQ_Buffer) != DataSize) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Send Address error(0x%08x) \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, pRQ->_address);
			free(RQ_Buffer);
			return SEND_FAIL;
		}

		// Get ACK
		if (receiveData(1, RQ_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive Data error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, RQ_Buffer[0]);
			free(RQ_Buffer);
			return READ_FAIL;
		}

		if (RQ_Buffer[0] != ACK ) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive ACK error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, RQ_Buffer[0]);
			free(RQ_Buffer);
			return CMD_FAIL;
		}
		mdelay(5);

		Checksum = 0x00;

		// send data size to be writen
		RQ_Buffer[0] = pRQ->_length-1;
		Checksum = /*Checksum ^*/ RQ_Buffer[0];

		if (sendData(1, RQ_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Send Data errors\n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__);
			free(RQ_Buffer);
			return SEND_FAIL;
		}

		mdelay(1);

		// Send data
		if (sendData(pRQ->_length, pRQ->_data)!= pRQ->_length ) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Send Data error\n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__);
			free(RQ_Buffer);
			return SEND_FAIL;
		}

		mdelay(1);

		if (ACK_VALUE == STM32_ACK) {
			for (i = 0; i< pRQ->_length; i++)
				Checksum = Checksum ^ pRQ->_data[i];

			// send Checksum
			RQ_Buffer[0] = Checksum;

			if (sendData(1, RQ_Buffer) != 1) {
				printf("## \e[31m[%s():%s:%d\t]\e[0m Send Checksum error:\n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__);
				free(RQ_Buffer);
				return SEND_FAIL;
			}
		}

		// Get ACK
		if (receiveData(1, RQ_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive Data error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, RQ_Buffer[0]);
			free(RQ_Buffer);
			return READ_FAIL;
		}
		if (RQ_Buffer[0] != ACK ) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive ACK error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, RQ_Buffer[0]);
			free(RQ_Buffer);
			return CMD_FAIL;
		}
		free(RQ_Buffer);
		}
		break;

	case ERASE_EXT_CMD				: //Erase from one to all the NVM sectors
		printf("## SendCMD:ERASE_EXT_CMD, ACK:0x%x \n", Get_ACK);
		{
		//memcpy(pRQ->_target, Target, sizeof(TARGET_DESCRIPTOR));

		if ((pRQ->_wbSectors  & 0xFF00 ) != 0xFF00)
		{
			BYTE checksum = ((pRQ->_wbSectors)-1) >> 8 ;
			checksum = checksum ^ ((pRQ->_wbSectors)-1) ;

			RQ_Buffer = (LPBYTE) malloc(pRQ->_length*2 + 3);  /* N ( 2 bytes)  and Checksum */
			memset(RQ_Buffer,0xFF, pRQ->_length*2 + 3);

			RQ_Buffer[0] = ((pRQ->_wbSectors)-1) >> 8;
			RQ_Buffer[1] = ((pRQ->_wbSectors)-1);

			for (i = 2; i<= pRQ->_wbSectors*2 ;i+=2)
			{
				RQ_Buffer[i] = pRQ->_data[i-1];
				RQ_Buffer[i+1] = pRQ->_data[i-2];


				checksum = checksum ^ pRQ->_data[i-1];
				checksum = checksum ^ pRQ->_data[i-2];


			}
			RQ_Buffer[pRQ->_wbSectors*2 + 2] = checksum;
		}
		else
		{
			RQ_Buffer = (LPBYTE) malloc(3);

			RQ_Buffer[0] = pRQ->_wbSectors >> 8;
			RQ_Buffer[1] = pRQ->_wbSectors;
			RQ_Buffer[2] = RQ_Buffer[0] ^ RQ_Buffer[1];
		}

		if (ACK_VALUE == STM32_ACK) DataSize = 3;
		else DataSize = 1;

		if (sendData(pRQ->_length*2 + DataSize, RQ_Buffer) != pRQ->_length*2 + DataSize) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Send Data error:\n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__);
			free(RQ_Buffer);
			return SEND_FAIL;
		}

		// Get ACK
		if (receiveData(1, RQ_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive Data error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, RQ_Buffer[0]);
			free(RQ_Buffer);
			return READ_FAIL;
		}

		if (RQ_Buffer[0] != ACK ) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive ACK error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, RQ_Buffer[0]);
			free(RQ_Buffer);
			return CMD_FAIL;
		}
		free(RQ_Buffer);
		}
		break;

	case ERASE_CMD					: //Erase from one to all the NVM sectors
		printf("## SendCMD:ERASE_CMD, ACK:0x%x \n", Get_ACK);
		{
		//memcpy(pRQ->_target, Target, sizeof(TARGET_DESCRIPTOR));
		if (pRQ->_nbSectors != 0xFF)
		{
			BYTE checksum = /*0x00 ^ */(pRQ->_nbSectors-1);

			RQ_Buffer = (LPBYTE) malloc(pRQ->_length + 2);
			memset(RQ_Buffer,0xFF, pRQ->_length + 2);

			RQ_Buffer[0] = pRQ->_nbSectors - 1;
			for (i = 1; i<= pRQ->_nbSectors ;i++)
			{
				RQ_Buffer[i] = pRQ->_data[i-1];
				checksum = checksum ^ pRQ->_data[i-1];
			}
			RQ_Buffer[pRQ->_nbSectors + 1] = checksum;
		}
		else
		{
			RQ_Buffer = (LPBYTE) malloc(2);

			RQ_Buffer[0] = pRQ->_nbSectors;
			RQ_Buffer[1] = ~pRQ->_nbSectors;
		}

		if (ACK_VALUE == STM32_ACK) DataSize = 2;
		else DataSize = 1;

		if (sendData(pRQ->_length + DataSize, RQ_Buffer) != pRQ->_length + DataSize) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Send Data error:\n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__);
			free(RQ_Buffer);
			return SEND_FAIL;
		}

		// Get ACK
		if (receiveData(1, RQ_Buffer) != 1) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive Data error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, RQ_Buffer[0]);
			free(RQ_Buffer);
			return READ_FAIL;
		}

		if (RQ_Buffer[0] != ACK ) {
			printf("## \e[31m[%s():%s:%d\t]\e[0m Receive ACK error:0x%x \n", __FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__, RQ_Buffer[0]);
			free(RQ_Buffer);
			return CMD_FAIL;
		}
		free(RQ_Buffer);
		}
		break;

	case WRITE_PROTECT_CMD			: //Enable the write protection in a permanent way for some sectors
		break;

	case WRITE_TEMP_UNPROTECT_CMD	: //Disable the write protection in a temporary way for all NVM sectors
		break;

	case WRITE_PERM_UNPROTECT_CMD	: //Disable the write protection in a permanent way for all NVM sectors
		break;

	case READOUT_PROTECT_CMD		: //Disable the readout protection in a temporary way
		break;

	case READOUT_TEMP_UNPROTECT_CMD	: //Disable the readout protection in a permanent way
		break;

	case READOUT_PERM_UNPROTECT_CMD	:
		break;

	}

	return NXCBL_SUCCESS;
}

#if 0	// Don't use..
static BYTE NXCBL_SET_COMMAND(unsigned char cmd)
{
    BYTE Result = NXCBL_SUCCESS;
    NXCBL_Request *pRQ;

    pRQ = (NXCBL_Request*)malloc(sizeof(NXCBL_Request));
    pRQ->_target = (TARGET_DESCRIPTOR*)malloc(sizeof(TARGET_DESCRIPTOR));
    pRQ->_cmd = cmd;

    Result = Send_RQ(pRQ);
    if (Result == NXCBL_SUCCESS) {
        printf("[%s][0x%02X] : OK\n",__func__,cmd);
    }
    else {
        printf("## \e[31m%s():%d fail\e[0m : 0x%x \n", __FUNCTION__, __LINE__, Result);
    }

    free(pRQ->_target);
    free(pRQ);
    return Result;
}
#endif

#if 0	// Don't use..
static BYTE NXCBL_GET_VER_ROPS(LPBYTE Version, LPBYTE ROPEnabled, LPBYTE ROPDisabled)
{
	BYTE Result = NXCBL_SUCCESS;
	NXCBL_Request *pRQ;

	if (!Version || !ROPEnabled || !ROPDisabled) return INPUT_PARAMS_MEMORY_ALLOCATION_ERROR;

	pRQ = (NXCBL_Request*)malloc(sizeof(NXCBL_Request));
	pRQ->_target = (TARGET_DESCRIPTOR*)malloc(sizeof(TARGET_DESCRIPTOR));
	pRQ->_cmd = GET_VER_ROPS_CMD;

	Result = Send_RQ(pRQ);
	if (Result == NXCBL_SUCCESS) {
		memcpy(ROPEnabled, &pRQ->_target->ROPD, 1);
		memcpy(ROPDisabled, &pRQ->_target->ROPE, 1);
		memcpy(Version, &pRQ->_target->Version, 1);

		printf("Version:0x%x\n", *Version);
		printf("ROPEnabled:0x%x\n", *ROPEnabled);
		printf("ROPDisabled:0x%x\n", *ROPDisabled);
	}
	else {
		printf("## \e[31m%s():%d fail\e[0m : 0x%x \n", __FUNCTION__, __LINE__, Result);
	}

	free(pRQ->_target);
	free(pRQ);
	return Result;
}
#endif

#if 0	// Don't use..
static BYTE NXCBL_GET(LPBYTE Version)
{
	BYTE Result = NXCBL_SUCCESS;

	if(ACK_VALUE == STM32_ACK)
	{
		NXCBL_Request *pRQ = (NXCBL_Request*)malloc(sizeof(NXCBL_Request));
		pRQ->_target = (TARGET_DESCRIPTOR*)malloc(sizeof(TARGET_DESCRIPTOR));
		pRQ->_cmd = GET_CMD;

		Result = Send_RQ(pRQ);
		if (Result == NXCBL_SUCCESS)
			memcpy(Version, &pRQ->_target->Version,1);
		else
			printf("## \e[31m%s():%d fail\e[0m : 0x%x \n", __FUNCTION__, __LINE__, Result);

		free(pRQ->_target);
		free(pRQ);
	}

	return Result;
}
#endif

static BYTE NXCBL_GET_ID(LPBYTE size, LPBYTE pID)
{
	BYTE Result = NXCBL_SUCCESS;
	int i = 0;
	NXCBL_Request *pRQ;

	if (!size ) return INPUT_PARAMS_MEMORY_ALLOCATION_ERROR;

	pRQ = (NXCBL_Request*)malloc(sizeof(NXCBL_Request));
	pRQ->_target = (TARGET_DESCRIPTOR*)malloc(sizeof(TARGET_DESCRIPTOR));
	pRQ->_cmd = GET_ID_CMD;
	pRQ->_target->PID = pID;

	Result = Send_RQ(pRQ) ;
	if (Result == NXCBL_SUCCESS) {
		memcpy(size, &pRQ->_target->PIDLen, 1);
		if (pID) {
			memcpy(pID, pRQ->_target->PID, *size);

			printf("## PID:0x");
			for(i=0; i<(*size); i++) {
				printf("%02x", pID[i]);
			}
			printf("\n");
		}
	}
	else {
		printf("## \e[31m%s():%d fail\e[0m : 0x%x \n", __FUNCTION__, __LINE__, Result);
	}

	free(pRQ->_target);
	free(pRQ);
	return Result;
}

static BYTE NXCBL_READ(DWORD Address, BYTE Size, LPBYTE pData)
{
	BYTE Result = NXCBL_SUCCESS;
	NXCBL_Request *pRQ;

	if (!pData) return INPUT_PARAMS_MEMORY_ALLOCATION_ERROR;

	pRQ = (NXCBL_Request*)malloc(sizeof(NXCBL_Request));
	pRQ->_target = (TARGET_DESCRIPTOR*)malloc(sizeof(TARGET_DESCRIPTOR));
    pRQ->_cmd = READ_CMD;
    pRQ->_address = Address;

	//if((Size%2) != 0) Size++;
	pRQ->_length  = Size ;

	if (pData) {
		pRQ->_data = pData;
		memset(pRQ->_data, 0xFF, MAX_DATA_SIZE);
	}

	Result = Send_RQ(pRQ);
	if (Result == NXCBL_SUCCESS) {
		if (pData) memcpy(pData, pRQ->_data, Size);
	} else {
		printf("## \e[31m%s():%d fail\e[0m : 0x%x \n", __FUNCTION__, __LINE__, Result);
	}

	free(pRQ->_target);
	free(pRQ);
	return Result;
}
#if 0
static BYTE NXCBL_GO(DWORD Address)
{
	BYTE Result = NXCBL_SUCCESS;
	NXCBL_Request *pRQ;

	pRQ = (NXCBL_Request*)malloc(sizeof(NXCBL_Request));
	pRQ->_target = (TARGET_DESCRIPTOR*)malloc(sizeof(TARGET_DESCRIPTOR));
    pRQ->_cmd = GO_CMD;
    pRQ->_address = Address;

	Result = Send_RQ(pRQ);

	free(pRQ->_target);
	free(pRQ);
	return Result;
}
#endif
static BYTE NXCBL_WRITE(DWORD address, BYTE size, LPBYTE pData)
{
	BYTE Result = NXCBL_SUCCESS;
	NXCBL_Request *pRQ;

	if (!pData) return INPUT_PARAMS_MEMORY_ALLOCATION_ERROR;

	pRQ = (NXCBL_Request*)malloc(sizeof(NXCBL_Request));

	pRQ->_cmd     = WRITE_CMD;
	pRQ->_address = address;
	pRQ->_length  = size;
	pRQ->_data 	  = pData;

	Result = Send_RQ(pRQ);
	if (Result != NXCBL_SUCCESS)
		printf("## \e[31m%s():%d fail\e[0m : 0x%x \n", __FUNCTION__, __LINE__, Result);

	free(pRQ);
	return Result;
}

static BYTE NXCBL_ERASE(WORD NbSectors, LPBYTE pSectors)
{
	BYTE Result;
	NXCBL_Request *pRQ;

	if ((!pSectors) && (NbSectors != 0xFFFF)) return INPUT_PARAMS_MEMORY_ALLOCATION_ERROR;

	pRQ = (NXCBL_Request*)malloc(sizeof(NXCBL_Request));
	pRQ->_target = (TARGET_DESCRIPTOR*)malloc(sizeof(TARGET_DESCRIPTOR));

	pRQ->_cmd = ERASE_EXT_CMD;

	if (NbSectors == 0xFFFF)
	{
		pRQ->_wbSectors = 0xFFFF;
		pRQ->_length = 0;

		Result = Send_RQ(pRQ);
		if (Result != NXCBL_SUCCESS) {
			printf("## \e[31m%s():%d fail\e[0m : 0x%x \n", __FUNCTION__, __LINE__, Result);
			free(pRQ->_target);
			free(pRQ);
			return Result;
		}
	}
	else
	{
		WORD nErase = NbSectors / 10;
		WORD Remain = NbSectors % 10;
		int i = 0;
		int j = 0; /*  This is for WORD */

		if (nErase > 0)
		{
			for (i = 0; i< nErase; i++)
			{
				BYTE Convert[0xFF];
				pRQ->_length = 10;
				pRQ->_wbSectors = 10;
				pRQ->_data = (LPBYTE)malloc(10*2);

				for (j = 0; j< 10*2; j++)
				{
					Convert[j]=  pSectors [i*10*2+ j];
				}

				memcpy(pRQ->_data, Convert, 10*2);

				Result = Send_RQ(pRQ);
				if (Result != NXCBL_SUCCESS) {
					printf("## \e[31m%s():%d fail\e[0m : 0x%x \n", __FUNCTION__, __LINE__, Result);
					free(pRQ->_target);
					free(pRQ);
					return Result;
				}
			}
		}

		if (Remain > 0)
		{
			BYTE Convert[0xFF];
			pRQ->_length = Remain;
			pRQ->_wbSectors = Remain;
			pRQ->_data = (LPBYTE)malloc(Remain*2);

			for (j = 0; j< Remain*2; j++)
			{
				Convert[j]=  pSectors [i*10*2+j];
			}

			memcpy(pRQ->_data, Convert, Remain*2);

			Result = Send_RQ(pRQ);
			if (Result != NXCBL_SUCCESS) {
				printf("## \e[31m%s():%d fail\e[0m : 0x%x \n", __FUNCTION__, __LINE__, Result);
				free(pRQ->_target);
				free(pRQ);
				return Result;
			}
		}
	}

	free(pRQ->_target);
	free(pRQ);

	return NXCBL_SUCCESS;
}

#if 0	// Don't use..
static BYTE NXCBL_VERIFY(DWORD Address, LPBYTE pData, DWORD Length,BOOL bTruncateLeadFFForDnLoad)
{
	int i;
	LPBYTE Holder = pData;
	BYTE   Result = NXCBL_SUCCESS;
	LPBYTE buffer = (LPBYTE) malloc(MAX_DATA_SIZE);

    DWORD nbuffer = Length / MAX_DATA_SIZE;
	DWORD ramain  = Length % MAX_DATA_SIZE;

	LPBYTE Empty = (LPBYTE) malloc(MAX_DATA_SIZE);
	memset(Empty, 0xFF, MAX_DATA_SIZE);

	if (nbuffer > 0) {
	  for(i=1; i<=nbuffer; i++) {
		BOOL AllFFs = FALSE;
		if((memcmp(Empty, pData, MAX_DATA_SIZE) == 0) && bTruncateLeadFFForDnLoad) {
           AllFFs = TRUE;
		}

		if(!AllFFs) {
			memset(buffer, 0xFF, MAX_DATA_SIZE);
			Result = NXCBL_READ(Address, MAX_DATA_SIZE, buffer);
			if (Result != NXCBL_SUCCESS) {
				printf("## \e[31m%s():%d fail\e[0m : 0x%x \n", __FUNCTION__, __LINE__, Result);
				goto out;
			}
			if (memcmp(pData, buffer, MAX_DATA_SIZE) != 0) {
				Result = CMD_NOT_ALLOWED; // verify fail
				printf("## \e[31m%s():%d fail\e[0m : 0x%x \n", __FUNCTION__, __LINE__, Result);
				goto out;
			}
		}
		pData = pData + MAX_DATA_SIZE;
		Address += MAX_DATA_SIZE;
	  }
	}

	if (ramain > 0) {
		BOOL AllFFs = FALSE;
		if((memcmp(Empty, pData, ramain) == 0) && bTruncateLeadFFForDnLoad)
           AllFFs = TRUE;

		if(!AllFFs) {
			memset(buffer, 0xFF, MAX_DATA_SIZE);
			Result = NXCBL_READ(Address, ramain, buffer);
			if (Result != NXCBL_SUCCESS) {
				printf("## \e[31m%s():%d fail\e[0m : 0x%x \n", __FUNCTION__, __LINE__, Result);
				goto out;
			}
			if (memcmp(pData, buffer, ramain) != 0) {
				Result = CMD_NOT_ALLOWED; // verify fail
				printf("## \e[31m%s():%d fail\e[0m : 0x%x \n", __FUNCTION__, __LINE__, Result);
				goto out;
			}
		}
	}
	pData = Holder;

out:
	free(buffer);
	free(Empty);
	return Result;
}
#endif

static BYTE NXCBL_DNLOAD(DWORD Address, LPBYTE pData, DWORD Length, BOOL bTruncateLeadFFForDnLoad)
{
	int i;
	BYTE   Result = NXCBL_SUCCESS;
	LPBYTE Holder = pData;
	LPBYTE buffer = (LPBYTE) malloc(MAX_DATA_SIZE);
	LPBYTE Empty = (LPBYTE) malloc(MAX_DATA_SIZE);

	DWORD nbuffer = (DWORD)(Length / MAX_DATA_SIZE);
	DWORD ramain  = (DWORD)(Length % MAX_DATA_SIZE);
	DWORD Newramain = ramain;

	memset(Empty, 0xFF, MAX_DATA_SIZE);

	if (nbuffer > 0) {
		for(i=1; i<=nbuffer; i++) {
			BOOL AllFFs = FALSE;

			memset(buffer, 0xFF, MAX_DATA_SIZE);
			memcpy(buffer, pData, MAX_DATA_SIZE);

			if((memcmp(Empty, buffer, MAX_DATA_SIZE) == 0) && bTruncateLeadFFForDnLoad) {
				AllFFs = TRUE;
			}

			if(!AllFFs) {
				Result = NXCBL_WRITE(Address, MAX_DATA_SIZE, buffer);
				if (Result != NXCBL_SUCCESS) {
					printf("## \e[31m%s():%d fail\e[0m : 0x%x \n", __FUNCTION__, __LINE__, Result);
					free(buffer);
					free(Empty);
					return Result;
				}
			}

			pData = pData + MAX_DATA_SIZE;
			Address += MAX_DATA_SIZE;
		}
	}

	if (ramain > 0) {
		BOOL AllFFs = FALSE;

		memset(buffer, 0xFF, MAX_DATA_SIZE);

		Result = NXCBL_READ(Address,  Newramain, buffer);   // removed in version 2.8.0 upon customer case @ end of flash area.
		if (Result != NXCBL_SUCCESS) {
			printf("## \e[31m%s():%d fail\e[0m : 0x%x \n", __FUNCTION__, __LINE__, Result);
			free(buffer);
			free(Empty);
			return Result;
		}

		memcpy(buffer, pData, ramain);

		if((memcmp(Empty, buffer, ramain) == 0) && bTruncateLeadFFForDnLoad)
			AllFFs = TRUE;

		if(!AllFFs) {
			Result = NXCBL_WRITE(Address, Newramain, buffer);
			if (Result != NXCBL_SUCCESS) {
				printf("## \e[31m%s():%d fail\e[0m : 0x%x \n", __FUNCTION__, __LINE__, Result);
				free(buffer);
				free(Empty);
				return Result;
			}
		}
	}
	pData = Holder;

	free(buffer);
	free(Empty);
	return Result;
}

static int NXCBL_Init_BL(void)
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

            switch (rdata)
            {
            case STM32_NACK:
                printf("## NACK Received: 0x%x\n", rdata);
                break;

            case STM32_ACK:
                printf("## ACK Received: 0x%x\n", rdata);
                break;

            default:
                printf("## Undefined device : 0x%x\n", rdata);
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

//SUKER not used function
/* static int NXCBL_CHECK_VERSION(char *version) */
/* { */
/* 	int ret = 0; */
/*         u8  buffer[10]; */

/* 	memset(buffer, 0x00, sizeof(s8)*10); */

/* 	UART_Read(buffer, 2, 1000000); */
/* 	buffer[9] = 0; */

/* 	printf("micom version : %s \n\n", buffer); */

/* 	if (strncmp(buffer, version, 2) != 0) { */
/* 		printf("## \e[31m%s():%d Version fail\e[0m : bin_version:%s, read_version:%s \n", __FUNCTION__, __LINE__, version, buffer); */
/* 		ret = -1; */
/* 	} */

/* 	return ret; */
/* } */

static void NXCBL_RESET(void)
{
	/*
	U32 grp , bit, pad;
	U32 gpio = (PAD_GPIO_E + 15);//((PAD_GPIO_C + 4) | PAD_FUNC_ALT1);

	grp = PAD_GET_GROUP(gpio);
	bit = PAD_GET_BITNO(gpio);
	pad = PAD_GET_FUNC(gpio);

	NX_GPIO_SetPadFunction(grp, bit, pad);
	NX_GPIO_SetOutputEnable(grp, bit, CTRUE);

	NX_GPIO_SetOutputValue(grp, bit, CFALSE);

	mdelay(100);

	NX_GPIO_SetOutputValue(grp, bit, CTRUE);

	mdelay(100);
	*/
}

static void NXCBL_BOOTMODE(int mode)
{
	/*
	U32 grp , bit, pad;
	U32 gpio = (PAD_GPIO_C + 15);//((PAD_GPIO_C + 19) | PAD_FUNC_ALT1);

	grp = PAD_GET_GROUP(gpio);
	bit = PAD_GET_BITNO(gpio);
	pad = PAD_GET_FUNC(gpio);

	NX_GPIO_SetPadFunction(grp, bit, pad);
	NX_GPIO_SetOutputEnable(grp, bit, CTRUE);

	if (mode)
		NX_GPIO_SetOutputValue(grp, bit, CTRUE);
	else
		NX_GPIO_SetOutputValue(grp, bit, CFALSE);
	*/
}

static int do_micom_fusing(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i = 0;
	int res = 0;
	unsigned long addr;

	u8 *p, *pos;
        //SUKER not used function
	//char *bin_version;

        //SUKER not used function
	//BYTE Version, size, pID;
        BYTE size, pID;
        
	//BYTE ROPEnabled, ROPDisabled;
	BYTE Result = NXCBL_SUCCESS;
	DWORD Address;
	//BOOL optimize = FALSE;
	BOOL optimize = TRUE;

	printf("\n");

	addr = simple_strtoul(argv[1], NULL, 16);

	if (addr == 1)	goto down_load;
	if (addr == 2)	goto erase;
	if (addr == 3) {
		PWM_init(1);
		NXCBL_BOOTMODE(0);
		NXCBL_RESET();
		goto normal_boot;
	}

	if (argc != 2) {
		printf("## [%s():%d] \e[31m argc(%d) error.\e[0m\n", __func__,__LINE__, argc);
		goto ret_error;
	}

	addr = simple_strtoul(argv[1], NULL, 16);
	p = (u8*)addr;
        //SUKER warninig fixed
	if (p < (u8*)(0x48000000)) {
                printf("## [%s():%d] \e[31mload address(0x%x) error.\e[0m\n", __func__,__LINE__, (unsigned int)addr);
		goto ret_error;
	}

        //SUKER not used function
	//bin_version = (char*)p;

	PWM_init(1);								// pwm init
	NXCBL_BOOTMODE(0);						// STM32 Boot mode
	NXCBL_RESET();							// STM32 Reset

        //SUKER warninig fixed
	printf("## LOAD address  : 0x%x \n", (unsigned int)p);
/*
	if(0 == NXCBL_CHECK_VERSION(bin_version))
		goto normal_boot;
*/

	PWM_Disable(1);
	NXCBL_BOOTMODE(1);
	NXCBL_RESET();

	// STM32 init
	Result = NXCBL_Init_BL();
	if (Result != NXCBL_SUCCESS)
		goto ret_error;

	mdelay(20);
#if 1
	//NXCBL_GET(&Version);
	NXCBL_GET_ID(&size, &pID);
	mdelay(20);
#endif
	Result = NXCBL_ERASE(0xFFFF, NULL);
	if (Result != NXCBL_SUCCESS)
		goto ret_error;

	printf("\n## Downloading... \n");
	Address = 0x08000000;
	for (i=0; i < 0x8000; i+=SZ_1K) {
		pos = (p+i);
		Result = NXCBL_DNLOAD(Address, pos, SZ_1K, optimize);
		if (Result != NXCBL_SUCCESS) {
			printf("## \e[31m%s():%d fail\e[0m : 0x%x \n", __FUNCTION__, __LINE__, Result);
			goto ret_error;
		}
		Address += SZ_1K;
	}
	printf("exit\n");

	PWM_init(1);
	NXCBL_BOOTMODE(0);
	NXCBL_RESET();

	printf("## STM32 Normal Booting \n");
	return res;

ret_error:
	printf("exit\n");
	PWM_init(1);
	NXCBL_BOOTMODE(0);
	NXCBL_RESET();
	printf("## STM32 ret_error!!! \n");
	return -1;

down_load:
	printf("## STM32 Download Mode \n");
	PWM_Disable(1);
	NXCBL_BOOTMODE(1);
	NXCBL_RESET();
	return 0;

erase:
	printf("## STM32 Erase All\n");
	PWM_Disable(1);
	NXCBL_BOOTMODE(1);
	NXCBL_RESET();

	NXCBL_Init_BL();
	NXCBL_ERASE(0xFFFF, NULL);

	PWM_init(1);
	NXCBL_BOOTMODE(0);
	NXCBL_RESET();
	return 0;

normal_boot:
	printf("## STM32 Normal Booting \n");
	return 0;
}

U_BOOT_CMD(
	micom,	3,	1,	do_micom_fusing,
	"micom fusing from memory",
	" <addr>\n"
	"    - addr : micom image load address \n"
	"  ex> micom 0x48000000 \n"
	"\n"
	"  etc> micom 1  : Download Mode \n"
	"       micom 2  : Erase \n"
	"       micom 3  : Normal Booting \n"
);



