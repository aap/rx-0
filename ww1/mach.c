#include "../common/fe.h"

#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>


typedef u16 word;	// NB: distinct from Word!!!!
typedef struct WWI WWI;
typedef struct ExUnit ExUnit;
typedef struct Scope Scope;
typedef struct Papertape Papertape;

struct ExUnit
{
	int active;	// reading or recording
	void (*init)(WWI *ww, ExUnit *eu);
	void (*stop)(WWI *ww, ExUnit *eu);	// TODO: maybe remove this
	void (*record)(WWI *ww, ExUnit *eu);
	void (*read)(WWI *ww, ExUnit *eu);
};

struct Scope
{
	ExUnit eu;
	// deflection is part of WWI
	int fd;	// or multiple for the different channels...
	// Light gun
	int gun;
	int gunx, guny;
};

struct Papertape
{
	ExUnit eu;
	// 0 - ptr & ptp
	// 1 - petr (printer)
	// 2 - printer
	// 3 - (printer)
	int out_fd[4];
	int in_fd[4];
	int wbyw;
	int p7;

	int dly;
};

struct WWI
{
	/* Switches and simulation */
	word pc_sw;	// reset value for PC
	int order_by_order;
	int run;
	int iostop;

	/* control */
	word pc, prevpc;
	word cs;
	/* storage */
	word ss;	// storage switch
	u32 ff_regs;	// bits in here identify FF registers
	word ffs[32];	// FF storage
	word tss[32];	// toggle switch storage and FF reset switches

	word core[04000];
	word par;
	word cr;	// not used much;
	/* arithmetic */
	word ar;
	int src;
	word ac, cry;
	word br;
	word ac0_carry, sam;
	word sc;
	int sign;
	/* IO */
	word ior;
	int ioc_out;	// is output
	int ioc_counter;	// 4 bits
	int interlock;
	int ios;
	ExUnit *active_unit;

	int hdefl, vdefl;
	Scope *scope;
	Papertape *ptape;

	int console_fd;
};

WWI ww1;
Scope scope;
Papertape ptape;

int isrunning(void) { return ww1.run; }
int isstopped(void) { return 0; }

#define MS_SELECT(a) ((a) & 0177740)
#define IS_FF(a) ((ww->ff_regs >> ((a)&037))&1)

void
deposit(Addr a, Word w)
{
	WWI *ww = &ww1;
	if(MS_SELECT(a))
		ww->core[a] = w;
	else if(IS_FF(a))
		ww->ffs[a & 037] = w;
	else
		ww->tss[a & 037] = w;
}

Word
examine(Addr a)
{
	WWI *ww = &ww1;
	if(MS_SELECT(a))
		return ww->core[a];
	else if(IS_FF(a))
		return ww->ffs[a & 037];
	else
		return ww->tss[a & 037];
}

void
depositreg(int a, Word w)
{
	switch(a){
	case REG_PC: ww1.pc = w & ADDRMASK; break;
	case REG_AC: ww1.ac = w & WORDMASK; break;
	case REG_A: ww1.ar = w & WORDMASK; break;
	case REG_B: ww1.br = w & WORDMASK; break;
	case REG_SAM:
		ww1.ac0_carry = ww1.sam = 0;
		if(w == 1)
			ww1.ac0_carry = 1;
		else if(w == 0177776)
			ww1.sam = 1;
		break;
	default:
		// Set toggle switch storage
		if(a < 32)
			ww1.tss[a] = w;
		break;
	}
}

Word
examinereg(int a)
{
	word w;
	switch(a){
	case REG_PC: return ww1.pc;
	case REG_AC: return ww1.ac;
	case REG_A: return ww1.ar;
	case REG_B: return ww1.br;
	case REG_SAM:
		w = 0;
		if(ww1.ac0_carry) w |= 1;
		if(ww1.sam) w |= 0177776;
		return w;
	default:
		// Read toggle switch storage
		if(a < 32)
			return ww1.tss[a];
		break;
	}
	return 0;
}

char*
disasm(Word w)
{
	static char s[100];
	char *p;
	int op, x;
	char *opstr, *sym;;

	op = w>>11 & 037;
	x = w & ADDRMASK;
	p = s;
	memset(s, 0, 100);
	switch(op){
	case 000: opstr = "si"; goto symop;
	case 001: opstr = "rs"; goto symop;
	case 002: opstr = "bi"; goto symop;
	case 003: opstr = "rd"; goto symop;
	case 004: opstr = "bo"; goto symop;
	case 005: opstr = "rc"; goto symop;
	case 006: opstr = "sd"; goto symop;
	case 007: opstr = "cf"; goto symop;
	case 010: opstr = "ts"; goto symop;
	case 011: opstr = "td"; goto symop;
	case 012: opstr = "ta"; goto symop;
	case 013: opstr = "ck"; goto symop;
	case 014: opstr = "ad"; goto symop;
	case 015: opstr = "ex"; goto symop;
	case 016: opstr = "cp"; goto symop;
	case 017: opstr = "sp"; goto symop;
	case 020: opstr = "ca"; goto symop;
	case 021: opstr = "cs"; goto symop;
	case 022: opstr = "ad"; goto symop;
	case 023: opstr = "su"; goto symop;
	case 024: opstr = "cm"; goto symop;
	case 025: opstr = "sa"; goto symop;
	case 026: opstr = "ao"; goto symop;
	case 027: opstr = "dm"; goto symop;
	case 030: opstr = "mr"; goto symop;
	case 031: opstr = "mh"; goto symop;
	case 032: opstr = "dv"; goto symop;
	case 033: opstr = "slr\0slh"; goto sh_rot;
	case 034: opstr = "srr\0srh"; goto sh_rot;
	case 035: opstr = "sf"; goto symop;
	case 036: opstr = "clc\0clh"; goto sh_rot;
	case 037: opstr = "md"; goto symop;

	sh_rot:
		if(x & 01000)
			opstr += 4;
		x &= ~01000;
		p += sprintf(p, "%s %o", opstr, x);
		break;

	symop:
		p += sprintf(p, "%s ", opstr);
		sym = x == 0 ? nil : findsym(x);
		if(sym)
			p += sprintf(p, "%s", sym);
		else
			p += sprintf(p, "%o", x);
		break;
	}
	return s;
}






void
scope_init(WWI *ww, ExUnit *eu)
{
}
// TODO: does this generalize perhaps?
void
scope_stop(WWI *ww, ExUnit *eu)
{
	eu->active = 0;
}
void
scope_record(WWI *ww, ExUnit *eu)
{
	Scope *s = (Scope*)eu;
	if(s->fd >= 0){
		u32 cmd = 0;
		int x = ww->hdefl;
		int y = ww->vdefl;
		// Convert [-1.0,+1.0] to [0,1023]
		if(x & 02000) x++;
		if(y & 02000) y++;
		x += 02000;
		y += 02000;
		x &= 03777;
		y &= 03777;
		x >>= 1;
		y >>= 1;
		cmd = x;
		cmd |= y<<10;
		cmd |= 7<<20;
		if(write(s->fd, &cmd, 4) < 4)
			s->fd = -1;
usleep(100);
	}
	ww->interlock = 0;
	eu->active = 0;
}
void
scope_read(WWI *ww, ExUnit *eu)
{
	// TODO: light gun
}


void
ptape_init(WWI *ww, ExUnit *eu)
{
	Papertape *pt = (Papertape*)eu;
	pt->wbyw = !!(ww->ios & 2);
	pt->p7 = !(ww->ios & 1);

	ww->ioc_counter = pt->wbyw ? 015 : 017;
	pt->dly = 1000;
}
void
ptape_stop(WWI *ww, ExUnit *eu)
{
	eu->active = 0;
}
void
ptape_record(WWI *ww, ExUnit *eu)
{
	Papertape *pt = (Papertape*)eu;
	if(pt->dly){
		pt->dly--;
		return;
	}

	int u = (ww->ios>>3) & 3;
	u8 c = ww->ior>>10 & 077;
	if(pt->p7) c |= 0100;
	write(pt->out_fd[u], &c, 1);

	if(pt->wbyw) ww->ior <<= 5;
	if(ww->ioc_counter++ == 017){
		ww->interlock = 0;
		eu->active = 0;
	}

	pt->dly = 1000;
}
void
ptape_read(WWI *ww, ExUnit *eu)
{
	Papertape *pt = (Papertape*)eu;
	if(pt->dly){
		pt->dly--;
		return;
	}
	int u = (ww->ios>>3) & 3;
	if(eu->active && hasinput(pt->in_fd[u])){
		u8 c;
		read(pt->in_fd[u], &c, 1);
		if(pt->wbyw) ww->ior <<= 5;
		ww->ior |= c & 077;

		if(ww->ioc_counter++ == 017){
			ww->interlock = 0;
			eu->active = 0;
		}

		pt->dly = 1000;
	}
}

void
si_misc(WWI *ww)
{
	if(ww->ios == 0)
		ww->run = 0;
	else if(ww->ios == 1)
		ww->run = 0;
}

void
si_mt(WWI *ww)
{
	// Magtape
}

void
si_pt(WWI *ww)
{
	// Papertape and flexowriter
	ww->active_unit = (ExUnit*)ww->ptape;
	ww->ioc_out = !!(ww->ios & 4);
}

void
si_int_regs(WWI *ww)
{
	// intervention registers
}

void
si_mite(WWI *ww)
{
	// no idea what a MITE buffer is...
}

void
si_ind_lights(WWI *ww)
{
	// later indicator lights
}

void
si_scope(WWI *ww)
{
	// Displays and light guns
	ww->active_unit = (ExUnit*)ww->scope;
	ww->ioc_out = 1;
}

void
si_drum(WWI *ww)
{
}

typedef uint16_t word;

enum {
	// TODO
	SI = 0,
	RS = 1,
	BI = 2,
	RD = 3,
	BO = 4,
	RC = 5,

	SD = 6,
	CF = 7,//TODO

	TS = 8,
	TD = 9,
	TA = 10,
	CK = 11,
	AB = 12,
	EX = 13,
	CP = 14,
	SP = 15,
	CA = 16,
	CS = 17,
	AD = 18,
	SU = 19,
	CM = 20,
	SA = 21,
	AO = 22,
	DM = 23,
	MR = 24,
	MH = 25,
	DV = 26,
	SL = 27,
	SR = 28,
	SF = 29,
	CL = 30,
	MD = 31
};

enum
{
	ALARM_OV = 1,
	ALARM_DV,
	ALARM_CK,
};

#define WD(x) ((x) & 0177777)
#define SGN(x) ((x) & 0100000)
#define INST(o,x) ((o)<<11 | (x)&03777)


static void
pstate(WWI *ww)
{
	printf("%06o: AC/%06o BR/%06o AR/%06o SAM/%o PC/%04o\n",
		ww->prevpc, ww->ac, ww->br, ww->ar, ww->sam, ww->pc);
}

static word
readms(WWI *ww)
{
	// stop TPD to read memory into PAR
	if(MS_SELECT(ww->ss))
		ww->par |= ww->core[ww->ss];
}

static void
writems(WWI *ww)
{
	// stop TPD to write PAR into memory
	if(MS_SELECT(ww->ss))
		ww->core[ww->ss] = ww->par;
}

static word
readstore(WWI *ww)
{
	if(MS_SELECT(ww->ss))
		return ww->par;
	else if(IS_FF(ww->ss))
		return ww->ffs[ww->ss & 037];
	else
		return ww->tss[ww->ss & 037];
}

static void
clearstore(WWI *ww)
{
	ww->par = 0;
	if(!MS_SELECT(ww->ss) && IS_FF(ww->ss))
		ww->ffs[ww->ss & 037] = 0;
}

static void
clearstore_rt(WWI *ww)
{
	ww->par &= ~03777;	// not sure if this is right
	if(!MS_SELECT(ww->ss) && IS_FF(ww->ss))
		ww->ffs[ww->ss & 037] &= ~03777;
}

static void
writestore(WWI *ww, word w)
{
	ww->par |= w;
	if(!MS_SELECT(ww->ss) && IS_FF(ww->ss))
		ww->ffs[ww->ss & 037] |= w;
}

static void
writestore_rt(WWI *ww, word w)
{
	ww->par |= w & 03777;
	if(!MS_SELECT(ww->ss) && IS_FF(ww->ss))
		ww->ffs[ww->ss & 037] |= w & 03777;
}

static void
decode_ios(WWI *ww)
{
	ww->ioc_out = 1;	// or 0?
	ww->active_unit = nil;
	switch(ww->ios>>6 & 7){
	case 0: si_misc(ww); break;
	case 1: si_mt(ww); break;
	case 2: si_pt(ww); break;
	case 3: si_int_regs(ww); break;
	case 4: si_mite(ww); break;
	case 5: si_ind_lights(ww); break;	// earlier camera
	case 6: si_scope(ww); break;
	case 7: si_drum(ww); break;
	}
}

static void
padd(WWI *ww, word a)
{
	// also ac0 carry, which we have to be careful to clear
	ww->cry |= ww->ac & a;
	ww->ac ^= a;
}

static void
carry(WWI *ww)
{
	int s = ww->ac + (ww->cry<<1);
	if(s & 0200000)
		s++;
	// overflow
	ww->ac0_carry = SGN(~ww->ac) && SGN(s^ww->cry);
	ww->ac = s;
	ww->cry = 0;
}

static void
wwalarm(WWI *ww, int a)
{
	static char *strs[] = {
		"NA",
		"OV alarm",
		"DV alarm",
		"CK alarm"
	};
	printf("%s at %o\n", strs[a], ww->prevpc);
	ww->run = 0;
}

static void
arithcheck(WWI *ww)
{
	if(ww->ac0_carry)
		wwalarm(ww, ALARM_OV);
	ww->ac0_carry = 0;
}

static void
end_around_carry(WWI *ww)
{
	// fake
	padd(ww, 1);
	carry(ww);
}

static void
roundoff(WWI *ww)
{
	if(SGN(ww->br))
		end_around_carry(ww);
}

static void
dvshift(WWI *ww)
{
	ww->br = ww->br<<1 | ~ww->ac>>15&1;
	ww->ac = ww->ac<<1 | ww->ac>>15&1;
	ww->ac0_carry = 0;
}

static void
shift_left(WWI *ww)
{
	ww->ac = ww->ac<<1&077776 | ww->br>>15&1;
	ww->br = ww->br<<1;
//	ww->br = ww->br<<1 | ww->br&1;	// this is earlier
}

static void
shift_right(WWI *ww)
{
	ww->br = ww->ac<<15 | ww->br>>1;
	ww->ac = ww->ac>>1;
}

static void
shift_carry(WWI *ww)
{
	// Apply carry once and shift AR:BR right
	ww->br = ww->ac<<15 | ww->br>>1;
	ww->ac >>= 1;
	ww->ac ^= ww->cry;
	ww->cry &= ww->ac ^ ww->cry;
}

static void
cycle_left(WWI *ww)
{
	word ac0 = ww->ac>>15;
	ww->ac = ww->ac<<1 | ww->br>>15&1;
	ww->br = ww->br<<1 | ac0;
}

static void
ac_sign(WWI *ww)
{
	if(SGN(ww->ac)){
		ww->ac = ~ww->ac;
		ww->sign = !ww->sign;
	}
}

static void
ar_sign(WWI *ww)
{
	if(SGN(ww->ar)){
		ww->ar = ~ww->ar;
		ww->sign = !ww->sign;
	}
}

static void
prod_sign(WWI *ww)
{
	if(ww->sign){
		ww->ac = ~ww->ac;
		ww->sign = !ww->sign;
	}
}

static void
special_carry(WWI *ww)
{
	if(ww->ac0_carry) ww->ac |= 1;
	if(ww->sam) ww->ac |= ~1;
	ww->ac0_carry = 0;
	ww->sam = 0;
}

static void
step(WWI *ww)
{
	int div_alarm = 1;

	if(ww->iostop)
	switch(ww->cs){
	case SI: goto si_wait;
	case RC: goto rc_wait;
	case RD: goto rd_wait;
	}

	ww->sign = 0;
	assert(ww->cry == 0);

	// TP1
	ww->ss = 0;
	// TP2
	ww->cr = 0;
	ww->ss |= ww->pc;
	// TP3
	ww->par = 0;
	readms(ww);
	// TP4
	ww->par |= readstore(ww);
	ww->ss = 0;
	ww->cs = 0;
	ww->sc = 037;
	ww->src = 0;
	// TP5
	ww->ss |= ww->par&03777;
	ww->cs |= ww->par>>11;
	ww->sc ^= ww->par&037;
	ww->src |= (ww->par>>9) & 1;

	// TP7 actually
	ww->prevpc = ww->pc;
	ww->pc = ww->pc+1 & 03777;

	switch(ww->cs){
	case SI:
	si_wait:
		// TP6
		if(ww->ioc_out)
			if(ww->interlock){
				ww->iostop = 1;
				return;
			}
		ww->iostop = 0;
		// TP7
		ww->ios = 0;
		ww->scope->eu.stop(ww, (ExUnit*)ww->scope);
		ww->ptape->eu.stop(ww, (ExUnit*)ww->ptape);
		ww->interlock = 0; // ? but has to be done somewhere
		// IOS delay start
		// TP7.5
		ww->ios |= ww->par & 03777;
		ww->ior = 0;
		decode_ios(ww);
		// TP8
		ww->vdefl = 0;
		// TP1
		ww->vdefl |= ww->ac>>5;
		if(ww->ioc_out)
			ww->ior |= ww->ac;
		// IOC reset (si)
		if(ww->active_unit){
			ww->active_unit->init(ww, ww->active_unit);
			if(!ww->ioc_out){
				ww->active_unit->active = 1;
				ww->interlock = 1;
			}
		}
		break;
	case RC:
	rc_wait:
		// TP6
		if(ww->interlock){
			ww->iostop = 1;
			return;
		}
		ww->iostop = 0;
		ww->par = 0;
		readms(ww);
		// TP7.5
		ww->ior = 0;
		// TP8
		ww->hdefl = 0;
		if((ww->ios & 0700) == 0600)
			ww->ior |= readstore(ww);
		// TP8.5
		ww->par = 0;
		// TP1
		ww->par |= ww->ac;	// and count
		if((ww->ios & 0700) != 0600)
			ww->ior |= ww->ac;
		ww->hdefl |= ww->ac>>5;
		// TP2
		// sense BC#3
		// TP3
		// IOC reset (rc)
		// initiate record
		if(ww->active_unit){
			ww->active_unit->init(ww, ww->active_unit);
			if(ww->ioc_out){
				ww->active_unit->active = 1;
				ww->interlock = 1;
				
			}
		}
		break;
	case RD:
	rd_wait:
		// TP6
		if(ww->interlock){
			ww->iostop = 1;
			return;
		}
		ww->iostop = 0;
		ww->ar = 0;
		// TP6.5
		ww->par = 0;
		ww->ar |= ww->ior;
		ww->par |= ww->ior;	// and count
		// TP 7.5
		ww->ior = 0;
		// TP8
		// TP1
		ww->ac = 0;
		// TP2
		padd(ww, ww->ar);
		// IOC reset (rd)
		if(ww->active_unit){
			ww->active_unit->init(ww, ww->active_unit);
			if(!ww->ioc_out){
				ww->active_unit->active = 1;
				ww->interlock = 1;
			}
		}
		// sense BC#3
		break;

	case CK:
		// TP6
		ww->ar = 0;
		ww->ac0_carry = ww->sam = 0;
		ww->par = 0;
		readms(ww);
		// TP7
		ww->cr ^= readstore(ww);
		// TP8
		ww->cr ^= ww->ac;
		// TP1
		// TODO: PC+1 in SPECIAL mode
		if(ww->cr)
			wwalarm(ww, ALARM_CK);
		break;

	case CA:
		// TP6
		ww->ar = 0;
		ww->ac = 0;
		ww->src = 0;
		ww->br = 0;
		ww->par = 0;
		readms(ww);
		// TP7
		ww->ar |= readstore(ww);
		special_carry(ww);
		// TP2
		padd(ww, ww->ar);
		// TP3
		carry(ww);
		// TP4
		arithcheck(ww);
		break;
	case CS:
		// TP6
		ww->ar = 0;
		ww->ac = 0;
		ww->src = 0;
		ww->br = 0;
		ww->par = 0;
		readms(ww);
		// TP7
		ww->ar |= readstore(ww);
		special_carry(ww);
		// TP2
		padd(ww, ~ww->ar);
		// TP3
		carry(ww);
		// TP4
		arithcheck(ww);
		break;
	case CM:
		// TP6
		ww->ar = 0;
		ww->ac = 0;
		ww->src = 0;
		ww->br = 0;
		ww->par = 0;
		readms(ww);
		// TP7
		ww->ar |= readstore(ww);
		special_carry(ww);
		// TP1
		ar_sign(ww);
		// TP2
		padd(ww, ww->ar);
		// TP3
		carry(ww);
		// TP4
		arithcheck(ww);
		break;

	case SA:
		// TP6
		ww->ar = 0;
		ww->ac0_carry = ww->sam = 0;
		ww->par = 0;
		readms(ww);
		// TP7
		ww->ar |= readstore(ww);
		// TP2
		padd(ww, ww->ar);
		// TP3
		carry(ww);
		// TP4
		if(ww->ac0_carry){
			if(!SGN(ww->ac)){
				ww->ac0_carry = 0;
				ww->sam = 1;
			}
			ww->ac ^= 0100000;
		}
		break;

	case AD:
		// TP6
		ww->ar = 0;
		ww->ac0_carry = ww->sam = 0;
		ww->par = 0;
		readms(ww);
		// TP7
		ww->ar |= readstore(ww);
		// TP2
		padd(ww, ww->ar);
		// TP3
		carry(ww);
		// TP4
		arithcheck(ww);
		break;
	case SU:
		// TP6
		ww->ar = 0;
		ww->ac0_carry = ww->sam = 0;
		ww->par = 0;
		readms(ww);
		// TP7
		ww->ar |= readstore(ww);
		// TP2
		padd(ww, ~ww->ar);
		// TP3
		carry(ww);
		// TP4
		arithcheck(ww);
		break;
	case DM:
		// TP6
		ww->ar = 0;
		ww->ac0_carry = ww->sam = 0;
		ww->br = 0;
		ww->par = 0;
		readms(ww);
		// TP7
		ww->ar |= readstore(ww);
		ww->br |= ww->ac;
		// TP8
		ac_sign(ww);	// hm, usually TP6, but that doesn't work
		// TP1
		ar_sign(ww);
		// TP2
		padd(ww, ~ww->ar);
		// TP3
		carry(ww);
		assert((ww->sam|ww->ac0_carry) == 0);
		break;
	case AB:
		// TP6
		ww->ar = 0;
		ww->ac = 0;
		ww->ac0_carry = ww->sam = 0;
		ww->par = 0;
		readms(ww);
		// TP7
		ww->ar |= readstore(ww);
		ww->ac |= ww->br;
		padd(ww, ww->ar);	// probably delayed
		// TP8
		carry(ww);
		clearstore(ww);
		// TP1
		writestore(ww, ww->ac);
		writems(ww);
		// TP4
		arithcheck(ww);
		break;
	case AO:
		// TP6
		ww->ar = 0;
		ww->ac = 0;
		ww->ac0_carry = ww->sam = 0;
		ww->par = 0;
		readms(ww);
		// TP7
		ww->ar |= readstore(ww);
		padd(ww, ww->ar);	// delayed
		// TP8
		end_around_carry(ww);
		clearstore(ww);
		// TP1
		writestore(ww, ww->ac);
		writems(ww);
		// TP4
		arithcheck(ww);
		break;

	case TS:
		// TP8
		clearstore(ww);
		// TP1
		writestore(ww, ww->ac);
		writems(ww);
		break;
	case TA:
		// TP6
		ww->par = 0;
		readms(ww);
		// TP8
		clearstore_rt(ww);
		// TP1
		writestore_rt(ww, ww->ar);
		writems(ww);
		break;
	case TD:
		// TP6
		ww->par = 0;
		readms(ww);
		// TP8
		clearstore_rt(ww);
		// TP1
		writestore_rt(ww, ww->ac);
		writems(ww);
		break;

	case EX:
		// TP6
		ww->ar = 0;
		ww->par = 0;
		readms(ww);
		// TP7
		ww->ar |= readstore(ww);
		// TP8
		clearstore(ww);
		// TP1
		writestore(ww, ww->ac);
		ww->ac = 0;		// does this work?
		writems(ww);
		// TP2
		padd(ww, ww->ar);
		break;

	case SD:
		// TP6
		ww->ar = 0;
		ww->ac0_carry = ww->sam = 0;
		ww->par = 0;
		readms(ww);
		// TP7
		ww->ar |= readstore(ww);
		// TP2
		padd(ww, ww->ar);
		// TP4
		ww->cry = 0;	// rather uncertain
		break;
	case MD:
		// TP6
		ww->ar = 0;
		ww->ac0_carry = ww->sam = 0;
		ww->par = 0;
		readms(ww);
		// TP7
		ww->ar |= readstore(ww);
		// TP8
		ww->ac = ~ww->ac;
		ww->ar = ~ww->ar;
		// TP1
		ww->ar |= ww->ac;
		ww->ac = 0;	// rather uncertain
		// TP2
		padd(ww, ~ww->ar);
		break;

	case SP:
		// TP6
		ww->ar = 0;
	sp7:	// TP7 - PC+1 happens here
		// TP8
		ww->ar |= ww->pc;
		ww->pc = 0;	// delayed
		// TP1
		ww->pc = ww->par & 03777;
		break;
	case CP:
		// TP6
		ww->ar = 0;
		// TP7
		if(SGN(ww->ac))
			goto sp7;
		break;

	case SR:
		// TP6
		ww->ac0_carry = ww->sam = 0;
		ac_sign(ww);
		// TP7
		ww->sc++;
		// TP2
		if(!(ww->sc++ & 040)) for(;;){
			shift_right(ww);
			if(ww->sc++ & 040) break;
		}
		// TP3
		if(!ww->src) roundoff(ww);
		// TP4
		if(!ww->src) ww->br = 0;
		prod_sign(ww);
		arithcheck(ww);
		break;

	case SL:
		// TP6
		ww->ac0_carry = ww->sam = 0;
		ac_sign(ww);
		// TP7
		ww->sc++;
		// TP2
		if(!(ww->sc++ & 040)) for(;;){
			shift_left(ww);
			if(ww->sc++ & 040) break;
		}
		// TP3
		if(!ww->src) roundoff(ww);
		// TP4
		if(!ww->src) ww->br = 0;
		prod_sign(ww);
		arithcheck(ww);
		break;
	case CL:
		// TP6
		//ww->ac0_carry = ww->sam = 0;	// probably not
		// TP7
		ww->sc++;
		// TP2
		if(!(ww->sc++ & 040)) for(;;){
			cycle_left(ww);
			if(ww->sc++ & 040) break;
		}
		// TP4
		if(!ww->src) ww->br = 0;
		break;

	case SF:
		// TP6
		ww->ar = 0;
		ww->ac0_carry = ww->sam = 0;
		ww->sc = 0;
		ac_sign(ww);
		ww->par = 0;
		readms(ww);
		// TP7(?) --- was TP6 in 1950, but no ES read there
		if(!(ww->ac & 040000)) for(;;){
			cycle_left(ww);
			if(ww->sc++ & 040) break;
			if(ww->ac & 040000) break;
		}
		// TP8
		clearstore_rt(ww);
		// TP1
		ww->ar |= ww->sc;
		writestore_rt(ww, ww->sc);
		writems(ww);
		// TP4
		prod_sign(ww);
		break;

	case MH:
	case MR:
		// TP6
		ww->ar = 0;
		ww->br = 0;
		ww->ac0_carry = ww->sam = 0;
		ww->par = 0;
		ac_sign(ww);
		readms(ww);
		// TP7
		ww->ar |= readstore(ww);
		ww->br |= ww->ac;
		// TP1
		ar_sign(ww);
		ww->ac = 0;
		ww->src = ww->cs == MH;
		// TP2
		ww->sc = 022;
		for(;;){
			if(ww->br & 1){
				padd(ww, ww->ar);
				ww->br &= ~1;
			}else{
				shift_carry(ww);
				if(ww->sc++ & 040) break;
			}
		}
		// TP3
		carry(ww);
		if(!ww->src) roundoff(ww);
		// TP4
		if(!ww->src) ww->br = 0;
		prod_sign(ww);
		assert(ww->ac0_carry == 0);
		assert(ww->sam == 0);
		break;

	case DV:
		// TP6
		ww->ar = 0;
		ww->br = 0;
		ww->ac0_carry = ww->sam = 0;
		ww->par = 0;
		ac_sign(ww);
		readms(ww);
		// TP7
		ww->ar |= readstore(ww);
		// TP1
		ar_sign(ww);
		// TP2
		padd(ww, ~ww->ar);
		ww->sc = 020;
		for(;;){
			// first divide pulse
			carry(ww);

			// second divide pulse
			dvshift(ww);
			// shift out of AC0 triggers this
			if(ww->br & 1){
				padd(ww, ~ww->ar);
				if(div_alarm){
					wwalarm(ww, ALARM_DV);
					return;
				}
			}else{
				padd(ww, ww->ar);
				div_alarm = 0;
			}
			if(ww->sc++ & 040) break;
		}
		ww->ac = 0;
		ww->ac0_carry = 0;
		ww->cry = 0;
		prod_sign(ww);
		break;

	case RS:
	case BI:
	case BO:
	default:
		printf("INVALID %d\n", ww->cs);
	}
//	pstate(ww);
}

void
readmem(WWI *ww, FILE *f)
{
	char buf[100], *s;
	word a, w;

	a = 0;
	while(s = fgets(buf, 100, f)){
		while(*s){
			if(*s == ';')
				break;
			else if('0' <= *s && *s <= '7'){
				w = strtol(s, &s, 8);
				if(*s == ':'){
					a = w;
					s++;
				}else if(a < 04000)
					ww->core[a++] = w;
			}else
				s++;
		}
	}
}

void
readsyms(FILE *f)
{
	char buf[100], *s;
	Word w;

	while(s = fgets(buf, 100, f)){
		while(isspace(*s)) s++;
		while(!isspace(*s)) s++;
		*s++ = '\0';
		w = strtol(s, &s, 8);
		defsym(buf, w);
	}
}

void
printmem(WWI *ww)
{
	word a;
	for(a = 0; a < 04000; a++)
		if(ww->core[a])
			printf("%04o/%06o\n", a, ww->core[a]);
}



enum {
	CMD_NONE,
	CMD_SET_PC,
	CMD_STEP,
	CMD_RUN,
	CMD_STOP
};

typedef struct Msg Msg;
struct Msg
{
	int cmd;
	Word arg;
};
Channel *cmdchan;
Msg msg_none;

static void
start_over(WWI *ww)
{
}

void
handlemsg(WWI *ww, Msg msg)
{
	switch(msg.cmd){
	case CMD_NONE:
		break;
	case CMD_SET_PC:
		ww->pc_sw = msg.arg & ADDRMASK;
		ww->pc = ww->pc_sw;
		break;
	case CMD_STEP:
		ww->order_by_order = 1;
		ww->run = 1;
		break;
	case CMD_RUN:
		ww->order_by_order = 0;
		ww->run = 1;
		break;
	case CMD_STOP:
		ww->run = 0;
		break;
	}
}

void
handleio(WWI *ww)
{
	if(ww->active_unit && ww->active_unit->active){
		if(ww->ioc_out)
			ww->active_unit->record(ww, ww->active_unit);
		else
			ww->active_unit->read(ww, ww->active_unit);
	}

	if(ww->scope->fd >= 0 && hasinput(ww->scope->fd)){
		u32 input;
		if(readn(ww->scope->fd, &input, 4) < 0){
			ww->scope->fd = -1;
			return;
		}
		if(input & 0x80000000){
			// light pen/gun
			ww->scope->gunx = input & 01777;
			ww->scope->guny = (input>>10) & 01777;
			ww->scope->gun = (input>>20) & 1;
		}else{
			int id = input>>24;
			// we'll use FF register 2 for the input bits
			if(id == 0){
				ww->ffs[2] &= ~0140000;	// need upper 2 bits
				if(input & 0200) ww->ffs[2] |= 0100000;	// up
				if(input & 0100) ww->ffs[2] |= 0040000;	// down
			}else{
				ww->ffs[2] &= ~0030000;	// need next 2 bits
				if(input & 0200) ww->ffs[2] |= 0020000;	// up
				if(input & 0100) ww->ffs[2] |= 0010000;	// down
			}
		}
	}
}

void*
simthread(void *arg)
{
	WWI *ww = (WWI*)arg;
	Msg msg;
	for(;;){
		switch(channbrecv(cmdchan, &msg)){
		case -1:	// channel closed
			return nil;
		case 0:	break;	// nothing
		case 1:
			handlemsg(ww, msg);
			break;
		}

		if(ww->run){
			step(ww);
			if(ww->order_by_order)
				ww->run = 0;
		}else
			usleep(1000);

		handleio(ww);
	}
	return nil;
}

void*
consthread(void *arg)
{
	WWI *ww = (WWI*)arg;
	FILE *f;
	f = fdopen(ww->console_fd, "w");

	for(;;){
		fprintf(f, "\033[1J\033[H");	// clear
		fprintf(f, "CS/%02o\n", ww->cs);
		fprintf(f, "PC/%04o\n", ww->pc);
		fprintf(f, "PR/%06o\n", ww->par);
		fprintf(f, "AC/%06o\n", ww->ac);
		fprintf(f, "AR/%06o\n", ww->ar);
		fprintf(f, "BR/%06o\n", ww->br);
		usleep(50000);
	}
}



void
cpu_start(Addr a)
{
	Msg msg;
	msg.cmd = CMD_SET_PC;
	msg.arg = a;
	chansend(cmdchan, &msg);
	msg.cmd = CMD_RUN;
	chansend(cmdchan, &msg);
	chansend(cmdchan, &msg_none);
}

void cpu_readin(Addr a);

void
cpu_setpc(Addr pc)
{
	Msg msg;
	msg.cmd = CMD_SET_PC;
	msg.arg = pc;
	chansend(cmdchan, &msg);
	chansend(cmdchan, &msg_none);
}

void cpu_stopinst(void)
{
	Msg msg;
	msg.cmd = CMD_STOP;
	chansend(cmdchan, &msg);
	chansend(cmdchan, &msg_none);
}

void cpu_stopmem(void)
{
}

void cpu_cont(void);

void
cpu_nextinst(void)
{
	Msg msg;
	msg.cmd = CMD_STEP;
	chansend(cmdchan, &msg);
	chansend(cmdchan, &msg_none);
}

void
cpu_nextmem(void)
{
}

void cpu_exec(Word inst);
void cpu_ioreset(void);
void cpu_printflags(void);


void
initmach(void)
{
	int i;

	memset(&ww1, 0, sizeof(ww1));
	// FF storage at 2,3,4,5,7
	ww1.ff_regs = (1<<2) | (1<<3) | (1<<4) | (1<<5) | (1<<6);
	ww1.scope = &scope;
	ww1.ptape = &ptape;

	for(i = 0; i < 4; i++){
		ptape.out_fd[i] = -1;
		ptape.in_fd[i] = -1;
	}
	ptape.out_fd[0] = open("ptp", O_RDWR | O_CREAT | O_TRUNC, 0664);
	ptape.in_fd[0] = open("ptr", O_RDONLY);
	ptape.in_fd[1] = open("petr", O_RDONLY);
	ptape.out_fd[2] = open("/tmp/fl", O_RDWR);
	ptape.eu.read = ptape_read;
	ptape.eu.record = ptape_record;
	ptape.eu.init = ptape_init;
	ptape.eu.stop = ptape_stop;

	scope.eu.read = scope_read;
	scope.eu.record = scope_record;
	scope.eu.init = scope_init;
	scope.eu.stop = scope_stop;

	ww1.pc = 040;

	FILE *mf;
	mf = mustopen("out.mem", "r");
	if(mf){
		readmem(&ww1, mf);
		fclose(mf);
	}
	mf = fopen("out.sym", "r");
	if(mf){
		readsyms(mf);
		fclose(mf);
	}


/*
	// shift test
	ww1.ac = 01234;
	ww1.core[0100] = 0155003;
*/
/*
	// multiplication test
	ww1.ac = 077777;
	ww1.core[01000] = 077777;
	ww1.core[0100] = 0145000;
*/
/*
	// division test
	ww1.ac = 036000;
	ww1.core[01000] = 060000;
	ww1.core[0100] = 0151000;
	ww1.core[0101] = 0154017;
*/
/*
	// sf test
	ww1.ac = 052000 >> 2;
	ww1.core[01000] = 0177777;
	ww1.core[0100] = 0165000;
*/


	scope.fd = dial("localhost", 3400);


/*
	// hardcoded? nice!
	ww1.console_fd = open("/dev/pts/19", O_RDWR);
	if(ww1.console_fd >= 0)
		threadcreate(consthread, &ww1);
*/

	cmdchan = chancreate(sizeof(Msg), 1);
	threadcreate(simthread, &ww1);
}

void
deinitmach(void)
{
}
