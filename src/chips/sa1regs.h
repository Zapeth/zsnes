#ifndef SA1REGS_H
#define SA1REGS_H

#include "../types.h"

extern void RTC2800();
extern void RTC2801w();
extern void SDD1Reset();
extern void initSA1regs();
extern void initSA1regsw();
extern void initSDD1regs();

extern u1 SA1IRQEnable;
extern u1 SA1RegP;
extern u2 SA1xd;
extern u4 RTCPtr;

#endif
