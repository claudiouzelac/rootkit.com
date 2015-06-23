/******************************************************************************
  Author		: Kdm (Kodmaker@syshell.org)
  WebSite		: http://www.syshell.org

  Copyright (C) 2003,2004 Kdm
  *****************************************************************************
  This file is part of NtIllusion.

  NtIllusion is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  NtIllusion is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with NtIllusion; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  ******************************************************************************/

#ifndef _KNTIPROCESS_H
#define _KNTIPROCESS_H

//	oo NtQuerySystemInformation oo
#define STATUS_SUCCESS   ((UINT)0x00000000L)
typedef enum _FINDEX_INFO_LEVELS
{
  FindExInfoStandard
} FINDEX_INFO_LEVELS;

typedef enum _FINDEX_SEARCH_OPS
{
  FindExSearchNameMatch, 
  FindExSearchLimitToDirectories, 
  FindExSearchLimitToDevices
} FINDEX_SEARCH_OPS;
typedef struct _UNICODE_STRING {
  USHORT  Length;
  USHORT  MaximumLength;
  PWSTR   Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _STRING {
  USHORT  Length;
  USHORT  MaximumLength;
  PCHAR   Buffer;
} ANSI_STRING, *PANSI_STRING;
typedef struct _SYSTEM_THREAD_INFORMATION {
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    ULONG WaitTime;
    PVOID StartAddress;
    DWORD ClientId;
    DWORD Priority;
    LONG BasePriority;
    ULONG ContextSwitches;
    ULONG ThreadState;
    ULONG WaitReason;
} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;
typedef struct _SYSTEM_PROCESS_INFORMATION {
    DWORD          NextEntryDelta;           // relative offset
    DWORD          dThreadCount;
    DWORD          dReserved01;
    DWORD          dReserved02;
    DWORD          dReserved03;
    DWORD          dReserved04;
    DWORD          dReserved05;
    DWORD          dReserved06;
    FILETIME       ftCreateTime;     // relative to 01-01-1601
    FILETIME       ftUserTime;       // 100 nsec units
    FILETIME       ftKernelTime;     // 100 nsec units
    UNICODE_STRING ProcessName;
    DWORD          BasePriority;
    DWORD          dUniqueProcessId;
    DWORD          dParentProcessID;
    DWORD          dHandleCount;
    DWORD          dReserved07;
    DWORD          dReserved08;
    DWORD          VmCounters;
    DWORD          dCommitCharge;   // bytes
    SYSTEM_THREAD_INFORMATION  ThreadInfos[1];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;
typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation = 0,
    SystemPerformanceInformation = 2,
    SystemTimeOfDayInformation = 3,
    SystemProcessInformation = 5,
    SystemProcessorPerformanceInformation = 8,
    SystemInterruptInformation = 23,
    SystemExceptionInformation = 33,
    SystemRegistryQuotaInformation = 37,
    SystemLookasideInformation = 45
} SYSTEM_INFORMATION_CLASS;

DWORD WINAPI MyNtQuerySystemInformation(DWORD SystemInformationClass, 
			PVOID SystemInformation, ULONG SystemInformationLength,
			PULONG ReturnLength);

#endif