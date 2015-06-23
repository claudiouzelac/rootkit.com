extern "C"
{
	#include "ntddk.h"
}

#include "module.h"

PMODULE_LIST   g_pml;

////////////////////////////////////////////////////////////////////
// MODULE INFO MANAGEMENT
//////////////////////////////////////////////////////////////////// 

////////////////////////////////////////////////////////////////////
// PMODULE_LIST GetListOfModules
// Parameters:
//		IN PNTSTATUS	pointer to NTSTATUS variable. Useful for debug.
// Returns:
//		OUT PMODULE_LIST	pointer to MODULE_LIST defined in 
//							ProcessName.h

PMODULE_LIST GetListOfModules(PNTSTATUS pns)
{
	ULONG		 ul_NeededSize;
	ULONG		 *pul_ModuleListAddress = NULL;
    NTSTATUS     ns;
    PMODULE_LIST pml   = NULL;

	ZwQuerySystemInformation(SystemModuleInformation, &ul_NeededSize, 0, &ul_NeededSize);
	pul_ModuleListAddress = (ULONG *) ExAllocatePool(PagedPool, ul_NeededSize);
	if (!pul_ModuleListAddress) // ExAllocatePool failed by returning NULL
	{
		if (pns != NULL) 
		{
			*pns = STATUS_INSUFFICIENT_RESOURCES; 
		}
		return (PMODULE_LIST) pul_ModuleListAddress;
	}
	ns = ZwQuerySystemInformation(SystemModuleInformation, pul_ModuleListAddress, ul_NeededSize * sizeof(*pul_ModuleListAddress), 0);
    if (ns != STATUS_SUCCESS)   // ZwQuerySystemInformation failed
    {
		// Free allocated paged kernel memory
		ExFreePool((PVOID) pul_ModuleListAddress);
		if (pns != NULL) 
		{
			*pns = ns;
        }
		return NULL;
    }
	pml = (PMODULE_LIST) pul_ModuleListAddress;
    if (pns != NULL) 
	{
		*pns = ns;
    }
	return pml;
}

//////////////////////////////////////////////////////////////////////
// BOOL IsAddressInModule
// Parameters:
//		IN ULONG		address of function we are testing
//		IN CHAR*		name of Module we are looking in
//		IN PNTSTATUS	pointer to ntstatus returned by GetListOfModules
// Returns:
//		OUT BOOL		if the address was found or not in the specified
//						module

BOOLEAN IsAddressInModule (ULONG address, char *ac_ModuleName, PNTSTATUS pns)
{
    int        count;
    NTSTATUS     ns;
    PMODULE_LIST pml = NULL;
	char tempName[512];

	if (ac_ModuleName == NULL)
	{
			if (pns != NULL) *pns = STATUS_INVALID_BUFFER_SIZE;
			return FALSE;
	}

	if (g_pml == NULL)
	{
	    if ((pml = GetListOfModules(&ns)) != NULL)
		{
			g_pml = pml;
			for (count = 0; count < pml->d_Modules; count++)
			{
				// Although we are looking for the address in a particular module,
				// I test the address first to see if it is within the proper bounds
				// because testing a DWORD value is quicker than doing a stricmp
				if((address >= (DWORD)pml->a_Modules[count].p_Base) && (address <= ((DWORD)pml->a_Modules[count].p_Base + pml->a_Modules[count].d_Size)))
				{
					if (_stricmp(ac_ModuleName, (char*)pml->a_Modules[count].a_bPath + pml->a_Modules[count].w_NameOffset) == 0)
					{
						if (pns != NULL) *pns = ns;
						return TRUE;
					}
				}
			}
			// We just got a new list of the modules, but the module with that address was not found.
			if (pns != NULL) *pns = STATUS_FILE_INVALID;
			return FALSE;
		}
		else
		{
			if (pns != NULL) *pns = ns;
			return FALSE;
		}

	}
	else // The g_pml was not NULL. Evidently we have cached the list of modules.
    {
        for (count = 0; count < g_pml->d_Modules; count++)
        {
			// Although we are looking for the address in a particular module,
			// I test the address first to see if it is within the proper bounds
			// because testing a DWORD value is quicker than doing a stricmp
			if((address >= (DWORD)g_pml->a_Modules[count].p_Base) && (address <= ((DWORD)g_pml->a_Modules[count].p_Base + g_pml->a_Modules[count].d_Size)))
			{
				if (_stricmp(ac_ModuleName, (char*)g_pml->a_Modules[count].a_bPath + g_pml->a_Modules[count].w_NameOffset) == 0)
				{
					if (pns != NULL) *pns = STATUS_SUCCESS;
					return TRUE;
				}
			}
		}
	}

	// If we reach here, we have searched through the list of cached modules but not found the correct one.
	// It is time to renew g_pml and search again.
	if (g_pml != NULL)
	{
		ExFreePool((PVOID) g_pml);
		g_pml = NULL;
	}
    if ((g_pml = GetListOfModules(&ns)) != NULL)
    {
        for (count = 0; count < g_pml->d_Modules; count++)
        {
			// Although we are looking for the address in a particular module,
			// I test the address first to see if it is within the proper bounds
			// because testing a DWORD value is quicker than doing a stricmp
			if((address >= (DWORD)g_pml->a_Modules[count].p_Base) && (address <= ((DWORD)g_pml->a_Modules[count].p_Base + g_pml->a_Modules[count].d_Size)))
			{
				if (_stricmp(ac_ModuleName, (char*)g_pml->a_Modules[count].a_bPath + g_pml->a_Modules[count].w_NameOffset) == 0)
				{
					if (pns != NULL) *pns = ns;
					return TRUE;
				}
			}
		}
		if (pns != NULL) *pns = STATUS_FILE_INVALID;
	}
	else {
		if (pns != NULL) *pns = ns;
	}

	return FALSE;

}

/////////////////////////////////////////////////////////////////////
// void WhatModuleIsAddressIn
// Parameters:
//		IN ULONG		address we are looking for corresponding 
//						module of
//		OUT CHAR*		pointer to character array of module name if
//						address is found
//		IN PNTSTATUS	pointer to ntstatus returned by GetListOfModules


void WhatModuleIsAddressIn(ULONG address, char *mod_name, PNTSTATUS pns)
{
    int          count;
    NTSTATUS     ns;
    PMODULE_LIST pml = NULL;

	if (mod_name == NULL)
	{
		if (pns != NULL) *pns = STATUS_INVALID_BUFFER_SIZE;
		return;
	}

	if (g_pml == NULL)
	{
		if ((pml = GetListOfModules(&ns)) == NULL)
		{
			if (pns != NULL) *pns = ns;
			return;
		}
		else {
			g_pml = pml;
		}
			
	}

    for (count = 0; count < g_pml->d_Modules; count++)
    {
		if ((address >= (DWORD)g_pml->a_Modules[count].p_Base) && (address <= ((DWORD)g_pml->a_Modules[count].p_Base + g_pml->a_Modules[count].d_Size)))
		{
			if (pns != NULL) *pns = STATUS_SUCCESS;
			RtlCopyMemory(mod_name, g_pml->a_Modules[count].a_bPath, MAXIMUM_FILENAME_LENGTH-1);
			//ExFreePool((PVOID) g_pml);
			return;
		}
	}

	// If we get this far, the module name was not found in the cached list of modules. We will renew 
	// the global pointer to the list of modules and search one last time.
	if (g_pml != NULL)
	{
		ExFreePool((PVOID) g_pml);
		g_pml = NULL;
	}
    if ((g_pml = GetListOfModules(&ns)) != NULL)
    {
        for (count = 0; count < g_pml->d_Modules; count++)
        {
			if ((address >= (DWORD)g_pml->a_Modules[count].p_Base) && (address <= ((DWORD)g_pml->a_Modules[count].p_Base + g_pml->a_Modules[count].d_Size)))
			{
				if (pns != NULL) *pns = STATUS_SUCCESS;
				RtlCopyMemory(mod_name, g_pml->a_Modules[count].a_bPath, MAXIMUM_FILENAME_LENGTH-1);
				return;
			}
		}
		if (pns != NULL) *pns = STATUS_FILE_INVALID;
	}
	else {
		if (pns != NULL) *pns = ns;
	}

	return;
}


/////////////////////////////////////////////////////////////////////
// PVOID GetModuleBase( char* ModuleName )
// Parameters:
//		IN CHAR		name of the driver we want to look up the base 
//				    address for
//		OUT PVOID   pointer to module base
// 
//		ERRORS: ERROR_RETRIEVING_MODULE_LIST
//				MODULE_NOT_FOUND
int GetModuleBase( char* ModuleName, MODULE* mod )
{
	PMODULE_LIST pml = {0};
	NTSTATUS ntStatus = 0;

	if ((pml = GetListOfModules(&ntStatus)) != NULL) 
	{
		for (int count = 0; count < pml->d_Modules; count++)
		{
			if (_stricmp(ModuleName, (char*)pml->a_Modules[count].a_bPath + pml->a_Modules[count].w_NameOffset) == 0)
			{
				mod->Base = (DWORD)pml->a_Modules[count].p_Base;
				mod->End  = ((DWORD)pml->a_Modules[count].p_Base + pml->a_Modules[count].d_Size);
				return true;
			}//end if
		}//end for
		return MODULE_NOT_FOUND;
	}//end if

	return ERROR_RETRIEVING_MODULE_LIST;
}//end GetModuleBase
