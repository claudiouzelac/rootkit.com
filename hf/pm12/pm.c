#include <ntddk.h>
#include <stdarg.h>
#include <stdio.h>
#include <ntverp.h>
#include <ntifs.h>

#include "pm.h"


PDEVICE_OBJECT PMNameDriverDeviceObject = NULL;
ULONG out_size;


NTSTATUS PMNameDriverIO(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  IoCompleteRequest(Irp,IO_NO_INCREMENT);
  return Irp->IoStatus.Status;
}

NTSTATUS PMNameDriverIOControl(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
  PIO_STACK_LOCATION stack;
  UCHAR *in_buffer, *out_buffer;
  ULONG code,ret,pid,handle_object,rs1,rs2;
  UCHAR buffer[1024];
  PEPROCESS eprocess;
  HANDLE hCallingProcess,hTargetProcess,hTargetToken,hProcess,hToken;
  PROCESS_ACCESS_TOKEN NewAccessToken;
  OBJECT_ATTRIBUTES oa;
  CLIENT_ID ClientId;

  stack = IoGetCurrentIrpStackLocation(Irp);
  out_size = stack->Parameters.DeviceIoControl.OutputBufferLength;
  code = stack->Parameters.DeviceIoControl.IoControlCode;

  in_buffer = out_buffer = Irp->AssociatedIrp.SystemBuffer;

  ret = STATUS_SUCCESS;

  switch(code) 
  {
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
    case IOCTL_IMPERSONATE_PROCESS:
    {
      hCallingProcess = ((DIB_IMPERSONATE_PROCESS *)in_buffer)->hCalling; 
      hTargetProcess = ((DIB_IMPERSONATE_PROCESS *)in_buffer)->hTarget; 
      ((DOB_IMPERSONATE_PROCESS *)out_buffer)->status1 = 0;
      Irp->IoStatus.Information = sizeof(ULONG);

      if (NT_SUCCESS(ZwOpenProcessToken(hTargetProcess,TOKEN_ALL_ACCESS,
                                        &hTargetToken)))
      {
        oa.Length = sizeof(oa);
        oa.RootDirectory = 0;
        oa.ObjectName = NULL;
        oa.Attributes = 0;
        oa.SecurityDescriptor = NULL;
        oa.SecurityQualityOfService = NULL;

	rs1 = ZwDuplicateToken(hTargetToken,TOKEN_ALL_ACCESS,&oa,FALSE,
                               TokenPrimary,&NewAccessToken.Token);
	ZwClose(hTargetToken);

        NewAccessToken.Thread = 0;
        rs2 = ZwSetInformationProcess(hCallingProcess,ProcessAccessToken,
                                      &NewAccessToken,sizeof(NewAccessToken));
        ((DOB_IMPERSONATE_PROCESS *)out_buffer)->status1 = rs1;
        ((DOB_IMPERSONATE_PROCESS *)out_buffer)->status2 = rs2;
        Irp->IoStatus.Information += sizeof(ULONG);
      }
      break;
    }
    case IOCTL_KILL_PROCESS:
    {
      pid = ((DIB_KILL_PROCESS *)in_buffer)->dwProcessId; 
      ((DOB_KILL_PROCESS *)out_buffer)->status = 0;
      Irp->IoStatus.Information = sizeof(ULONG);

      ClientId.UniqueProcess = pid;
      ClientId.UniqueThread = 0;

      oa.Length = sizeof(oa);
      oa.RootDirectory = 0;
      oa.ObjectName = NULL;
      oa.Attributes = 0;
      oa.SecurityDescriptor = NULL;
      oa.SecurityQualityOfService = NULL;

      rs1=ZwOpenProcess(&hProcess,PROCESS_TERMINATE,&oa,&ClientId);

      if (NT_SUCCESS(rs1)) rs1 = ZwTerminateProcess(hProcess,0);

      ((DOB_KILL_PROCESS *)out_buffer)->status = rs1;
   
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

VOID PMNameDriverUnload(IN PDRIVER_OBJECT DriverObject)
{
  UNICODE_STRING win32DeviceName;

  RtlInitUnicodeString(&win32DeviceName,DOS_DEVICE_NAME);
  IoDeleteSymbolicLink(&win32DeviceName);

  IoDeleteDevice(PMNameDriverDeviceObject);
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
                                          &PMNameDriverDeviceObject)))
    return STATUS_NO_SUCH_DEVICE;

  PMNameDriverDeviceObject->Flags |= DO_BUFFERED_IO;
  RtlInitUnicodeString(&win32DeviceName,DOS_DEVICE_NAME);

  if (!NT_SUCCESS(status = IoCreateSymbolicLink(&win32DeviceName,
                                                &ntDeviceName)))
    return STATUS_NO_SUCH_DEVICE;

  DriverObject->MajorFunction[IRP_MJ_CREATE        ] = PMNameDriverIO;
  DriverObject->MajorFunction[IRP_MJ_CLOSE         ] = PMNameDriverIO;
  DriverObject->MajorFunction[IRP_MJ_READ          ] = PMNameDriverIO;
  DriverObject->MajorFunction[IRP_MJ_WRITE         ] = PMNameDriverIO;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PMNameDriverIOControl;
  DriverObject->DriverUnload                         = PMNameDriverUnload;

  return STATUS_SUCCESS;
}
