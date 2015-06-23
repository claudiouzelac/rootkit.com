#ifndef __K_TDI_INTERFACE_H
 #define __K_TDI_STREAM_SOCKET_H

extern "C"
{
 #include "ntddk.h"
 #include "tdi.h"                       // \ddk\src\network\inc
 #include "tdikrnl.h"                   // \ddk\src\network\inc
 #include "tdiinfo.h"                   // \ddk\src\network\inc
 #include "smpletcp.h"
}

#include "../Include/KNew.h"
#include "../KMutexSynchroObject/KMutexSynchroObject.h"
#include "../KInterlockedCounter/KInterlockedCounter.h"

class KTdiInterface;

//****************************************************************************//

#define MAX_FAST_ENTITY_BUFFER ( sizeof(TDIEntityID) * 10 )
#define MAX_FAST_ADDRESS_BUFFER ( sizeof(IPAddrEntry) * 4 )
#define TL_INSTANCE                 0

#define D_BIG_TO_LITTLE_ENDIAN(dAddress) \
				 ((((dAddress) & 0xFF000000L) >> 24) | \
				  (((dAddress) & 0x00FF0000L) >> 8) | \
				  (((dAddress) & 0x0000FF00L) << 8) | \
				  (((dAddress) & 0x000000FFL) << 24))
#define D_LITTLE_TO_BIG_ENDIAN(dAddress) \
				 ((((dAddress) & 0xFF000000L) >> 24) | \
				  (((dAddress) & 0x00FF0000L) >> 8) | \
				  (((dAddress) & 0x0000FF00L) << 8) | \
				  (((dAddress) & 0x000000FFL) << 24))
#define W_BIG_TO_LITTLE_ENDIAN(wPort) \
				 ((((wPort) & 0xFF00) >> 8) | \
				  (((wPort) & 0x00FF) << 8))
#define W_LITTLE_TO_BIG_ENDIAN(wPort) \
				 ((((wPort) & 0xFF00) >> 8) | \
				  (((wPort) & 0x00FF) << 8))


class KTdiInterface
{
   typedef struct tag_TDI_CALL_INFO
   {
     KTdiInterface*         m_pThis;
     KEVENT                   m_kEvent;
     NTSTATUS                 m_NtStatus;
     PIRP                     m_pIrp;
     PDEVICE_OBJECT           m_pDeviceObject;
     PIO_STATUS_BLOCK         m_pIoStatusBlock;
   } TDI_CALL_INFO, *PTDI_CALL_INFO;

  public:
   explicit
   KTdiInterface();
   virtual ~KTdiInterface();

   virtual BOOLEAN   Open(IN PWSTR pwszProtocol);
   virtual BOOLEAN   Close();

  protected:
   BOOLEAN  TdiOpenTransport(IN USHORT wPort);
   BOOLEAN  TdiOpenConnection();
   BOOLEAN  TdiCloseTransport();
   BOOLEAN  TdiCloseConnection();

   NTSTATUS TdiOpen(IN PWSTR pProtocol, IN ULONG dEaLength, IN PFILE_FULL_EA_INFORMATION pEaInfo, OUT PHANDLE phHandle, OUT PFILE_OBJECT *ppObject);
   NTSTATUS TdiClose(IN HANDLE hHandle, IN PFILE_OBJECT pObject);
   NTSTATUS TdiCallEx(IN PIRP pIrp, IN PDEVICE_OBJECT pDeviceObject, IN OUT PIO_STATUS_BLOCK pIoStatusBlock);
   NTSTATUS TdiCall(IN PIRP pIrp, IN PDEVICE_OBJECT pDeviceObject, IN OUT PIO_STATUS_BLOCK pIoStatusBlock, IN BOOLEAN bWait = TRUE, IN PKEVENT pkEvent = NULL);
   NTSTATUS TdiQueryDeviceControl(IN PFILE_OBJECT pObject, IN ULONG dIoControlCode, IN PVOID InputBuffer, IN ULONG InputBufferSize, IN OUT PVOID OutputBuffer, IN ULONG OutputBufferSize, OUT PULONG pdReturn);
   NTSTATUS TdiQueryInformationEx(IN PFILE_OBJECT pObject, IN ULONG dEntity, IN ULONG dInstance, IN ULONG dClass, IN ULONG dType, IN ULONG dId, IN PVOID pOutputBuffer, IN PULONG pdOutputLength);
   NTSTATUS TdiQueryAddress(IN PFILE_OBJECT pObject, IN PULONG pdAddress);
   BOOLEAN  SetEventHandler(IN int nEventType, IN PVOID pEventHandler, IN PVOID HandlerContext);

  private:
   KTdiInterface(const KTdiInterface&);
   KTdiInterface& operator=(const KTdiInterface& right);

   static void TdiCallThread(IN TDI_CALL_INFO* pTdiCallInfo);
   static NTSTATUS TdiCallCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);


  protected:
   HANDLE             m_hTdiTransport;                    // handle to tdi transport
   PFILE_OBJECT       m_pTdiTransportObject;              // pointer to tdi transport object
   HANDLE             m_hTdiConnection;                   // handle to tdi Connection
   PFILE_OBJECT       m_pTdiConnectionObject;             // pointer to tdi Connection object
   
   USHORT             m_nLocalPort;
   ULONG              m_dwLocalAddress;

   PWSTR              m_pwszProtocol;

   KMutexSynchroObject m_KSynchroObject;
   BOOLEAN            m_bOpen;

  private:
   //LONG               m_dwTreatIrpsCount;
   KInterlockedCounter  m_TreatIrpsCount;
};

#endif //__K_TDI_INTERFACE_H
