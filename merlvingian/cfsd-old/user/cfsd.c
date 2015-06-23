/*
 *   Clandestine File System Driver
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
 *   General header information  
 *
 *
 *   REFERENCE for this code :
 *    swapbuffers.c - DDK
 *    OSR ListServer Discussion Groups - http://www.osronline.com/page.cfm?name=search
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <assert.h>
#include <strsafe.h>

#include "cfsd.h"
#include "..\inc\crossover.h"


/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
void
PrintBanner()
{
 printf("Clandestine File System Driver - User Interface, Copyright (C) Jason Todd 2005\n");
 printf("Clandestine File System Driver comes with ABSOLUTELY NO WARRANTY\n\n");
 printf("** THIS IS A NASTY HACK OF A USER MODE INTERFACE JUST TO ALLOW FOR NON COMPILED CHANGES TO FILE NAME MATCHING. THIS DRIVER IS STILL UNDER HEAVY DEVELOPMENT WITH THIS INTERFACE AS QUICK EXAMPLE FOR THOSE WHO CANT RECOMPILE THE DRIVER**\n\n");

 printf("USAGE - cfsd <file name>\n");
 printf("cfsd NOTES.TXT\n\n");
 printf("Wild Cards are accepted in file names but explorer gives very odd results at times so use patern matching at your own risk\n\n");
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
int _cdecl
main( int argc,
      char *argv[] )
{

HRESULT    hResult = S_OK;
HANDLE     hPort = INVALID_HANDLE_VALUE;


DWORD BytesReturned = 0;


  PrintBanner();

  if ( argc < 2 )
  {
 printf("You *MUST* specify a file name\n");
 exit(0);
  }


 printf( "Attemping connection to cfsd filter - " );

    hResult = FilterConnectCommunicationPort( USER_COMMUNICATION_PORT_NAME,
                                              0,
                                              NULL,
                                              0,
                                              NULL,
                                              &hPort );

    if (IS_ERROR( hResult )) 
    {
 printf("Did you use the 'net start cfsd' command?\n");
 printf( "FAILURE ERROR: 0x%08x\n", hResult );
     exit(1);
    }
    else
    {
 printf( "SUCCESS\n" );
    }


 printf("Injecting (%s) into file name match criteria\n",argv[1] );



         hResult = FilterSendMessage( hPort,
                                      argv[1],
                                      sizeof( CHAR ) * strlen(argv[1]),
                                      NULL,
                                      0,
                                      &BytesReturned );


    if ( hPort != INVALID_HANDLE_VALUE ) 
    {
     CloseHandle( hPort );
    }


}