typedef struct idtentry {
        unsigned short OffsetLow;
        unsigned short Selector;
        unsigned char Reserved;
        unsigned char Type:4;
        unsigned char Always0:1;
        unsigned char Dpl:2;
        unsigned char Present:1;
        unsigned short OffsetHigh;
} IdtEntry_t, *PIdtEntry_t;
