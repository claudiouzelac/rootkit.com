//
// keyboard.c : chpie
//

#include <string.h>

// Sheet of Ordinary scancodes
char dataSheet[128][20] = {
	 "ERROR", "(ESC)",
	 "1!", "2@", "3#", "4$", "5%", "6^", "7&", "8*", "9(", "0)", "-_", "=+", "BackSpace",
	 "Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[{", "]}",
	 "Enter", "LCTRL", "A", "S", "D", "F", "G", "H", "J", "K", "L", ";:", "\'\"",
	 "`~", "LSHIFT", "|\\",
	 "Z", "X", "C", "V", "B", "N", "M", ",<", ".>", "/?", "RSHIFT",
	 "Keypad-*", "LALT", "SP", "Caps-Lock",
	 "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
	 "Num-Lock", "Scroll-Lock",
	 "Keypad-7", "Keypad-8", "Keypad-9", "Keypad--", "Keypad-4", "Keypad-5", "Keypad-6", "Keypad-+",
	 "Keypad-1", "Keypad-2", "Keypad-3", "Keypad-0", "Keypad-.", "Alt-SysRq"
};

char escapeSheet[128][20]; // escape E0

void init() // Initializing expansion scancodes
{
	strcpy(escapeSheet[0x1c], "Keypad-Enter");
	strcpy(escapeSheet[0x1d], "RCtrl");
	strcpy(escapeSheet[0x2a], "Fake-LShift");
	strcpy(escapeSheet[0x35], "Keypad-/");
	strcpy(escapeSheet[0x36], "Fake-RShift");
	strcpy(escapeSheet[0x37], "Ctrl-PrtScn");
	strcpy(escapeSheet[0x38], "RAlt");
	strcpy(escapeSheet[0x46], "Ctrl-Break");
	strcpy(escapeSheet[0x47], "Grey-Home");
	strcpy(escapeSheet[0x48], "Grey-Up");
	strcpy(escapeSheet[0x49], "Grey-PgUp");
	strcpy(escapeSheet[0x4b], "Grey-Left");
	strcpy(escapeSheet[0x4d], "Grey-Right");
	strcpy(escapeSheet[0x4f], "Grey-End");
	strcpy(escapeSheet[0x50], "Grey-Down");
	strcpy(escapeSheet[0x51], "Grey-PgDn");
	strcpy(escapeSheet[0x52], "Grey-Insert");
	strcpy(escapeSheet[0x53], "Grey-Delete");
	strcpy(escapeSheet[0x5b], "Left-Window");
	strcpy(escapeSheet[0x5c], "Right-Window");
	strcpy(escapeSheet[0x5d], "Menu");

	strcpy(dataSheet[0x57], "F11");
	strcpy(dataSheet[0x58], "F12");

	strcpy(dataSheet[0x71], "Chinese/KOR");
	strcpy(dataSheet[0x72], "Korea/ENG");
}

char* KeyboardSignal(unsigned char scancode, unsigned char escape, char *ret, int size)
{
	unsigned char buffer[2];

	init();

	buffer[0] = scancode;
	buffer[1] = escape;

	if (buffer[0] & 0x80) // clear MSB
	_asm
	{
		shl buffer[0], 1
		shr buffer[0], 1
	}

	if (escape == 0x00)
		strcpy(ret, dataSheet[buffer[0]]);
	else
	{
		if (escape == 0xE0)
			strcpy(ret, escapeSheet[buffer[0]]);
		//
	}

	return ret;
}