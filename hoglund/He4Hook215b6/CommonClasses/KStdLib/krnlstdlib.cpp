#include "krnlstdlib.h"

LONG CompareStringN(PCHAR lpszString1, PCHAR lpszString2, BOOLEAN bCaseInsensitive, ULONG dwLen)
{
  LONG    nRet;
  STRING  String1, String2;
  ULONG   dwLen1, dwLen2;
  CHAR    cSym1, cSym2;

  dwLen1 = strlen(lpszString1);
  dwLen2 = strlen(lpszString2);
  if (dwLen < dwLen1)
  {
    cSym1 = lpszString1[dwLen];
    lpszString1[dwLen] = 0;
  }

  if (dwLen < dwLen2)
  {
    cSym2 = lpszString2[dwLen];
    lpszString2[dwLen] = 0;
  }

  RtlInitString(&String1, lpszString1);
  RtlInitString(&String2, lpszString2);

  if (dwLen < dwLen1)
    lpszString1[dwLen] = cSym1;
  if (dwLen < dwLen2)
    lpszString2[dwLen] = cSym2;

  nRet = RtlCompareString(&String1, &String2, bCaseInsensitive);

  return nRet;
}

LONG CompareString(PCHAR lpszString1, PCHAR lpszString2, BOOLEAN bCaseInsensitive)
{
  LONG    nRet;
  STRING  String1, String2;

  RtlInitString(&String1, lpszString1);
  RtlInitString(&String2, lpszString2);
  nRet = RtlCompareString(&String1, &String2, bCaseInsensitive);

  return nRet;
}

LONG CompareStringW(PWSTR lpszString1, PWSTR lpszString2, BOOLEAN bCaseInsensitive)
{
  LONG    nRet;
  UNICODE_STRING  String1, String2;

  RtlInitUnicodeString(&String1, lpszString1);
  RtlInitUnicodeString(&String2, lpszString2);
  nRet = RtlCompareUnicodeString(&String1, &String2, bCaseInsensitive);

  return nRet;
}

VOID ConvertToUpper(PCHAR Dest, PCHAR Source, ULONG Len)
{
  ULONG   i;
  
  for (i = 0; i < Len; ++i)
  {
    if (Source[i] >= 'a' && Source[i] <= 'z')
    {
      Dest[i] = Source[i] - 'a' + 'A';
    }
    else
    {
      Dest[i] = Source[i];
    }
  }
}

VOID ConvertToUpperW(PWSTR Dest, PWSTR Source, ULONG Len)
{
  ULONG   i;
  
  for (i = 0; i < Len; ++i)
  {
    if (Source[i] >= L'a' && Source[i] <= L'z')
    {
      Dest[i] = Source[i] - L'a' + L'A';
    }
    else
    {
      Dest[i] = Source[i];
    }
  }
}

PCHAR strncatZ(PCHAR dest, PCHAR source, int length)
{       
  int     origlen = strlen(dest);

  strncpy(dest+origlen, source, length);
  dest[origlen+length] = 0;
  return(dest);
}

int __strcmpi(char *lpszString1, char *lpszString2)
{
  return strcmp(_strlow(lpszString1), _strlow(lpszString2));
}

char *_strlow(char *lpszString)
{
  int i, nSize = strlen(lpszString);

  for (i=0; i<nSize; ++i)
  {
    lpszString[i] = (char)tolower((int)lpszString[i]);
  }

  return lpszString;
}

char *_strup(char *lpszString)
{
  int i, nSize = strlen(lpszString);

  for (i=0; i<nSize; ++i)
  {
    lpszString[i] = (char)toupper((int)lpszString[i]);
  }

  return lpszString;
}

BOOLEAN Match(char *string, char *pattern)
{
  for (; '*'^*pattern; ++pattern, ++string) 
  {
    if (!*string)
      return (!*pattern);
    if (toupper(*string)^toupper(*pattern) && '?'^*pattern)
      return FALSE;
  }
  /* two-line patch to prevent *too* much recursiveness: */
  while ('*' == pattern[1])
    ++pattern;

  do
  {
    if (Match(string, pattern + 1))
      return TRUE;
  }
  while (*string++);

  return FALSE;
}

BOOLEAN MatchW(WCHAR *string, WCHAR *pattern)
{
  for (; L'*'^*pattern; ++pattern, ++string) 
  {
    if (!*string)
      return (!*pattern);
    if (towupper(*string)^towupper(*pattern) && L'?'^*pattern)
      return FALSE;
  }
  /* two-line patch to prevent *too* much recursiveness: */
  while (L'*' == pattern[1])
    ++pattern;

  do
  {
    if (MatchW(string, pattern + 1))
      return TRUE;
  }
  while (*string++);

  return FALSE;
}

PWSTR _skipdelimit(PWSTR pwszString, PWSTR pwszDelimit)
{
  PWSTR pwszDelimitTmp;
  
  pwszDelimitTmp = pwszDelimit;
  while (*pwszDelimitTmp != 0)
  {
    while (!(*pwszString ^ *pwszDelimitTmp))
    {
      ++pwszString;
      pwszDelimitTmp = pwszDelimit-1;
      break;
    }
    ++pwszDelimitTmp;
  }

  return pwszString;
}

PWSTR _nextdelimit(PWSTR pwszString, PWSTR pwszDelimit)
{
  PWSTR pwszLast = pwszString;

  PWSTR pwszDelimitTmp = pwszDelimit;
  while (*pwszLast != 0)
  {
    if (*pwszDelimitTmp == 0)
    {
      pwszDelimitTmp = pwszDelimit;
      ++pwszLast;
    }
    if (!(*pwszLast ^ *pwszDelimitTmp))
      break;
    ++pwszDelimitTmp;
  }
  
  return pwszLast;
}

PWSTR _wcstok(PWSTR pwszString, PWSTR pwszDelimit, PWSTR* ppwszLast)
{
  if (pwszString == NULL)
    pwszString = *ppwszLast;

  pwszString = _skipdelimit(pwszString, pwszDelimit);

  PWSTR pwszLast = _nextdelimit(pwszString, pwszDelimit);
  
  if (*pwszLast != 0)
  {
    *pwszLast = 0;
    ++pwszLast;
  }
  *ppwszLast = pwszLast;

  if (*pwszString != 0)
    return pwszString;
  return NULL;
}
