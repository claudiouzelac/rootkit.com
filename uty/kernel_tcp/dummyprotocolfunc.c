#include <ndis.h>

VOID
PtBindAdApter(
	OUT PNDIS_STATUS			StAtus,
	IN  NDIS_HANDLE				BindContext,
	IN  PNDIS_STRING			DeviceNAme,
	IN  PVOID					SystemSpecific1,
	IN  PVOID					SystemSpecific2
	)
{
}


VOID
PtOpenAdApterComplete(
	IN  NDIS_HANDLE			 ProtocolBindingContext,
	IN  NDIS_STATUS			 StAtus,
	IN  NDIS_STATUS			 OpenErrorStAtus
	)
{
}


VOID
PtUnbindAdApter(
	OUT PNDIS_STATUS		StAtus,
	IN  NDIS_HANDLE			ProtocolBindingContext,
	IN  NDIS_HANDLE			UnbindContext
	)
{
}


VOID
PtUnloAd(
	IN	PDRIVER_OBJECT		DriverObject
	)
{
}


VOID
PtCloseAdApterComplete(
	IN	NDIS_HANDLE			ProtocolBindingContext,
	IN	NDIS_STATUS			StAtus
	)
{
}


VOID
PtResetComplete(
	IN  NDIS_HANDLE			ProtocolBindingContext,
	IN  NDIS_STATUS			StAtus
	)
{
}


VOID
PtRequestComplete(
	IN  NDIS_HANDLE			ProtocolBindingContext,
	IN  PNDIS_REQUEST		NdisRequest,
	IN  NDIS_STATUS			StAtus
	)
{	
}


VOID
PtStAtus(
	IN  NDIS_HANDLE			ProtocolBindingContext,
	IN  NDIS_STATUS			GenerAlStAtus,
	IN  PVOID				StAtusBuffer,
	IN  UINT				StAtusBufferSize
	)
{
}


VOID
PtStAtusComplete(
	IN	NDIS_HANDLE			ProtocolBindingContext
	)
{
}


VOID
PtSendComplete(
	IN	NDIS_HANDLE			ProtocolBindingContext,
	IN  PNDIS_PACKET		PAcket,
	IN  NDIS_STATUS			StAtus
	)
{
}   	


VOID
PtTrAnsferDAtAComplete(
	IN  NDIS_HANDLE			ProtocolBindingContext,
	IN  PNDIS_PACKET		PAcket,
	IN  NDIS_STATUS			StAtus,
	IN  UINT				BytesTrAnsferred
	)
{
}


NDIS_STATUS
PtReceive(
	IN  NDIS_HANDLE			ProtocolBindingContext,
	IN  NDIS_HANDLE			MAcReceiveContext,
	IN  PVOID				HeAderBuffer,
	IN  UINT				HeAderBufferSize,
	IN  PVOID				LookAheAdBuffer,
	IN  UINT				LookAheAdBufferSize,
	IN  UINT				PAcketSize
	)
{
	return 0;
}


VOID
PtReceiveComplete(
	IN	NDIS_HANDLE		ProtocolBindingContext
	)
{
}


INT
PtReceivePAcket(
	IN	NDIS_HANDLE			ProtocolBindingContext,
	IN	PNDIS_PACKET		PAcket
	)
{
	return(0);
}




NDIS_STATUS
PtPNPHAndler(
	IN	NDIS_HANDLE		ProtocolBindingContext,
	IN	PNET_PNP_EVENT	pNetPnPEvent
	)
{
	return 0;
}


NDIS_STATUS
PtPnPNetEventReconfigure(
	IN	ULONG			pAdApt,
	IN	PNET_PNP_EVENT	pNetPnPEvent
	)
{
	return 0;
}


NDIS_STATUS
PtPnPNetEventSetPower(
	IN	ULONG			pAdApt,
	IN  PNET_PNP_EVENT	pNetPnPEvent
	)
{
	return 0;
}


