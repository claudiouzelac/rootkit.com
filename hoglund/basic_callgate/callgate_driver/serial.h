#ifndef __serial__h__
#define __serial__h__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void SendInformationToSerialPort( char *text );
void InitSerialPort();

// print to console
void __cdecl conprintf (
        const char *format,
        ...
        );

extern PUCHAR COM_PORT;

// offsets from base port address
#define RECV_BUFFER_REGISTER		0x00
#define XMT_HOLDING_REGISTER		0x00
#define FIFO_CTRL_REGISTER			0x02
#define LINE_CTRL_REGISTER			0x03
#define MODEM_CTRL_REGISTER			0x04
#define LINE_STATUS_REGISTER		0x05
#define MODEM_STATUS_REGISTER		0x06

// baud rate
#define DIVISOR_LATCH_LSB			0x00
#define DIVISOR_LATCH_MSB			0x01

// bit masks, modem control
#define SERIAL_MCR_OUT1				0x04
#define SERIAL_MCR_OUT2				0x08
#define SERIAL_MCR_DTR				0x01

// bit masks, fifo control
#define SERIAL_FCR_ENABLE			0x01
#define SERIAL_FCR_RCVR_RESET		0x02
#define SERIAL_FCR_TXMT_RESET		0x04
// bit masks, line control
#define SERIAL_LCR_DLAB				0x80 
#define SERIAL_LCR_BREAK			0x40 // device sending a break?
#define SERIAL_LCR_1_STOP			0x00 // 1 stop bit
#define SERIAL_LCR_NO_PARITY		0x00 // no parity
#define SERIAL_LCR_8BIT_DATA		0x03 // 8 bits

#endif