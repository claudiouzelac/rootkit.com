#ifndef __HE4_TDI_STREAM_SOCKET_H
 #define __HE4_TDI_STREAM_SOCKET_H

extern "C"
{
 #include "ntddk.h"
}

#include "../Include/KNew.h"
#include "../KMutexSynchroObject/KMutexSynchroObject.h"
#include "../KInterlockedCounter/KInterlockedCounter.h"
#include "../KTdiInterface/KTdiInterface.h"

class KTdiStreamSocket;

//****************************************************************************//

class KTdiStreamSocket : public KTdiInterface
{
  public:
   explicit
   KTdiStreamSocket();
   virtual ~KTdiStreamSocket();

   virtual BOOLEAN   Open(USHORT nLocalPort);
   virtual BOOLEAN   Close();
           BOOLEAN   Bind();
           BOOLEAN   Unbind();
           BOOLEAN   Connect(IN USHORT wPort, IN ULONG dwAddress, ULONG dwTimeOut = 0);
           BOOLEAN   Disconnect();
           BOOLEAN   Listen(IN USHORT wRemotePort = 0, IN ULONG dwRemoteAddress = 0);
           BOOLEAN   Accept(ULONG dwTimeout = 0);
           ULONG     Send(PVOID pData, ULONG dwSize);
           ULONG     Receive(PVOID pData, ULONG dwSize);
           BOOLEAN   IsConnected() {return m_bConnected;};

  protected:
           BOOLEAN   TdiOpenSocket(USHORT nLocalPort);
   virtual NTSTATUS  ConnectEventHandler(IN LONG RemoteAddressLength, IN PTA_IP_ADDRESS RemoteAddress, IN LONG UserDataLength, IN PVOID UserData, IN LONG OptionsLength, IN PVOID Options, OUT CONNECTION_CONTEXT* ConnectionContext, OUT PIRP *AcceptIrp);

  private:
   KTdiStreamSocket(const KTdiStreamSocket&);
   KTdiStreamSocket& operator=(const KTdiStreamSocket& right);

   static NTSTATUS   AcceptCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
   static NTSTATUS   ClientEventConnect(IN PVOID TdiEventContext, IN LONG RemoteAddressLength, IN PVOID RemoteAddress, IN LONG UserDataLength, IN PVOID UserData, IN LONG OptionsLength, IN PVOID Options, OUT CONNECTION_CONTEXT* ConnectionContext, OUT PIRP *AcceptIrp);


  protected:
   BOOLEAN            m_bListen;
   BOOLEAN            m_bConnected;
   USHORT             m_nRemotePort;
   ULONG              m_nRemoteAddress;

   KEVENT             m_kListenEvent;
   KEVENT             m_kAcceptDestroyEvent;
   PTDI_CONNECTION_INFORMATION  m_pRequestListenInfo;
   PTDI_CONNECTION_INFORMATION  m_pReturnListenInfo;
   IO_STATUS_BLOCK    m_ListenStatusBlock;

   BOOLEAN            m_bBind;

   //KEVENT             m_kReceiveInProgressEvent;
   //KEVENT             m_kSendInProgressEvent;

   KMutexSynchroObject m_KSynchroObject;
   BOOLEAN            m_bOpen;

  private:
   //LONG               m_dwTreatAcceptIrpsCount;
   KInterlockedCounter  m_TreatAcceptIrpsCount;
};

#endif //__HE4_TDI_STREAM_SOCKET_H
