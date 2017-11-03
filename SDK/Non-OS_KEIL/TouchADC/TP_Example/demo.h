#define DBG_PRINTF		sysprintf
//#define DBG_PRINTF(...)

INT32 KeyPad(UINT32 u32Channel);
INT32 VoltageDetection(UINT32 u32Channel);
INT32 TouchPanel(void);
INT32 Integration(void);
INT32 Emu_RegisterBitToggle(void);
INT32 EmuTouch_Reset(void);
