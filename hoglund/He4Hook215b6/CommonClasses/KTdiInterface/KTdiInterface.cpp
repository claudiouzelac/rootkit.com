#include "KTdiInterface.h"
//#include "../KLocker/KLocker.h"

#define ALIGN_4(x) \
        (((x) & 0x00000003) ? (((x) & 0xfffffffc) + 4) : (x))



KTdiInterface::KTdiInterface()
               : m_bOpen(FALSE)
{             
  //m_dwTreatIrpsCount = 0;
  m_TreatIrpsCount = 0;

  m_nLocalPort = 0;
  m_dwLocalAddress = 0;
  
  m_hTdiTransport = NULL;
  m_pTdiTransportObject = NULL;
  m_hTdiConnection = NULL;
  m_pTdiConnectionObject = NULL;
}

KTdiInterface::~KTdiInterface()
{
  Close();
}

BOOLEAN KTdiInterface::Open(IN PWSTR pwszProtocol)
{
  //KLocker locker(&m_KSynchroObject);

  if (Close() != TRUE)
    return FALSE;

  //m_dwTreatIrpsCount = 0;
  m_TreatIrpsCount = 0;

  if (pwszProtocol != NULL)
  {
    m_pwszProtocol = new WCHAR[wcslen(pwszProtocol)+sizeof(WCHAR)];
    if (m_pwszProtocol != NULL)
    {
      wcscpy(m_pwszProtocol, pwszProtocol);
      m_bOpen = TRUE;
    }
  }
  
  return m_bOpen;
}

BOOLEAN KTdiInterface::Close()
{
  //KLocker locker(&m_KSynchroObject);

  if (m_bOpen == TRUE)
  {
    //if (m_dwTreatIrpsCount == 0)
    if (m_TreatIrpsCount.CompareExchange(0, 0) == TRUE)
    {
      m_bOpen = FALSE;

      TdiCloseConnection();
      TdiCloseTransport();

      m_nLocalPort = 0;
      m_dwLocalAddress = 0;

      delete[] m_pwszProtocol;
      m_pwszProtocol = NULL;
    
      //m_dwTreatIrpsCount = 0;
      m_TreatIrpsCount = 0;
    }
  }

  return !m_bOpen;
}

BOOLEAN KTdiInterface::TdiOpenTransport(IN USHORT wPort)
{
  PTA_IP_ADDRESS            pAddress;                               // transport address
  ULONG                     dEaLength;                              // buffer size
  PFILE_FULL_EA_INFORMATION pEaInfo;                                // pointer to ea
  NTSTATUS                  dStatus;                                // current status
  BOOLEAN                   bRes = FALSE;

  if (m_bOpen == TRUE)
  {
    TdiCloseTransport();

    m_nLocalPort = wPort;

    dEaLength = sizeof ( FILE_FULL_EA_INFORMATION ) +                 // account for ea
                sizeof ( TdiTransportAddress) +                       // account for transport
                sizeof ( TA_IP_ADDRESS ) + 1;                         // account for ip address
    dEaLength = ALIGN_4(dEaLength);
    pEaInfo = (PFILE_FULL_EA_INFORMATION) new char[dEaLength];
    if (pEaInfo)                                                      // validate pointer
    {
      RtlZeroMemory(pEaInfo, dEaLength);                              // clear eabuffer
      pEaInfo->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;           // size
      RtlCopyMemory(pEaInfo->EaName,                                  // copy transport name
                    TdiTransportAddress, 
                    sizeof(TdiTransportAddress));
      pEaInfo->EaValueLength = sizeof(TA_IP_ADDRESS);                 // size of data
      pAddress = (PTA_IP_ADDRESS)(pEaInfo->EaName + sizeof(TdiTransportAddress));
      pAddress->TAAddressCount = 1;                                   // number of addresses
      pAddress->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;//sizeof(TDI_ADDRESS_IP);
      pAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
      pAddress->Address[0].Address[0].sin_port = W_LITTLE_TO_BIG_ENDIAN(wPort); // local port
      pAddress->Address[0].Address[0].in_addr = 0L;//D_LITTLE_TO_BIG_ENDIAN(0x7f000001);//0L;                   // local address
      dStatus = STATUS_SUCCESS;
      dStatus = TdiOpen (                                             // open tdi device
                         m_pwszProtocol,                              // tdi device name
                         dEaLength,                                   // length of ea info
                         pEaInfo,                                     // pointer to ea info
                         &m_hTdiTransport,                            // return transport handle
                         &m_pTdiTransportObject);                     // return transport object
      if (NT_SUCCESS(dStatus))
        bRes = TRUE;
      
      delete[] pEaInfo;                                               // free buffer
    }
  }

  return bRes;
}

BOOLEAN KTdiInterface::TdiCloseTransport()
{
  TdiClose(m_hTdiTransport, m_pTdiTransportObject);
  m_hTdiTransport = NULL;
  m_pTdiTransportObject = NULL;

  return TRUE;
}

BOOLEAN KTdiInterface::TdiOpenConnection()
{
  PTA_IP_ADDRESS            pAddress;                               // transport address
  ULONG                     dEaLength;                              // buffer size
  PFILE_FULL_EA_INFORMATION pEaInfo;                                // pointer to ea
  NTSTATUS                  dStatus;                                // current status
  ULONG*                    _this;
  BOOLEAN                   bRes = FALSE;

  if (m_bOpen == TRUE)
  {
    TdiCloseConnection();

    dEaLength = sizeof (FILE_FULL_EA_INFORMATION) +                   // account for ea
                sizeof (TdiConnectionContext) +                       // account for transport
                sizeof (CONNECTION_CONTEXT) + 1;                      // account for this
    dEaLength = ALIGN_4(dEaLength);
    pEaInfo = (PFILE_FULL_EA_INFORMATION) new char[dEaLength];
    if (pEaInfo)                                                      // validate pointer
    {
      RtlZeroMemory(pEaInfo, dEaLength);                              // clear eabuffer
      pEaInfo->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;          // size
      RtlCopyMemory(pEaInfo->EaName,                                  // copy transport name
                    TdiConnectionContext, 
                    sizeof(TdiConnectionContext));
      pEaInfo->EaValueLength = sizeof(ULONG);                         // size of data
      _this = (ULONG*)(pEaInfo->EaName + sizeof(TdiConnectionContext));
      *_this  = (ULONG)this;                                          // number of addresses
    
      dStatus = STATUS_SUCCESS;
      dStatus = TdiOpen (                                             // open tdi device
                         m_pwszProtocol,                              // tdi device name
                         dEaLength,                                   // length of ea info
                         pEaInfo,                                     // pointer to ea info
                         &m_hTdiConnection,                           // return transport handle
                         &m_pTdiConnectionObject);                    // return transport object
      if (NT_SUCCESS(dStatus))
        bRes = TRUE;

      delete[] pEaInfo;                                               // free buffer
    }
  }

  return bRes;
}

BOOLEAN KTdiInterface::TdiCloseConnection()
{
  TdiClose(m_hTdiConnection, m_pTdiConnectionObject);
  m_hTdiConnection = NULL;
  m_pTdiConnectionObject = NULL;

  return TRUE;
}

NTSTATUS 
KTdiInterface::TdiOpen(
   IN PWSTR pProtocol,
   IN ULONG dEaLength,
   IN PFILE_FULL_EA_INFORMATION pEaInfo,
   OUT PHANDLE phHandle,
   OUT PFILE_OBJECT* ppObject)
{
  UNICODE_STRING    uName;                                          // local name
  OBJECT_ATTRIBUTES ObjectAttrib;                                   // local object attribute
  IO_STATUS_BLOCK   IoStatusBlock;                                  // local io status return
  NTSTATUS          dStatus;                                        // current status

  RtlInitUnicodeString(&uName, pProtocol);                          // get device name
  InitializeObjectAttributes( &ObjectAttrib,                        // return object attribute
                          &uName,                                   // resource name
                          OBJ_CASE_INSENSITIVE,                     // attributes
                          NULL,                                     // root directory
                          NULL );                                   // security descriptor
  dStatus = ZwCreateFile(
                          phHandle,                                 // return file handle
                          GENERIC_READ | GENERIC_WRITE,             // desired access
                          &ObjectAttrib,                            // local object attribute
                          &IoStatusBlock,                           // local io status
                          0L,                                       // initial allocation size
                          FILE_ATTRIBUTE_NORMAL,                    // file attributes
                          FILE_SHARE_READ | FILE_SHARE_WRITE,       // share access
                          FILE_OPEN/*_IF*/,                             // create disposition
                          FILE_NO_INTERMEDIATE_BUFFERING/*0L*/,                                       // create options
                          pEaInfo,                                  // eabuffer
                          dEaLength );                              // ealength
  if (NT_SUCCESS(dStatus))                                          // check for valid return
  {
    dStatus  = ObReferenceObjectByHandle(                           // reference file object
                            *phHandle,                              // handle to open file
                            0, //GENERIC_READ | GENERIC_WRITE,           // access mode
                            NULL,                                   // object type
                            KernelMode,                             // access mode
                            (PVOID*)ppObject,                       // pointer to object
                            NULL );                                 // handle information
    if (!NT_SUCCESS(dStatus))                                   
    {
      DbgPrint ("KTdiInterface::TdiOpen: ObReferenceObjectByHandle is ERROR - %08x!!!\n", dStatus);
      ZwClose(*phHandle);                                           // close handle
    }
  }
  else
  {
    DbgPrint ("KTdiInterface::TdiOpen: ZwCreateFile is ERROR - %08x!!!\n", dStatus);
  }
  return ( dStatus );
}

NTSTATUS 
KTdiInterface::TdiClose(
   IN HANDLE hHandle,
   IN PFILE_OBJECT pObject)
{
  if (pObject)                                                      // validate pointers
    ObDereferenceObject(pObject);                                   // release the object

  if (hHandle)
  {
    NTSTATUS NtStatus = ZwClose(hHandle);
    if (NT_SUCCESS(NtStatus) == FALSE)                                               // close handle
      DbgPrint ("KTdiInterface::TdiClose: ZwClose is ERROR - %08x!!!\n", NtStatus);
  }
  return (STATUS_SUCCESS);
}

void KTdiInterface::TdiCallThread(IN TDI_CALL_INFO* pTdiCallInfo)
{
  KTdiInterface* _this = pTdiCallInfo->m_pThis;

  pTdiCallInfo->m_NtStatus = _this->TdiCall(pTdiCallInfo->m_pIrp, pTdiCallInfo->m_pDeviceObject, pTdiCallInfo->m_pIoStatusBlock);
  
  KeSetEvent(&pTdiCallInfo->m_kEvent, 0, FALSE);
}

NTSTATUS 
KTdiInterface::TdiCallEx( 
   IN PIRP pIrp,
   IN PDEVICE_OBJECT pDeviceObject,
   IN OUT PIO_STATUS_BLOCK pIoStatusBlock)
{
  WORK_QUEUE_ITEM WorkItem;
  TDI_CALL_INFO   TdiCallInfo;

  TdiCallInfo.m_pThis = this;
  TdiCallInfo.m_NtStatus = 0;
  TdiCallInfo.m_pIrp = pIrp;
  TdiCallInfo.m_pDeviceObject = pDeviceObject;
  TdiCallInfo.m_pIoStatusBlock = pIoStatusBlock;

  KeInitializeEvent(&TdiCallInfo.m_kEvent, NotificationEvent, FALSE); 
  ExInitializeWorkItem(&WorkItem, (PWORKER_THREAD_ROUTINE)TdiCallThread, &TdiCallInfo);
  ExQueueWorkItem(&WorkItem, DelayedWorkQueue);
  KeWaitForSingleObject(&TdiCallInfo.m_kEvent, Executive, KernelMode, FALSE, NULL);

  return TdiCallInfo.m_NtStatus;
}

NTSTATUS
KTdiInterface::TdiCallCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
  KTdiInterface* _this = (KTdiInterface*) Context;

  if (Irp->UserIosb != NULL)
    *Irp->UserIosb = Irp->IoStatus;

  if (Irp->MdlAddress != NULL)
  {
    MmUnlockPages(Irp->MdlAddress);
    IoFreeMdl(Irp->MdlAddress);
  }

  if (Irp->UserEvent != NULL)
    KeSetEvent(Irp->UserEvent, 0, FALSE);

  IoFreeIrp(Irp);
  
  if (_this != NULL)
    --(_this->m_TreatIrpsCount);
    //InterlockedDecrement(&(_this->m_dwTreatIrpsCount));

  return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS 
KTdiInterface::TdiCall( 
   IN PIRP pIrp,
   IN PDEVICE_OBJECT pDeviceObject,
   IN OUT PIO_STATUS_BLOCK pIoStatusBlock,
   IN BOOLEAN bWait,
   IN PKEVENT pkEvent)
{
  KEVENT kEvent;                                                    // signaling event
  NTSTATUS dStatus = STATUS_INSUFFICIENT_RESOURCES;                 // local status

  //InterlockedIncrement(&m_dwTreatIrpsCount);
  //InterlockedIncrement(&m_dwTreatIrpsCount);
  ++m_TreatIrpsCount;
  ++m_TreatIrpsCount;

  if (bWait == TRUE)
  {
    if (pkEvent == NULL)
    {
      pkEvent = &kEvent;
      KeInitializeEvent(pkEvent, NotificationEvent, FALSE);         // reset notification event
    }
    pIrp->UserEvent = pkEvent;                                      // pointer to event
  }
  pIrp->UserIosb = pIoStatusBlock;                                  // pointer to status block

  IoSetCompletionRoutine(pIrp, TdiCallCompletion, (PVOID)this, TRUE, TRUE, TRUE);

  dStatus = IoCallDriver(pDeviceObject, pIrp);                      // call next driver
  if (dStatus == STATUS_PENDING && bWait == TRUE)                   // make all request synchronous
  {
    (void)KeWaitForSingleObject ( 
                    (PVOID)pkEvent,                                 // signaling object
                    Suspended,                                      // wait reason
                    KernelMode,                                     // wait mode
                    TRUE,                                           // alertable
                    NULL );                                         // timeout
    dStatus = pIoStatusBlock->Status;
  }
  
  //InterlockedDecrement(&m_dwTreatIrpsCount);
  --m_TreatIrpsCount;

  return ( dStatus );                                               // return with status
}

NTSTATUS 
KTdiInterface::TdiQueryDeviceControl( 
   IN PFILE_OBJECT pObject,
   IN ULONG dIoControlCode,
   IN PVOID InputBuffer,
   IN ULONG InputBufferSize,
   IN OUT PVOID OutputBuffer,
   IN ULONG OutputBufferSize,
   OUT PULONG pdReturn)
{
  PIRP pIrp;                                                        // local i/o request
  PIO_STACK_LOCATION pIoStack;                                      // I/O Stack Location
  PDEVICE_OBJECT pDeviceObject;                                     // local device object
  IO_STATUS_BLOCK IoStatusBlock;                                    // return status
  NTSTATUS dStatus = STATUS_INVALID_PARAMETER;                      // default return status

  if (pObject)
  {
    pDeviceObject = IoGetRelatedDeviceObject ( pObject );           // get device object
    pIrp = IoBuildDeviceIoControlRequest (
                dIoControlCode,
                pDeviceObject,
                InputBuffer,
                InputBufferSize,
                OutputBuffer,
                OutputBufferSize,
                FALSE,
                NULL,                                               // pointer to event
                NULL );                                             // pointer to return buffer
    if (pIrp == NULL)
    {
      DbgPrint ( "ERROR: IoBuildDeviceIoControlRequest\n" );
      dStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
      pIoStack = IoGetNextIrpStackLocation ( pIrp );                // get the iostack
      pIoStack->DeviceObject = pDeviceObject;                       // store current device object
      pIoStack->FileObject = pObject;                               // store file object in the stack
      dStatus = TdiCall ( pIrp, pDeviceObject, &IoStatusBlock );
      if (pdReturn)                                                 // requested by user?
        *pdReturn = IoStatusBlock.Information;                      // return information size
    }
  }
  return ( dStatus );                                               // return with status
}


NTSTATUS 
KTdiInterface::TdiQueryInformationEx(
   IN PFILE_OBJECT pObject,
   IN ULONG dEntity,
   IN ULONG dInstance,
   IN ULONG dClass,
   IN ULONG dType,
   IN ULONG dId,
   IN PVOID pOutputBuffer,
   IN PULONG pdOutputLength )
{
  TCP_REQUEST_QUERY_INFORMATION_EX QueryInfo;                       // local query information

  RtlZeroMemory ( &QueryInfo, sizeof ( TCP_REQUEST_QUERY_INFORMATION_EX ) ); // input buffer length
  QueryInfo.ID.toi_entity.tei_entity = dEntity;
  QueryInfo.ID.toi_entity.tei_instance = dInstance;
  QueryInfo.ID.toi_class = dClass;
  QueryInfo.ID.toi_type = dType;
  QueryInfo.ID.toi_id = dId;
  return ( TdiQueryDeviceControl (                                  // send down device control call
                 pObject,                                           // current transport/connection object
                 IOCTL_TCP_QUERY_INFORMATION_EX,                    // control code
                 &QueryInfo,                                        // input buffer
                 sizeof ( TCP_REQUEST_QUERY_INFORMATION_EX ),       // input buffer length
                 pOutputBuffer,                                     // output buffer
                 *pdOutputLength,                                   // output buffer length
                 pdOutputLength ) );                                // return information
}


NTSTATUS 
KTdiInterface::TdiQueryAddress(
   IN PFILE_OBJECT pObject,
   IN PULONG pdAddress)
{
  ULONG        i, j;                                                // local loop control
  TDIEntityID* pEntityBuffer = NULL;                                // buffer for ENTITY_LIST_ID
  ULONG        dEntityCount;                                        // number of entities
  ULONG        dEntityType;                                         // entity type
  IPSNMPInfo   SnmpInfo;                                            // ip information
  IPAddrEntry* pIpAddress = NULL;                                   // ip address buffer
  ULONG        dBufferSize;                                         // buffer length
  NTSTATUS     dStatus = STATUS_INVALID_PARAMETER;                  // local status

  __try
  {
     *pdAddress = 0L;
     dBufferSize = MAX_FAST_ENTITY_BUFFER;                          // default buffer size
     for (j = 0; j < 2; ++j)
     {
       pEntityBuffer = (TDIEntityID*) new char[dBufferSize];        // allocate buffer
       if (pEntityBuffer == NULL)                                   // validate pointer
       {
         DbgPrint ("ERROR: ExAllocatePoolWithTag\n");
         dStatus = STATUS_INSUFFICIENT_RESOURCES;
         break;
       }
       else
       {
          // ********************************************
          // 1-Query device for entity buffer
          // ********************************************
         dStatus = TdiQueryInformationEx ( 
                               pObject,                             // control object
                               GENERIC_ENTITY,                      // entity
                               TL_INSTANCE,                         // instance
                               INFO_CLASS_GENERIC,                  // class
                               INFO_TYPE_PROVIDER,                  // type
                               ENTITY_LIST_ID,                      // id
                               pEntityBuffer,                       // output buffer
                               &dBufferSize );                      // output buffer size
         if (dStatus == STATUS_BUFFER_TOO_SMALL)                    // check for a buffer error
         {
           DbgPrint ( "ERROR: Buffer too small\n", dStatus );
           delete[] pEntityBuffer;                                  // free buffer
           pEntityBuffer = NULL;
         }
         else
         {
           if (!NT_SUCCESS(dStatus))                                // check return code
             DbgPrint ( "ERROR: Unable to get entity\n", dStatus );
           break;
         }
       }
     }

      // *********************************
      // Scan the entities looking for IP.
      // *********************************
     if (NT_SUCCESS(dStatus))
     {
       dEntityCount = dBufferSize / sizeof(TDIEntityID);            // determine number of entities
       for (i = 0; i < dEntityCount; ++i)                           // loop through all of them
       {
          // ********************************************
          // 2-Query device for entity type
          // ********************************************
         if (pEntityBuffer[i].tei_entity == CL_NL_ENTITY)
         {
           dBufferSize = sizeof(dEntityType);                       // store buffer size
           if ( NT_SUCCESS ( dStatus ) )                            // validate pointer
           {
             dStatus = TdiQueryInformationEx ( 
                                  pObject,                          // control object
                                  CL_NL_ENTITY,                     // entity
                                  pEntityBuffer[i].tei_instance,    // instance
                                  INFO_CLASS_GENERIC,               // class
                                  INFO_TYPE_PROVIDER,               // type
                                  ENTITY_TYPE_ID,                   // id
                                  &dEntityType,                     // output buffer
                                  &dBufferSize );                   // output buffer size
             if (!NT_SUCCESS(dStatus) || (dEntityType != CL_NL_IP)) // check for IP entity type
               DbgPrint("ERROR: Unable to get entity type\n", dStatus);
           }
           
            // ***************************************
            // 3-Query device for snmp info.
            // We found an IP entity. Now lookup its 
            // addresses.  Start by querying the number
            // of addresses supported by this interface.
            // ***************************************
           if (NT_SUCCESS(dStatus))
           {
             dBufferSize = sizeof(SnmpInfo);                        // store buffer size
             dStatus = TdiQueryInformationEx( 
                                  pObject,                          // control object
                                  CL_NL_ENTITY,                     // entity
                                  pEntityBuffer[i].tei_instance,    // instance
                                  INFO_CLASS_PROTOCOL,              // class
                                  INFO_TYPE_PROVIDER,               // type
                                  IP_MIB_STATS_ID,                  // id
                                  &SnmpInfo,                        // output buffer
                                  &dBufferSize);                    // output buffer size
             if (!NT_SUCCESS(dStatus) || (SnmpInfo.ipsi_numaddr == 0))
               DbgPrint ( "ERROR: Unable to get snmp\n", dStatus );
           }

            // ***************************************
            // 4-Query device for all ip addresses
            // ***************************************
           if (NT_SUCCESS(dStatus))
           {
             dBufferSize = SnmpInfo.ipsi_numaddr * sizeof(IPAddrEntry);
             for (j = 0; j < 2; ++j)
             {
               pIpAddress = (IPAddrEntry *) new char[dBufferSize]; // allocate buffer
               if (pIpAddress == NULL)
               {
                 DbgPrint ( "ERROR: ExAllocatePoolWithTag\n" );
                 dStatus = STATUS_INSUFFICIENT_RESOURCES;
                 break;
               }
               else
               {
                 dStatus = TdiQueryInformationEx ( 
                                      pObject,                     // control object
                                      CL_NL_ENTITY,                // entity
                                      pEntityBuffer[i].tei_instance,// instance
                                      INFO_CLASS_PROTOCOL,         // class
                                      INFO_TYPE_PROVIDER,          // type
                                      IP_MIB_ADDRTABLE_ENTRY_ID,   // id
                                      pIpAddress,                  // output buffer
                                      &dBufferSize );              // output buffer size
                 if (dStatus == STATUS_BUFFER_TOO_SMALL)           // check for a buffer error
                 {
                   DbgPrint ( "ERROR: Buffer too small\n", dStatus );
                   delete[] pIpAddress;                            // free buffer
                   pIpAddress = NULL;                              // reset pointer
                 }
                 else
                 {
                   if (!NT_SUCCESS(dStatus))                       // check return code
                     DbgPrint ( "ERROR: Unable to get address\n", dStatus );
                   else 
                   {
                     if (pdAddress)
                     {
                       *pdAddress =                                // store real ip address
                           D_BIG_TO_LITTLE_ENDIAN(pIpAddress->iae_addr);
                     }
                   }
                   break;                                          // break for loop
                 }
               }
             }
           }
         }
       }
     }
  }
  __finally
  {
    if (pEntityBuffer)                                             // validate pointer
      delete[] pEntityBuffer;                                      // free buffer
    if (pIpAddress)                                                // validate buffer
       delete[] pIpAddress;                                        // free buffer
    if (NT_SUCCESS(dStatus) && (*pdAddress == 0L))
      dStatus = STATUS_INVALID_PARAMETER;
  }

  return (dStatus);                                                // return with status
}

BOOLEAN KTdiInterface::SetEventHandler(IN int nEventType, IN PVOID pEventHandler, IN PVOID HandlerContext)
{
  BOOLEAN                      bRes = FALSE;
  PIRP                         pIrp = NULL, pIrpError = NULL;
  PDEVICE_OBJECT               pDeviceObject;
  NTSTATUS                     NtStatus;
  IO_STATUS_BLOCK              IoStatusBlock;

  __try
  {
    if (m_bOpen == TRUE && m_pTdiTransportObject != NULL)
    {
      pDeviceObject = IoGetRelatedDeviceObject(m_pTdiTransportObject);

      pIrp = TdiBuildInternalDeviceControlIrp(
                      TDI_SET_EVENT_HANDLER, 
                      pDeviceObject, 
                      m_pTdiTransportObject,
                      NULL, 
                      NULL);
      pIrpError = pIrp;
      if (pIrp != NULL)
      {
        TdiBuildSetEventHandler (
                 pIrp, 
                 pDeviceObject, 
                 m_pTdiTransportObject,
                 NULL, 
                 NULL,
                 nEventType,
                 pEventHandler,
                 HandlerContext);
        
        pIrpError = NULL;
        NtStatus = TdiCall(pIrp, pDeviceObject, &IoStatusBlock);
        if (NT_SUCCESS(NtStatus))
        {
          DbgPrint ("SetEventHandler: OK (%08x)!!!\n", NtStatus);
          bRes = TRUE;
        }
        else
        {
          DbgPrint ("SetEventHandler: ERROR (%08x)!!!\n", NtStatus);
        }
      }
    }
  }
  __finally
  {
    if (pIrpError != NULL)
      IoFreeIrp(pIrpError);
  }

  return bRes;
}
