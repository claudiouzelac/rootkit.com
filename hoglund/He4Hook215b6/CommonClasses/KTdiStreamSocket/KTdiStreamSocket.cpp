#include "KTdiStreamSocket.h"
//#include "../KLocker/KLocker.h"

#define ALIGN_4(x) \
        (((x) & 0x00000003) ? (((x) & 0xfffffffc) + 4) : (x))



KTdiStreamSocket::KTdiStreamSocket()
                : KTdiInterface(),
m_bOpen(FALSE)
{
  m_pRequestListenInfo = NULL;
  
  m_bBind = FALSE;
  m_bConnected = FALSE;
  m_bListen = FALSE;

  //m_dwTreatAcceptIrpsCount = 0;
  m_TreatAcceptIrpsCount = 0;
}

KTdiStreamSocket::~KTdiStreamSocket()
{
  //Disconnect();
  Close();
}

BOOLEAN KTdiStreamSocket::Open(IN USHORT nLocalPort)
{
  //KLocker locker(&m_KSynchroObject);

  //Disconnect();
  if (Close() == FALSE)
    return FALSE;

  if (KTdiInterface::Open(DD_TCP_DEVICE_NAME) == TRUE)
  {
    //m_KSynchroObject.Lock();
   
    if (TdiOpenSocket(nLocalPort) == TRUE)
    {
      DbgPrint ("\tm_hTdiTransport = %08x\n"
                "\tm_pTdiTransportObject = %08x\n"
                "\tm_hTdiConnection = %08x\n"
                "\tm_pTdiConnectionObject = %08x\n",
                 m_hTdiTransport,
                 m_pTdiTransportObject,
                 m_hTdiConnection,
                 m_pTdiConnectionObject);
   
      //KeInitializeEvent(&m_kReceiveInProgressEvent, SynchronizationEvent, FALSE);
      //KeInitializeEvent(&m_kSendInProgressEvent, SynchronizationEvent, FALSE);
      
      m_bOpen = TRUE;
    }
   
    //m_KSynchroObject.UnLock();
  }

  return m_bOpen;
}

BOOLEAN KTdiStreamSocket::Close()
{
  //KLocker locker(&m_KSynchroObject);

  //Disconnect();

  //m_KSynchroObject.Lock();

  //if (m_bOpen == TRUE && m_dwTreatAcceptIrpsCount == 0)
  if (m_bOpen == TRUE && m_TreatAcceptIrpsCount.CompareExchange(0, 0) == TRUE)
  {
    Disconnect();
    Unbind();
    
    m_bOpen = FALSE;
    m_bBind = FALSE;
    m_bConnected = FALSE;
    m_bListen = FALSE;
  }

  if (m_bOpen == TRUE || KTdiInterface::Close() == FALSE)
    return FALSE;

  //m_KSynchroObject.UnLock();

  return !m_bOpen;
}

BOOLEAN KTdiStreamSocket::Bind()
{
  //KLocker locker(&m_KSynchroObject);

  BOOLEAN         bRes = FALSE;
  PIRP            pIrp = NULL, pIrpError = NULL;
  PDEVICE_OBJECT  pDeviceObject;
  NTSTATUS        NtStatus;
  IO_STATUS_BLOCK IoStatusBlock;

  __try
  {
    if (m_bOpen == TRUE && Unbind() == TRUE)
    {
      pDeviceObject = IoGetRelatedDeviceObject(m_pTdiConnectionObject);
      
      pIrp = TdiBuildInternalDeviceControlIrp(
                    TDI_ASSOCIATE_ADDRESS, 
                    pDeviceObject, 
                    m_pTdiConnectionObject,
                    NULL, 
                    NULL);
      pIrpError = pIrp;
      if (pIrp != NULL)
      {
        TdiBuildAssociateAddress(
               pIrp, 
               pDeviceObject, 
               m_pTdiConnectionObject,
               NULL,
               NULL, 
               m_hTdiTransport);
        
        pIrpError = NULL;
        NtStatus = TdiCall(pIrp, pDeviceObject, &IoStatusBlock);
        if (NT_SUCCESS(NtStatus))
        {
          m_bBind = TRUE;
          bRes = TRUE;
        }
        else
        {
          DbgPrint ("TdiBind: ERROR (%08x)!!!\n", NtStatus);
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

BOOLEAN KTdiStreamSocket::Unbind()
{
  //KLocker locker(&m_KSynchroObject);

  BOOLEAN         bRes = TRUE;
  PIRP            pIrp = NULL, pIrpError = NULL;
  PDEVICE_OBJECT  pDeviceObject;
  NTSTATUS        NtStatus;
  IO_STATUS_BLOCK IoStatusBlock;

  __try
  {
    if (m_bOpen == TRUE && m_bBind == TRUE && m_bConnected == FALSE && m_bListen == FALSE)
    {
      bRes = FALSE;

      pDeviceObject = IoGetRelatedDeviceObject(m_pTdiConnectionObject);
      
      pIrp = TdiBuildInternalDeviceControlIrp(
                    TDI_DISASSOCIATE_ADDRESS, 
                    pDeviceObject, 
                    m_pTdiConnectionObject,
                    NULL, 
                    NULL);
      pIrpError = pIrp;
      if (pIrp != NULL)
      {
        TdiBuildDisassociateAddress(
               pIrp, 
               pDeviceObject, 
               m_pTdiConnectionObject,
               NULL,
               NULL);
        
        pIrpError = NULL;
        NtStatus = TdiCall(pIrp, pDeviceObject, &IoStatusBlock);
        if (NT_SUCCESS(NtStatus))
        {
          m_bBind = FALSE;
          bRes = TRUE;
        }
        else
        {
          DbgPrint ("TdiUnbind: ERROR (%08x)!!!\n", NtStatus);
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

BOOLEAN KTdiStreamSocket::Connect(IN USHORT wPort, IN ULONG dwAddress, ULONG dwTimeOut)
{
  //KLocker locker(&m_KSynchroObject);

  BOOLEAN                      bRes = FALSE;
  PIRP                         pIrp = NULL, pIrpError = NULL;
  PDEVICE_OBJECT               pDeviceObject;
  NTSTATUS                     NtStatus;
  PTDI_CONNECTION_INFORMATION  pRequestConnectionInfo = NULL;
  PTDI_CONNECTION_INFORMATION  pReturnConnectionInfo;
  PTA_IP_ADDRESS               pRequestAddress;
  PTDI_ADDRESS_IP              pIp;
  IO_STATUS_BLOCK              IoStatusBlock;
  LARGE_INTEGER                TimeOut;
  PLARGE_INTEGER               pTimeOut = NULL;

  __try
  {
    if (m_bOpen == TRUE && m_bBind == TRUE && m_bListen == FALSE && Disconnect() == TRUE)
    {
      m_nRemotePort = wPort;
      m_nRemoteAddress = dwAddress;

      if (dwTimeOut != 0)
      {
        pTimeOut = &TimeOut;
        TimeOut.QuadPart = dwTimeOut * 10000; // msec -> 100 nsec intervals
        TimeOut.QuadPart = -TimeOut.QuadPart;
      }
      
      pDeviceObject = IoGetRelatedDeviceObject(m_pTdiConnectionObject);
      
      pRequestConnectionInfo = (PTDI_CONNECTION_INFORMATION) new char[sizeof(TDI_CONNECTION_INFORMATION) + sizeof(TA_IP_ADDRESS)];
      if (pRequestConnectionInfo != NULL)
      {
        memset(pRequestConnectionInfo, 0, sizeof(TDI_CONNECTION_INFORMATION) + sizeof(TA_IP_ADDRESS));

        pReturnConnectionInfo = NULL;

        pRequestConnectionInfo->RemoteAddressLength = sizeof(TA_IP_ADDRESS);
        pRequestConnectionInfo->RemoteAddress = (PUCHAR)pRequestConnectionInfo + sizeof(TDI_CONNECTION_INFORMATION);

        pRequestAddress = (PTA_IP_ADDRESS)(pRequestConnectionInfo->RemoteAddress);
        pRequestAddress->TAAddressCount = 1;
        pRequestAddress->Address[0].AddressLength = sizeof(TDI_ADDRESS_IP);
        pRequestAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        
        pIp = (PTDI_ADDRESS_IP)(pRequestAddress->Address[0].Address);
        pIp->sin_port = W_LITTLE_TO_BIG_ENDIAN(m_nRemotePort);
        pIp->in_addr = D_LITTLE_TO_BIG_ENDIAN(m_nRemoteAddress);;

        pIrp = TdiBuildInternalDeviceControlIrp(
                      TDI_CONNECT, 
                      pDeviceObject, 
                      m_pTdiConnectionObject,
                      NULL, 
                      NULL);
        pIrpError = pIrp;
        if (pIrp != NULL)
        {
          TdiBuildConnect(
                 pIrp, 
                 pDeviceObject, 
                 m_pTdiConnectionObject,
                 NULL, 
                 NULL,
                 pTimeOut, 
                 pRequestConnectionInfo,
                 pReturnConnectionInfo);
          
          pIrpError = NULL;
          NtStatus = TdiCall(pIrp, pDeviceObject, &IoStatusBlock);
          if (NT_SUCCESS(NtStatus))
          {
            m_bConnected = TRUE;
            bRes = TRUE;
          }
          else
          {
            DbgPrint ("TdiConnect: ERROR (%08x)!!!\n", NtStatus);
          }
        }
        delete[] pRequestConnectionInfo;
        pRequestConnectionInfo = NULL;
      }
    }
  }
  __finally
  {
    if (pIrpError != NULL)
      IoFreeIrp(pIrpError);
    if (pRequestConnectionInfo != NULL)
      delete[] pRequestConnectionInfo;
  }

  return bRes;
}

BOOLEAN KTdiStreamSocket::Disconnect()
{
  //KLocker locker(&m_KSynchroObject);

  BOOLEAN                      bRes = TRUE;
  PIRP                         pIrp = NULL, pIrpError = NULL;
  PDEVICE_OBJECT               pDeviceObject;
  NTSTATUS                     NtStatus;
  PTDI_CONNECTION_INFORMATION  pRequestConnectionInfo = NULL;
  PTDI_CONNECTION_INFORMATION  pReturnConnectionInfo;
  PTA_IP_ADDRESS               pRequestAddress;
  PTDI_ADDRESS_IP              pIp;
  IO_STATUS_BLOCK              IoStatusBlock;

  __try
  {
    if (m_bOpen == TRUE && (m_bConnected == TRUE || m_bListen == TRUE))
    {
      bRes = FALSE;
      
      pDeviceObject = IoGetRelatedDeviceObject(m_pTdiConnectionObject);
      
      pRequestConnectionInfo = (PTDI_CONNECTION_INFORMATION) new char[sizeof(TDI_CONNECTION_INFORMATION) + sizeof(TA_IP_ADDRESS)];
      if (pRequestConnectionInfo != NULL)
      {
        memset(pRequestConnectionInfo, 0, sizeof(TDI_CONNECTION_INFORMATION) + sizeof(TA_IP_ADDRESS));

        pReturnConnectionInfo = NULL;

        pRequestConnectionInfo->RemoteAddressLength = sizeof(TA_IP_ADDRESS);
        pRequestConnectionInfo->RemoteAddress = (PUCHAR)pRequestConnectionInfo + sizeof(TDI_CONNECTION_INFORMATION);

        pRequestAddress = (PTA_IP_ADDRESS)(pRequestConnectionInfo->RemoteAddress);
        pRequestAddress->TAAddressCount = 1;
        pRequestAddress->Address[0].AddressLength = sizeof(TDI_ADDRESS_IP);
        pRequestAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        
        pIp = (PTDI_ADDRESS_IP)(pRequestAddress->Address[0].Address);
        pIp->sin_port = W_LITTLE_TO_BIG_ENDIAN(m_nRemotePort);
        pIp->in_addr = D_LITTLE_TO_BIG_ENDIAN(m_nRemoteAddress);

        pIrp = TdiBuildInternalDeviceControlIrp(
                      TDI_DISCONNECT, 
                      pDeviceObject, 
                      m_pTdiConnectionObject,
                      NULL, 
                      NULL);
        pIrpError = pIrp;
        if (pIrp != NULL)
        {
          TdiBuildDisconnect(
                 pIrp, 
                 pDeviceObject, 
                 m_pTdiConnectionObject,
                 NULL, 
                 NULL,
                 NULL,           // timeout
                 TDI_DISCONNECT_ABORT, //TDI_DISCONNECT_RELEASE,
                 pRequestConnectionInfo,
                 pReturnConnectionInfo);
          
          pIrpError = NULL;

          BOOLEAN bConnected = m_bConnected;
          BOOLEAN bListen = m_bListen;
          
          m_bConnected = FALSE;
          m_bListen = FALSE;

          if (bListen == TRUE)
          {
            KeSetEvent(&m_kAcceptDestroyEvent, 0, FALSE);
            SetEventHandler(TDI_EVENT_CONNECT, (PVOID)NULL, (PVOID)NULL);
          }

          NtStatus = TdiCall(pIrp, pDeviceObject, &IoStatusBlock);
          
          if (NT_SUCCESS(NtStatus))
          {
            DbgPrint ("TdiDisconnect: SUCCESS (%08x)!!!\n", NtStatus);
          }
          else
          {
            DbgPrint ("TdiDisconnect: ERROR (%08x)!!!\n", NtStatus);
          }

          if (bConnected == TRUE)
            m_bConnected = FALSE;
          if (bListen == TRUE)
          {
            m_bListen = FALSE;
            delete[] m_pRequestListenInfo;
            m_pRequestListenInfo = NULL;
          }
          bRes = TRUE;
        }
        delete[] pRequestConnectionInfo;
        pRequestConnectionInfo = NULL;
      }
    }
  }
  __finally
  {
    if (pIrpError != NULL)
      IoFreeIrp(pIrpError);
    if (pRequestConnectionInfo != NULL)
      delete[] pRequestConnectionInfo;
  }

  return bRes;
}

NTSTATUS 
KTdiStreamSocket::ClientEventConnect(
       IN PVOID TdiEventContext,
       IN LONG RemoteAddressLength,
       IN PVOID RemoteAddress,
       IN LONG UserDataLength,
       IN PVOID UserData,
       IN LONG OptionsLength,
       IN PVOID Options,
       OUT CONNECTION_CONTEXT* ConnectionContext,
       OUT PIRP *AcceptIrp
      )
{
  KTdiStreamSocket* _this = (KTdiStreamSocket*)TdiEventContext;
  NTSTATUS NtStatus = STATUS_CONNECTION_REFUSED;

  DbgPrint ("KTdiStreamSocket::ClientEventConnect: START!!!\n");

  if (_this != NULL)
  {
    NtStatus = _this->ConnectEventHandler(RemoteAddressLength, (PTA_IP_ADDRESS)RemoteAddress, UserDataLength, UserData, OptionsLength, Options, ConnectionContext, AcceptIrp);
  }
  else
  {
    *ConnectionContext = (CONNECTION_CONTEXT)NULL;
    *AcceptIrp = NULL;
  }

  return NtStatus;
}

NTSTATUS
KTdiStreamSocket::AcceptCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
  KTdiStreamSocket* _this = (KTdiStreamSocket*) Context;

  if (Irp->UserIosb != NULL)
    *Irp->UserIosb = Irp->IoStatus;

//  if (Irp->MdlAddress != NULL)
//  {
//    MmUnlockPages(Irp->MdlAddress);
//    IoFreeMdl(Irp->MdlAddress);
//  }

  if (Irp->UserEvent != NULL)
    KeSetEvent(Irp->UserEvent, 0, FALSE);

  IoFreeIrp(Irp);
  
  if (_this != NULL)
    //InterlockedDecrement(&(_this->m_dwTreatAcceptIrpsCount));
    --(_this->m_TreatAcceptIrpsCount);

  DbgPrint ("AcceptCompletion: %08x!!!\n", Irp->IoStatus.Status);

  return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS 
KTdiStreamSocket::ConnectEventHandler(
       IN LONG RemoteAddressLength,
       IN PTA_IP_ADDRESS RemoteAddress,
       IN LONG UserDataLength,
       IN PVOID UserData,
       IN LONG OptionsLength,
       IN PVOID Options,
       OUT CONNECTION_CONTEXT* ConnectionContext,
       OUT PIRP *AcceptIrp
      )
{
  PTDI_CONNECTION_INFORMATION  pReturnConnectionInfo;
  NTSTATUS                     NtStatus = STATUS_CONNECTION_REFUSED;
  USHORT                       nRemotePort;
  ULONG                        nRemoteAddress;
  PDEVICE_OBJECT               pDeviceObject;
  PTA_IP_ADDRESS               pReturnAddress;
  PIRP                         pIrp = NULL;
  PTA_IP_ADDRESS               pRequestAddress;
  PTDI_ADDRESS_IP              pIp;

  *ConnectionContext = (CONNECTION_CONTEXT)NULL;
  *AcceptIrp = NULL;

  if (m_bConnected == TRUE)
  {
    return NtStatus;
  }

  if (
         RemoteAddressLength >= sizeof(TA_IP_ADDRESS)
      && RemoteAddress != NULL
      && RemoteAddress->Address[0].AddressType == TDI_ADDRESS_TYPE_IP
     )
  {
    nRemotePort = W_BIG_TO_LITTLE_ENDIAN(RemoteAddress->Address[0].Address[0].sin_port);
    nRemoteAddress = D_BIG_TO_LITTLE_ENDIAN(RemoteAddress->Address[0].Address[0].in_addr);

    DbgPrint ("ConnectEventHandler: %08x : %04x !!!\n", nRemoteAddress, nRemotePort);
    
    
    NtStatus = STATUS_MORE_PROCESSING_REQUIRED;
    if (m_nRemoteAddress != NULL)
    {
      if (m_nRemoteAddress != nRemoteAddress || m_nRemotePort != nRemotePort)
      {
        NtStatus = STATUS_CONNECTION_REFUSED;
      }
    }

    if (NtStatus == STATUS_MORE_PROCESSING_REQUIRED)
    {
      NtStatus = STATUS_INSUFFICIENT_RESOURCES;
      
      m_pRequestListenInfo = (PTDI_CONNECTION_INFORMATION) new char[2*sizeof(TDI_CONNECTION_INFORMATION) + 2*sizeof(TA_IP_ADDRESS) + sizeof(ULONG)];
      if (m_pRequestListenInfo != NULL)
      {
        memset(m_pRequestListenInfo, 0, sizeof(TDI_CONNECTION_INFORMATION) + sizeof(TA_IP_ADDRESS) + sizeof(ULONG));

        m_pReturnListenInfo = (PTDI_CONNECTION_INFORMATION)((PUCHAR)m_pRequestListenInfo + sizeof(TDI_CONNECTION_INFORMATION) + sizeof(TA_IP_ADDRESS));
        m_pReturnListenInfo->RemoteAddressLength = sizeof(TA_IP_ADDRESS);
        m_pReturnListenInfo->RemoteAddress = (PUCHAR)m_pRequestListenInfo + sizeof(TDI_CONNECTION_INFORMATION);

        pReturnAddress = (PTA_IP_ADDRESS)(m_pReturnListenInfo->RemoteAddress);
        pReturnAddress->TAAddressCount = 1;
        pReturnAddress->Address[0].AddressLength = sizeof(TDI_ADDRESS_IP);
        pReturnAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        
        if (m_nRemoteAddress != 0)
        {
          m_pRequestListenInfo->RemoteAddressLength = sizeof(TA_IP_ADDRESS);
          m_pRequestListenInfo->RemoteAddress = (PUCHAR)m_pRequestListenInfo + sizeof(TDI_CONNECTION_INFORMATION);

          pRequestAddress = (PTA_IP_ADDRESS)(m_pRequestListenInfo->RemoteAddress);
          pRequestAddress->TAAddressCount = 1;
          pRequestAddress->Address[0].AddressLength = sizeof(TDI_ADDRESS_IP);
          pRequestAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        
          pIp = (PTDI_ADDRESS_IP)(pRequestAddress->Address[0].Address);
          pIp->sin_port = W_LITTLE_TO_BIG_ENDIAN(m_nRemotePort);
          pIp->in_addr = D_LITTLE_TO_BIG_ENDIAN(m_nRemoteAddress);;
        }
        else
        {
          m_pRequestListenInfo->RemoteAddressLength = 0;
          m_pRequestListenInfo->RemoteAddress = NULL;
        }

        memset(&m_ListenStatusBlock, 0, sizeof(IO_STATUS_BLOCK));

        pDeviceObject = IoGetRelatedDeviceObject(m_pTdiConnectionObject);

        pIrp = TdiBuildInternalDeviceControlIrp(
                      TDI_ACCEPT, 
                      pDeviceObject, 
                      m_pTdiConnectionObject,
                      &m_kListenEvent, 
                      &m_ListenStatusBlock);
        if (pIrp != NULL)
        {
          TdiBuildAccept(
                 pIrp, 
                 pDeviceObject, 
                 m_pTdiConnectionObject,
                 AcceptCompletion, 
                 (PVOID)this,
                 m_pRequestListenInfo,
                 m_pReturnListenInfo);

          *ConnectionContext = (CONNECTION_CONTEXT)this;
          *AcceptIrp = pIrp;
          
          IoSetNextIrpStackLocation(pIrp);

          //InterlockedIncrement(&m_dwTreatAcceptIrpsCount);
          ++m_TreatAcceptIrpsCount;
          
          DbgPrint ("ConnectEventHandler: OK!!!\n");

          NtStatus = STATUS_MORE_PROCESSING_REQUIRED;
        }
      }
    }
    
  }

  return NtStatus;
}


BOOLEAN KTdiStreamSocket::Listen(IN USHORT wRemotePort, IN ULONG dwRemoteAddress)
{
  BOOLEAN                      bRes = FALSE;

  __try
  {
    if (m_bOpen == TRUE && m_bBind == TRUE && m_bConnected == FALSE && Disconnect() == TRUE)
    {
      m_nRemotePort = wRemotePort;
      m_nRemoteAddress = dwRemoteAddress;

      KeInitializeEvent(&m_kAcceptDestroyEvent, NotificationEvent, FALSE);
      KeInitializeEvent(&m_kListenEvent, NotificationEvent, FALSE);
      if (SetEventHandler(TDI_EVENT_CONNECT, (PVOID)ClientEventConnect, (PVOID)this))
      {
        DbgPrint ("TdiListen: OK!!!\n");
        m_bListen = TRUE;
        bRes = TRUE;
      }
    }
  }
  __finally
  {
  }

  return bRes;
}

BOOLEAN KTdiStreamSocket::Accept(ULONG dwTimeOut)
{
  //KLocker locker(&m_KSynchroObject);

  BOOLEAN                      bRes = FALSE;
  NTSTATUS                     NtStatus;
  PVOID                        pkEvents[2];
  LARGE_INTEGER                TimeOut;
  PLARGE_INTEGER               pTimeOut = NULL;

  //m_KSynchroObject.Lock();

  __try
  {
    if (m_bOpen == TRUE && m_bBind == TRUE && m_bConnected == FALSE && m_bListen == TRUE)
    {
      if (dwTimeOut != 0)
      {
        pTimeOut = &TimeOut;
        TimeOut.QuadPart = dwTimeOut * 10000; // msec -> 100 nsec intervals
        TimeOut.QuadPart = -TimeOut.QuadPart;
      }
      
      pkEvents[0] = &m_kListenEvent;
      pkEvents[1] = &m_kAcceptDestroyEvent;

      NtStatus  = KeWaitForMultipleObjects(2, pkEvents, WaitAny, Suspended, KernelMode, TRUE, pTimeOut, NULL);
      
      if (NtStatus == STATUS_WAIT_0)
      {
        if (NT_SUCCESS(m_ListenStatusBlock.Status))
        {
          SetEventHandler(TDI_EVENT_CONNECT, (PVOID)NULL, (PVOID)NULL);
          m_bConnected = TRUE;
          bRes = TRUE;
        
          DbgPrint ("TdiAccept: OK (%08x : %04x)!!!\n", m_nRemoteAddress, m_nRemotePort);
        }
        else
        {
          DbgPrint ("TdiAccept: ERROR (%08x)!!!\n", NtStatus);
        }
      }
    }
  }
  __finally
  {
  }

  //m_KSynchroObject.UnLock();

  return bRes;
}

/*
BOOLEAN KTdiStreamSocket::Listen(IN USHORT wRemotePort, IN ULONG dwRemoteAddress)
{
  //KLocker locker(&m_KSynchroObject);

  BOOLEAN                      bRes = FALSE;
  PIRP                         pIrp = NULL, pIrpError = NULL;
  PDEVICE_OBJECT               pDeviceObject;
  NTSTATUS                     NtStatus;
  //PTDI_CONNECTION_INFORMATION  pRequestListenInfo = NULL;
  //PTDI_CONNECTION_INFORMATION  pReturnConnectionInfo;
  PTA_IP_ADDRESS               pRequestAddress;
  PTDI_ADDRESS_IP              pIp;
  IO_STATUS_BLOCK              IoStatusBlock;

  __try
  {
    if (m_bOpen == TRUE && m_bBind == TRUE && m_bConnected == FALSE && Disconnect() == TRUE)
    {
      m_nRemotePort = wPort;
      m_nRemoteAddress = dwAddress;

      pDeviceObject = IoGetRelatedDeviceObject(m_pTdiConnectionObject);
      
      m_pRequestListenInfo = (PTDI_CONNECTION_INFORMATION) new char[2*sizeof(TDI_CONNECTION_INFORMATION) + 2*sizeof(TA_IP_ADDRESS) + sizeof(ULONG)];
      if (m_pRequestListenInfo != NULL)
      {
        memset(m_pRequestListenInfo, 0, sizeof(TDI_CONNECTION_INFORMATION) + sizeof(TA_IP_ADDRESS) + sizeof(ULONG));

        m_pReturnListenInfo = (PTDI_CONNECTION_INFORMATION)((PUCHAR)m_pRequestListenInfo + sizeof(TDI_CONNECTION_INFORMATION) + sizeof(TA_IP_ADDRESS));
        m_pReturnListenInfo->RemoteAddressLength = sizeof(TA_IP_ADDRESS);
        m_pReturnListenInfo->RemoteAddress = (PUCHAR)m_pRequestListenInfo + sizeof(TDI_CONNECTION_INFORMATION);

        m_pRequestListenInfo->Options = (PVOID) ((PUCHAR)m_pReturnListenInfo + sizeof(TDI_CONNECTION_INFORMATION) + sizeof(TA_IP_ADDRESS));
        *((ULONG*)(m_pRequestListenInfo->Options)) = TDI_QUERY_ACCEPT;
        m_pRequestListenInfo->OptionsLength = sizeof(ULONG);

        if (m_nRemoteAddress != 0)
        {
          m_pRequestListenInfo->RemoteAddressLength = sizeof(TA_IP_ADDRESS);
          m_pRequestListenInfo->RemoteAddress = (PUCHAR)m_pRequestListenInfo + sizeof(TDI_CONNECTION_INFORMATION);

          pRequestAddress = (PTA_IP_ADDRESS)(m_pRequestListenInfo->RemoteAddress);
          pRequestAddress->TAAddressCount = 1;
          pRequestAddress->Address[0].AddressLength = sizeof(TDI_ADDRESS_IP);
          pRequestAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        
          pIp = (PTDI_ADDRESS_IP)(pRequestAddress->Address[0].Address);
          pIp->sin_port = W_LITTLE_TO_BIG_ENDIAN(m_nRemotePort);
          pIp->in_addr = D_LITTLE_TO_BIG_ENDIAN(m_nRemoteAddress);;
        }
        else
        {
          m_pRequestListenInfo->RemoteAddressLength = 0;
          m_pRequestListenInfo->RemoteAddress = NULL;
        }

        pIrp = TdiBuildInternalDeviceControlIrp(
                      TDI_LISTEN, 
                      pDeviceObject, 
                      m_pTdiConnectionObject,
                      NULL, 
                      NULL);
        pIrpError = pIrp;
        if (pIrp != NULL)
        {
          TdiBuildListen(
                 pIrp, 
                 pDeviceObject, 
                 m_pTdiConnectionObject,
                 NULL, 
                 NULL,
                 TDI_QUERY_ACCEPT, // flags
                 m_pRequestListenInfo,
                 m_pReturnListenInfo);
          
          pIrpError = NULL;
          KeInitializeEvent(&m_kAcceptDestroyEvent, NotificationEvent, FALSE);
          KeInitializeEvent(&m_kListenEvent, NotificationEvent, FALSE);
          pIrp->UserEvent = &m_kListenEvent;
          NtStatus = TdiCall(pIrp, pDeviceObject, &IoStatusBlock, FALSE);
          if (NT_SUCCESS(NtStatus))
          {
            DbgPrint ("TdiListen: OK (%08x)!!!\n", NtStatus);
            m_bListen = TRUE;
            bRes = TRUE;
          }
          else
          {
            DbgPrint ("TdiListen: ERROR (%08x)!!!\n", NtStatus);
            delete[] m_pRequestListenInfo;
            m_pRequestListenInfo = NULL;
          }
        }
      }
    }
  }
  __finally
  {
    if (pIrpError != NULL)
      IoFreeIrp(pIrpError);
    if (m_bListen == FALSE && m_pRequestListenInfo != NULL)
      delete[] m_pRequestListenInfo;
  }

  return bRes;
}

BOOLEAN KTdiStreamSocket::Accept(ULONG dwTimeOut)
{
  //KLocker locker(&m_KSynchroObject);

  BOOLEAN                      bRes = FALSE;
  PIRP                         pIrp = NULL, pIrpError = NULL;
  PDEVICE_OBJECT               pDeviceObject;
  NTSTATUS                     NtStatus;
  PTA_IP_ADDRESS               pReturnAddress;
  PTDI_ADDRESS_IP              pIp;
  IO_STATUS_BLOCK              IoStatusBlock;
  PVOID                        pkEvents[2];
  LARGE_INTEGER                TimeOut;
  PLARGE_INTEGER               pTimeOut = NULL;

  //m_KSynchroObject.Lock();

  __try
  {
    if (m_bOpen == TRUE && m_bBind == TRUE && m_bConnected == FALSE && m_bListen == TRUE)
    {
      if (dwTimeOut != 0)
      {
        pTimeOut = &TimeOut;
        TimeOut.QuadPart = dwTimeOut * 10000; // msec -> 100 nsec intervals
        TimeOut.QuadPart = -TimeOut.QuadPart;
      }
      
      pkEvents[0] = &m_kListenEvent;
      pkEvents[1] = &m_kAcceptDestroyEvent;

      NtStatus  = KeWaitForMultipleObjects(2, pkEvents, WaitAny, Suspended, KernelMode, FALSE, pTimeOut, NULL);
      
      if (NtStatus == 0)
      {
        pDeviceObject = IoGetRelatedDeviceObject(m_pTdiConnectionObject);
      
        pReturnAddress = (PTA_IP_ADDRESS)(m_pReturnListenInfo->RemoteAddress);
        pReturnAddress->TAAddressCount = 1;
        pReturnAddress->Address[0].AddressLength = sizeof(TDI_ADDRESS_IP);
        pReturnAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        
        pIrp = TdiBuildInternalDeviceControlIrp(
                      TDI_ACCEPT, 
                      pDeviceObject, 
                      m_pTdiConnectionObject,
                      NULL, 
                      NULL);
        pIrpError = pIrp;
        if (pIrp != NULL)
        {
          TdiBuildAccept(
                 pIrp, 
                 pDeviceObject, 
                 m_pTdiConnectionObject,
                 NULL, 
                 NULL,
                 m_pRequestListenInfo,
                 m_pReturnListenInfo);
          
          pIrpError = NULL;
          NtStatus = TdiCall(pIrp, pDeviceObject, &IoStatusBlock, TRUE);
          if (NT_SUCCESS(NtStatus))
          {
            m_bConnected = TRUE;
            bRes = TRUE;
          
            pIp = (PTDI_ADDRESS_IP)(pReturnAddress->Address[0].Address);
            m_nRemotePort = W_BIG_TO_LITTLE_ENDIAN(pIp->sin_port);
            m_nRemoteAddress = D_BIG_TO_LITTLE_ENDIAN(pIp->in_addr);
          
            DbgPrint ("TdiAccept: OK (%08x : %04x)!!!\n", m_nRemoteAddress, m_nRemotePort);
          }
          else
          {
            DbgPrint ("TdiAccept: ERROR (%08x)!!!\n", NtStatus);
          }
        }
      }
    }
  }
  __finally
  {
    if (pIrpError != NULL)
      IoFreeIrp(pIrpError);
  }

  //m_KSynchroObject.UnLock();

  return bRes;
}
*/
ULONG KTdiStreamSocket::Send(PVOID pData, ULONG dwSize)
{
  //KLocker locker(&m_KSynchroObject);

  PIRP                         pIrp = NULL, pIrpError = NULL;
  PMDL                         pMdl;
  PDEVICE_OBJECT               pDeviceObject;
  NTSTATUS                     NtStatus;
  IO_STATUS_BLOCK              IoStatusBlock;
  ULONG                        dwBytesSended = 0;

  //m_KSynchroObject.Lock();

  __try
  {
    if (m_bOpen == TRUE && m_bConnected == TRUE && dwSize != 0)
    {
      pDeviceObject = IoGetRelatedDeviceObject(m_pTdiConnectionObject);

      pIrp = TdiBuildInternalDeviceControlIrp ( 
                     TDI_SEND,                                       // sub function
                     pDeviceObject,                                  // pointer to device object
                     m_pTdiConnectionObject,                         // pointer to control object
                     NULL,                                           // pointer to event
                     NULL);                                          // pointer to return buffer

      pIrpError = pIrp;
      if (pIrp == NULL)                                              // validate pointer
      {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
      }
      else
      {
        pMdl = IoAllocateMdl(
                           pData,                                    // buffer pointer - virtual address
                           dwSize,                                   // length
                           FALSE,                                    // not secondary
                           FALSE,                                    // don't charge quota
                           NULL);                                    // don't use irp
        if (pMdl != NULL)                                            // validate mdl pointer
        {
          __try
          {
            MmProbeAndLockPages(pMdl, KernelMode, IoModifyAccess);    // probe & lock
          } 
          __except(EXCEPTION_EXECUTE_HANDLER)
          {
            DbgPrint("EXCEPTION: MmProbeAndLockPages\n");
            IoFreeMdl(pMdl);
            pMdl = NULL;
          }
        }

        if (pMdl != NULL)
        {
          TdiBuildSend(
                 pIrp, 
                 pDeviceObject,
                 m_pTdiConnectionObject,
                 NULL, 
                 NULL,
                 pMdl, 
                 0,
                 dwSize);
          pIrpError = NULL;

          //m_KSynchroObject.UnLock();

          NtStatus = TdiCall(pIrp, pDeviceObject, &IoStatusBlock);

          //m_KSynchroObject.Lock();

          if (NT_SUCCESS(NtStatus))
          {
            dwBytesSended = IoStatusBlock.Information;
          }
          else
          {
            DbgPrint ("TdiSend: ERROR (%08x)!!!\n", NtStatus);
            Disconnect();
          }
        }
      }
    }
  }
  __finally
  {
    if (pIrpError != NULL)
      IoFreeIrp(pIrpError);
  }

  //m_KSynchroObject.UnLock();

  return dwBytesSended;
}

ULONG KTdiStreamSocket::Receive(PVOID pData, ULONG dwSize)
{
  //KLocker locker(&m_KSynchroObject);

  PIRP                         pIrp = NULL, pIrpError = NULL;
  PMDL                         pMdl;
  PDEVICE_OBJECT               pDeviceObject;
  NTSTATUS                     NtStatus;
  IO_STATUS_BLOCK              IoStatusBlock;
  ULONG                        dwBytesRecv = 0;
  PVOID                        pMdlBuffer = NULL;

  //m_KSynchroObject.Lock();

  __try
  {
    if (m_bOpen == TRUE && m_bConnected == TRUE && dwSize != 0)
    {
      pDeviceObject = IoGetRelatedDeviceObject(m_pTdiConnectionObject);

      pMdlBuffer = new char[dwSize];

      pIrp = TdiBuildInternalDeviceControlIrp ( 
                     TDI_RECEIVE,                                    // sub function
                     pDeviceObject,                                  // pointer to device object
                     m_pTdiConnectionObject,                         // pointer to control object
                     NULL,                                           // pointer to event
                     NULL);                                // pointer to return buffer

      pIrpError = pIrp;
      if (pIrp == NULL || pMdlBuffer == NULL)                        // validate pointer
      {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
      }
      else
      {
        pMdl = IoAllocateMdl(
                           pMdlBuffer,                               // buffer pointer - virtual address
                           dwSize,                                   // length
                           FALSE,                                    // not secondary
                           FALSE,                                    // don't charge quota
                           NULL);                                    // don't use irp
        if (pMdl != NULL)                                            // validate mdl pointer
        {
          __try
          {
            MmProbeAndLockPages(pMdl, KernelMode, IoModifyAccess);    // probe & lock
          } 
          __except(EXCEPTION_EXECUTE_HANDLER)
          {
            DbgPrint("EXCEPTION: MmProbeAndLockPages\n");
            IoFreeMdl(pMdl);
            pMdl = NULL;
          }
        }

        if (pMdl != NULL)
        {
          TdiBuildReceive(
                 pIrp, 
                 pDeviceObject,
                 m_pTdiConnectionObject,
                 NULL, 
                 NULL,
                 pMdl, 
                 0,
                 dwSize);
          pIrpError = NULL;

          //m_KSynchroObject.UnLock();

          NtStatus = TdiCall(pIrp, pDeviceObject, &IoStatusBlock);

          //m_KSynchroObject.Lock();

          if (NT_SUCCESS(NtStatus))
          {
            dwBytesRecv = IoStatusBlock.Information;
            memcpy(pData, pMdlBuffer, dwBytesRecv);
          }
          else
          {
            DbgPrint ("TdiRecv: ERROR (%08x)!!!\n", NtStatus);
            Disconnect();
          }
        }
      }
    }
  }
  __finally
  {
    if (pIrpError != NULL)
      IoFreeIrp(pIrpError);
    if (pMdlBuffer != NULL)
      delete[] pMdlBuffer;
  }

  //m_KSynchroObject.UnLock();

  return dwBytesRecv;
}


//
// protected functions
//

BOOLEAN KTdiStreamSocket::TdiOpenSocket(USHORT nLocalPort)
{
  BOOLEAN bRes = FALSE;
  if (TdiOpenTransport(nLocalPort) == TRUE)
  {
    //NTSTATUS NtStatus = TdiQueryAddress(m_pTdiTransportObject, &m_dwLocalAddress);
    //if (NT_SUCCESS(NtStatus))
    //{
    //  DbgPrint ( "Local IP Address = %X\n", m_dwLocalAddress );
      if (TdiOpenConnection() == FALSE)
      {
        TdiCloseTransport();
        m_hTdiConnection = NULL;
        m_pTdiConnectionObject = NULL;
        DbgPrint ( "ERROR: unable open connection\n" );
      }
      else
      {
        bRes = TRUE;
      }
    //}
    //else
    //{
    //  TdiCloseTransport();
    //  DbgPrint ( "ERROR: unable to determine local IP address\n" );
    //}
  }
  else
  {
    m_hTdiTransport = NULL;
    m_pTdiTransportObject = NULL;
  }
  
  return bRes;
}

