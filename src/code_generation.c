#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <search.h>

#include "code_generation.h"
#include "utils.h"

#define die(...) die_("codegen", __VA_ARGS__)

struct LabelAddress {
    char            *label;
    uint_fast16_t   address;
    bool            undefined;
};

static uint_least8_t machineCode[BUFFER_SIZE];
static uint_fast16_t instructionPointer      = 0;
static uint_fast16_t numberOfUndefinedLabels = 0;

extern void
cgInit(void) {
    if (hcreate(MAX_LABELS)) return;
    die("Can't create hash table for labels: %s", strerror(errno));
}

extern void
cgEmitLabel(char *label) {
    ENTRY entry = { .key = label };
    ENTRY *item = hsearch(entry, FIND);
    struct LabelAddress *la = item ? item->data : NULL;
    if (la) {
        if (!la->undefined) { /* Second definition. */
            die("label(%s) defined multiple times.", label);
        }
        /* First definition. Label was used before. */
        uint_fast16_t address = MIN_ADDRESS + instructionPointer;
        uint_fast16_t i, j = la->address;
        do {
            i = j;
            j = (machineCode[i] & 0x0f) << 8 | machineCode[i + 1];
            machineCode[i    ] = (machineCode[i] & 0xf0) | ((address >> 8) & 0x0f);
            machineCode[i + 1] = address & 0xff;
        } while (j);
        la->undefined = false;
        la->address   = address;
        --numberOfUndefinedLabels;
    } else { /* First definition. Label was not used before. */
        la = emalloc(sizeof(struct LabelAddress));
        *la = (struct LabelAddress) {
            .label     = label,
            .address   = MIN_ADDRESS + instructionPointer,
            .undefined = false,
        };
        entry.data = la;
        hsearch(entry, ENTER);
    }
}

extern void
cgSaveMachineCodeToFile(const char *path) {
    if (numberOfUndefinedLabels) die("There are undefined labels");
    FILE *f = fopen(path, "w");
    if (!f) die("Can't open file, %s", strerror(errno));
    size_t n = fwrite(machineCode, sizeof(machineCode[0]), (size_t)instructionPointer, f);
    if (instructionPointer != n) die("Can't write to file: %s", strerror(errno));
    fclose(f);
}


static void
emit(uint_fast16_t instruction) {
    if (BUFFER_SIZE <= instructionPointer) {
        die("Number of instructions exceeds memory limit.");
    }
    machineCode[instructionPointer++] = (instruction >> 8u) & 0xff;
    machineCode[instructionPointer++] = instruction & 0xff;
}

static void
check(uint_fast16_t x, unsigned n) {
    assert(n < 16);
    uint_fast16_t max = (uint_fast16_t)1 << n;
    if (x < max) return;
    die("%"PRIuLEAST16" is too big number.", x);
}

static void
emit_hnnni(uint_fast16_t h, uint_fast16_t nnn) {
    check(h, 4);
    check(nnn, 12);
    emit(h << 12 | nnn);
}

static void
emit_hnnnl(uint_fast16_t h, char *label) {
    ENTRY entry = { .key = label };
    ENTRY *item = hsearch(entry, FIND);
    struct LabelAddress *la = item ? item->data : NULL;
    if (!la) { /* If label was neither defined nor used before. */
        /* Save instruction pointer in the table of labels to fill address when
         * it will be defined. */
        la = emalloc(sizeof(struct LabelAddress));
        *la = (struct LabelAddress) {
            .label      = label,
            .address    = instructionPointer,
            .undefined  = true,
        };
        entry.data = la;
        hsearch(entry, ENTER);
        emit_hnnni(h, 0);
        ++numberOfUndefinedLabels;
    } else if (la->undefined) { /* If label was not defined but was used before. */
        uint_fast16_t previousUsage = la->address;
        /* Update table of labels with new last usage site. */
        la->address = instructionPointer;
        emit_hnnni(h, previousUsage);
    } else { /* If label was defined. */
        emit_hnnni(h, la->address);
    }
}

static void
emit_hxkk(uint_fast16_t h, uint_fast16_t x, uint_fast16_t kk) {
    check(h, 4);
    check(x, 4);
    check(kk, 8);
    emit(h << 12 | x << 8 | kk);
}

static void
emit_hxyn(uint_fast16_t h, uint_fast16_t x, uint_fast16_t y, uint_fast16_t n) {
    check(h, 4);
    check(x, 4);
    check(y, 4);
    check(n, 4);
    emit(h << 12 | x << 8 | y << 4 | n);
}

typedef uint_fast16_t Instr;

extern void cgEmitCls(void)                      { emit(0x00e0); }
extern void cgEmitRet(void)                      { emit(0x00ee); }
extern void cgEmitJpAddri(Instr addr)            { emit_hnnni(0x1, addr); }
extern void cgEmitJpAddrl(char *label)           { emit_hnnnl(0x1, label); }
extern void cgEmitCallAddri(Instr addr)          { emit_hnnni(0x2, addr); }
extern void cgEmitCallAddrl(char *label)         { emit_hnnnl(0x2, label); }
extern void cgEmitSeVxByte(Instr x, Instr byte)  { emit_hxkk(0x3, x, byte); }
extern void cgEmitSneVxByte(Instr x, Instr byte) { emit_hxkk(0x4, x, byte); }
extern void cgEmitSeVxVy(Instr x, Instr y)       { emit_hxyn(0x5, x, y, 0); }
extern void cgEmitLdVxByte(Instr x, Instr byte)  { emit_hxkk(0x6, x, byte); }
extern void cgEmitAddVxByte(Instr x, Instr byte) { emit_hxkk(0x7, x, byte); }
extern void cgEmitLdVxVy(Instr x, Instr y)       { emit_hxyn(0x8, x, y, 0); }
extern void cgEmitOrVxVy(Instr x, Instr y)       { emit_hxyn(0x8, x, y, 1); }
extern void cgEmitAndVxVy(Instr x, Instr y)      { emit_hxyn(0x8, x, y, 2); }
extern void cgEmitXorVxVy(Instr x, Instr y)      { emit_hxyn(0x8, x, y, 3); }
extern void cgEmitAddVxVy(Instr x, Instr y)      { emit_hxyn(0x8, x, y, 4); }
extern void cgEmitSubVxVy(Instr x, Instr y)      { emit_hxyn(0x8, x, y, 5); }
extern void cgEmitShrVx(Instr x)                 { emit_hxkk(0x8, x, 0x06); }
extern void cgEmitSubnVxVy(Instr x, Instr y)     { emit_hxyn(0x8, x, y, 7); }
extern void cgEmitShlVx(Instr x)                 { emit_hxkk(0x8, x, 0x0E); }
extern void cgEmitSneVxVy(Instr x, Instr y)      { emit_hxyn(0x9, x, y, 0); }
extern void cgEmitLdIAddri(Instr addr)           { emit_hnnni(0xA, addr); }
extern void cgEmitLdIAddrl(char *label)          { emit_hnnnl(0xA, label); }
extern void cgEmitJpV0Addri(Instr addr)          { emit_hnnni(0xB, addr); }
extern void cgEmitJpV0Addrl(char *label)         { emit_hnnnl(0xB, label); }
extern void cgEmitRndVxByte(Instr x, Instr byte) { emit_hxkk(0xC, x, byte); }
extern void cgEmitDrwVxVyNibble(Instr x, Instr y, Instr nibble) { emit_hxyn(0xD, x, y, nibble); }
extern void cgEmitSkpVx(Instr x)  { emit_hxkk(0xE, x, 0x9E); }
extern void cgEmitSknpVx(Instr x) { emit_hxkk(0xE, x, 0xA1); }
extern void cgEmitLdVxDt(Instr x) { emit_hxkk(0xF, x, 0x07); }
extern void cgEmitLdVxK(Instr x)  { emit_hxkk(0xF, x, 0x0A); }
extern void cgEmitLdDtVx(Instr x) { emit_hxkk(0xF, x, 0x15); }
extern void cgEmitLdStVx(Instr x) { emit_hxkk(0xF, x, 0x18); }
extern void cgEmitAddIVx(Instr x) { emit_hxkk(0xF, x, 0x1E); }
extern void cgEmitLdFVx(Instr x)  { emit_hxkk(0xF, x, 0x29); }
extern void cgEmitLdBVx(Instr x)  { emit_hxkk(0xF, x, 0x33); }
extern void cgEmitLdIIVx(Instr x) { emit_hxkk(0xF, x, 0x55); }
extern void cgEmitLdVxII(Instr x) { emit_hxkk(0xF, x, 0x65); }
