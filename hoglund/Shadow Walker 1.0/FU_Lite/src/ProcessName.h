///////////////////////////////////////////////////////////////////////////////////////
// Filename ProcessName.h
// 
// Author: fuzen_op
// Email:  fuzen_op@yahoo.com or fuzen_op@rootkit.com
//
// Description: Globals and function prototypes used by ProcessName.c
//
// Date:    5/27/2003
// Version: 1.0

//////////////////////////////////////////////////////////////////////
// Force everything into the data section. 
/////////////////////////////////////////////////////////////////////
#pragma data_seg(".data")

int GetLocationOfProcessName(PEPROCESS, char *); // Get offset of process name in
