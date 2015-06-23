#include <ndis.h>
VOID
PtOpenAdApterComplete(
	IN  NDIS_HANDLE			 ProtocolBindingContext,
	IN  NDIS_STATUS			 Status,
	IN  NDIS_STATUS			 OpenErrorStatus
	);

VOID
PtCloseAdApterComplete(
	IN	NDIS_HANDLE			ProtocolBindingContext,
	IN	NDIS_STATUS			Status
	);

VOID
PtSendComplete(
	IN	NDIS_HANDLE			ProtocolBindingContext,
	IN  PNDIS_PACKET		Packet,
	IN  NDIS_STATUS			Status
	);

VOID
PtTrAnsferDAtAComplete(
	IN  NDIS_HANDLE			ProtocolBindingContext,
	IN  PNDIS_PACKET		Packet,
	IN  NDIS_STATUS			Status,
	IN  UINT				BytesTransferred
	);

VOID
PtResetComplete(
	IN  NDIS_HANDLE			ProtocolBindingContext,
	IN  NDIS_STATUS			Status
	);

VOID
PtRequestComplete(
	IN  NDIS_HANDLE			ProtocolBindingContext,
	IN  PNDIS_REQUEST		NdisRequest,
	IN  NDIS_STATUS			Status
	);

NDIS_STATUS
PtReceive(
	IN  NDIS_HANDLE			ProtocolBindingContext,
	IN  NDIS_HANDLE			MacReceiveContext,
	IN  PVOID				HeaderBuffer,
	IN  UINT				HeaderBufferSize,
	IN  PVOID				LookAheadBuffer,
	IN  UINT				LookAheadBufferSize,
	IN  UINT				PacketSize
	);

VOID
PtReceiveComplete(
	IN	NDIS_HANDLE		ProtocolBindingContext
	);

VOID
PtStAtus(
	IN  NDIS_HANDLE			ProtocolBindingContext,
	IN  NDIS_STATUS			GeneralStatus,
	IN  PVOID				StatusBuffer,
	IN  UINT				StatusBufferSize
	);

VOID
PtStAtusComplete(
	IN	NDIS_HANDLE			ProtocolBindingContext
	);

VOID
PtBindAdApter(
	OUT PNDIS_STATUS			Status,
	IN  NDIS_HANDLE				BindContext,
	IN  PNDIS_STRING			DeviceName,
	IN  PVOID					SystemSpecific1,
	IN  PVOID					SystemSpecific2
	);

VOID
PtUnbindAdApter(
	OUT PNDIS_STATUS		Status,
	IN  NDIS_HANDLE			ProtocolBindingContext,
	IN  NDIS_HANDLE			UnbindContext
	);

VOID
PtUnloAd(
	IN	PDRIVER_OBJECT		DriverObject
	);

INT
PtReceivePAcket(
	IN	NDIS_HANDLE			ProtocolBindingContext,
	IN	PNDIS_PACKET		Packet
	);

NDIS_STATUS
PtPNPHAndler(
	IN	NDIS_HANDLE		ProtocolBindingContext,
	IN	PNET_PNP_EVENT	pNetPnPEvent
	);












