#include "i386.h"


//
// Define the common portion of the keyboard/mouse device extension.
//
typedef struct _PORT_KEYBOARD_EXTENSION {
    //
    // Pointer back to the this extension's device object.
    //
    PDEVICE_OBJECT      Self;
 
    PKINTERRUPT InterruptObject;

} PORT_KEYBOARD_EXTENSION, *PPORT_KEYBOARD_EXTENSION;
