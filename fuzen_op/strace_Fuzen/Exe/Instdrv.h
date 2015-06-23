#include <windows.h>

BOOL LoadDeviceDriver( const char * Name, const char * Path, 
					  HANDLE * lphDevice, PDWORD Error, BOOL extended );
BOOL UnloadDeviceDriver( const char * Name );