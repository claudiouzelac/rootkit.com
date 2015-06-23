;====================[ The Smallest TCP Port Redirector ]=======================
;
;
;programmed by Holy_Father <holy_father@phreaker.net>
;Copyright (c) 2000,forever ExEwORx
;birthday: 8.9.2002
;version: 1.0
;
;compiled with MASM 6.14 with ALIGN:4 
;total size: 2512b
;write no output, silently terminates when error
;it is multithreaded and stable on Windows NT 4.0, Windows 2000 and Windows XP
;
;usage:	sredir.exe listen_on_port redir_to_ip redir_to_port
;	redir_to_ip must be IP address in A.B.C.D format 
;		    no DNS implemented
;
;example: sredir.exe 100 212.80.76.18 80
;
;no other comments, cuz code is comment :)
;

.386p
.model flat, stdcall

include kernel32.inc
include winsock2.inc

LocalAlloc		PROTO :DWORD,:DWORD
LocalFree		PROTO :DWORD
ExitThread		PROTO :DWORD
ExitProcess 		PROTO :DWORD
GetCommandLineA 	PROTO
Sleep 			PROTO :DWORD
CloseHandle		PROTO :DWORD
CreateThread		PROTO :DWORD,:DWORD,:DWORD,:DWORD,:DWORD,:DWORD
TerminateThread		PROTO :DWORD,:DWORD
WaitForMultipleObjects  PROTO :DWORD,:DWORD,:DWORD,:DWORD

bind			PROTO :DWORD,:DWORD,:DWORD
listen			PROTO :DWORD,:DWORD
recv			PROTO :DWORD,:DWORD,:DWORD,:DWORD
send			PROTO :DWORD,:DWORD,:DWORD,:DWORD
closesocket		PROTO :DWORD
inet_addr		PROTO :DWORD
WSAIoctl		PROTO :DWORD,:DWORD,:DWORD,:DWORD,:DWORD,:DWORD,:DWORD,
			      :DWORD,:DWORD
WSAStartup 		PROTO :DWORD,:DWORD
WSACleanup 		PROTO
WSACreateEvent		PROTO
WSASocketA		PROTO :DWORD,:DWORD,:DWORD,:DWORD,:DWORD,:DWORD
WSAConnect		PROTO :DWORD,:DWORD,:DWORD,:DWORD,:DWORD,:DWORD,:DWORD
WSAEnumNetworkEvents	PROTO :DWORD,:DWORD,:DWORD
WSAAccept		PROTO :DWORD,:DWORD,:DWORD,:DWORD,:DWORD
WSAEventSelect		PROTO :DWORD,:DWORD,:DWORD
WSAWaitForMultipleEvents PROTO :DWORD,:DWORD,:DWORD,:DWORD,:DWORD

SOMAXCONN		equ 07FFFFFFFh
IPPROTO_TCP		equ 006h
SOCK_STREAM		equ 001h
AF_INET			equ 002h
FIONREAD		equ 04004667Fh
WAITFOREVENTSTIMEOUT	equ 0FAh
WSA_WAIT_TIMEOUT	equ 00102h
SOCK_ADDR_SIZE		equ 010h
FD_READ			equ 001h
FD_ACCEPT		equ 008h
FD_CLOSE		equ 020h
FD_ALL_EVENTS		equ 003FFh
LMEM_FIXED		equ 000h

.data

.code
start:
	mov	ebp,esp
	sub	esp,001ECh
	lea	eax,[ebp-01ECh]
	push	eax
	push	0202h
	call	WSAStartup
	test	eax, eax
	jnz	@end

	xor	eax,eax
	call	GetCommandLineA
	mov	esi,eax
	xor	eax,eax
	lodsb
	cmp	al,022h
	setz	al
	mov	[ebp-004h],eax
 @Next_char:
	lodsb
	test	eax,eax
	jz	@end
	sub	al,020h
	jz	@Space_found
	dec	eax
	dec	eax
	setnz	al
	and	[ebp-004h],eax
	jmp	@Next_char

 @Space_found:
	cmp	byte ptr [ebp-004h],000h
	jnz	@Next_char

	call	@Find_arg
	mov	edi,esi
	push	020h
	pop	eax
	call	@arg_len
	mov	ecx,eax
	lea	edi,[ebp-0100h]
	push	edi
	rep	movsb
	mov	[edi],cl
	call	@IntToStr
	mov	[ebp-004h],eax
	inc	eax
	jz	@end
	inc	edi
	push	edi
	call	@Find_arg
	xchg	esi,edi
	push	020h
	pop	eax
	call	@arg_len
	mov	ecx,eax
	xchg	esi,edi
	rep	movsb
	mov	[edi],cl
	call	inet_addr
	mov	[ebp-008h],eax
	inc	eax
	jz	@end
	call	@Find_arg
	inc	edi
	push	edi
	xchg	esi,edi
	xor	eax,eax
	call	@arg_len
	mov	ecx,eax
	xchg	esi,edi
	rep	movsb
	mov	[edi],cl
	call	@IntToStr
	mov	[ebp-00Ch],eax
	inc	eax
	jz	@end
	
	mov	eax,[ebp-00Ch]
	shl	eax,010h
	mov	ax,[ebp-004h]
	push	eax
	push	dword ptr [ebp-008h]
	call	@Server
 @end:
	call	WSACleanup
	push	000h
	call	ExitProcess

 @IntToStr:
	push	esi
	xor	eax,eax
	xor	edx,edx
	mov	esi,[esp+008h]
 @IntToStr_next_char:
	lodsb
	test	eax,eax
	jz	@IntToStr_end
	imul	edx,edx,00Ah
	cmp	al,030h
	jb	@IntToStr_error
	cmp	al,039h
	ja	@IntToStr_error
	sub	eax,030h
	add	edx,eax
	jmp	@IntToStr_next_char
 @IntToStr_error:
	xor	edx,edx
	dec	edx
 @IntToStr_end:
	mov	eax,edx
	pop	esi
	ret

 @arg_len:				;@arg -> edi, char -> eax 
	push	edi
	xor	ecx,ecx
	dec	ecx
	repnz	scasb
	not	ecx
	dec	ecx
	mov	eax,ecx
	pop	edi
	ret

 @Find_arg:				;str -> esi -> esi
  lodsb
  cmp al,020h
  jz @Find_arg
  dec esi
  ret

 @Server:
	push	ebp
	mov	ebp,esp
	sub	esp,034h

;	-030		-	NewClient.Host.sin_family:Word
;	-02E		-	NewClient.Host.sin_port:Word
;	-02C		-	NewClient.Host.sin_addr:TInAddr
;	-028..-024	-	NewClient.Host.sin_zero:array[0..7] of Char
;	-020		-	NewClient.Socket:TSocket
;	-01C		-	TID:Cardinal;
;	-018		-	ServerEventHandle:THandle
;	-014		-	ServerHost.sin_family:Word
;	-012		-	ServerHost.sin_port:Word
;	-010		-	ServerHost.sin_addr:TInAddr
;	-00C..-008	-	ServerHost.sin_zero:array[0..7] of Char
;	-004		-	ServerSocket:TSocket
;	+008		-	FinalAddr:TInAddr
;	+00C		-	ListenPort:Word
;	+010		-	FinalPort:Word

	push	esi
	push	edi
	push	ebx

	xor	eax,eax
	mov	[ebp-010h],eax
	push	eax
	push	eax
	push	eax
	push	IPPROTO_TCP
	push	SOCK_STREAM
	push	AF_INET
	call	WSASocketA
	mov	[ebp-004h],eax
	inc	eax
	jz	@Server_end

	mov	eax,[ebp+00Ch]
	xchg	ah,al
	mov	[ebp-012h],ax
	mov	word ptr [ebp-014h],AF_INET

	push	010h
	lea	eax,[ebp-014h]
	push	eax
	push	dword ptr [ebp-004h]
	call	bind
	inc	eax
	jz	@Server_end
	push	SOMAXCONN
	push	dword ptr [ebp-004h]
	call	listen
	jnz	@Server_end
 @Server_loop:
	lea	eax,[ebp-018h]
	push	eax
	push	[ebp-004h]
	call	@EventSelect
	test	eax,eax
	jz	@Server_end
	push	[ebp-018h]
	push	[ebp-004h]
	call	@WaitForEvents
	test	eax,eax
	jnz	@Server_proc_events
	push	019h
	call	Sleep
	jmp	@Server_loop
 @Server_proc_events:
	and	eax,FD_ACCEPT
	jz	@Server_loop
	xor	eax,eax
	push	eax
	push	eax
	push	eax
	lea	eax,[ebp-030h]
	push	eax
	push	dword ptr [ebp-004h]
	call	WSAAccept
	mov	[ebp-020h],eax
	inc	eax
	jz	@Server_loop
	push	01Ch
	push	LMEM_FIXED
	call	LocalAlloc
	test	eax,eax
	jz	@Server_close_newsock
	mov	ecx,[ebp-020h]
	mov	[eax],ecx
	lea	esi,[ebp-030h]
	lea	edi,[eax+004h]
	movsd
	movsd
	movsd
	movsd
	lea	esi,[ebp+008h]
	movsd
	movsd

	lea	ecx,[ebp-01Ch]
	push	ecx
	xor	ecx,ecx
	push	ecx
	push	eax
	push	offset @NewClientThread
	push	ecx
	push	ecx
	call	CreateThread
	jmp	@Server_loop
 @Server_close_newsock:
	push	dword ptr [ebp-020h]
	call	CloseSocket
	jmp	@Server_loop
 @Server_end:
	push	dword ptr [ebp-018h]
	call	CloseHandle
	push	dword ptr [ebp-004h]
	call	CloseSocket
	jmp	@end

 @EventSelect:
	call	WSACreateEvent
	test	eax,eax
	jz	@EventSelect_fail
	mov	ecx,[esp+008h]
	mov	[ecx],eax

	push	FD_ALL_EVENTS
	push	eax
	push	[esp+00Ch]
	call	WSAEventSelect
	inc	eax
	jnz	@EventSelect_end
 @EventSelect_fail:
	xor	eax,eax
 @EventSelect_end:
	ret	008h

 @WaitForEvents:
	push	ebp
	mov	ebp,esp
	sub	esp,02Ch
	push	000h
	push	WAITFOREVENTSTIMEOUT
	push	000h
	lea	eax,[ebp+00Ch]
	push	eax
	push	1
	call	WSAWaitForMultipleEvents
	inc	eax
	jz	@WaitForEvents_end
	sub	eax,WSA_WAIT_TIMEOUT+1
	jz	@WaitForEvents_end
	lea	eax,[ebp-02Ch]
	push	eax
	push	dword ptr [ebp+00Ch]
	push	dword ptr [ebp+008h]
	call	WSAEnumNetworkEvents
	inc	eax
	jz	@WaitForEvents_end
	mov	eax,[ebp-02Ch]
 @WaitForEvents_end:
	leave
	jmp	@EventSelect_end

 @NewClientThread:
	mov	ebp,esp
	sub	esp,070h

;	-070		-	RedirThreadHandle:THandle
;	-06C		-	ClientThreadHandle:THandle
;	-068		-	Redir.ThreadArgs.MainItem:PTcpItem
;	-064		-	Redir.ThreadArgs.OtherItem:PTcpItem
;	-060		-	Redir.ThreadArgs.ThreadType:Cardinal
;	-05C		-	Redir.ThreadArgs.Events:Longint
;	-058		-	Redir.ThreadArgs.EventHandle:THandle
;	-054		-	Redir.ThreadArgs.Active:Boolean
;	-050		-	Redir.ThreadArgs.Host.sin_family:Word
;	-04E		-	Redir.ThreadArgs.Host.sin_port:Word
;	-04C		-	Redir.ThreadArgs.Host.sin_addr:TInAddr
;	-048..-044	-	Redir.ThreadArgs.Host.sin_zero:array[0..7] 
;	-040		-	Redir.ThreadArgs.Socket
;	-038		-	Redir.ThreadID:Cardinal
;	-034		-	Client.ThreadArgs.MainItem:PTcpItem
;	-030		-	Client.ThreadArgs.OtherItem:PTcpItem
;	-02C		-	Client.ThreadArgs.ThreadType:Cardinal
;	-028		-	Client.ThreadArgs.Events:Longint
;	-024		-	Client.ThreadArgs.EventHandle:THandle
;	-020		-	Client.ThreadArgs.Active:Boolean
;	-01C		-	Client.ThreadArgs.Host.sin_family:Word
;	-01A		-	Client.ThreadArgs.Host.sin_port:Word
;	-018		-	Client.ThreadArgs.Host.sin_addr:TInAddr
;	-014..-010	-	Client.ThreadArgs.Host.sin_zero:array[0..7] 
;	-00C		-	Client.ThreadArgs.Socket
;	-008		-	Client.ThreadArgs.Connected
;	-004		-	Client.ThreadID:Cardinal
;	+004		-	AArgs:Pointer
;		+000		AArgs.NewSocket
;		+004		AArgs.NewHost.sin_family:Word
;		+006		AArgs.NewHost.sin_port:Word
;		+008		AArgs.NewHost.sin_addr:TInAddr
;		+00C..+010	AArgs.NewHost.sin_zero:array[0..7] of Char
;		+014		AArgs.FinalAddr:TInAddr
;		+018		AArgs.ListenPort:Word
;		+01A		AArgs.FinalPort:Word

	xor	eax,eax
	lea	edi,[ebp-070h]
	push	01Ch
	pop	ecx
	rep	stosd

	push	eax
	push	eax
	push	eax
	push	IPPROTO_TCP
	push	SOCK_STREAM
	push	AF_INET
	call	WSASocketA
	mov	[ebp-00Ch],eax
	inc	eax
	jz	@NewClientThread_close_newsock

	push	001h
	pop	eax
	mov	[ebp-020h],eax
	mov	[ebp-054h],eax
	mov	[ebp-060h],eax

	mov	edx,[ebp+004h]
	movzx	ax,byte ptr [edx+01Ah]
	xchg	ah,al
	mov	[ebp-01Ah],ax
	mov	word ptr [ebp-01Ch],AF_INET
	mov	eax,[edx+014h]
	mov	[ebp-018h],eax
	mov	eax,[edx]
	mov	[ebp-040h],eax
	lea	esi,[edx+004h]
	lea	edi,[ebp-050h]
	movsd
	movsd
	movsd
	movsd

	lea	eax,[ebp-058h]
	push	eax
	push	dword ptr [ebp-040h]
	call	@EventSelect
	test	eax,eax
	jz	@NewClientThread_close_clientsock

	lea	eax,[ebp-068h]
	mov	[ebp-030h],eax
	mov	[ebp-068h],eax
	lea	eax,[ebp-034h]
	mov	[ebp-034h],eax
	mov	[ebp-064h],eax

	lea	eax,[ebp-004h]
	push	eax
	push	000h
	lea	eax,[ebp-034h]
	push	eax
	push	offset @ThreadProc
	push	000h
	push	000h
	call	CreateThread
	test	eax,eax
	jz	@NewClientThread_close_clientsock
	mov	[ebp-06Ch],eax

	push	019h
	call	Sleep

	lea	eax,[ebp-038h]
	push	eax
	push	000h
	lea	eax,[ebp-068h]
	push	eax
	push	offset @ThreadProc
	push	000h
	push	000h
	call	CreateThread
	test	eax,eax
	jz	@NewClientThread_term_clientthread
	mov	[ebp-070h],eax

	push	-001h
	push	000h
	lea	eax,[ebp-070h]
	push	eax
	push	002h
	call	WaitForMultipleObjects
	xor	eax,eax
	mov	[ebp-054h],eax
	mov	[ebp-020h],eax
	mov	[ebp-008h],eax
	push	032h
	call	Sleep

	push	dword ptr [ebp-040h]
	call	CloseSocket
	push	dword ptr [ebp-00Ch]
	call	CloseSocket

	push	0FAh
	call	Sleep

	push	000h
	push	dword ptr [ebp-070h]
	call	TerminateThread
 @NewClientThread_term_clientthread:
	push	000h
	push	dword ptr [ebp-06Ch]
	call	TerminateThread
 @NewClientThread_close_clientsock:
	push	dword ptr [ebp-00Ch]
	call	CloseSocket

	push	dword ptr [ebp-058h]
	call	CloseHandle
	push	dword ptr [ebp-024h]
	call	CloseHandle

 @NewClientThread_close_newsock:
	mov	eax,[ebp+004h]
	push	dword ptr [eax]
	call	CloseSocket
	push	dword ptr [ebp+004h]
	call	LocalFree
	push	000h
	call	ExitThread

 @ThreadProc:
	mov	ebp,esp
	sub	esp,00Ch

;	-00C		-	LBuffer:Pointer
;	-008		-	LBytes:Cardinal
;	-004		-	LSocket:TSocket
;	+004		-	AArgs:Pointer
;		+000		AArgs.ThreadArgs.MainItem:PTcpItem
;		+004		AArgs.ThreadArgs.OtherItem:PTcpItem
;		+008		AArgs.ThreadArgs.ThreadType:Cardinal
;		+00C		AArgs.ThreadArgs.Events:Longint
;		+010		AArgs.ThreadArgs.EventHandle:THandle
;		+014		AArgs.ThreadArgs.Active:Boolean
;		+018		AArgs.ThreadArgs.Host.sin_family:Word
;		+01A		AArgs.ThreadArgs.Host.sin_port:Word
;		+01C		AArgs.ThreadArgs.Host.sin_addr:TInAddr
;		+020..+024	AArgs.ThreadArgs.Host.sin_zero:array[0..7]
;		+028    	AArgs.ThreadArgs.Socket
;		+02C    	AArgs.ThreadArgs.Connected - client only
 
	mov	esi,[ebp+004h]
	mov	eax,[esi+008h]
	test	eax,eax
	jnz	@ThreadProc_redir
	mov	eax,[esi+02Ch]
	test	eax,eax
	jnz	@ThreadProc_client_connected
	push	eax
	push	eax
	push	eax
	push	eax
	push	SOCK_ADDR_SIZE
	lea	eax,[esi+018h]
	push	eax
	push	dword ptr [esi+028h]
	call	WSAConnect
	inc	eax
	jz	@ThreadProc_error
	lea	eax,[esi+010h]
	push	eax
	push	dword ptr [esi+028h]
	call	@EventSelect
	mov	[esi+02Ch],eax
	test	eax,eax
	jz	@ThreadProc_error
	jmp	@ThreadProc_client_connected
 @ThreadProc_redir:
	mov	edi,[esi+004h]
 @ThreadProc_redir_waitforcon:
	push	019h
	call	Sleep
	mov	eax,[edi+02Ch]
	test	eax,eax
	jz	@ThreadProc_redir_waitforcon
 @ThreadProc_client_connected:
	mov	eax,[esi+014h]
	test	eax,eax
	jz	@ThreadProc_closesock
	mov	eax,[esi+004h]
	mov	eax,[eax+014h]
	test	eax,eax
	jz	@ThreadProc_closesock

	push	dword ptr [esi+010h]
	push	dword ptr [esi+028h]
	call	@WaitForEvents

	test	eax,eax
	jz	@ThreadProc_client_connected
	mov	[esi+00Ch],eax
	and	eax,FD_READ
	jnz	@ThreadProc_read
 @ThreadProc_af_read:
	mov	eax,[esi+00Ch]
	and	eax,FD_CLOSE
	jnz @ThreadProc_closesock
	jmp	@ThreadProc_client_connected
 @ThreadProc_read:
	push	dword ptr [esi+028h]
	call	@BytesToRecv
	test	eax,eax
	jz	@ThreadProc_af_read
	mov	edi,eax
	push	eax
	push	LMEM_FIXED
	call	LocalAlloc
	test	eax,eax
	jz	@ThreadProc_closesock
	mov	[ebp-00Ch],eax
	push	000h
	push	edi
	push	eax
	push	dword ptr [esi+028h]
	call	recv
	mov	[ebp-008h],eax
	inc	eax
	jz	@ThreadProc_read_free
 @ThreadProc_read_loop:
	push	000h
	push	dword ptr [ebp-008h]
	push	dword ptr [ebp-00Ch]
	mov	eax,[esi+004h]
	mov	eax,[eax+028h]
	push	eax
	call	send
	inc	eax
	jz	@ThreadProc_read_free
	dec	eax
	sub	[ebp-008h],eax
	jnz	@ThreadProc_read_loop
 @ThreadProc_read_free:
	push	dword ptr [ebp-00Ch]
	call	LocalFree
	jmp	@ThreadProc_read
 @ThreadProc_closesock:
	push	dword ptr [esi+028h]
	call	CloseSocket
 @ThreadProc_error:
	push	000h
	call	ExitThread

 @BytesToRecv:
	xor	eax,eax
	push	eax
	push	eax
	push	eax
	push	eax
	lea	ecx,[esp+00Ch]
	push	ecx
	push	004h
	sub	ecx,004h
	push	ecx
	push	eax
	push	eax
	push	FIONREAD
	push	[esp+02Ch]
	call	WSAIoctl
	inc	eax
	jz	@BytesToRecv_end
	mov	eax,[esp]
 @BytesToRecv_end:
	pop	ecx
	pop	ecx
	ret	004h

end start