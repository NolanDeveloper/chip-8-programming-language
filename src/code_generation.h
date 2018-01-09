#define MIN_ADDRESS             0x200
#define MAX_ADDRESS             0x1000
#define BUFFER_SIZE             (MAX_ADDRESS - MIN_ADDRESS)
#define PLACEHOLDER_ADDRESS     MAX_ADDRESS
#define MAX_LABELS              2048

extern void cgInit(void);

extern void cgEmitLabel(char *label);

extern void cgSaveMachineCodeToFile(const char *path);

extern void cgEmitCls(void);
extern void cgEmitRet(void);
extern void cgEmitJpAddri(uint_fast16_t addr);
extern void cgEmitJpAddrl(char *label);
extern void cgEmitCallAddri(uint_fast16_t addr);
extern void cgEmitCallAddrl(char *label);
extern void cgEmitSeVxByte(uint_fast16_t x, uint_fast16_t byte);
extern void cgEmitSneVxByte(uint_fast16_t x, uint_fast16_t byte);
extern void cgEmitSeVxVy(uint_fast16_t x, uint_fast16_t y);
extern void cgEmitLdVxByte(uint_fast16_t x, uint_fast16_t byte);
extern void cgEmitAddVxByte(uint_fast16_t x, uint_fast16_t byte);
extern void cgEmitLdVxVy(uint_fast16_t x, uint_fast16_t y);
extern void cgEmitOrVxVy(uint_fast16_t x, uint_fast16_t y);
extern void cgEmitAndVxVy(uint_fast16_t x, uint_fast16_t y);
extern void cgEmitXorVxVy(uint_fast16_t x, uint_fast16_t y);
extern void cgEmitAddVxVy(uint_fast16_t x, uint_fast16_t y);
extern void cgEmitSubVxVy(uint_fast16_t x, uint_fast16_t y);
extern void cgEmitShrVx(uint_fast16_t x);
extern void cgEmitSubnVxVy(uint_fast16_t x, uint_fast16_t y);
extern void cgEmitShlVx(uint_fast16_t x);
extern void cgEmitSneVxVy(uint_fast16_t x, uint_fast16_t y);
extern void cgEmitLdIAddri(uint_fast16_t addr);
extern void cgEmitLdIAddrl(char *label);
extern void cgEmitJpV0Addri(uint_fast16_t addr);
extern void cgEmitJpV0Addrl(char *label);
extern void cgEmitRndVxByte(uint_fast16_t x, uint_fast16_t byte);
extern void cgEmitDrwVxVyNibble(uint_fast16_t x, uint_fast16_t y, uint_fast16_t nibble);
extern void cgEmitSkpVx(uint_fast16_t x);
extern void cgEmitSknpVx(uint_fast16_t x);
extern void cgEmitLdVxDt(uint_fast16_t x);
extern void cgEmitLdVxK(uint_fast16_t x);
extern void cgEmitLdDtVx(uint_fast16_t x);
extern void cgEmitLdStVx(uint_fast16_t x);
extern void cgEmitAddIVx(uint_fast16_t x);
extern void cgEmitLdFVx(uint_fast16_t x);
extern void cgEmitLdBVx(uint_fast16_t x);
extern void cgEmitLdIIVx(uint_fast16_t x);
extern void cgEmitLdVxII(uint_fast16_t x);
