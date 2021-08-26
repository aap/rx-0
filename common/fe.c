#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <setjmp.h>
#include <stdarg.h>

#include "fe.h"

struct termios tiosaved;
int canreset;
char erasec, killc, intrc;
jmp_buf errjmp;

void
panic(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	quit();
}

FILE*
mustopen(const char *name, const char *mode)
{
	FILE *f;
	if(f = fopen(name, mode), f == nil)
		panic("couldn't open file: %s", name);
	return f;
}

int
raw(int fd)
{
	struct termios tio;
	if(tcgetattr(fd, &tio) < 0) return -1;
	tiosaved = tio;
	cfmakeraw(&tio);
//	tio.c_iflag |= ICRNL;
	tio.c_oflag |= OPOST | ONLCR;
	if(tcsetattr(fd, TCSAFLUSH, &tio) < 0) return -1;
	canreset = 1;
	return 0;
}

int
reset(int fd)
{
	if(canreset && tcsetattr(0, TCSAFLUSH, &tiosaved) < 0) return -1;
	return 0;
}

#define ALT 033

char
tyi(void)
{
	char c;
	read(0, &c, 1);
	return c;
}

#define CTL(c) ((c) & 037)

void
tyo_(char c)
{
	write(1, &c, 1);
}

void
tyo(char c)
{
	static char *alt = "◊";
	static char *ctrl = "↑";

	if(c < 040){
		switch(c){
		case ALT:
			write(1, alt, strlen(alt));
			return;
		case CTL('J'):
		case CTL('M'):
		case CTL('H'):
		case CTL('I'):
			write(1, &c, 1);
			break;
		default:
			write(1, ctrl, strlen(ctrl));
			c += 0100;
			write(1, &c, 1);
		}
	}else
		write(1, &c, 1);
}

enum {
	BUF_ERR = ~0,	// dangerous if word isn't int!!!
	BUF_EMPTY = ~0-1,

	/* typeout modes */
	MODE_NONE = 0,
	MODE_NUM,
	MODE_SYM,
	MODE_ASCII,
	MODE_SIXBIT,
	MODE_SQUOZE,

	/* Symbol flags */
	SYM_HALFKILL = 1,	// don't use for typeout

	/* flags */
	CF = 1,		// one altmode
	CCF = 2,	// two altmodes

	/* line modes */
	LM_COLON = 1,
	LM_LOAD,
	LM_DUMP,
	LM_MOUNT,
	LM_UNMOUNT
};

#define BUFLEN 200

char buf[BUFLEN+1];
char *bufstart = buf;
int nbuf;

int flags;
char ch;
int base = 8;
Addr dot;
Addr addr;
int opened;
int regopened;
Word q;
Word number;
int hasnum;
int permmode = MODE_SYM;
int curmode;
struct Assembler
{
	Word exp;
	int op;
	int gotexp;
};
struct Assembler as;
struct Assembler astack[8];
int asp;
Addr starta;

void
typenum(Word n)
{
	Word d;
	d = n % base;
	n /= base;
	if(n)
		typenum(n);
	tyo(d + '0');
}

void
typestr(char *str)
{
	while(*str)
		tyo(*str++);
}

void
typestrnl(char *str)
{
	while(*str)
		tyo(*str++);
}

void
typestrnl_(char *str)
{
	while(*str)
		tyo_(*str++);
}

void
err(char *str)
{
	typestr(str);
	longjmp(errjmp, 0);
}

void
num(void)
{
	number = number*base + ch-'0';
	hasnum = 1;
}

void
ins(void)
{
	buf[nbuf++] = ch;
	nbuf %= BUFLEN;
	buf[nbuf] = '\0';
}

Word
parsenum(void)
{
	Word w;
	char *c;

	w = 0;
	for(c = bufstart; c < &buf[nbuf]; c++){
		if(*c >= '0' && *c <= '9')
			w = w*base + *c-'0';
		else
			return BUF_ERR;
		w &= WORDMASK;
	}
	return w;
}

typedef struct Symbol Symbol;
struct Symbol
{
	char sym[12];
	int flags;
	Word value;
};

Symbol symtab[SYMTABSIZE] = {
#include "syms.inc"
};

void
defsym(char *sym, Word val)
{
	int i;
	for(i = 0; i < SYMTABSIZE; i++)
		if(symtab[i].sym[0] == '\0')
			goto found;
	err("?SYMTAB? ");
found:
	strncpy(symtab[i].sym, sym, 12);
	symtab[i].sym[11] = '\0';
	symtab[i].value = val;
}

char*
findsym(Word val)
{
	int i;
	for(i = 0; i < SYMTABSIZE; i++)
		if(symtab[i].sym[0] &&
		   !(symtab[i].flags & SYM_HALFKILL) &&
		   symtab[i].value == val)
			return symtab[i].sym;
	return nil;
}

Word
parsesym(void)
{
	Word i;

	if(strcmp(bufstart, ".") == 0)
		return dot;

	for(i = 0; i < SYMTABSIZE; i++)
		if(symtab[i].sym[0] && strcasecmp(bufstart, symtab[i].sym) == 0)
			return symtab[i].value;
	return BUF_ERR;
}

/* parse input buffer, error if can't parse
 * return 0 if empty */
int
parse(Word *t)
{
	Word w;

	w = parsenum();
	if(bufstart >= &buf[nbuf])
		return 0;
	if((w = parsenum()) == BUF_ERR && (w = parsesym()) == BUF_ERR)
		err("?U? ");
	nbuf = 0;
	*t = w;
	return 1;
}

void
resetexp(void)
{
	as.exp = 0;
	as.op = 0;
	as.gotexp = 0;
}

/* combine word t into current expression */
void
combine(Word t)
{
	switch(as.op){
	case '+':
	default:
		as.exp += t;
		break;
	case '-':
		as.exp += ~t;
		break;
	case '*':
		as.exp *= t;
		break;
	case '|':
		as.exp /= t;
		break;
	}
	as.gotexp = 1;
	as.op = 0;
}

/* assemble storage word and set *w
 * if no word, don't change *w and return 0 */
int
endword(Word *w)
{
	Word t;

	if(!parse(&t)){
		if(!as.gotexp)
			return 0;
		t = 0;
	}
	combine(t);
	t = as.exp;
	t &= WORDMASK;
	*w = t;
	resetexp();
	return 1;
}


void
prword(int mode, Word wd)
{
	switch(mode){
	case MODE_SYM:
		typestr(disasm(wd));
		break;

	default:
		typenum(wd);
	}
}

char*
symaddr(Addr a)
{
	static char s[100];
	char *p, *sym;
	p = s;
	memset(s, 0, 100);
	sym = a == 0 ? nil : findsym(a);
	if(sym)
		p += sprintf(p, "%s", sym);
	else
		p += sprintf(p, "%o", a);
	return s;
}

void
praddr(int mode, Addr a)
{
	switch(mode){
	case MODE_SYM:
		typestr(symaddr(a));
		break;

	default:
		typenum(a);
	}
}

void
typeout(int mode)
{
	endword(&q);
	prword(mode, q);
	typestr("   ");
}

void
propen(Addr a)
{
	praddr(curmode, a);
	tyo('/');
}

void
aclose(int ins)
{
	// this is a bit ugly...
	// we don't want ^M to set Q if no location is open
	Word t;
	t = q;
	if(endword(&q) && ins){
		if(opened)
			deposit(addr, q);
		else if(regopened)
			depositreg(addr, q);
	}
	addr = q & ADDRMASK;
	if(!opened && !regopened)
		q = t;
	opened = 0;
	regopened = 0;
}

void
aopen(int mode)
{
	opened = 1;
	q = examine(addr);
	typestr("   ");
	if(mode != MODE_NONE)
		typeout(mode);
}

void
aropen(int mode)
{
	regopened = 1;
	q = examinereg(addr);
	typestr("   ");
	if(mode != MODE_NONE)
		typeout(mode);
}

void
zerocore(void)
{
	Addr a;
	for(a = 0; a < MAXMEM; a++)
		deposit(a, 0);
	putchar('\n');
}

void
quit(void)
{
	reset(0);
	putchar('\n');

	deinitmach();
	exit(0);
}

int started;

int
threadmain(int argc, char *argv[])
{
	char chu;
	Word t;

	initmach();

	raw(0);
	erasec = tiosaved.c_cc[VERASE];
	killc = tiosaved.c_cc[VKILL];
	intrc = tiosaved.c_cc[VINTR];

	setjmp(errjmp);
	nbuf = 0;
	opened = 0;
	regopened = 0;
	resetexp();
	flags = 0;
	curmode = permmode;
	number = 0;
	hasnum = 0;
//	linemode = 0;

	for(;;){
		if(hasinput(0)){
			ch = tyi();
		}else{
			if(started && (!isrunning() || isstopped())){
				t = examinereg(REG_PC);
				/* TODO: maybe do something different on stop? */
				typestrnl("\n");
				typenum(t);
				if(!isrunning())
					typestr(">>");
				else
					typestr("<<");
				t = examine(t);
				prword(MODE_SYM, t);
				typestr("   ");

				/* show E */
				t = t&ADDRMASK;
				dot = t;
				praddr(MODE_SYM, dot);
				typestr("/   ");
				t = examine(t);
				prword(MODE_SYM, t);
				q = t;
				typestr("    ");

				started = 0;
			}
			usleep(1000);
			continue;
		}
		chu = toupper(ch);
		if(ch == erasec || ch == CTL('H') || ch == CTL('?')){
			/* can't backspace in all cases */
			if((flags & CF) || nbuf <= 0)
				err("\n");
			typestr("\b \b");
			nbuf--;
		}else if(ch == killc)
			err("");
		else if(ch == intrc)
			break;
		else{
			tyo(ch);

			if((flags & CF) == 0 &&
			   (ch >= 'A' && ch <= 'Z' ||
			    ch >= 'a' && ch <= 'z')){
				ins();
			}else switch(chu){
			case CTL('D'):
				err("XXX? ");
				break;

			case '.': case '%': case '$':
				ins();
				break;

			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
			case '8': case '9':
				if(flags & CF){
					num();
					goto keepflags;
				}
				ins();
				break;

			case 'G':
				if(!endword(&t))
					t = starta;
				if(flags & CCF)
					starta = t & ADDRMASK;
				if(hasnum && number == 0)
					cpu_setpc(t);
				else
					cpu_start(t);
				started = 1;
				break;
			case CTL('N'):
				endword(&t);
				if(flags & CF)
					cpu_nextmem();
				else
					cpu_nextinst();
				started = 1;
				break;
			case CTL('Z'):
				nbuf = 0;
				if(flags & CF)
					cpu_stopmem();
				else
					cpu_stopinst();
				break;

			case 'Q':
				combine(q);
				break;

			case 'Z':
				if(flags & CCF)
					zerocore();
				else
					err(" ?? ");
				break;

			case ALT:
				if(flags & CF)
					flags |= CCF;
				flags |= CF;
				goto keepflags;

			case ':':
				defsym(bufstart, as.gotexp ? as.exp : dot);
				nbuf = 0;
				typestr("   ");
				resetexp();
				break;

			/* expressions */
			case ' ':
			case '+':
			case '-':
			case '*':
			case '|':
				if(!parse(&t)){
					if(ch == '-')
						as.op = '-';
					break;
				}
				combine(t);
				as.op = ch;
				break;

			/* opening */
			case '/':
				if(flags & CF){
					aclose(0);
					aropen(MODE_NUM);
				}else{
					aclose(0);
					dot = addr;
					aopen(curmode);
				}
				break;
			case '\\':
				aclose(0);
				aopen(curmode);
				break;
			case '[':
				aclose(0);
				dot = addr;
				aopen(MODE_NUM);
				break;
			case ']':
				aclose(0);
				dot = addr;
				aopen(MODE_SYM);
				break;
			case '^':
				typestrnl("\n");
				aclose(1);
				dot = (dot-1) & ADDRMASK;
				addr = dot;
				propen(addr);
				aopen(curmode);
				break;
			case CTL('J'):
				aclose(1);
				dot = (dot+1) & ADDRMASK;
				addr = dot;
				propen(addr);
				aopen(curmode);
				break;

			case CTL('M'):
				tyo('\n');
				aclose(1);
				curmode = permmode;
				break;

			/* typeout */
			case '_':
				typeout(MODE_SYM);
				break;
			case '=':
				typeout(MODE_NUM);
				break;

			default:
				err(" ?? ");
				break;
			}
		}
		flags = 0;
		number = 0;
		hasnum = 0;
keepflags:;
	}

	quit();

	return 0;
}
