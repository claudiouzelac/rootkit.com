#ifndef __KRNL_STDLIB_H
 #define __KRNL_STDLIB_H

extern "C"
{
 #include "ntddk.h"
}

//#include "He4Types.h"

LONG     CompareStringN(PCHAR lpszString1, PCHAR lpszString2, BOOLEAN bCaseInsensitive, ULONG dwLen);
LONG     CompareString(PCHAR lpszString1, PCHAR lpszString2, BOOLEAN bCaseInsensitive);
LONG     CompareStringW(PWSTR lpszString1, PWSTR lpszString2, BOOLEAN bCaseInsensitive);
VOID     ConvertToUpper(PCHAR Dest, PCHAR Source, ULONG Len);
VOID     ConvertToUpperW(PWSTR Dest, PWSTR Source, ULONG Len);
PCHAR    strncatZ(PCHAR dest, PCHAR source, int length);
int      __strcmpi(char *lpszString1, char *lpszString2);
char*    _strlow(char *lpszString);
char*    _strup(char *lpszString);
BOOLEAN  Match(char*string, char *pattern);
BOOLEAN  MatchW(WCHAR *string, WCHAR *pattern);
PWSTR    _skipdelimit(PWSTR pwszString, PWSTR pwszDelimit);
PWSTR    _nextdelimit(PWSTR pwszString, PWSTR pwszDelimit);
PWSTR    _wcstok(PWSTR pwszString, PWSTR pwszDelimit, PWSTR* ppwszLast);
#endif //__KRNL_STDLIB_H