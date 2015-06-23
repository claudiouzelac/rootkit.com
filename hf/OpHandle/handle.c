#include <ntddk.h>
#include <stdarg.h>
#include <stdio.h>
#include <ntverp.h>
#include <ntifs.h>

#include "handle.h"


PDEVICE_OBJECT HwndNameDriverDeviceObject = NULL;
ULONG out_size;

int handle_fobject(PFILE_OBJECT fobject, PUCHAR obuffer) 
{
  ULONG length;
  ANSI_STRING astring;
  PUCHAR fname, cur_pointer;
  PFILE_OBJECT related_fobject;
  UCHAR status;

  fname=obuffer+12;
  status=0;
	
  if(fobject->DeviceObject!=NULL) 
  {
    if(NT_SUCCESS(ObQueryNameString(fobject->DeviceObject, 
                  (POBJECT_NAME_INFORMATION)fname,out_size-20,&length))) 
    {
      if(NT_SUCCESS(RtlUnicodeStringToAnsiString(&astring,
                                                 (PUNICODE_STRING)fname,TRUE))) 
      {
        *fname='\0';
        strncpy(fname, astring.Buffer, astring.Length+1);
        status=1;

        fname+=astring.Length;
        *fname='\0';

        RtlFreeAnsiString(&astring);
      }
    }
  }

  if((length=fobject->FileName.Length>>1)||(fobject->RelatedFileObject!=NULL)) 
  {
    related_fobject=fobject->RelatedFileObject;
    if(length&&(fobject->FileName.Buffer[0]!='\\')) 
      while(related_fobject!=NULL) 
      {
        length+=related_fobject->FileName.Length>>1;
        related_fobject=related_fobject->RelatedFileObject;
      }

    if(length) 
    {
      RtlUnicodeStringToAnsiString(&astring,&(fobject->FileName),TRUE);
      strncpy(cur_pointer=fname+(length-(fobject->FileName.Length>>1)),
              astring.Buffer,astring.Length+1);
      status=1;
      RtlFreeAnsiString(&astring);

      related_fobject=fobject->RelatedFileObject;

      if(fobject->FileName.Buffer[0]!='\\') 
        while(related_fobject!=NULL) 
        {
          *(cur_pointer-1)='\\';
          cur_pointer-=(related_fobject->FileName.Length>>1);//+1; 

          RtlUnicodeStringToAnsiString(&astring,&(related_fobject->FileName), 
                                       TRUE);
          strncpy(cur_pointer, astring.Buffer, astring.Length+1);
          status=1;      
          RtlFreeAnsiString(&astring);

          related_fobject=related_fobject->RelatedFileObject;
        }
    }
  }	
  return(status?(fname-obuffer)+length:0);
}

NTSTATUS HwndNameDriverIO(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  IoCompleteRequest(Irp,IO_NO_INCREMENT);
  return Irp->IoStatus.Status;
}

NTSTATUS HwndNameDriverIOControl(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
  PIO_STACK_LOCATION stack;
  UCHAR *in_buffer, *out_buffer;
  ULONG code,ret,pid,handle_object,return_length;
  UCHAR buffer[1024];
  PEPROCESS eprocess;
  HANDLE handle,hProcess,hToken;

  stack = IoGetCurrentIrpStackLocation(Irp);
  out_size = stack->Parameters.DeviceIoControl.OutputBufferLength;
  code = stack->Parameters.DeviceIoControl.IoControlCode;

  in_buffer = out_buffer = Irp->AssociatedIrp.SystemBuffer;

  ret = STATUS_SUCCESS;

  switch(code) 
  {
    case IOCTL_GET_NAME_STRING:
    {
      pid = ((DIB_NAME_STRING *)in_buffer)->pid;
      handle = ((DIB_NAME_STRING *)in_buffer)->hwnd; 

      ((DOB_NAME_STRING *)out_buffer)->status = 0;
      Irp->IoStatus.Information = sizeof(ULONG);

      if(NT_SUCCESS(PsLookupProcessByProcessId((PVOID)pid,&eprocess))) 
      {
        KeAttachProcess(eprocess);
        if(NT_SUCCESS(ObReferenceObjectByHandle(handle,0x80000000,0,0,
                                                (void *)&handle_object,0)))
        { 
          if(*(USHORT *)handle_object==5 && *((USHORT *)handle_object+1)==0x70)
          {
            if(return_length=handle_fobject((PFILE_OBJECT)handle_object,
               out_buffer)) 
            {
                ((DOB_NAME_STRING *)out_buffer)->status=1;
                Irp->IoStatus.Information+=return_length;
                *((USHORT *)out_buffer+2)=(USHORT)(return_length-12);
            }
          } 
           else 
          {
            if(NT_SUCCESS(ObQueryNameString((void *)handle_object,
                                          (POBJECT_NAME_INFORMATION)buffer,
                                          sizeof(buffer),&return_length))) 
              if(((UNICODE_STRING *)buffer)->Buffer!=NULL) 
              {
                ((DOB_NAME_STRING *)out_buffer)->name.MaximumLength \
                  = (USHORT)out_size-20;
                ((DOB_NAME_STRING *)out_buffer)->name.Buffer \
                  = (char *)((ULONG *)out_buffer+3);
                if(NT_SUCCESS(RtlUnicodeStringToAnsiString(
                                &((DOB_NAME_STRING *)out_buffer)->name,
                                (UNICODE_STRING *)buffer,FALSE)))
                {
                  ((DOB_NAME_STRING *)out_buffer)->status = 1;
                  Irp->IoStatus.Information += 8+
                  ((DOB_NAME_STRING *)out_buffer)->name.Length;
                }
              }  
              ObDereferenceObject((void *)handle_object);
          }
        }
        KeDetachProcess();
        ObDereferenceObject((void *)eprocess);
      }
      break;
    }
    case IOCTL_GET_TOKEN_HANDLE:
    {
      hProcess = ((DIB_TOKEN_HANDLE *)in_buffer)->hwnd; 
      ((DOB_TOKEN_HANDLE *)out_buffer)->status = 0;
      Irp->IoStatus.Information = sizeof(ULONG);

      if (NT_SUCCESS(ZwOpenProcessToken(hProcess,TOKEN_QUERY,&hToken)))
      {
        ((DOB_TOKEN_HANDLE *)out_buffer)->status = 1;
        ((DOB_TOKEN_HANDLE *)out_buffer)->hwnd = hToken;
        Irp->IoStatus.Information += 4;
      }
      break;
    }
    default:
     ((DOB_UNKNOWN *)out_buffer)->status = 0; 
     Irp->IoStatus.Information = sizeof(DOB_UNKNOWN);
     ret = STATUS_INVALID_DEVICE_REQUEST;
     break;
  }

  Irp->IoStatus.Status = ret;
  IoCompleteRequest(Irp,IO_NO_INCREMENT);
  return ret;
}

VOID HwndNameDriverUnload(IN PDRIVER_OBJECT DriverObject)
{
  UNICODE_STRING win32DeviceName;

  RtlInitUnicodeString(&win32DeviceName,DOS_DEVICE_NAME);
  IoDeleteSymbolicLink(&win32DeviceName);

  IoDeleteDevice(HwndNameDriverDeviceObject);
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
                     IN PUNICODE_STRING RegistryPath)
{
  UNICODE_STRING ntDeviceName;
  UNICODE_STRING win32DeviceName;
  NTSTATUS status;

  RtlInitUnicodeString(&ntDeviceName,NT_DEVICE_NAME);

  if (!NT_SUCCESS(status = IoCreateDevice(DriverObject,0,&ntDeviceName,
                                          FILE_DEVICE_UNKNOWN,0,FALSE,
                                          &HwndNameDriverDeviceObject)))
    return STATUS_NO_SUCH_DEVICE;

  HwndNameDriverDeviceObject->Flags |= DO_BUFFERED_IO;
  RtlInitUnicodeString(&win32DeviceName,DOS_DEVICE_NAME);

  if (!NT_SUCCESS(status = IoCreateSymbolicLink(&win32DeviceName,
                                                &ntDeviceName)))
    return STATUS_NO_SUCH_DEVICE;

  DriverObject->MajorFunction[IRP_MJ_CREATE        ] = HwndNameDriverIO;
  DriverObject->MajorFunction[IRP_MJ_CLOSE         ] = HwndNameDriverIO;
  DriverObject->MajorFunction[IRP_MJ_READ          ] = HwndNameDriverIO;
  DriverObject->MajorFunction[IRP_MJ_WRITE         ] = HwndNameDriverIO;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HwndNameDriverIOControl;
  DriverObject->DriverUnload                         = HwndNameDriverUnload;

  return STATUS_SUCCESS;
}
