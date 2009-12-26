#include <string.h>

#include "../asm_call.h"
#include "../c_intrf.h"
#include "../cpu/dspproc.h"
#include "../cpu/execute.h"
#include "../cpu/regs.h"
#include "../cpu/spc700.h"
#include "../gblvars.h"
#include "../initc.h"
#include "../input.h"
#include "../macros.h"
#include "../video/procvidc.h"
#include "../zmovie.h"
#include "../zstate.h"
#include "c_gui.h"
#include "c_guimisc.h"
#include "c_guimouse.h"
#include "gui.h"
#include "guikeys.h"
#include "guimisc.h"
#include "guimouse.h"
#include "guiwindp.h"

#ifdef __MSDOS__
#	include "../dos/joy.h"
#endif


static void CalibrateDispA(void)
{
	memset(pressed, 0, 256); // XXX Probably should be sizeof(pressed)
	GUIUnBuffer();
	DisplayBoxes();
	DisplayMenu();
	GUIBox3D(75, 103, 192, 135);
	GUIOuttextShadowed(80, 108, "PRESS THE TOP LEFT");
	GUIOuttextShadowed(80, 116, "CORNER AND PRESS A");
	GUIOuttextShadowed(80, 124, "BUTTON OR KEY");
	vidpastecopyscr();
	asm_call(GUIWaitForKey);
}


static void CalibrateDispB(void)
{
	memset(pressed, 0, 256); // XXX Probably should be sizeof(pressed)
	GUIUnBuffer();
	DisplayBoxes();
	DisplayMenu();
	GUIBox3D(75, 103, 192, 143);
	GUIOuttextShadowed(80, 108, "PRESS THE BOTTOM");
	GUIOuttextShadowed(80, 116, "RIGHT CORNER AND");
	GUIOuttextShadowed(80, 124, "PRESS A BUTTON OR");
	GUIOuttextShadowed(80, 132, "KEY");
	vidpastecopyscr();
	asm_call(GUIWaitForKey);
}


#define ConfigureKey2(i, player)                                 \
do                                                               \
{                                                                \
	switch (i)                                                     \
	{                                                              \
		case  0: guicpressptr = &player##upk;    break; /* Up     */ \
		case  1: guicpressptr = &player##downk;  break; /* Down   */ \
		case  2: guicpressptr = &player##leftk;  break; /* Left   */ \
		case  3: guicpressptr = &player##rightk; break; /* Right  */ \
		case  4: guicpressptr = &player##startk; break; /* Start  */ \
		case  5: guicpressptr = &player##selk;   break; /* Select */ \
		case  6: guicpressptr = &player##Ak;     break; /* A      */ \
		case  7: guicpressptr = &player##Bk;     break; /* B      */ \
		case  8: guicpressptr = &player##Xk;     break; /* X      */ \
		case  9: guicpressptr = &player##Yk;     break; /* Y      */ \
		case 10: guicpressptr = &player##Lk;     break; /* L      */ \
		case 11: guicpressptr = &player##Rk;     break; /* R      */ \
	}                                                              \
}                                                                \
while (0)

void SetAllKeys(void)
{
	static char const guipresstext4b[][21] =
	{
		"FOR UP              ",
		"FOR DOWN            ",
		"FOR LEFT            ",
		"FOR RIGHT           ",
		"FOR START           ",
		"FOR SELECT          ",
		"FOR A (RIGHT BUTTON)",
		"FOR B (DOWN BUTTON) ",
		"FOR X (TOP BUTTON)  ",
		"FOR Y (LEFT BUTTON) ",
		"FOR THE L BUTTON    ",
		"FOR THE R BUTTON    "
	};

	memset(pressed, 0, sizeof(pressed));

	GUICBHold = 0;
	switch (cplayernum)
	{
		default: keycontrolval = &pl1contrl; break;
		case 1:  keycontrolval = &pl2contrl; break;
		case 2:  keycontrolval = &pl3contrl; break;
		case 3:  keycontrolval = &pl4contrl; break;
		case 4:  keycontrolval = &pl5contrl; break;
	}

	// Check if controller is set
	if (*keycontrolval == 0) return; // XXX original compares dword instead of byte, former makes no sense
	u4 i = 0;
	u4 n = lengthof(guipresstext4b);
	guipressptr = guipresstext4b[0];
	do
	{
		switch (cplayernum)
		{
			case 0: ConfigureKey2(i, pl1); break;
			case 1: ConfigureKey2(i, pl2); break;
			case 2: ConfigureKey2(i, pl3); break;
			case 3: ConfigureKey2(i, pl4); break;
			case 4: ConfigureKey2(i, pl5); break;
		}
		guipresstestb();
		guipressptr += lengthof(*guipresstext4b); // XXX ugly, crosses lines of the array
		++i;
	}
	while (--n != 0);
}

#undef ConfigureKey2


void CalibrateDev1(void)
{
	u4 const player = cplayernum;
	u1 const contrl = *GUIInputRefP[player];
	GUICBHold = 0;

	u4 port = 0x201;
#ifdef __MSDOS__
	switch (player)
	{
		case 0: if (pl1p209 != 0) goto port209; break;
		case 1: if (pl2p209 != 0) goto port209; break;
		case 2: if (pl3p209 != 0) goto port209; break;
		case 3: if (pl4p209 != 0) goto port209; break;
		case 4: if (pl5p209 != 0) goto port209; break;
port209:
			port = 0x209;
			break;
	}
#endif

	if (contrl <= 1 || 6 <= contrl) return;
	void (* const f)() = contrl != 18 && contrl != 5 ? GetCoords : GetCoords3;
	asm volatile("call *%0" :: "r" (f), "d" (port) : "cc", "memory", "eax", "ecx"); // XXX asm_call
	u4 joybcx = JoyX;
	u4 joybcy = JoyY;
	CalibrateDispA();
	asm volatile("call *%0" :: "r" (f), "d" (port) : "cc", "memory", "eax", "ecx"); // XXX asm_call
	u4 joyblx = JoyX;
	u4 joybly = JoyY;
	CalibrateDispB();
	asm volatile("call *%0" :: "r" (f), "d" (port) : "cc", "memory", "eax", "ecx"); // XXX asm_call
#ifdef __MSDOS__
	if (port == 0x209)
	{
		CalibXmin209 = JoyMinX209 = (joybcx + joyblx) / 2;
		CalibYmin209 = JoyMinY209 = (joybcy + joybly) / 2;
		CalibXmax209 = JoyMaxX209 = (joybcx + JoyX)   / 2;
		CalibYmax209 = JoyMaxY209 = (joybcy + JoyY)   / 2;
	}
	else
#endif
	{
		CalibXmin    = JoyMinX    = (joybcx + joyblx) / 2;
		CalibYmin    = JoyMinY    = (joybcy + joybly) / 2;
		CalibXmax    = JoyMaxX    = (joybcx + JoyX)   / 2;
		CalibYmax    = JoyMaxY    = (joybcy + JoyY)   / 2;
	}
}


void SetDevice(void)
{
	GUICBHold = 0;
	u4 const player = cplayernum;
	u4 const contrl = GUIcurrentinputcursloc;
	*GUIInputRefP[player] = contrl;
#ifdef __MSDOS__
	u1 p209 = 0;
	switch (player)
	{
		case 0: p209 = pl1p209; break;
		case 1: p209 = pl2p209; break;
		case 2: p209 = pl3p209; break;
		case 3: p209 = pl4p209; break;
		case 4: p209 = pl5p209; break;
	}
	if (p209 != 0)
	{
		CalibXmin209 = 0;
		asm volatile("call %P0" :: "X" (SetInputDevice209), "b" (player << 8 | contrl) : "cc", "memory"); // XXX asm_call
	}
	else
#endif
	{
		CalibXmin = 0;
		SetInputDevice(contrl, player);
	}
	UpdateDevices();
	MultiTap = SFXEnable != 1 && (pl3contrl != 0 || pl4contrl != 0 || pl5contrl != 0);
}


void GUIDoReset(void)
{
#ifdef __MSDOS__
	asm_call(DOSClearScreen);
#endif
	Clear2xSaIBuffer();

	MovieStop();
	RestoreSystemVars();

	// reset the snes
	init65816();
	procexecloop();

	spcPCRam = SPCRAM + 0xFFC0;
	spcS     = 0x1EF;
	spcRamDP = SPCRAM;
	spcA     = 0;
	spcX     = 0;
	spcY     = 0;
	spcP     = 0;
	spcNZ    = 0;
	GUIQuit  = 2;
	memset(&Voice0Status, 0, sizeof(Voice0Status));
}
