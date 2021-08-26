/* Machine dependent */
typedef uint32_t u32, Word;	// good to have some extra bits
typedef uint16_t u16, Addr;
typedef uint8_t u8;
#define ADDRMASK 03777
#define WORDMASK 0177777
#define MAXMEM (2*1024)
#define SYMTABSIZE 1024

enum {
	REG_PC = 32,
	REG_AC,
	REG_A,
	REG_B,
	REG_SAM
};
