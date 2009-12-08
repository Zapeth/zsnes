#include <string.h>

#include "../asm_call.h"
#include "../c_init.h"
#include "../cfg.h"
#include "../cpu/c_execute.h"
#include "../cpu/dspproc.h"
#include "../cpu/execute.h"
#include "../cpu/regs.h"
#include "../effects/burn.h"
#include "../effects/smoke.h"
#include "../effects/water.h"
#include "../endmem.h"
#include "../intrf.h"
#include "../macros.h"
#include "../ui.h"
#include "../vcache.h"
#include "../video/makevid.h"
#include "../video/procvid.h"
#include "../zstate.h"
#include "../ztimec.h"
#include "c_gui.h"
#include "c_guitools.h"
#include "gui.h"
#include "guicheat.h"
#include "guifuncs.h"
#include "guikeys.h"
#include "guimouse.h"
#include "guiwindp.h"

#ifdef __MSDOS__
#	include "../dos/vesa2.h"
#endif

#ifdef __OPENGL__
#	include "../linux/sdlintrf.h"
#endif


static char const* guimsgptr;


static void loadmenuopen(u4 const param1) // XXX better parameter name
{
	GUIpmenupos = GUIcmenupos;
	GUIcmenupos = 0;
	if (GUIwinactiv[param1] != 1)
	{
		GUIwinorder[GUIwinptr++] = param1;
		GUIwinactiv[param1]      = 1;
		if (savewinpos == 0)
		{
			GUIwinposx[param1] = GUIwinposxo[param1];
			GUIwinposy[param1] = GUIwinposyo[param1];
		}
	}
	else
	{
		// look for match
		u4 i = 0;
		u1 bl; // XXX better variable name
		do bl = GUIwinorder[i++]; while (bl != param1);
		for (; i != GUIwinptr; ++i)
			GUIwinorder[i - 1] = GUIwinorder[i];
		GUIpclicked        = 0;
		GUIwinorder[i - 1] = bl;
	}
}


void GUIBox3D(u4 const x1, u4 const y1, u4 const x2, u4 const y2)
{
	GUIBox(x1, y1, x2, y2, 160);
	GUIBox(x1, y1, x2, y1, 162);
	GUIBox(x1, y1, x1, y2, 161);
	GUIBox(x2, y1, x2, y2, 159);
	GUIBox(x1, y2, x2, y2, 158);
}


void GUIOuttextShadowed(u4 const x, u4 const y, char const* const text)
{
	GUIOuttext(x + 1, y + 1, text, 220 - 15);
	GUIOuttext(x,     y,     text, 220);
}


static char const guiftimemsg8[] = "PRESS SPACEBAR TO PROCEED.";


static void guifirsttimemsg(void)
{
	memset(pressed, 0, 256); // XXX maybe should be sizeof(pressed)
	pressed[0x2C] = 0; // XXX redundant

	do
	{
		GUIBox3D(43, 75, 213, 163);
		GUIOuttextShadowed(51,  80, " ONE-TIME USER REMINDER : ");
		GUIOuttextShadowed(51,  95, "  PLEASE BE SURE TO READ  ");
		GUIOuttextShadowed(51, 103, "THE DOCUMENTATION INCLUDED");
		GUIOuttextShadowed(51, 111, " WITH ZSNES FOR IMPORTANT");
		GUIOuttextShadowed(51, 119, " INFORMATION AND ANSWERS");
		GUIOuttextShadowed(51, 127, "    TO COMMON PROBLEMS");
		GUIOuttextShadowed(51, 135, "      AND QUESTIONS.");
		GUIOuttextShadowed(51, 150, guiftimemsg8);
		asm_call(vidpastecopyscr);
		asm_call(GUIUnBuffer);
		asm_call(DisplayBoxes);
		DisplayMenu();
		asm_call(JoyRead);
	}
	while (pressed[0x39] == 0);
}


static void horizonfixmsg(void)
{
	memset(pressed, 0, 256); // XXX maybe should be sizeof(pressed)
	pressed[0x2C] = 0; // XXX redundant

	do
	{
		GUIBox3D(43, 75, 213, 163);
		GUIOuttextShadowed(51, 80, "     WELCOME TO ZSNES");
		char const* msg = guimsgptr;
		GUIOuttextShadowed(51, 95, msg);
		msg += 32;
		GUIOuttextShadowed(51, 103, msg);
		msg += 32;
		GUIOuttextShadowed(51, 111, msg);
		msg += 32;
		GUIOuttextShadowed(51, 119, msg);
		GUIOuttextShadowed(51, 150, guiftimemsg8);
		asm_call(vidpastecopyscr);
		asm_call(GUIUnBuffer);
		asm_call(DisplayBoxes);
		DisplayMenu();
		asm_call(JoyRead);
	}
	while (pressed[0x39] == 0);
}


void StartGUI(void)
{
	static u1 MouseInitOkay = 0;

#ifdef __OPENGL__
	if (FilteredGUI == 0 && BilinearFilter == 1) blinit = 1;
#endif
	GUILoadPos = 0;
#ifdef __MSDOS__
	if (TripBufAvail == 0) Triplebufen = 0;
#endif
	if (MMXSupport != 1 || newgfx16b == 0)
	{
		En2xSaI  = 0;
		hqFilter = 0;
	}
	if (En2xSaI != 0)
	{
#ifdef __MSDOS__
		Triplebufen = 0;
#endif
		hqFilter  = 0;
		scanlines = 0;
		antienab  = 0;
	}
	if (hqFilter != 0)
	{
		En2xSaI   = 0;
		scanlines = 0;
		antienab  = 0;
	}

	memset(SpecialLine, 0, sizeof(SpecialLine));

	GUIOn    = 1;
	GUIOn2   = 1;
	NumCombo = GUIComboGameSpec != 0 ? NumComboLocl : NumComboGlob;
#ifdef __MSDOS__
	asm_call(ResetTripleBuf);
#endif

	if (GUIwinposx[16] == 0)
	{
		GUIwinposx[16] =  3;
		GUIwinposy[16] = 22;
	}

	GUICTimer = 0;
	{ // Initialize volume
		u4 vol = (u4)MusicRelVol * 128 / 100;
		if (vol > 127) vol = 127;
		MusicVol = vol;
	}
	CheatSearchStatus = 0;
	if (newgfx16b != 0) memset(vidbufferofsb, 0, 256 * 144 * 4);
	ShowTimer = 1;
	if ((GetDate() & 0xFFFF) == 0x0C25) OkaySC = 1;
	lastmouseholded = 1;
	if (GUIwinposx[15] == 0)
	{ // Movie menu fix
		GUIwinposx[15] = 50;
		GUIwinposy[15] = 50;
	}
	PrevResoln = resolutn;
	resolutn   = 224;

	GUIPalConv   = 0;
	MousePRClick = 1;

	if (MouseInitOkay != 1)
	{
		MouseInitOkay = 1;
		if (MouseDis != 1)
		{
			u4 res;
			asm volatile("call %P1" : "=a" (res) : "X" (Init_Mouse) : "cc", "memory", "edx", "ecx");
			if (res == 0) MouseDis = 1;
		}
	}

	if (pressed[KeyQuickLoad] & 1)
	{
		GUIcmenupos = 0;
		loadmenuopen(1);
	}
	memset(pressed, 0, 256 + 128 + 32); // XXX 32 probably should be 64
	pressed[1]  = 2;
	GUIescpress = 1;

	// set Video cursor location
	u4 eax = cvidmode;
	GUIcurrentvideocursloc = eax;
	u4 ebx = NumVideoModes;
	if (ebx > 20)
	{
		ebx -= 20;
		if (eax > ebx) eax = ebx;
		GUIcurrentvideoviewloc = eax;
	}
	else
	{
		GUIcurrentvideoviewloc = 0;
	}

	SaveSramData();
	GUIQuickLoadUpdate();

	asm_call(LoadDetermine);

	if (AutoState != 0 && romloadskip == 0) SaveSecondState();

	asm_call(GUIInit);
	memset(pressed, 0, 256); // XXX probably + 128 + 64 missing, maybe even completely redundant (has been zeroed above)

	if (GUIwinptr != 0)
	{
		GUIcmenupos = 0;
	}
	else if (esctomenu != 0)
	{
		GUIcmenupos = 2;
		GUIcrowpos  = 0;
		GUICYLocPtr = MenuDat2;
		if (esctomenu != 1) GUIcmenupos = 0;
	}
	if (GUIwinactiv[1] != 0)
	{
		GUIcurrentfilewin = 0;
		GetLoadData();
	}
	GUIHold = 0;
	// clear 256 bytes from hirestiledat
	memset(hirestiledat, 0, sizeof(hirestiledat));
	curblank = 0;
	asm_call(InitGUI);

	if (CheatWinMode != 0) LoadCheatSearchFile();

	GUIQuit = 0;
	while (GUIQuit != 2)
	{
		if (GUIQuit == 1)
		{
			asm_call(GUIDeInit);

			resolutn = PrevResoln;
			asm_call(endprog);
			return;
		}
		GUIQuit = 0;
		if (MouseDis != 1)
		{
			asm_call(ProcessMouse);
			if (videotroub == 1) return;
		}
		asm_call(GUIUnBuffer);
		if (GUIEffect == 1) asm_call(DrawSnow);
		if (GUIEffect == 2) DrawWater();
		if (GUIEffect == 3) DrawWater();
		if (GUIEffect == 4) DrawBurn();
		if (GUIEffect == 5) DrawSmoke();

		if (GUIEditStringcWin != 0)
		{
			u1* eax = GUIEditStringcLen;
			if (eax)
			{
				if (GUIEditStringLTxt >= 8)
				{
					eax[0]            = '_';
					eax[1]            = '\0';
					GUIEditStringLstb = 1;
				}
				if (GUIEditStringLTxt == 0) GUIEditStringLTxt = 16;
			}
		}

		asm_call(DisplayBoxes);

		if (GUIEditStringLstb == 1)
		{
			GUIEditStringLstb    = 0;
			GUIEditStringcLen[0] = '\0';
		}

		DisplayMenu();
		if (MouseDis != 1) asm_call(DrawMouse);
		if (FirstTimeData == 0)
		{
			guifirsttimemsg();
			FirstTimeData = 1;
		}
		if (!guimsgptr && (GetDate() & 0xFFFF) == 0x0401)
		{
			guimsgptr = (char const*)horizon_get(GetTime());
			horizonfixmsg();
		}
		if (GUICTimer != 0)
		{
			GUIOuttext(21, 211, GUICMessage, 50);
			GUIOuttext(20, 210, GUICMessage, 63);
		}
		asm_call(vidpastecopyscr);
		asm_call(GUIgetcurrentinput);
	}
	memset(spcBuffera, 0, 256 * 1024);
	asm_call(GUIDeInit);
#ifdef __MSDOS__
	asm_call(DOSClearScreen);
	if (cbitmode == 0) asm_call(dosmakepal);
#endif
	t1cc = 1;

	GUISaveVars();

	MousePRClick = 1;
	prevbright   = 0;
	resolutn     = PrevResoln;

	CheatOn = NumCheats != 0;

	if (CopyRamToggle == 1)
	{
		CopyRamToggle = 0;
		// copy 128k ram
		memcpy(vidbuffer + 129600, wramdata, 128 * 1024);
	}

	if (CheatWinMode == 2) CheatWinMode = 1;

	if (CheatWinMode != 0) SaveCheatSearchFile();

	memset(vidbuffer, 0, 288 * 120 * 4);

	memset(vidbufferofsb, 0, 256 * 144 * 4);

	asm_call(AdjustFrequency);
	GUIOn    = 0;
	GUIOn2   = 0;
	GUIReset = 0;
	StartLL  = 0;
	StartLR  = 0;
	continueprog();
}


void guimencodermsg(void)
{
	memset(pressed, 0, 256); // XXX maybe should be sizeof(pressed)
	pressed[0x2C] = 0; // XXX redundant

	do
	{
		GUIBox3D(43, 75, 213, 163);
		GUIOuttextShadowed(51,  95, " MENCODER IS MISSING: ");
		GUIOuttextShadowed(51, 133, "PRESS SPACE TO PROCEED");
		asm_call(vidpastecopyscr);
		asm_call(GUIUnBuffer);
		asm_call(DisplayBoxes);
		DisplayMenu();
		asm_call(JoyRead);
	}
	while (pressed[0x39] == 0);
}


void guilamemsg(void)
{
	memset(pressed, 0, 256); // XXX maybe should be sizeof(pressed)
	pressed[0x2C] = 0; // XXX redundant

	do
	{
		GUIBox3D(43, 75, 213, 163);
		GUIOuttextShadowed(51, 95, " LAME IS MISSING: ");
		GUIOuttextShadowed(51,133, "PRESS SPACE TO PROCEED");
		asm_call(vidpastecopyscr);
		asm_call(GUIUnBuffer);
		asm_call(DisplayBoxes);
		DisplayMenu();
		asm_call(JoyRead);
	}
	while (pressed[0x39] == 0);
}


u1* GetAnyPressedKey(void)
{
	for (u1* i = pressed; i != endof(pressed); ++i)
		if (*i != 0) return i;
	return 0;
}


static u4 GetMouseButtons(void)
{
	if (MouseDis == 1) return 0;

	u4 ebx;
	asm("call *%1" : "=b" (ebx) : "r" (Get_MouseData) : "cc", "memory", "ecx", "edx");
	if (lhguimouse == 1)
		asm("call *%1" : "+b" (ebx) : "r" (SwapMouseButtons) : "cc");
	return ebx;
}


void guiprevideo(void)
{
	memset(pressed, 0, 256); // XXX maybe should be sizeof(pressed)

	asm_call(GUIUnBuffer);
	asm_call(DisplayBoxes);
	DisplayMenu();
	GUIBox3D(43, 90, 213, 163);
	GUIOuttextShadowed(55,  95, "ZSNES WILL NOW ATTEMPT");
	GUIOuttextShadowed(55, 103, " TO CHANGE YOUR VIDEO");
	GUIOuttextShadowed(55, 111, " MODE.  IF THE CHANGE");
	GUIOuttextShadowed(55, 119, "IS UNSUCCESSFUL,  WAIT");
	GUIOuttextShadowed(55, 127, " 10 SECONDS AND VIDEO");
	GUIOuttextShadowed(55, 135, "MODE WILL BE RESTORED.");
	GUIOuttextShadowed(55, 150, "    PRESS ANY KEY.");
	asm_call(vidpastecopyscr);
	pressed[0x2C] = 0; // XXX redundant
	for (;;)
	{
		asm_call(JoyRead);

		u1* const key = GetAnyPressedKey();
		if (key)
		{
			*key = 0;
			return;
		}
		if (GetMouseButtons() & 0x01) return;
	}
}


void guicheaterror(void)
{
	memset(pressed, 0, sizeof(pressed));

	for (;;)
	{
		asm_call(GUIUnBuffer);
		asm_call(DisplayBoxes);
		DisplayMenu();
		GUIBox3D(75, 95, 192, 143);
		GUIOuttextShadowed(80, 100, "INVALID CODE!  YOU");
		GUIOuttextShadowed(80, 108, "MUST ENTER A VALID");
		GUIOuttextShadowed(80, 116, "GAME GENIE,PAR, OR");
		GUIOuttextShadowed(80, 124, "GOLD FINGER CODE.");
		GUIOuttextShadowed(80, 134, "PRESS ANY KEY.");
		asm_call(vidpastecopyscr);
		asm_call(JoyRead);

		if (GetAnyPressedKey())       break;
		if (GetMouseButtons() & 0x01) break;
	}
	while ((u1)Check_Key() != 0) // XXX asm_call
		asm_call(Get_Key);
	GUIcurrentcheatwin = 1;
	GUIpclicked        = 1;
}


static void GUIDMHelp(u4 const x1, u4 const x2, char const* const text, u4 const param4)
{
	GUItextcolor[0] = 46;
	GUItextcolor[1] = 42;
	GUItextcolor[2] = 38;
	GUItextcolor[3] = 44;
	GUItextcolor[4] = 40;
	if (GUIcmenupos == param4)
	{
		GUItextcolor[0] = 38;
		GUItextcolor[1] = 40;
		GUItextcolor[2] = 46;
		GUItextcolor[3] = 40;
		GUItextcolor[4] = 44;
	}
	GUIBox(x1,  3, x2,  3, GUItextcolor[0]);
	GUIBox(x1,  4, x2, 12, GUItextcolor[1]);
	GUIBox(x1, 13, x2, 13, GUItextcolor[2]);
	GUIBox(x1,  3, x1, 12, GUItextcolor[3]);
	GUIBox(x2,  4, x2, 13, GUItextcolor[4]);
	GUIOuttext(x1 + 5, 7, text, 44);
	GUIOuttext(x1 + 4, 6, text, 62);
}


static void GUIDMHelpB(u4 const x1, u4 const x2, char const* const text, u4 const param4)
{
	GUItextcolor[0] = 46;
	GUItextcolor[1] = 42;
	GUItextcolor[2] = 38;
	GUItextcolor[3] = 44;
	GUItextcolor[4] = 40;
	if (GUIcwinpress == param4)
	{
		GUItextcolor[0] = 38;
		GUItextcolor[1] = 40;
		GUItextcolor[2] = 46;
		GUItextcolor[3] = 40;
		GUItextcolor[4] = 44;
	}
	GUIBox(x1,  3, x2,  3, GUItextcolor[0]);
	GUIBox(x1,  4, x2, 13, GUItextcolor[1]);
	GUIBox(x1, 14, x2, 14, GUItextcolor[2]);
	GUIBox(x1,  3, x1, 13, GUItextcolor[3]);
	GUIBox(x2,  4, x2, 14, GUItextcolor[4]);
	GUIOuttext(x1 + 3, 7, text, 44);
	GUIOuttext(x1 + 2, 6, text, 62);
}


#ifdef __WIN32__
static void GUIDMHelpB2(u4 const x1, u4 const x2, char const* const text, u4 const param4)
{
	GUItextcolor[0] = 46;
	GUItextcolor[1] = 42;
	GUItextcolor[2] = 38;
	GUItextcolor[3] = 44;
	GUItextcolor[4] = 40;
	if (GUIcwinpress == param4)
	{
		GUItextcolor[0] = 38;
		GUItextcolor[1] = 40;
		GUItextcolor[2] = 46;
		GUItextcolor[3] = 40;
		GUItextcolor[4] = 44;
	}
	GUIBox(x1, 3, x2, 3, GUItextcolor[0]);
	GUIBox(x1, 4, x2, 6, GUItextcolor[1]);
	GUIBox(x1, 7, x2, 7, GUItextcolor[2]);
	GUIBox(x1, 3, x1, 6, GUItextcolor[3]);
	GUIBox(x2, 4, x2, 7, GUItextcolor[4]);
	GUIOuttext(x1 + 3, 5, text, 44);
	GUIOuttext(x1 + 2, 4, text, 62);
}


static void GUIDMHelpB3(u4 const x1, u4 const x2, char const* const text, u4 const param4)
{
	GUItextcolor[0] = 46;
	GUItextcolor[1] = 42;
	GUItextcolor[2] = 38;
	GUItextcolor[3] = 44;
	GUItextcolor[4] = 40;
	if (GUIcwinpress == param4)
	{
		GUItextcolor[0] = 38;
		GUItextcolor[1] = 40;
		GUItextcolor[2] = 46;
		GUItextcolor[3] = 40;
		GUItextcolor[4] = 44;
	}
	GUIBox(x1,  9, x2,  9, GUItextcolor[0]);
	GUIBox(x1, 10, x2, 12, GUItextcolor[1]);
	GUIBox(x1, 13, x2, 13, GUItextcolor[2]);
	GUIBox(x1,  9, x1, 12, GUItextcolor[3]);
	GUIBox(x2, 10, x2, 13, GUItextcolor[4]);
	GUIOuttext(x1 + 3, 11, text, 44);
	GUIOuttext(x1 + 2, 10, text, 62);
}
#endif


static void GUIMenuDisplay(u4 const n_cols, u4 n_rows, u1* dst, char const* text)
{
	u4 row = 0;
	do
	{
		u1 const al = *text;
		if (al != '\0')
		{
			++text;
			if (al != 2)
			{
				GUItextcolor[0] = 44;
				if (GUIcrowpos != row)
					GUIOutputString(dst + 289, text);
				GUItextcolor[0] = 63;
			}
			else
			{
				GUItextcolor[0] = 42;
				if (GUIcrowpos != row)
					GUIOutputString(dst + 289, text);
				GUItextcolor[0] = 57;
			}
			text = GUIOutputString(dst, text) + 1;
		}
		else
		{
			u1* d    = dst + 4 * 288;
			u4  cols = n_cols;
			do
			{
				d[   0] = 45;
				d[-289] = 40;
				d[ 289] = 42;
				++d;
			}
			while (--cols != 0);
			text += 14;
		}
		dst += 10 * 288;
		++row;
	}
	while (--n_rows != 0);
}


static void GUIDrawMenuM(u4 const x1, u4 const y1, u4 const p3, u4 const p4, char const* const text, u4 const p6, u4 const p7, u4 const p8, u4 const p9, u4 const p10)
{
	GUIShadow(p7, p8, p7 + 4 + p3 * 6, p8 + 3 + p4 * 10);
	GUIBox(x1, y1, x1 + 4 + p3 * 6, y1 + 3 + p4 * 10, 43);

	u1* dst = vidbuffer + GUIcrowpos * 2880 + x1 + 17 + 18 * 288;
	GUIDrawBox(dst,           6 * p3 + 3, 1, 73);
	GUIDrawBox(dst + 288,     6 * p3 + 3, 7, 72);
	GUIDrawBox(dst + 288 * 8, 6 * p3 + 3, 1, 73);

	GUIBox(x1 + p10,        y1,     x1 + 4 + p3 * 6, y1, 47);
	GUIBox(x1,              y1,     x1,              p9, 45);
	GUIBox(x1,              p9,     x1 + 4 + p3 * 6, p9, 39);
	GUIBox(x1 + 4 + p3 * 6, 1 + y1, x1 + 4 + p3 * 6, p9, 41);
	GUIMenuDisplay(6 * p3, p4, vidbuffer + 16 + p6 + 20 * 288, text);

	GUIMenuL = x1 + 1;
	GUIMenuR = x1 + 6 * p3 + 3;
	GUIMenuD = 18 + p4 * 10;
}


void DisplayMenu(void)
{
	// Draw Shadow
	GUIShadow(5, 7, 235, 21);
	// Display Top Border
	GUIBox(0,  1, 229,  1, 71);
	GUIBox(0,  2, 229,  2, 70);
	GUIBox(0,  3, 229,  3, 69);
	GUIBox(0,  4, 229,  4, 68);
	GUIBox(0,  5, 229,  5, 67);
	GUIBox(0,  6, 229,  6, 66);
	GUIBox(0,  7, 229,  7, 65);
	GUIBox(0,  8, 229,  8, 64);
	GUIBox(0,  9, 229,  9, 65);
	GUIBox(0, 10, 229, 10, 66);
	GUIBox(0, 11, 229, 11, 67);
	GUIBox(0, 12, 229, 12, 68);
	GUIBox(0, 13, 229, 13, 69);
	GUIBox(0, 14, 229, 14, 70);
	GUIBox(0, 15, 229, 15, 71);

#ifdef __UNIXSDL__
	GUIShadow(238, 9, 247, 20);
	GUIShadow(249, 9, 257, 20);
#endif
#ifdef __WIN32__
	GUIShadow(238,  9, 247, 14);
	GUIShadow(238, 16, 247, 20);
	GUIShadow(249,  9, 257, 20);
#endif

#ifdef __UNIXSDL__
	GUIMenuItem[36] = 247;
	GUIDMHelpB(233, 242, GUIMenuItem + 36, 1);
	GUIMenuItem[36] = 'x';
	GUIDMHelpB(244, 253, GUIMenuItem + 36, 2);
#endif

#ifdef __WIN32__
	GUIMenuItem[36] = 249;
	GUIDMHelpB2(233, 242, GUIMenuItem + 36, 1);
	GUIMenuItem[36] = 248;
	GUIDMHelpB3(233, 242, GUIMenuItem + 36, 3);
	GUIMenuItem[36] = 'x';
	GUIDMHelpB( 244, 253, GUIMenuItem + 36, 2);
#endif

	// Display upper-left box
	GUIMenuItem[36] = 25;
	GUIDMHelp(4, 12, GUIMenuItem + 6, 1);
	GUIOuttext(4 + 3, 7, GUIMenuItem + 36, 44);
	GUIOuttext(4 + 2, 6, GUIMenuItem + 36, 62);
	// Display boxes
	GUIDMHelp( 17,  47, GUIMenuItem,      2);
	GUIDMHelp( 52,  94, GUIMenuItem +  7, 3);
	GUIDMHelp( 99, 135, GUIMenuItem + 14, 4);
	GUIDMHelp(140, 188, GUIMenuItem + 21, 5);
	GUIDMHelp(193, 223, GUIMenuItem + 29, 6);

	GUIMenuL = 0;
	GUIMenuR = 0;
	GUIMenuD = 0;

	/* format : x pos, y pos, #charx, #chary, name, xpos+2, xpos+5,22,
	 *          19+#chary*10, length of top menu box */
	if (GUIcmenupos == 1)
	{
		GUIDrawMenuM(4, 16, 30, 13, GUIPrevMenuData, 6, 9, 22, 149, 8); // 19+13*10
		GUICYLocPtr = MenuDat1;
	}
	if (GUIcmenupos == 2)
	{
		GUIDrawMenuM(17, 16, 10, 9, GUIGameMenuData, 19, 22, 22, 109, 30); // 19+9*10
		GUICYLocPtr = MenuDat2;
	}
	if (GUIcmenupos == 3)
	{
		GUIDrawMenuM(52, 16, 8, 11, GUIConfigMenuData, 54, 57, 22, 129, 42); // 19+11*10
		GUICYLocPtr = MenuDat3;
	}
	if (GUIcmenupos == 4)
	{
		GUIDrawMenuM(99, 16, 8, 3, GUICheatMenuData, 101, 104, 22, 49, 36); // 19+3*10
		GUICYLocPtr = MenuDat4;
	}
	if (GUIcmenupos == 5)
	{
#ifdef __MSDOS__
		GUIDrawMenuM(140, 16, 10, 2, GUINetPlayMenuData, 142, 145, 22, 39, 48); // 19+2*10
#else
		GUIDrawMenuM(140, 16, 10, 1, GUINetPlayMenuData, 142, 145, 22, 29, 48); // 19+1*10
#endif
		GUICYLocPtr = MenuDat5;
	}
	if (GUIcmenupos == 6)
	{
		GUIDrawMenuM(193, 16, 9, 7, GUIMiscMenuData, 195, 198, 22, 89, 30); // 19+5*10
		GUICYLocPtr = MenuDat6;
	}
}