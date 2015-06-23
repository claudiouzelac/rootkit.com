typedef unsigned long DWORD;
typedef unsigned long *PDWORD;
typedef unsigned short WORD;

BOOLEAN gb_Hooked;

// MakePtr is a macro that allows you to easily add to values (including
// pointers) together without dealing with C's pointer arithmetic.  It
// essentially treats the last two parameters as DWORDs.  The first
// parameter is used to typecast the result to the appropriate pointer type.
// by Jeffrey Richter?
#define MakePtr( cast, ptr, addValue ) (cast)( (DWORD)(ptr) + (DWORD)(addValue))

VOID MyImageLoadNotify(IN PUNICODE_STRING,
                       IN HANDLE,
					   IN PIMAGE_INFO);

NTSTATUS HookImportsOfImage(PIMAGE_DOS_HEADER, HANDLE);