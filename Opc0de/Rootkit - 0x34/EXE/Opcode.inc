LIST_ENTRY STRUCT
	Flink	DWORD	?	; PTR LIST_ENTRY
	Blink	DWORD	?	; PTR LIST_ENTRY
LIST_ENTRY ENDS


DBGKD_DEBUG_DATA_HEADER32 STRUCT
	List		LIST_ENTRY	<>
	OwnerTag	DWORD		?
	dSize		DWORD		?
DBGKD_DEBUG_DATA_HEADER32 ENDS

KDDEBUGGER_DATA32 STRUCT
					DWORD	12 dup(?)	;to complete
	Header		DBGKD_DEBUG_DATA_HEADER32  <>
	KernBase			DWORD	?
					DWORD	?
	BreakpointWithStatus 		DWORD	?
					DWORD	?
					DWORD	4 dup(?)
	KiCallUserMode 			DWORD	?
					DWORD	?
	KeUserCallbackDispatcher 	DWORD	?
					DWORD	?
	PsLoadedModuleList 		DWORD	?
					DWORD	?
	PsActiveProcessHead 		DWORD	?
					DWORD	?
	PspCidTable 			DWORD	?
					DWORD	?
	ExpSystemResourcesList 		DWORD	?
					DWORD	?
	ExpPagedPoolDescriptor 		DWORD	?
					DWORD	?
	ExpNumberOfPagedPools 		DWORD	?
					DWORD	?
	KeTimeIncrement 		DWORD	?
					DWORD	?
	KeBugCheckCallbackListHead 	DWORD	?
					DWORD	?
	KiBugcheckData 			DWORD	?
					DWORD	?
	IopErrorLogListHead 		DWORD	?
					DWORD	?
	ObpRootDirectoryObject 		DWORD	?
					DWORD	?
	ObpTypeObjectType 		DWORD	?
					DWORD	?
	MmSystemCacheStart 		DWORD	?
					DWORD	?
	MmSystemCacheEnd 		DWORD	?
					DWORD	?
	MmSystemCacheWs 		DWORD	?
					DWORD	?
	MmPfnDatabase 			DWORD	?
	MmSystemPtesStart 		DWORD	?
					DWORD	?
	MmSystemPtesEnd 		DWORD	?
					DWORD	?
	MmSubsectionBase 		DWORD	?
					DWORD	?
	MmNumberOfPagingFiles 		DWORD	?
					DWORD	?
	MmLowestPhysicalPage 		DWORD	?
					DWORD	?
	MmHighestPhysicalPage 		DWORD	?
					DWORD	?
	MmNumberOfPhysicalPages 	DWORD	?
					DWORD	?
	MmMaximumNonPagedPoolInBytes 	DWORD	?
					DWORD	?
	MmNonPagedSystemStart 		DWORD	?
					DWORD	?
	MmNonPagedPoolStart 		DWORD	?
					DWORD	?
	MmNonPagedPoolEnd 		DWORD	?
					DWORD	?
	MmPagedPoolStart 		DWORD	?
					DWORD	?
	MmPagedPoolEnd 			DWORD	?
					DWORD	?
	MmPagedPoolInformation 		DWORD	?
					DWORD	?
	MmPageSize 			DWORD	?
					DWORD	?
	MmSizeOfPagedPoolInBytes 	DWORD	?
					DWORD	?
	MmTotalCommitLimit 		DWORD	?
					DWORD	?
	MmTotalCommittedPages 		DWORD	?
					DWORD	?
	MmSharedCommit 			DWORD	?
					DWORD	?
	MmDriverCommit 			DWORD	?
					DWORD	?
	MmProcessCommit 		DWORD	?
					DWORD	?
	MmPagedPoolCommit 		DWORD	?
					DWORD	?
	MmExtendedCommit 		DWORD	?
					DWORD	?
	MmZeroedPageListHead 		DWORD	?
					DWORD	?
	MmFreePageListHead 		DWORD	?
					DWORD	?
	MmStandbyPageListHead 		DWORD	?
					DWORD	?
	MmModifiedPageListHead 		DWORD	?
					DWORD	?
	MmModifiedNoWritePageListHead 	DWORD	?
					DWORD	?
	MmAvailablePages 		DWORD	?
					DWORD	?
	MmResidentAvailablePages 	DWORD	?
					DWORD	?
	PoolTrackTable 			DWORD	?
					DWORD	?
	NonPagedPoolDescriptor 		DWORD	?
					DWORD	?
	MmHighestUserAddress 		DWORD	?
					DWORD	?
	MmSystemRangeStart 		DWORD	?
					DWORD	?
	MmUserProbeAddress 		DWORD	?
					DWORD	?
	KdPrintCircularBuffer 		DWORD	?
					DWORD	?
	KdPrintCircularBufferEnd 	DWORD	?
					DWORD	?
	KdPrintWritePointer 		DWORD	?
					DWORD	?
	KdPrintRolloverCount 		DWORD	?
					DWORD	?
	MmLoadedUserImageList 		DWORD	?
KDDEBUGGER_DATA32 ENDS	

