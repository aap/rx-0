#include "../common/fe.h"

typedef struct PDP1 PDP1;

struct PDP1
{
	Word ac;
	Word io;
	Word mb;
	Word ma;
	Word pc;
	Word ir;
	Word core[MAXMEM];

	Word ta;
	Word tw;

	int start_sw;
	int stop_sw;
	int continue_sw;
	int examine_sw;
	int deposit_sw;

	int run, run_enable;
	int cyc;
	int df1, df2;
	int bc1, bc2;
	int ov1, ov2;
	int rim;
	int sbm;
	int ioc;
	int ihs;
	int ios;
	int ioh;

	int ss;
	int pf;

	int single_cyc_sw;
	int single_inst_sw;

	int cychack;	// for cycle entry past TP0
};


PDP1 pdp1;

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
	case REG_SS: pdp1.ss = w & 077; break;
	case REG_PF: pdp1.pf = w & 077; break;
	}
}

Word
examinereg(int a)
{
	switch(a){
	case REG_PC: return pdp1.pc;
	case REG_AC: return pdp1.ac;
	case REG_IO: return pdp1.io;
	case REG_SS: return pdp1.ss;
	case REG_PF: return pdp1.pf;
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

#define IR pdp->ir
#define PC pdp->pc
#define MA pdp->ma
#define MB pdp->mb
#define AC pdp->ac
#define IO pdp->io

// 0
#define IR_AND (IR == 001)
#define IR_IOR (IR == 002)
#define IR_XOR (IR == 003)
#define IR_XCT (IR == 004)
// 5
// 6
#define IR_CALJDA (IR == 007)
#define IR_LAC (IR == 010)
#define IR_LIO (IR == 011)
#define IR_DAC (IR == 012)
#define IR_DAP (IR == 013)
#define IR_DIP (IR == 014)
#define IR_DIO (IR == 015)
#define IR_DZM (IR == 016)
// 17
#define IR_ADD (IR == 020)
#define IR_SUB (IR == 021)
#define IR_IDX (IR == 022)
#define IR_ISP (IR == 023)
#define IR_SAD (IR == 024)
#define IR_SAS (IR == 025)
#define IR_MUS (IR == 026)
#define IR_DIS (IR == 027)
#define IR_JMP (IR == 030)
#define IR_JSP (IR == 031)
#define IR_SKIP (IR == 032)
#define IR_SHRO (IR == 033)
#define IR_LAW (IR == 034)
#define IR_IOT (IR == 035)
// 36
#define IR_OPR (IR == 037)
#define IR_INCORR (IR==0 || IR==5 || IR==6 || IR==017 || IR==036)

#define B0 0400000
#define B1 0200000
#define B2 0100000
#define B3 0040000
#define B4 0020000
#define B5 0010000
#define B6 0004000
#define B7 0002000
#define B8 0001000
#define B9 0000400
#define B10 0000200
#define B11 0000100
#define B12 0000040
#define B13 0000020
#define B14 0000010
#define B15 0000004
#define B16 0000002
#define B17 0000001

static void
sc(PDP1 *pdp)
{
	pdp->df1 = 0;
	pdp->df2 = 0;
	pdp->ov1 = 0;
	pdp->ov2 = 0;
	pdp->ihs = 0;
	pdp->ios = 0;
	pdp->ioh = 0;
	pdp->ir = 0;
	pdp->pc = 0;
	pdp->ioc = 1;
	pdp->sbm = 0;
}

static void
spec(PDP1 *pdp)
{
	// PB
	pdp->run = 0;
	if(pdp->start_sw || pdp->deposit_sw || pdp->examine_sw)
		pdp->rim = 0;

	// SP1
	pdp->run_enable = 1;
	pdp->ma = 0;
	if(pdp->start_sw)
		pdp->cyc = 0;
	if(pdp->deposit_sw)
		pdp->ac = 0;
	if(pdp->start_sw || pdp->deposit_sw || pdp->examine_sw)
		sc(pdp);

	// SP2
	if(pdp->start_sw || pdp->deposit_sw || pdp->examine_sw)
		pdp->pc |= pdp->ta;
	if(pdp->deposit_sw || pdp->examine_sw || pdp->rim)
		pdp->cyc = 1;
	if(pdp->examine_sw)
		pdp->ir |= 020>>1;
	if(pdp->deposit_sw) {
		pdp->ir |= 024>>1;
		pdp->ac |= pdp->tw;
	}

	// SP3
	if(pdp->deposit_sw || pdp->examine_sw) {
		pdp->ma |= pdp->pc;
		pdp->cychack = 1;
	}

	// SP4
	if(pdp->start_sw || pdp->continue_sw)
		pdp->run = 1;

	pdp->start_sw = 0;
	pdp->stop_sw = 0;
	pdp->continue_sw = 0;
	pdp->examine_sw = 0;
	pdp->deposit_sw = 0;
}

static int
decflg(int n)
{
	switch(n&7) {
	case 1: return 040;
	case 2: return 020;
	case 3: return 010;
	case 4: return 004;
	case 5: return 002;
	case 6: return 001;
	case 7: return 077;
	}
	return 0;
}

static void
shro(PDP1 *pdp)
{
	int ac = AC;
	int io = IO;
	switch((MB>>9) & 017) {
	case 001:	// RAL
		ac = (AC&~B0)<<1 | (AC&B0)>>17;
		break;
	case 002:	// RIL
		io = (IO&~B0)<<1 | (IO&B0)>>17;
		break;
	case 003:	// RCL
		ac = (AC&~B0)<<1 | (IO&B0)>>17;
		io = (IO&~B0)<<1 | (AC&B0)>>17;
		break;
	case 005:	// SAL
		ac = (AC&B0) | (AC&~(B0|B1))<<1 | (AC&B17);
		break;
	case 006:	// SIL
		io = (IO&B0) | (IO&~(B0|B1))<<1 | (IO&B17);
		break;
	case 007:	// SCL
		ac = (AC&B0) | (AC&~(B0|B1))<<1 | (IO&B0)>>17;
		io = (IO&~B0)<<1 | (IO&B17);
		break;
	case 011:	// RAR
		ac = (AC&B17)<<17 | AC>>1;
		break;
	case 012:	// RIR
		io = (IO&B17)<<17 | IO>>1;
		break;
	case 013:	// RCR
		ac = (IO&B17)<<17 | AC>>1;
		io = (AC&B17)<<17 | IO>>1;
		break;
	case 015:	// SAR
		ac = (AC&B0) | AC>>1;
		break;
	case 016:	// SIR
		io = (IO&B0) | IO>>1;
		break;
	case 017:	// SCR
		ac = (AC&B0) | AC>>1;
		io = (AC&B17)<<17 | IO>>1;
		break;
	}
	AC = ac;
	IO = io;
}

static void
cycle0(PDP1 *pdp)
{
	switch(pdp->cychack) {
	default:
	// TP0
	if(IR_SHRO && (MB & B12)) shro(pdp);
	MA |= PC;

	// TP1
	if(IR_SHRO && (MB & B11)) shro(pdp);
	// emc = 0

	// TP2
	if(IR_SHRO && (MB & B10)) shro(pdp);
	PC = (PC+1) & ADDRMASK;
	if(IR_IOT) {
		if(!pdp->ioh && !pdp->ihs) {
			pdp->ioc = 1;
			pdp->ios = 0;
		} else {
			pdp->ioc = 0;
		}
	}
	pdp->ihs = 0;

	// TP3
	if(IR_SHRO && (MB & B9)) shro(pdp);
	MB = 0;

	case 4:
	// TP4
	MB |= pdp->core[MA];
	pdp->core[MA] = 0;
	IR = 0;

	// TP5
	IR |= MB>>13;

	// TP6
	if((MB & B5) && !IR_SHRO && !IR_SKIP &&
	                !IR_LAW && !IR_OPR && !IR_IOT && !IR_CALJDA)
		pdp->df1 = 1;

	// TP6a
	if(IR_IOT && !(MB & B5) && pdp->ioh) {
		pdp->ioc = 1;
		pdp->ihs = 1;
		pdp->ioh = 0;
	}

	// TP7
	if(IR_SHRO && (MB & B17)) shro(pdp);
	if(IR_JSP && !pdp->df1 || IR_LAW || IR_OPR && (MB & B10)) AC = 0;
	if(IR_OPR && (MB & B6)) IO = 0;
	if(IR_IOT) {
		if((MB & B5) && !pdp->ioh && !pdp->ihs) pdp->ioh = 1;
// TODO: IOT pulse 7
	}

	// TP8
	if(!pdp->df1) {
		if(IR_JSP) AC |= PC, PC = 0;
		if(IR_JMP) PC = 0;
	}
	if(IR_SKIP) {
		int skip = 0;
		if((MB & B7) && !(IO&B0)) skip = 1;
		if((MB & B8) && !pdp->ov1) skip = 1;
		if((MB & B9) && (AC&B0)) skip = 1;
		if((MB & B10) && !(AC&B0)) skip = 1;
		if((MB & B11) && AC==0) skip = 1;
		if((MB&070) && !(pdp->ss&decflg(MB>>3))) skip = 1;
		if((MB&007) && !(pdp->pf&decflg(MB))) skip = 1;
		if(MB & B5) skip = !skip;
		if(skip) PC = (PC+1) & ADDRMASK;
	}
	if(IR_SHRO && (MB & B16)) shro(pdp);
	if(IR_LAW) AC |= MB & 0007777;
	if(IR_OPR) {
		if(MB & B7) AC |= pdp->tw;
		if(MB & B11) AC |= PC;
		if(MB & B14) pdp->pf |= decflg(MB);
		else pdp->pf &= ~decflg(MB);
	}

	// TP9
	pdp->core[MA] = MB;	// approximate
	if(!pdp->df1 && (IR_JMP || IR_JSP)) PC |= MB & ADDRMASK;
	if(IR_SKIP && (MB & B8)) pdp->ov1 = 0;
	if(IR_SHRO && (MB & B15)) shro(pdp);
	if(IR_OPR && (MB & B8) || IR_LAW && (MB & B5)) AC ^= 0777777;
	if(IR_IOT && !pdp->ihs && pdp->ios) pdp->ioh = 0;
	if(IR_OPR && (MB & B9) ||
	   IR_INCORR ||
	   pdp->single_cyc_sw ||
	   pdp->single_inst_sw && !pdp->df1 && pdp->ir >= 030 ||
	   !pdp->run_enable)
		pdp->run = 0;

	// TP9A
	if(IR_SHRO && (MB & B14)) shro(pdp);
	if(IR_IOT && pdp->ioh) PC = (PC-1) & ADDRMASK, pdp->df1 = pdp->df2 = 0;

	// TP10
	if(pdp->run) MA = 0;
	if(pdp->df1 || pdp->ir < 030) pdp->cyc = 1;
	if(IR_SHRO && (MB & B13)) shro(pdp);
	if(IR_IOT) {
		if(pdp->ihs) pdp->ioh = 1;
		else if(!pdp->ioh) pdp->ios = 0;
// TODO: IOT pulse 10
	}
	}

	pdp->cychack = 0;
}

static void
defer(PDP1 *pdp)
{
	// TP0
	MA |= MB & ADDRMASK;

	// TP3
	MB = 0;

	// TP4
	MB |= pdp->core[MA];
	pdp->core[MA] = 0;

	if(MB & B5) {
		// TP6
		pdp->df2 = 1;

		// TP7
		if(IR_JSP) AC = 0;

		// TP8
		if(IR_JSP) AC |= PC, PC = 0;
		if(IR_JMP) PC = 0;

		// TP9
		if(IR_JSP || IR_JMP) PC |= MB & ADDRMASK;
	}
	pdp->core[MA] = MB;	// approximate
	if(IR_INCORR ||
	   pdp->single_cyc_sw ||
	   pdp->single_inst_sw && !pdp->df2 && pdp->ir >= 030 ||
	   !pdp->run_enable)
		pdp->run = 0;

	// TP10
	if(pdp->run) MA = 0;
	if(!pdp->df2) {
		pdp->df1 = 0;
		if(pdp->ir >= 030) pdp->cyc = 0;
	}
	pdp->df2 = 0;
}

static void
cycle1(PDP1 *pdp)
{
	switch(pdp->cychack) {
	default:
	// TP0
	if(IR_CALJDA && !(MB & B5))
		MA |= 0100;
	else
		MA |= MB & ADDRMASK;
	// EMA stuff
	if(IR_DIS) {
		int ac = (AC&~B0)<<1 | (IO&B0)>>17;
		IO = (IO&~B0)<<1 | (~AC&B0)>>17;
		AC = ac;
	}

	case 1:
	// TP1
	// emc = 0

	// TP2
	if(IR_DIS & !(IO & B17)) {
		if(AC == 0777777) AC = 1;
		else AC++;
	}

	// TP3
	MB = 0;
	if(IR_XCT) {
		pdp->cyc = 0;
		pdp->cychack = 4;
		return;
	}

	// TP4
	MB |= pdp->core[MA];
	pdp->core[MA] = 0;
	if(IR_SUB || IR_DIS && (IO & B17)) AC ^= 0777777;
	if(IR_LIO) IO = 0;

	// TP5
	if(IR_AND) AC &= MB;
	if(IR_IOR) AC |= MB;
	if(IR_IDX || IR_ISP || IR_LAC) AC = MB;
	if(IR_DIO || IR_DZM) MB = 0;
	if(IR_LIO) IO |= MB;
	if((IR_ADD || IR_SUB) && (AC&B0) == (MB&B0)) pdp->ov2 = 1;
	if(IR_XOR || IR_ADD || IR_SUB || IR_SAD || IR_SAS ||
	   IR_DIS || IR_MUS && (IO & B17))
		AC ^= MB;

	// TP6
	if(IR_ADD || IR_SUB || IR_DIS || IR_MUS && (IO & B17)) {
		AC += (~AC & MB)<<1;
		if(AC & 01000000) AC++;
		AC &= 0777777;
	}
	if(IR_IDX || IR_ISP) {
		if(AC == 0777776) AC = 0;
		else if(AC == 0777777) AC = 1;
		else AC++;
	}

	// TP7
	if(IR_DAC || IR_IDX || IR_ISP) MB = AC;
	if(IR_CALJDA) MB = AC, AC = 0;
	if(IR_DAP) MB = MB&0770000 | AC&0007777;
	if(IR_DIP) MB = MB&0007777 | AC&0770000;
	if(IR_DIO) MB = IO;

	// TP8
	if(IR_MUS) {
		int ac = AC>>1;
		IO = (AC&B17)<<17 | IO>>1;
		AC = ac;
	}
	if(IR_CALJDA) AC |= PC, PC = 0;
	if(IR_SAS && AC==0 || IR_SAD && AC!=0 ||
	   IR_ISP && !(AC & B0))
		PC = (PC+1) & ADDRMASK;

	// TP9
	pdp->core[MA] = MB;	// approximate
	if(IR_CALJDA) PC |= MA;
	if(IR_SUB) AC ^= 0777777;
	if(IR_SAD || IR_SAS) AC ^= MB;
	if((IR_ADD || IR_SUB) && (AC&B0) == (MB&B0)) pdp->ov2 = 0;
	if(IR_INCORR ||
	   pdp->single_cyc_sw ||
	   pdp->single_inst_sw ||
	   !pdp->run_enable)
		pdp->run = 0;

	// TP9A
	if((IR_ADD || IR_DIS) && AC == 0777777) AC = 0;
	if(IR_CALJDA) PC = (PC+1) & ADDRMASK;

	// TP10
	if(pdp->ov2) pdp->ov1 = 1;
	pdp->ov2 = 0;
	pdp->cyc = 0;
	if(pdp->run) MA = 0;
	}
}

enum {
	CMD_NONE,
	CMD_SET_PC,
	CMD_STEP,
	CMD_STEP_INST,
	CMD_RUN,
	CMD_CONT,
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

void
handlemsg(PDP1 *pdp, Msg msg)
{
	switch(msg.cmd){
	case CMD_NONE:
		break;
	case CMD_SET_PC:
		pdp->pc = msg.arg & ADDRMASK;
		break;
	case CMD_STEP:
		pdp->single_inst_sw = 0;
		pdp->single_cyc_sw = 1;
		pdp->continue_sw = 1;
		spec(pdp);
		break;
	case CMD_STEP_INST:
		pdp->single_inst_sw = 1;
		pdp->single_cyc_sw = 0;
		pdp->continue_sw = 1;
		spec(pdp);
		break;
	case CMD_RUN:
		pdp->ta = pdp->pc;	// a bit dumb
		pdp->single_inst_sw = 0;
		pdp->single_cyc_sw = 0;
		pdp->start_sw = 1;
		spec(pdp);
		break;
	case CMD_CONT:
		pdp->single_inst_sw = 0;
		pdp->single_cyc_sw = 0;
		pdp->continue_sw = 1;
		spec(pdp);
		break;
	case CMD_STOP:
		pdp->run_enable = 0;
		break;
	}
}

void*
simthread(void *arg)
{
	PDP1 *pdp = (PDP1*)arg;
	Msg msg;
	for(;;){
		switch(channbrecv(cmdchan, &msg)){
		case -1:	// channel closed
			return nil;
		case 0: break;  // nothing
		case 1:
			handlemsg(pdp, msg);
			break;
		}

		// a cycle takes 5μs
		if(pdp->run) {
			!pdp->cyc ? cycle0(pdp) :
			pdp->df1 ? defer(pdp)
				: cycle1(pdp);
//printf("%s %s  hack/%d\n", pdp->cyc ? "cyc" : "   ", pdp->df1 ? "df1" : "   ", pdp->cychack);
		}
//		handleio(pdp);
	}
	return nil;
}



static void
dly(void)
{
	// force a few cycles to happen
	chansend(cmdchan, &msg_none);
	chansend(cmdchan, &msg_none);
	chansend(cmdchan, &msg_none);
	chansend(cmdchan, &msg_none);
	chansend(cmdchan, &msg_none);
	chansend(cmdchan, &msg_none);
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
// TODO
	Msg msg;
	msg.cmd = CMD_STOP;
	chansend(cmdchan, &msg);
	chansend(cmdchan, &msg_none);
}

void cpu_stopmem(void)
{
	Msg msg;
	msg.cmd = CMD_STOP;
	chansend(cmdchan, &msg);
	chansend(cmdchan, &msg_none);
}

void cpu_cont(void)
{
	Msg msg;
	msg.cmd = CMD_CONT;
	chansend(cmdchan, &msg);
	chansend(cmdchan, &msg_none);
}

void
cpu_nextinst(void)
{
	Msg msg;
	msg.cmd = CMD_STEP_INST;
	chansend(cmdchan, &msg);
	dly();
	while(pdp1.run)
		dly();
}

void
cpu_nextmem(void)
{
	Msg msg;
	msg.cmd = CMD_STEP;
	chansend(cmdchan, &msg);
	dly();
}

void cpu_exec(Word inst);
void cpu_ioreset(void);
void cpu_printflags(void);


void
initmach(void)
{
	memset(&pdp1, 0, sizeof(pdp1));
	pdp1.ioc = 1;

	int i = 0100;
	pdp1.core[i++] = 0760400;

	cmdchan = chancreate(sizeof(Msg), 1);
	threadcreate(simthread, &pdp1);
}

void
deinitmach(void)
{
}

