#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "util.h"
#include "threading.h"

#define nil NULL

#include "mach.h"

/* FE devices */
enum
{
	DEV_PTR = 0,
	DEV_PTP,
};

struct dev
{
	char *devname;
	int mode;
	int fd;
	char *path;
	void (*mntcb)(struct dev*);
	void (*unmntcb)(struct dev*);
};
extern struct dev devtab[];


extern Addr starta;
extern int started;
extern char *helpstr, *colhelpstr;

void defsym(char *sym, Word val);
char *findsym(Word v);

void panic(char *fmt, ...);
FILE *mustopen(const char *name, const char *mode);
void quit(void);
void err(char *str);
void typestr(char *str);
void typestrnl(char *str);

void c_listf(int argc, char *argv[]);

void coloncmd(char *line);
void docmd(char *cmd, char *line);

int isrunning(void);
int isstopped(void);

void deposit(Addr a, Word w);
Word examine(Addr a);
void depositreg(int a, Word w);
Word examinereg(int a);
char *disasm(Word w);
void cpu_start(Addr a);
void cpu_readin(Addr a);
void cpu_setpc(Addr pc);
void cpu_stopinst(void);
void cpu_stopmem(void);
void cpu_cont(void);
void cpu_nextinst(void);
void cpu_nextmem(void);
void cpu_exec(Word inst);
void cpu_ioreset(void);
void cpu_printflags(void);

void mnt_ptp(struct dev*);
void unmnt_ptp(struct dev*);
void fe_svc(void);

void initcrt(const char *host);
void initnetmem(const char *host, int port);

void initmach(void);
void deinitmach(void);
