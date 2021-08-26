/* Machine dependent */
typedef uint32_t u32, Word;
typedef uint16_t u16, Addr;
typedef uint8_t u8;
#define ADDRMASK 0177777
#define WORDMASK 0777777
#define MAXMEM (64*1024)
#define SYMTABSIZE 1024

enum {
	REG_PC,
	REG_AC,
	REG_LR
};
