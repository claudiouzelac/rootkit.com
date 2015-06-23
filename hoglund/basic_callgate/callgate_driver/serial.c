
#include "ntddk.h"
#include "serial.h"
#include <string.h>

// serial output
// hard code to the port you want to use
PUCHAR COM_PORT = (PUCHAR)0x3F8;

////////////////////////////////////////////////////////////////////////////////////
// Initialize the serial port for use
// This took a fair amount of hacking to get working for some reason, I tried
// many variations and ordering of operations. Still doesn't work stand-alone
// but begins working after running portmon once, or if hyperterminal is up
// and connected to COM1 - so these two tools must do something to init the
// port that I am not doing properly here...
////////////////////////////////////////////////////////////////////////////////////
void InitSerialPort()
{
	UCHAR oldModemControl;
	UCHAR aByte;
	UCHAR regContents;
	int i;

	//---------------------------------------------------------------------------------------------------------------------
	// read modem control
	oldModemControl = READ_PORT_UCHAR( COM_PORT + MODEM_CTRL_REGISTER );
	// adjust out2, turns off interrupts
	WRITE_PORT_UCHAR( COM_PORT + MODEM_CTRL_REGISTER, oldModemControl & ~SERIAL_MCR_OUT2 );
	
	// reset the FIFO
	//---------------------------------------------------------------------------------------------------------------------
	WRITE_PORT_UCHAR( COM_PORT + FIFO_CTRL_REGISTER, 0); //disable FIFO
	aByte = READ_PORT_UCHAR( COM_PORT + RECV_BUFFER_REGISTER ); //read a byte
	WRITE_PORT_UCHAR( COM_PORT + FIFO_CTRL_REGISTER, SERIAL_FCR_ENABLE | SERIAL_FCR_RCVR_RESET | SERIAL_FCR_TXMT_RESET );
	
	// setup DLAB to ON, make sure break is off
	//---------------------------------------------------------------------------------------------------------------------
	regContents = READ_PORT_UCHAR( COM_PORT + LINE_CTRL_REGISTER );
	regContents &= ~(SERIAL_LCR_DLAB | SERIAL_LCR_BREAK);
	WRITE_PORT_UCHAR( COM_PORT + LINE_CTRL_REGISTER, regContents );
	
	// read recv buffer until line status is clear, or give up after a while
	//---------------------------------------------------------------------------------------------------------------------
	for(i=0;i<42;i++){
		aByte = READ_PORT_UCHAR( COM_PORT + RECV_BUFFER_REGISTER );
		regContents = READ_PORT_UCHAR( COM_PORT + LINE_STATUS_REGISTER );
		if(!(regContents & 1)) break;
	}
	
	// read modem status until lower 4 bits are clear, or give up after a while
	//---------------------------------------------------------------------------------------------------------------------
	for(i=0;i<42;i++){
		regContents = READ_PORT_UCHAR( COM_PORT + MODEM_STATUS_REGISTER );
		if(!(regContents & 0x0F)) break;
	}
	
	// set baud rate 9600 (DLAB should be ON for this)
	// ----------------------------------------------------------------------------
	WRITE_PORT_UCHAR( COM_PORT + DIVISOR_LATCH_LSB, 0x0C );
	WRITE_PORT_UCHAR( COM_PORT + DIVISOR_LATCH_MSB, 0x00 );

	// set DTR
	// ----------------------------------------------------------------------------
	WRITE_PORT_UCHAR( COM_PORT + MODEM_CTRL_REGISTER, SERIAL_MCR_DTR );

	// set line control
	// StopBits: 1 Parity: NONE WordLength: 8
	// Set this after the baud rate so the DLAB doesn't get messed up
	// ----------------------------------------------------------------------------
	WRITE_PORT_UCHAR( COM_PORT + LINE_CTRL_REGISTER, SERIAL_LCR_1_STOP | SERIAL_LCR_NO_PARITY | SERIAL_LCR_8BIT_DATA );

	// set modem control 
	// not sure exactly what these flags mean
	//---------------------------------------------------------------------------------------------------------------------
	WRITE_PORT_UCHAR( COM_PORT + MODEM_CTRL_REGISTER, 0x0B );

	// set fifo control
	// not sure exactly what these flags mean
	//---------------------------------------------------------------------------------------------------------------------
	WRITE_PORT_UCHAR( COM_PORT + FIFO_CTRL_REGISTER, 0xC7 );

	// here is the original code I based this on:
	//---------------------------------------------------------------------------------------------------------------------
#if 0
	outportb(PORT1 + 1 , 0x00);  /* turn off interrupts */
	outportb(PORT1 + 3 , 0x80);  /* SET DLAB ON */
	outportb(PORT1 + 0 , 0x0C);  /* Set Baud rate - Divisor Latch Low Byte */
					/* Default 0x03 =  38,400 BPS */
					/*         0x01 = 115,200 BPS */
					/*         0x02 =  57,600 BPS */
					/*         0x06 =  19,200 BPS */
					/*         0x0C =   9,600 BPS */
					/*         0x18 =   4,800 BPS */
					/*         0x30 =   2,400 BPS */
	outportb(PORT1 + 1 , 0x00);  /* Set Baud rate - Divisor Latch High Byte */
	outportb(PORT1 + 3 , 0x03);  /* 8 Bits, No Parity, 1 Stop Bit */
	outportb(PORT1 + 2 , 0xC7);  /* FIFO Control Register */
	outportb(PORT1 + 4 , 0x0B);  /* Turn on DTR, RTS, and OUT2 */		
#endif

	// send a test character
	// WRITE_PORT_UCHAR(COM_PORT + XMT_HOLDING_REGISTER, 'R');
}

// the idea here is to send the text to the serial port
void SendInformationToSerialPort( char *text )
{
	char *c = text;
	
	//DbgPrint(text);

	while(*c != '\0')
	{
		char _c = *c;
		WRITE_PORT_UCHAR(COM_PORT + XMT_HOLDING_REGISTER, _c); 
		c++;
	}
}

char _gtt[1024];

// console printing function
// ---------------------------------------------------------------------------------
void __cdecl conprintf (
        const char *format,
        ...
        )
{
        va_list arglist;
        va_start(arglist, format);

		_vsnprintf(_gtt, 1022, format, arglist);
		
		SendInformationToSerialPort(_gtt);
}