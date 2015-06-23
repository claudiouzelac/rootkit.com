#ifndef __common__h__
#define __common__h__

#define DWORD unsigned long
#define ULONG unsigned long
#define WORD unsigned short
#define USHORT unsigned short
#define BOOL unsigned long
#define BYTE unsigned char
#define UCHAR unsigned char

#define MAKELONG(a, b) ((unsigned long) (((unsigned short) (a)) | ((unsigned long) ((unsigned short) (b))) << 16)) 
#endif