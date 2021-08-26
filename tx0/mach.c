#include "../common/fe.h"

#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct TX0 TX0;
typedef struct Flexowriter Flexowriter;

/*
 * PETR
 *  1,2: counter
 *  3: has data (flag)
 *  4: busy(?)
 * FLEXO
 *  punch
 *  print
 *  read
 * display
 * light pen
 */

struct TX0
{
	// Start-stop; 0 idle; 1 running
	int ss;
	// IO-stop; 0 stopping; 1 continue
	int ios;
	// Push button synch; 0 halt; 1 continue
	int pbs;
	// Cycle: 0 1
	int c;
	Word ac;
	Word lr;
	Word mbr;
	Word mar;
	Word pc, prevpc;
	Word ir;
	Word core[MAXMEM];

	Word tbr;
	Word tac;
	// Stop after cycle 0/1
	int sw_stop_c0;
	int sw_stop_c1;

	Flexowriter *fl;
	int fl_printing;
	/* Scope */
	int displaying;
	int scope_fd;
};

struct Flexowriter
{
	int in, out;
	int dly;	// speed limit the thing a bit. just a hack
};

TX0 tx0;
Flexowriter fl;

int isrunning(void) { return tx0.pbs; }
int isstopped(void) { return 0; }

void
deposit(Addr a, Word w)
{
	tx0.core[a] = w;
}

Word
examine(Addr a)
{
	return tx0.core[a];
}

void
depositreg(int a, Word w)
{
	switch(a){
	case REG_PC: tx0.pc = w & ADDRMASK; break;
	case REG_AC: tx0.ac = w & WORDMASK; break;
	case REG_LR: tx0.lr = w & WORDMASK; break;
	}
}

Word
examinereg(int a)
{
	switch(a){
	case REG_PC: return tx0.pc;
	case REG_AC: return tx0.ac;
	case REG_LR: return tx0.lr;
	}
	return 0;
}

static struct {
	char *sym;
	Word value;
} oprgroup[] = {
	{ "cll", 0700000 },
	{ "clr", 0640000 },
	{ "pen", 0600100 },
//	{ "tac", 0600004 },
	{ "tac", 0740004 },
	{ "com", 0600040 },
	{ "amb", 0600001 },
//	{ "tbr", 0600003 },
	{ "tbr", 0740023 },
	{ "mbl", 0600200 },
	{ "lmb", 0600002 },
	{ "shr", 0600400 },
	{ "pad", 0600420 },
	{ "cyr", 0600600 },
	{ "cry", 0600010 },
	{ "cla", 0740000 },
	{ "clc", 0740040 },
	{ "lac", 0740022 },
	{ "lcc", 0740062 },
	{ "lpd", 0600022 },
	{ "lad", 0600032 },
	{ "lcd", 0600072 },
	{ "cal", 0740200 },
	{ "lro", 0600200 },
	{ "alr", 0600201 },
	{ "alo", 0600221 },
	{ "alc", 0600261 },
	{ "cyl", 0600031 },
	{ "ios", 0760000 },
	{ "r1c", 0761000 },
	{ "r1r", 0761600 },
	{ "r1l", 0761031 },
	{ "dis", 0622000 },
	{ "r3c", 0763000 },
	{ "prt", 0624000 },
	{ "pnt", 0624600 },
	{ "pna", 0624021 },
	{ "pnc", 0624061 },
	{ "p6s", 0766000 },
	{ "p6h", 0626600 },
	{ "p7h", 0627600 },
	{ "p6a", 0626021 },
	{ "p7a", 0627021 },
	{ "hlt", 0630000 },
	{ nil, 0 }
};

char*
disasm(Word w)
{
	static char s[100];
	char *p;
	int op, x;
	char *opstr, *sym;;

	op = w>>16 & 3;
	x = w & ADDRMASK;
	p = s;
	memset(s, 0, 100);
	switch(op){
	case 00: opstr = "sto"; goto symop;
	case 01: opstr = "add"; goto symop;
	case 02: opstr = "trn"; goto symop;
	case 03: opstr = "opr";
		for(op = 0; oprgroup[op].sym; op++)
			if(oprgroup[op].value == w){
				p += sprintf(p, "%s", oprgroup[op].sym);
				return s;
			}
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
putfl(Flexowriter *fl, int c)
{
	char cc;
	cc = c & 077;
	write(fl->out, &cc, 1);
}
int
getfl(Flexowriter *fl)
{
	char c;
	read(fl->in, &c, 1);
	return c;
}

static Word
readcore(TX0 *tx)
{
	tx->mbr = tx->core[tx->mar] & 0777777;
	return tx->mbr;
}

static void
writecore(TX0 *tx)
{
	tx->core[tx->mar] = tx->mbr & 0777777;
}

static void
iostop(TX0 *tx)
{
	tx->ss = 0;
	tx->ios = 0;
}

static void
iorestart(TX0 *tx)
{
	tx->ios = 1;
}

static void
cycle(TX0 *tx)
{
	Word mbr_t;

	if(tx->ss){
		// execute instructions
		tx->mbr = 0;
		if(tx->c == 0){
//printf("cyc0\n"); fflush(stdout);
			tx->prevpc = tx->mar;
			readcore(tx);
			tx->pc = 0;
			tx->ir = tx->mbr>>16;
			tx->pc = tx->mar+1 & ADDRMASK;
			tx->mar = tx->mbr & ADDRMASK;

			switch(tx->ir){
			case 0:
			case 1:
				tx->c = 1;
				break;
			case 2:
				if((tx->ac & 0400000) == 0)
					tx->c = 1;
				break;
			case 3:
				// opr
				// 0.8
				if(tx->mar & 0100000) tx->ac &= 0000777;
				if(tx->mar & 0040000) tx->ac &= 0777000;
				if((tx->mar & 0030000) == 0020000) iostop(tx);
				switch(tx->mar>>9 & 7){
				case 0: break;	// no IO
				case 1: break;	// PETR 1 line
				case 2:		// display
					tx->displaying = 1;
					break;
				case 3: break;	// PETR 3 lines
				case 4:		// FLEXO print
					tx->fl_printing = 1;
					break;
				case 5: break;	// spare
				case 6: break;	// punch
				case 7: break;	// punch + 7th hole
				}
				tx->c = 1;
				break;
			}

			if(tx->sw_stop_c0)
				tx->pbs = 0;
		}else{
//printf("cyc1\n"); fflush(stdout);
			switch(tx->ir){
			case 0:
				// sto
				tx->mbr = tx->ac;
				writecore(tx);
				break;
			case 1:
				// add
				readcore(tx);
				tx->ac ^= tx->mbr;
				tx->ac += (~tx->ac & tx->mbr)<<1;
				if(tx->ac & 01000000) tx->ac += 1;
				tx->ac &= 0777777;
				break;
			case 2:
				// trn - branch not taken
				break;
			case 3:
				// opr
				// 1.1
				if((tx->mar & 0104) == 0004) tx->ac |= tx->tac;
				if((tx->mar & 0104) == 0100) ; // TODO: pen
				// 1.2
				if(tx->mar & 1) tx->mbr |= tx->mar & 2 ? tx->tbr : tx->ac;
				if(tx->mar & 040) tx->ac = ~tx->ac & 0777777;
				// 1.3
				mbr_t = tx->mbr;
				if((tx->mar & 3) == 2) tx->mbr = tx->lr;
				if((tx->mar & 0600) == 0200) tx->lr = mbr_t;
				// 1.4
				if(tx->mar & 020) tx->ac ^= tx->mbr;
				else if((tx->mar & 0600) == 0400)
					tx->ac = tx->ac>>1 | tx->ac&0400000;
				else if((tx->mar & 0600) == 0600)
					tx->ac = (tx->ac>>1 | tx->ac<<17) & 0777777;
				// 1.6
				if((tx->mar & 0030000) == 0030000)
					tx->pbs = 0;	// halt
				// 1.7
				if(tx->mar & 010){
					tx->ac += (~tx->ac & tx->mbr)<<1;
					if(tx->ac & 01000000) tx->ac += 1;
					tx->ac &= 0777777;
				}
				break;
			}
			tx->mar = tx->pc;
			tx->c = 0;

			if(tx->sw_stop_c1)
				tx->pbs = 0;
		}

		// stop machine
		if(tx->pbs == 0)
			tx->ss = 0;
		
	}else{
//printf("idle\n"); fflush(stdout);
//		usleep(1000);
		// idle
		if(tx->ios == 0){
			if(tx->pbs == 0)
				tx->ss = 0;
		}else
			tx->ss = tx->pbs;
	}
}

void
handleio(TX0 *tx)
{
	int c;

	if(tx->fl->dly == 0){
		if(hasinput(tx->fl->in)){
			c = getfl(tx->fl);
			tx->lr = 0400000;
			tx->lr |= (c&077) << 11;
		}
		// don't wanna poll this all the time
		// around 16cps max
		tx->fl->dly = 10000;
	}else
		tx->fl->dly--;
	if(tx->fl_printing){
		c = 0;
		if(tx->ac & 0000001) c |= 1;
		if(tx->ac & 0000010) c |= 2;
		if(tx->ac & 0000100) c |= 4;
		if(tx->ac & 0001000) c |= 010;
		if(tx->ac & 0010000) c |= 020;
		if(tx->ac & 0100000) c |= 040;
		putfl(tx->fl, c);
		tx->fl_printing = 0;
		iorestart(tx);
	}
	if(tx->displaying){
		if(tx->scope_fd >= 0){
			u32 cmd = 0;
			int x = (tx->ac>>9)&0777;
			int y = tx->ac&0777;
			// Convert [-1.0,+1.0] to [0,1023]
			if(x & 0400) x++;
			if(y & 0400) y++;
			x += 0400;
			y += 0400;
			x &= 0777;
			y &= 0777;
			x <<= 1;
			y <<= 1;
			cmd = x;
			cmd |= y<<10;
			cmd |= 7<<20;
			write(tx->scope_fd, &cmd, 4);
//	usleep(100);
		}
		tx->displaying = 0;
		iorestart(tx);
	}
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

void
handlemsg(TX0 *tx, Msg msg)
{
	switch(msg.cmd){
	case CMD_NONE:
		break;
	case CMD_SET_PC:
		tx->mar = tx->pc = msg.arg & ADDRMASK;
		break;
	case CMD_STEP:
		tx->sw_stop_c0 = 1;
		tx->sw_stop_c1 = 1;
		tx->pbs = 1;
		break;
	case CMD_RUN:
		tx->sw_stop_c0 = 0;
		tx->sw_stop_c1 = 0;
		tx->pbs = 1;
		break;
	case CMD_STOP:
		tx->pbs = 0;
		break;
	}
}

void*
simthread(void *arg)
{
	TX0 *tx = (TX0*)arg;
	Msg msg;
	for(;;){
		switch(channbrecv(cmdchan, &msg)){
		case -1:	// channel closed
			return nil;
		case 0:	break;	// nothing
		case 1:
			handlemsg(tx, msg);
			break;
		}

		// a cycle takes around 6μs
		cycle(tx);
		handleio(tx);
	}
	return nil;
}

void
readmem(TX0 *tx, FILE *f)
{
	char buf[100], *s;
	Word a, w;

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
				}else if(a < MAXMEM)
					tx->core[a++] = w & 0777777;
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
	dly();
}

void cpu_readin(Addr a);

void
cpu_setpc(Addr pc)
{
	Msg msg;
	msg.cmd = CMD_SET_PC;
	msg.arg = pc;
	chansend(cmdchan, &msg);
}

void cpu_stopinst(void)
{
	Msg msg;
	msg.cmd = CMD_STOP;
	chansend(cmdchan, &msg);
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
	dly();
	while(tx0.c){
		chansend(cmdchan, &msg);
		dly();
	}
	// make sure we see the correct PC
	tx0.pc = tx0.mar;
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
	memset(&fl, 0, sizeof(fl));
	fl.in = open("/tmp/fl", O_RDWR);
	if(fl.in < 0)
		panic("can't open /tmp/fl");
	fl.out = fl.in;

	memset(&tx0, 0, sizeof(tx0));
	tx0.pbs = 0;
	tx0.ios = 1;
	tx0.fl = &fl;
	tx0.mar = tx0.pc = 040;

	FILE *mf;
	mf = mustopen("out.mem", "r");
	if(mf){
		readmem(&tx0, mf);
		fclose(mf);
	}
	mf = fopen("out.sym", "r");
	if(mf){
		readsyms(mf);
		fclose(mf);
	}

	tx0.scope_fd = dial("localhost", 3400);

	cmdchan = chancreate(sizeof(Msg), 1);
	threadcreate(simthread, &tx0);
}

void
deinitmach(void)
{
}
