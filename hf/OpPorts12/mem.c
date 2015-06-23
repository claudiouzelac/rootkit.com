#include <ntddk.h>
#include <stdarg.h>
#include <stdio.h>
#include <ntverp.h>

#define IOCTL_READ_OBJ_INFO \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define NT_DEVICE_NAME          L"\\Device\\ReadMemDriver"
#define DOS_DEVICE_NAME         L"\\DosDevices\\ReadMemDriver"
#define DEVICE_NAME		L"\\\\.\\ReadMemDriver"

typedef struct
{
  UCHAR ObType,Alloc,Size,Res;
  PVOID Ptr1,Ptr2,Ptr3;
  ULONG Flags;
} OBJECT_INFO,*POBJECT_INFO;

typedef struct 
{
  PVOID ObjAddr;
} RMD_IN;

typedef struct 
{
  OBJECT_INFO Info;
  UCHAR Name[256];
} RMD_OUT;


PDEVICE_OBJECT ReadMemDriverDeviceObject = NULL;

NTSTATUS ReadMemDriverIO(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  IoCompleteRequest(Irp,IO_NO_INCREMENT);
  return Irp->IoStatus.Status;
}

NTSTATUS ReadMemDriverIOControl(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
  PIO_STACK_LOCATION stack;
  UCHAR *in_buffer, *out_buffer;
  ULONG code,len;
  PVOID ptr;
  ANSI_STRING AnsiName;
  PUNICODE_STRING UniName;

  stack = IoGetCurrentIrpStackLocation(Irp);
  code = stack->Parameters.DeviceIoControl.IoControlCode;
  in_buffer = out_buffer = Irp->AssociatedIrp.SystemBuffer;

  if (code==IOCTL_READ_OBJ_INFO) 
  {
    ptr=((RMD_IN *)in_buffer)->ObjAddr;
    ((RMD_OUT *)out_buffer)->Info=*(OBJECT_INFO *)ptr;

    UniName=(PUNICODE_STRING)(((ULONG)((RMD_OUT *)out_buffer)->Info.Ptr1)-0x24);
    if (UniName->Length!=0)
    {
      if (RtlUnicodeStringToAnsiString(&AnsiName,UniName,1)==0)
      { 
        len=AnsiName.Length;
        if (len>255) len=255;
        strncpy((char *)&((RMD_OUT *)out_buffer)->Name,AnsiName.Buffer,len);
        ((RMD_OUT *)out_buffer)->Name[len]=0;
        RtlFreeAnsiString(&AnsiName);
      }
    }
  }

  Irp->IoStatus.Information = sizeof(RMD_OUT);
  Irp->IoStatus.Status = STATUS_SUCCESS;
  IoCompleteRequest(Irp,IO_NO_INCREMENT);
  return STATUS_SUCCESS;
}

VOID ReadMemDriverUnload(IN PDRIVER_OBJECT DriverObject)
{
  UNICODE_STRING win32DeviceName;

  RtlInitUnicodeString(&win32DeviceName,DOS_DEVICE_NAME);
  IoDeleteSymbolicLink(&win32DeviceName);

  IoDeleteDevice(ReadMemDriverDeviceObject);
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
                                          &ReadMemDriverDeviceObject)))
    return STATUS_NO_SUCH_DEVICE;

  ReadMemDriverDeviceObject->Flags |= DO_BUFFERED_IO;
  RtlInitUnicodeString(&win32DeviceName,DOS_DEVICE_NAME);

  if (!NT_SUCCESS(status = IoCreateSymbolicLink(&win32DeviceName,
                                                &ntDeviceName)))
    return STATUS_NO_SUCH_DEVICE;

  DriverObject->MajorFunction[IRP_MJ_CREATE        ] = ReadMemDriverIO;
  DriverObject->MajorFunction[IRP_MJ_CLOSE         ] = ReadMemDriverIO;
  DriverObject->MajorFunction[IRP_MJ_READ          ] = ReadMemDriverIO;
  DriverObject->MajorFunction[IRP_MJ_WRITE         ] = ReadMemDriverIO;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ReadMemDriverIOControl;
  DriverObject->DriverUnload                         = ReadMemDriverUnload;

  return STATUS_SUCCESS;
}
