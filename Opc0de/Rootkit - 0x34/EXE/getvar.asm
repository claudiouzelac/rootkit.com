;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Get non-exported kernel variables
; by Edgar Barbosa, a.k.a "Opc0de"
; 
; To build, install Masm32 v8.0
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
.386
.model flat,stdcall
option casemap:none

include \masm32\include\windows.inc
include \masm32\include\kernel32.inc
include \masm32\include\comdlg32.inc
include \masm32\include\user32.inc
include \masm32\include\gdi32.inc
include \masm32\include\comctl32.inc
include \masm32\include\advapi32.inc
include \masm32\include\winioctl.inc

include common.inc
include Opcode.inc

includelib \masm32\lib\user32.lib
includelib \masm32\lib\kernel32.lib
includelib \masm32\lib\comdlg32.lib
includelib \masm32\lib\gdi32.lib
includelib \masm32\lib\comctl32.lib
includelib \masm32\lib\advapi32.lib

include \masm32\Macros\Strings.mac		;Four-F macros

IDC_BTN_DEBUG   equ 1001
IDC_LSV1 	equ 1002

DlgProc 	PROTO:DWORD,:DWORD,:DWORD,:DWORD
InsertColumnLV 	PROTO:HWND,:HWND,:DWORD,:DWORD,:DWORD
InsertItemLV 	PROTO:HWND,:HWND,:DWORD,:DWORD,:DWORD,:DWORD
BeginProcessing	PROTO:HWND
getTheVersion	PROTO
IOCTLcenter	PROTO:HWND
;==========================================================================
;	DATA
;==========================================================================
.data
DialogName 	db "OPCODE", 0
stFormat	db "0x%08X", 0
lvColuna1 	db "Variable", 0
lvColuna2 	db "Value", 0
stOut		db 15 dup ("0")
it0sub0		db "KernelBase", 0
it0sub1		db "KiCallUserMode", 0
it0sub2		db "KeUserCallbackDispatcher", 0
it0sub3		db "PsLoadedModuleList", 0
it0sub4		db "PsActiveProcessHead", 0
it0sub5		db "PspCidTable", 0
it0sub6		db "ExpNumberOfPagedPools", 0
it0sub7		db "ObpRootDirectoryObject", 0
it0sub8		db "ObpTypeObjectType", 0
it0sub9		db "MmSystemCacheStart", 0
it0suba		db "MmSystemCacheEnd", 0
it0subb		db "MmSystemCacheWs", 0
it0subc		db "MmPfnDatabase", 0
it0subd		db "MmSystemPtesStart", 0 
it0sube		db "MmSystemPtesEnd", 0
it0subf		db "MmSubsectionBase", 0
it0subg		db "MmNumberOfPagingFiles", 0
it0subh		db "MmLowestPhysicalPage", 0
it0subi		db "MmHighestPhysicalPage", 0
it0subj		db "MmNumberOfPhysicalPages", 0
it0subk		db "MmHighestUserAddress", 0
it0subl		db "MmSystemRangeStart", 0
it0subm		db "MmUserProbeAddress", 0
it0subn		db "MmLoadedUserImageList", 0

.data?
lvHWND dd ?

;==========================================================================
;	CODE
;==========================================================================
.code
start:
    invoke InitCommonControls
    invoke GetModuleHandle, NULL
    invoke DialogBoxParam, eax, addr DialogName, NULL, ADDR DlgProc, 0
    invoke ExitProcess, 0

;==========================================================================
;	DIALOG PROCEDURE
;==========================================================================
DlgProc proc hDlg:DWORD, uMsg:DWORD, wParam:DWORD, lParam:DWORD	

	.if uMsg==WM_INITDIALOG
		invoke getTheVersion
		.if eax == -1 
			invoke MessageBox, hDlg, $CTA0("OS version not supported - To run only in Windows XP or superior"), \
					 $CTA0("GetVar"), MB_OK + MB_ICONERROR
			invoke EndDialog,hDlg,0
		.endif
		invoke InsertColumnLV, hDlg, IDC_LSV1, addr lvColuna2, sizeof lvColuna2, 77
		invoke InsertColumnLV, hDlg, IDC_LSV1, addr lvColuna1, sizeof lvColuna1, 180
	.elseif uMsg==WM_COMMAND
		mov eax, wParam
		.if ax == IDC_BTN_DEBUG
			invoke BeginProcessing, hDlg
;			invoke InsertItemLV, hDlg, IDC_LSV1, 0, 0, addr it0sub0, sizeof it0sub0
;			invoke InsertItemLV, hDlg, IDC_LSV1, 0, 1, addr it0sub1, sizeof it0sub1
		.endif
	.elseif uMsg==WM_CLOSE
		invoke EndDialog, hDlg, 0
	.else
		mov eax,FALSE
		ret
	.endif
	mov eax,TRUE
	ret
DlgProc endp	
;==========================================================================
InsertColumnLV proc dlgHnd:HWND, lvid:HWND, lpColumnTxt:DWORD, szColumnTxt:DWORD, szPixelColumn:DWORD
	LOCAL lvcl:LV_COLUMN
	push eax
	mov lvcl.imask, LVCF_TEXT + LVCF_WIDTH + LVCF_FMT
	mov lvcl.fmt, LVCFMT_LEFT
	mov eax, lpColumnTxt
	mov lvcl.pszText, eax
	mov eax, szPixelColumn
	mov lvcl.lx, eax
	mov eax, szColumnTxt
        mov lvcl.cchTextMax, eax
	mov lvcl.iSubItem, 0
	invoke SendDlgItemMessage, dlgHnd, lvid, LVM_INSERTCOLUMN, 0, addr lvcl
	pop eax
	ret
InsertColumnLV endp
;==========================================================================
InsertItemLV proc dlgHnd:HWND, lvid:HWND, lvitem:DWORD, subitem:DWORD, lpItemTxt:DWORD, szItemTxt:DWORD
	LOCAL lvit:LV_ITEM
	push eax
	mov eax, lvitem
	mov lvit.iItem, eax
	mov lvit.imask, LVIF_TEXT
	mov eax, lpItemTxt
	mov lvit.pszText, eax
	mov eax, szItemTxt
	mov lvit.cchTextMax, eax
	mov eax, subitem
	mov lvit.iSubItem, eax
	.if eax == 0
		mov ebx, LVM_INSERTITEM
	.else
		mov ebx, LVM_SETITEM
	.endif
	invoke SendDlgItemMessage, dlgHnd, lvid, ebx, 0, addr lvit
	ret
InsertItemLV endp
;=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
; Central processing procedure
;=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
BeginProcessing proc hDlg:HWND
	LOCAL stPathBuffer[MAX_PATH]:CHAR
	LOCAL lpFilePart:DWORD
	LOCAL hService:HANDLE
	LOCAL hSCManager:HANDLE
	LOCAL _ss:SERVICE_STATUS
	invoke OpenSCManager, NULL, NULL, SC_MANAGER_ALL_ACCESS
	.if eax != NULL
		mov hSCManager, eax
		invoke GetFullPathName, $CTA0("getvar.sys"), sizeof stPathBuffer, \
					addr stPathBuffer, addr lpFilePart
		invoke CreateService, hSCManager, $CTA0("getvar"), $CTA0("Getvar module"), \
			SERVICE_START + SERVICE_STOP + DELETE, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, \
			SERVICE_ERROR_IGNORE, addr stPathBuffer, NULL, NULL, NULL, NULL, NULL
		.if eax != NULL
			mov hService, eax
			invoke StartService, hService, 0, NULL
			.if eax != NULL
				invoke IOCTLcenter, hDlg
				invoke ControlService, hService, SERVICE_CONTROL_STOP, addr _ss
			.else
				invoke MessageBox, NULL, $CTA0("StartService failed"), \
					$CTA0("Process Hunter"), MB_OK + MB_ICONSTOP
			.endif
			invoke DeleteService, hService
			invoke CloseServiceHandle, hService
		.else
			invoke MessageBox, NULL, $CTA0("Can't register driver. CreateServiceFailed"), \
					$CTA0("GetVar"), MB_OK + MB_ICONSTOP
		.endif
		invoke CloseServiceHandle, hSCManager
	.else
		invoke MessageBox, NULL, $CTA0("Can't connect to Service Control Manager."), \
					$CTA0("GetVar"),MB_OK + MB_ICONSTOP
	.endif

	ret
BeginProcessing endp
;=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
; Get specific OS data structures
;=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
getTheVersion proc
	LOCAL osver:OSVERSIONINFO

	lea eax, osver
	mov (OSVERSIONINFO PTR [eax]).dwOSVersionInfoSize, sizeof(OSVERSIONINFO)
	invoke GetVersionEx, addr osver
	lea eax, osver
	mov eax, (OSVERSIONINFO PTR [eax]).dwMajorVersion
	.if eax < 5
		mov eax, -1
	.else
		lea eax, osver
		mov eax, (OSVERSIONINFO PTR [eax]).dwMinorVersion
		.if eax < 1
			mov eax, -1
		.endif
	.endif
	ret
getTheVersion endp
;=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
; Control IOCTL messages
;=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
IOCTLcenter proc hDlg:HWND
	LOCAL hDevice:HANDLE
	LOCAL iocOSinbuffer:DWORD
	LOCAL iocOSoutbuffer[164]:DWORD
	LOCAL bytesRet:DWORD

	invoke CreateFile, $CTA0("\\\\.\\getvar"), GENERIC_READ + GENERIC_WRITE, \
			   0, NULL, OPEN_EXISTING, 0, NULL
	.if eax != INVALID_HANDLE_VALUE
		mov hDevice, eax
		invoke DeviceIoControl, hDevice, IOCTL_GET_VAR, \
		       addr iocOSinbuffer, sizeof iocOSinbuffer, \
		       addr iocOSoutbuffer, sizeof iocOSoutbuffer, \
		       addr bytesRet, NULL
			.if eax == 0
				invoke OutputDebugString, $CTA0("DeviceIoControl failed")
			.endif
		invoke CloseHandle, hDevice
	.endif
	lea eax, iocOSoutbuffer

	push eax
	mov ebx, (KDDEBUGGER_DATA32 ptr [eax]).KernBase
	invoke wsprintf, addr stOut, addr stFormat, ebx
	invoke InsertItemLV, hDlg, IDC_LSV1, 0, 0, addr it0sub0, sizeof it0sub0
	invoke InsertItemLV, hDlg, IDC_LSV1, 0, 1, addr stOut, sizeof stOut
	pop eax

	push eax
	mov ebx, (KDDEBUGGER_DATA32 ptr [eax]).KiCallUserMode
	invoke wsprintf, addr stOut, addr stFormat, ebx
	invoke InsertItemLV, hDlg, IDC_LSV1, 1, 0, addr it0sub1, sizeof it0sub1
	invoke InsertItemLV, hDlg, IDC_LSV1, 1, 1, addr stOut, sizeof stOut
	pop eax

	push eax
	mov ebx, (KDDEBUGGER_DATA32 ptr [eax]).PsActiveProcessHead
	invoke wsprintf, addr stOut, addr stFormat, ebx
	invoke InsertItemLV, hDlg, IDC_LSV1, 2, 0, addr it0sub4, sizeof it0sub4
	invoke InsertItemLV, hDlg, IDC_LSV1, 2, 1, addr stOut, sizeof stOut
	pop eax

	push eax
	mov ebx, (KDDEBUGGER_DATA32 ptr [eax]).PsLoadedModuleList
	invoke wsprintf, addr stOut, addr stFormat, ebx
	invoke InsertItemLV, hDlg, IDC_LSV1, 3, 0, addr it0sub3, sizeof it0sub3
	invoke InsertItemLV, hDlg, IDC_LSV1, 3, 1, addr stOut, sizeof stOut
	pop eax

	push eax
	mov ebx, (KDDEBUGGER_DATA32 ptr [eax]).PspCidTable
	invoke wsprintf, addr stOut, addr stFormat, ebx
	invoke InsertItemLV, hDlg, IDC_LSV1, 4, 0, addr it0sub5, sizeof it0sub5
	invoke InsertItemLV, hDlg, IDC_LSV1, 4, 1, addr stOut, sizeof stOut
	pop eax

	push eax
	mov ebx, (KDDEBUGGER_DATA32 ptr [eax]).ExpNumberOfPagedPools
	invoke wsprintf, addr stOut, addr stFormat, ebx
	invoke InsertItemLV, hDlg, IDC_LSV1, 5, 0, addr it0sub6, sizeof it0sub6
	invoke InsertItemLV, hDlg, IDC_LSV1, 5, 1, addr stOut, sizeof stOut
	pop eax

	push eax
	mov ebx, (KDDEBUGGER_DATA32 ptr [eax]).ObpRootDirectoryObject
	invoke wsprintf, addr stOut, addr stFormat, ebx
	invoke InsertItemLV, hDlg, IDC_LSV1, 6, 0, addr it0sub7, sizeof it0sub7
	invoke InsertItemLV, hDlg, IDC_LSV1, 6, 1, addr stOut, sizeof stOut
	pop eax

	push eax
	mov ebx, (KDDEBUGGER_DATA32 ptr [eax]).ObpTypeObjectType
	invoke wsprintf, addr stOut, addr stFormat, ebx
	invoke InsertItemLV, hDlg, IDC_LSV1, 7, 0, addr it0sub8, sizeof it0sub8
	invoke InsertItemLV, hDlg, IDC_LSV1, 7, 1, addr stOut, sizeof stOut
	pop eax

	push eax
	mov ebx, (KDDEBUGGER_DATA32 ptr [eax]).MmSystemCacheStart
	invoke wsprintf, addr stOut, addr stFormat, ebx
	invoke InsertItemLV, hDlg, IDC_LSV1, 8, 0, addr it0sub9, sizeof it0sub9
	invoke InsertItemLV, hDlg, IDC_LSV1, 8, 1, addr stOut, sizeof stOut
	pop eax

	push eax
	mov ebx, (KDDEBUGGER_DATA32 ptr [eax]).MmSystemCacheEnd
	invoke wsprintf, addr stOut, addr stFormat, ebx
	invoke InsertItemLV, hDlg, IDC_LSV1, 9, 0, addr it0suba, sizeof it0suba
	invoke InsertItemLV, hDlg, IDC_LSV1, 9, 1, addr stOut, sizeof stOut
	pop eax

	push eax
	mov ebx, (KDDEBUGGER_DATA32 ptr [eax]).MmSystemCacheWs
	invoke wsprintf, addr stOut, addr stFormat, ebx
	invoke InsertItemLV, hDlg, IDC_LSV1, 10, 0, addr it0subb, sizeof it0subb
	invoke InsertItemLV, hDlg, IDC_LSV1, 10, 1, addr stOut, sizeof stOut
	pop eax

	push eax
	mov ebx, (KDDEBUGGER_DATA32 ptr [eax]).MmPfnDatabase
	invoke wsprintf, addr stOut, addr stFormat, ebx
	invoke InsertItemLV, hDlg, IDC_LSV1, 11, 0, addr it0subc, sizeof it0subc
	invoke InsertItemLV, hDlg, IDC_LSV1, 11, 1, addr stOut, sizeof stOut
	pop eax

	ret
IOCTLcenter endp
;==========================================================================
end start	
;==========================================================================   
