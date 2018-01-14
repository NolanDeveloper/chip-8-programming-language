/* ToDo: add other arithmetic operations */
enum OperationType {
    OP_ASSIGN,
    OP_ADD,
    OP_SUBTRACT,
};

extern void cg_init(void);

extern void cg_emit_label(const char *label);
extern void cg_emit_call_label(const char *label);
extern void cg_emit_operation(enum OperationType operation, size_t x, size_t y);
extern void cg_emit_assign_constant(size_t x, uint_fast16_t constant);
extern void cg_emit_bss(void);

extern bool cg_lookup(char *name, size_t *variable);
extern size_t cg_make_variable(char *name);
extern void cg_get_rom(const void **rom, size_t *size);

