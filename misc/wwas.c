#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

typedef uint16_t word;
#define nil NULL

enum
{
	None = 0,
	W = 0177777,
	SYMLEN = 6,
	MAXSYM = 4000,
};


enum SymType
{
	SymValue = 1,
	SymPseudo,

	SymLabel = 010
};

enum TokType
{
	TokEol = 0200,
	TokSymbol,
	TokWord
};

typedef struct Token Token;
typedef struct Sym Sym;

struct Token
{
	int type;
	union {
		Sym *sym;
		word w;
	};
};

struct Sym
{
	char name[SYMLEN];
	int type;
	union {
		word val;
		void (*f)(void);
	};
};

FILE *infp, *outfp, *lstfp, *tmpfp;
Sym symtab[MAXSYM];
Sym *dot;
word startaddr;
int radix = 8;
char *filename;
int lineno = 1;
int pass2;
int lastlistline = ~0;
char line[80];
int peekc;
Token peekt;

word memory[04000];
word litdot;

void
err(int n, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "%s:%d: ", filename, lineno);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);

	if(n)
		exit(1);
}

void
panic(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(1);
}

FILE*
mustopen(const char *name, const char *mode)
{
	FILE *f;
	if(f = fopen(name, mode), f == nil)
		panic("couldn't open file: %s", name);
	return f;
}

void*
mustmalloc(size_t size)
{
	void *p;
	p = malloc(size);
	if(p == nil)
		panic("out of memory");
	return p;
}

void
list(char *left)
{
	if(left == nil)
		left = "          \t";
	if(line[0])
		fprintf(lstfp, "%s%05d    %s", left, lastlistline, line);
	else
		fprintf(lstfp, "%s\n", left);
	line[0] = '\0';
}

int
cmpsym(const void *s1, const void *s2)
{
	return strcmp(((Sym*)s1)->name, ((Sym*)s2)->name);
}

void
listsyms(void)
{
	Sym *s;

	qsort(symtab, MAXSYM, sizeof(Sym), cmpsym);
	fprintf(lstfp, "\n    SYMBOL TABLE\n");
	for(s = symtab; s < &symtab[MAXSYM]; s++){
		if((s->type&7) != SymValue)
			continue;
		fprintf(lstfp, "%-07s %06o\n", s->name, s->val);
	}
}

int
ch(void)
{
	int c;

tail:
	if(peekc){
		c = peekc;
		peekc = 0;
		return c;
	}

	/* listing */
	if(pass2 && lastlistline != lineno){
		/* if line hasn't been printed yet, do it now */
		if(line[0])
			list(nil);

		int pos = ftell(infp);
		char *p;
		for(p = line; p < &line[80-2]; p++){
			c = getc(infp);
			if(c == '\n' || c == EOF)
				break;
			*p = c;
		}
		if(!(p == line && c == EOF))
			*p++ = '\n';
		*p++ = '\0';
		fseek(infp, pos, SEEK_SET);
		lastlistline = lineno;
	}

	c = getc(infp);
	if(c == EOF)
		return EOF;
	if(c == '\n')
		lineno++;
	// We only want ASCII!!
	c &= 0177;
	if(tmpfp)
		putc(c, tmpfp);

	return c;
}

int
peek(void)
{
	peekc = ch();
	return peekc;
}

// like ch() but skip whitespace except newlines	
int
chsp(void)
{
	int c;
	while(c = ch(),
		c == ' ' || c == '\t' || c == '\r' || c == '\v');
	return c;
}

int
peeksp(void)
{
	peekc = chsp();
	return peekc;
}

/* Symbols */

/* Just find symbol, or return nil. */
Sym*
findsym(char *name)
{
	Sym *s;
	for(s = symtab; s < &symtab[MAXSYM]; s++)
		if(strncmp(s->name, name, SYMLEN) == 0)
			return s;
	return nil;
}

/* Get symbol, add to table if not present */
Sym*
getsym(char *name)
{
	Sym *s;
	s = findsym(name);
	if(s == nil){
		for(s = symtab; s < &symtab[MAXSYM]; s++)
			if(s->name[0] == 0)
				goto found;
		panic("symbol table full");
found:
		strncpy(s->name, name, SYMLEN);
		s->type = None;
		s->val = 0;
	}
	return s;
}

/* Parser */

int
ispunc(char c)
{
	static char *punct = " \t\n\r+-!,=;$*./&\"()[]<>";
	return strchr(punct, c) != nil;
}

Token
number(void)
{
	int c;
	Token t;

	t.type = TokWord;
	t.w = 0;
	while(isdigit(c = ch())){
		if(c-'0' >= radix)
			err(0, "warning: invalid digit in current base");
		t.w = t.w*radix + c-'0';
	}
	peekc = c;
	if(c && !ispunc(c)){
		err(0, "warning: ignored characters after number");
		while(c = ch(), c != EOF && !ispunc(c));
		peekc = c;
	}
	return t;
}

int
issymchar(int c)
{
	return isalnum(c) || c == '_' || c == '.';
}

Token
symbol(void)
{
	char symbuf[SYMLEN+1];
	int c;
	Token t;
	int len;

	len = 0;
	memset(symbuf, 0, sizeof(symbuf));
	while(c = ch(), issymchar(c))
		if(len<SYMLEN)
			symbuf[len++] = tolower(c);
	peekc = c;
	t.type = TokSymbol;
	t.sym = getsym(symbuf);
	return t;
}

Token
token(void)
{
	static char *self = "+-!,=$*&()[]<>";
	int c;
	Token t;

tail:
	if(peekt.type != None){
		t = peekt;
		peekt.type = None;
		return t;
	}

	while(c = chsp(), c != EOF){
		if(isdigit(c)){
			peekc = c;
			return number();
		}else if(isalpha(c)){
			peekc = c;
			t = symbol();
			return t;
		}else if(strchr(self, c)){
			t.type = c;
			return t;
		}else switch(c){
		case '.':
			t.type = TokSymbol;
			t.sym = dot;
			return t;
		case ';':
		case '\n':
			t.type = TokEol;
			return t;
		case '"':
			t.type = TokWord;
			t.w = ch() | 0200;
			return t;
		case '/':
			while(ch() != '\n');
			t.type = TokEol;
			return t;
		default:
			err(0, "warning: ignored character %c", ch());
		}
	}
	t.type = TokEol;
	return t;
}

/* Assembler */

/* write out a word */
void
putword(word w)
{
	static char left[64];

	if(pass2){
		/* listing */
		sprintf(left, "%04o  %06o\t", dot->val, w);
		list(left);

		fprintf(outfp, "%o: %o\n", dot->val++, w);
	}else{
		// this is probably useless...
		memory[dot->val++] = w;
	}
	dot->val &= W;
}

void
writelits(void)
{
	word a;
	static char left[64];

	fprintf(lstfp, "\n    LITERALS\n");
	for(a = litdot; a < 04000; a++){
		/* listing */
		sprintf(left, "%04o  %06o\t", a, memory[a]);
		list(left);

		fprintf(outfp, "%o: %o\n", a, memory[a]);
	}
}

word
putlit(word w)
{
	word a;

	for(a = litdot; a < 04000; a++)
		if(memory[a] == w)
			return a;
	if(litdot == 0)
		err(1, "error: too many literals");
	a = --litdot;
	memory[a] = w;
	return a;
}

word
combine(int op, word w1, word w2)
{
	switch(op){
	case 0:
		return w2;
	case '+':
		w1 += w2;
		if(w1 & 0200000)
			w1 += 1;
		return w1 & W;
	case '-':
		w1 += ~w2 & W;
		if(w1 & 0200000)
			w1 += 1;
		return w1 & W;
	case ' ':
	case '!':
		return (w1 | w2) & W;
	case '&':
		return (w1 & w2) & W;
	default:
		panic("unknown operator %c", op);
	}
	return 0;
}

word
expr(int mustdef)
{
	word w, v;
	Token t;
	int op;
	int stype;

	/* operators:
	 * + add
	 * - sub
	 * ! or
	 * & and
	 * space combine */

	op = 0;
	w = 0;
	// not quite sure about expression delimiter here...
	while(t = token(), t.type != TokEol &&
	      t.type != ')' && t.type != ']' && t.type != ','){
		switch(t.type){
		case TokWord:
			w = combine(op, w, t.w);
			op = ' ';
			break;

		case TokSymbol:
			stype = t.sym->type & 7;
			v = 0;
			if(stype == SymValue)
				v = t.sym->val;
			else if(stype != None || pass2)
				err(1, "error: bad symbol in expression");
			if(mustdef && stype == None)
				err(1, "error: undefined symbol");
			w = combine(op, w, v);
			op = ' ';
			break;

		case '(':
			v = expr(0);
			v = putlit(v);
			w = combine(op, w, v);
			op = ' ';
			t = token();
			if(t.type != ')' && t.type != TokEol)
				err(1, "err: literal not closed");
			if(t.type != ')')
				peekt = t;
			break;

		case '+':
		case '-':
		case '!':
		case '&':
			op = t.type;
			break;
		}
	}
	peekt = t;
	return w;
}

void
statement(void)
{
	Token t;
	Sym *s;
	word w;

	while(t = token(), t.type != TokEol){
		switch(t.type){
		case TokSymbol:
			s = t.sym;
			if(peek() == ','){
				peekc = 0;
				if(s->type == None ||
				   s->type & SymLabel && s->val == dot->val){
					s->type = SymValue | SymLabel;
					s->val = dot->val;
				}else{
					err(1, "error: redefinition by label");
				}
			}else if(peek() == '='){
				peekc = 0;
				w = expr(0);
				if(s->type == None || (s->type & SymLabel) == 0){
					s->type = SymValue;
					s->val = w;
				}else
					err(1, "error: redefinition by equ");
			}else{
				peekt = t;
				w = expr(0);
				putword(w);
			}
			break;
		case '-':
		case TokWord:
			peekt = t;
			w = expr(0);
			putword(w);
			break;

		case '$':
			t = token();
			if(t.type == TokWord)
				startaddr = t.w;
			else if(t.type == TokSymbol){
				startaddr = t.sym->val;
			}else
				err(1, "error: expected word or symbol");
			// end assembly
			peekc = EOF;
			return;

		case '*':
			w = expr(1);
			dot->val = w;
			break;

		default:
			err(1, "unknown token %c", t.type);
		}
	}
}

void
assemble(void)
{
	peekt.type = None;
	while(peek() != EOF)
		statement();

	// list last line
	if(pass2 && line[0])
		list(nil);
}

void
checkundef(void)
{
	int i;
	int e;

	e = 0;
	for(i = 0; i < MAXSYM; i++)
		if(symtab[i].name[0] && symtab[i].type == None){
			err(0, "error: %s undefined", symtab[i].name);
			e = 1;
		}
	if(e)
		err(1, "errors in first pass");
}

/* Init */

struct
{
	char *sym;
	word val;
} valtab[] = {
	{ ".", 0 },
	{ "si", 0000000 },
	{ "rs", 0004000 },
	{ "bi", 0010000 },
	{ "rd", 0014000 },
	{ "bo", 0020000 },
	{ "rc", 0024000 },
	{ "sd", 0030000 },
	{ "cf", 0034000 },
	{ "ts", 0040000 },
	{ "td", 0044000 },
	{ "ta", 0050000 },
	{ "ck", 0054000 },
	{ "ab", 0060000 },
	{ "ex", 0064000 },
	{ "cp", 0070000 },
	{ "sp", 0074000 },
	{ "ca", 0100000 },
	{ "cs", 0104000 },
	{ "ad", 0110000 },
	{ "su", 0114000 },
	{ "cm", 0120000 },
	{ "sa", 0124000 },
	{ "ao", 0130000 },
	{ "dm", 0134000 },
	{ "mr", 0140000 },
	{ "mh", 0144000 },
	{ "dv", 0150000 },
	{ "slr", 0154000 },
	{ "slh", 0155000 },
	{ "srr", 0160000 },
	{ "srh", 0161000 },
	{ "sf", 0164000 },
	{ "clc", 0170000 },
	{ "clh", 0171000 },
	{ "md", 0174000 },
	{ nil, 0 }
};


void
initsymtab(void)
{
	int i;
	Sym *s;

	for(i = 0; valtab[i].sym; i++){
		s = getsym(valtab[i].sym);
		s->type = SymValue;
		s->val = valtab[i].val;
	}
/*
	for(i = 0; pseudtab[i].sym; i++){
		s = getsym(sixbit(pseudtab[i].sym));
		s->type = SymPseudo;
		s->f = pseudtab[i].f;
	}
*/
	dot = findsym(".");
	dot->type = SymValue;
}


int
main(int argc, char *argv[])
{
	int i;

	infp = stdin;
	filename = "<stdin>";
	tmpfp = mustopen("tmptmp", "wb");

	initsymtab();

	litdot = 04000;
	dot->val = 040;
	pass2 = 0;
	assemble();

	checkundef();

	pass2 = 1;
	fclose(tmpfp);
	tmpfp = nil;
	infp = mustopen("tmptmp", "rb");
	outfp = mustopen("out.mem", "wb");

	lstfp = mustopen("out.lst", "w");
	litdot = 04000;
	dot->val = 040;
	lineno = 1;
	peekc = 0;
	assemble();

	writelits();

	fclose(infp);
	fclose(outfp);

	listsyms();

	printf("start: %o\n", startaddr);

	return 0;
}
