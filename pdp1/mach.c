#include "../common/fe.h"

typedef struct PDP1 PDP1;
typedef struct Flexowriter Flexowriter;

struct PDP1
{
	int run;
	Word ac;
	Word io;
	Word mb;
	Word ma;
	Word pc;
	Word ir;
	Word core[MAXMEM];

	Word tbr;
	Word tac;

	Flexowriter *fl;
};

struct Flexowriter
{
	int in, out;
};

PDP1 pdp1;
Flexowriter fl;

int isrunning(void) { return pdp1.run; }
int isstopped(void) { return 0; }

void
deposit(Addr a, Word w)
{
	pdp1.core[a] = w;
}

Word
examine(Addr a)
{
	return pdp1.core[a];
}

void
depositreg(int a, Word w)
{
	switch(a){
	case REG_PC: pdp1.pc = w & ADDRMASK; break;
	case REG_AC: pdp1.ac = w & WORDMASK; break;
	case REG_IO: pdp1.io = w & WORDMASK; break;
	}
}

Word
examinereg(int a)
{
	switch(a){
	case REG_PC: return pdp1.pc;
	case REG_AC: return pdp1.ac;
	case REG_IO: return pdp1.io;
	}
	return 0;
}

char*
disasm(Word w)
{
	static char s[100];
	char *p;
	int op, y;
	char *opstr, *sym;;

	op = w>>12 & 076;
	y = w & ADDRMASK;
	p = s;
	memset(s, 0, 100);
	switch(op){
	case 000:
	case 012: // jfd
	case 014:
	case 036:
	case 074:
		// no instruction
		sym = findsym(w);
		if(sym)
			p += sprintf(p, "%s", sym);
		else
			p += sprintf(p, "%o", w);
		break;
	case 002: opstr = "and"; goto memop;
	case 004: opstr = "ior"; goto memop;
	case 006: opstr = "xor"; goto memop;
	case 010: opstr = "xct"; goto memop;
	case 016: opstr = w & 010000 ? "jda" : "cal"; w &= ~010000; goto memop;
	case 020: opstr = "lac"; goto memop;
	case 022: opstr = "lio"; goto memop;
	case 024: opstr = "dac"; goto memop;
	case 026: opstr = "dap"; goto memop;
	case 030: opstr = "dip"; goto memop;
	case 032: opstr = "dio"; goto memop;
	case 034: opstr = "dzm"; goto memop;
	case 040: opstr = "add"; goto memop;
	case 042: opstr = "sub"; goto memop;
	case 044: opstr = "idx"; goto memop;
	case 046: opstr = "isp"; goto memop;
	case 050: opstr = "sad"; goto memop;
	case 052: opstr = "sas"; goto memop;
	case 054: opstr = "mus"; goto memop;
	case 056: opstr = "dis"; goto memop;
	case 060: opstr = "jmp"; goto memop;
	case 062: opstr = "jsp"; goto memop;
	case 064:	// skip
		if(y == 02000)
			p += sprintf(p, "spi ");
		else if(y == 01000)
			p += sprintf(p, "szo ");
		else if(y == 0400)
			p += sprintf(p, "sma ");
		else if(y == 0200)
			p += sprintf(p, "spa ");
		else if(y == 0100)
			p += sprintf(p, "sza ");
		else if((y&07770) == 0 && (y&7))
			p += sprintf(p, "szf %o ", y&7);
		else if((y&07707) == 0 && (y&070))
			p += sprintf(p, "szs %o ", y&070);
		else
			p += sprintf(p, "skp %o ", y);
		if(w & 010000)
			p += sprintf(p, "i ");
		break;
	case 066:	// shift
		p += sprintf(p, "%c", w&04000 ? 's' : 'r');
		p += sprintf(p, "%c", "xaic"[y>>9 & 3]);
		p += sprintf(p, "%c ", w&010000 ? 'r' : 'l');
		p += sprintf(p, "%o", y & 0777);
		break;
	case 070: opstr = "law";
	memop:
		p += sprintf(p, "%s ", opstr);
		if(w & 010000)
			p += sprintf(p, "i ");
		sym = findsym(y);
		if(sym)
			p += sprintf(p, "%s", sym);
		else
			p += sprintf(p, "%o", y);
		break;
	case 072:	// iot
		p += sprintf(p, "iot %o", w &017777);
		break;
	case 076:	// opr
		if(y == 04000)
			p += sprintf(p, "cli ");
		else if(y == 02200)
			p += sprintf(p, "lat ");
		else if(y == 01000)
			p += sprintf(p, "cma ");
		else if(y == 0400)
			p += sprintf(p, "hlt ");
		else if(y == 00200)
			p += sprintf(p, "cla ");
		else if(y == 00300)
			p += sprintf(p, "lap ");
		else if((y&07770) == 0 && (y&7))
			p += sprintf(p, "clf %o ", y&7);
		else if((y&07770) == 00010 && (y&7))
			p += sprintf(p, "stf %o ", y&7);
		else if(y == 0)
			p += sprintf(p, "nop ");
		else
			p += sprintf(p, "opr %o ", y);
		if(w & 010000)
			p += sprintf(p, "i ");
		break;
	}
	return s;
}

void
cpu_start(Addr a)
{
}

void cpu_readin(Addr a);

void
cpu_setpc(Addr pc)
{
}

void cpu_stopinst(void)
{
}

void cpu_stopmem(void)
{
}

void cpu_cont(void);

void
cpu_nextinst(void)
{
}

void
cpu_nextmem(void)
{
	// nothing
}

void cpu_exec(Word inst);
void cpu_ioreset(void);
void cpu_printflags(void);


void
initmach(void)
{
}

void
deinitmach(void)
{
}

