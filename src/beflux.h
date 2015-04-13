#ifndef BEFLUX_H
#define BEFLUX_H

#include <stdio.h>
#include <stdint.h>

#define bfx_word uint8_t
#define BANK_SIZE (bfx_word)(-1)

#define PROGRAM_WIDTH BANK_SIZE
#define PROGRAM_HEIGHT BANK_SIZE
#define PROGRAM_SIZE BANK_SIZE * BANK_SIZE

#define BFX_IP_E 0x00
#define BFX_IP_N 0x40
#define BFX_IP_W 0x80
#define BFX_IP_S 0xC0
#define BFX_IP_TURN_L 0x40
#define BFX_IP_TURN_B 0x80
#define BFX_IP_TURN_R 0xC0

#define BFX_MODE_HALT 0
#define BFX_MODE_NORMAL 1
#define BFX_MODE_STRING 2
#define BFX_MODE_STRING_ESC 3
#define BFX_MODE_FREED (bfx_word)(-1)

struct Beflux;

typedef void (*BfxFunc)(struct Beflux *);

typedef struct BfxStack {
  bfx_word data[BANK_SIZE];
  bfx_word size;
} BfxStack;

typedef struct Beflux {
  bfx_word *programs;
  bfx_word *registers;

  BfxFunc *op_bindings;
  BfxFunc *f_bindings;
  BfxFunc *m_bindings;
  BfxFunc pre_update;
  BfxFunc post_update;

  BfxStack frames[BANK_SIZE];
  BfxStack calls_x;
  BfxStack calls_y;

  bfx_word current_program;
  bfx_word current_frame;

  bfx_word mode;
  bfx_word status;
  bfx_word value;
  bfx_word value_width;

  bfx_word t_minor;
  bfx_word t_major;
  bfx_word loop_count;
  bfx_word wrap_offset;

  size_t tick;
  size_t timeout;

  FILE *in;
  FILE *out;
  FILE *err;

  struct {
    bfx_word x;
    bfx_word y;
    bfx_word dir;
    bfx_word wait;
  } ip;
} Beflux;

/*******************************************************************************
 * BfxStack Functions
 */
void bfx_stack_init(BfxStack *s);
void bfx_stack_push(BfxStack *s, bfx_word value);
bfx_word bfx_stack_pop(BfxStack *s);
bfx_word bfx_stack_top(BfxStack *s);
void bfx_stack_clear(BfxStack *s);

/*******************************************************************************
 * Beflux Functions
 */
void bfx_init(Beflux *bfx);
void bfx_destroy(Beflux *bfx);

/* I/O */
void bfx_load(Beflux *bfx, bfx_word prog, const char *filename);
void bfx_save(Beflux *bfx, bfx_word prog, const char *filename);
void bfx_read(Beflux *bfx, bfx_word prog, const bfx_word *src, size_t size);
void bfx_write(Beflux *bfx, bfx_word prog, bfx_word *dst, size_t size);

void bfx_warning(Beflux *bfx, const char *message);
void bfx_error(Beflux *bfx, const char *message);

/* Stack Manipulation */
void bfx_push(Beflux *bfx, bfx_word value);
bfx_word bfx_pop(Beflux *bfx);
bfx_word bfx_top(Beflux *bfx);
void bfx_clear(Beflux *bfx);

/* Execution */
bfx_word bfx_run(Beflux *bfx);
void bfx_update(Beflux *bfx);
void bfx_eval(Beflux *bfx, bfx_word op);

/* Program Manipulation */
bfx_word bfx_program_get(Beflux *bfx, bfx_word prog, bfx_word x, bfx_word y);
void bfx_program_set(Beflux *bfx, bfx_word prog, bfx_word x, bfx_word y, bfx_word value);

/* IP Manipulation */
void bfx_ip_reset(Beflux *bfx);
void bfx_ip_advance(Beflux *bfx);
bfx_word bfx_ip_get_op(Beflux *bfx);

/* Beflux Operators */
void bfx_op20(Beflux *);
void bfx_op21(Beflux *); void bfx_op22(Beflux *); void bfx_op23(Beflux *);
void bfx_op24(Beflux *); void bfx_op25(Beflux *); void bfx_op26(Beflux *);
void bfx_op27(Beflux *); void bfx_op28(Beflux *); void bfx_op29(Beflux *);
void bfx_op2a(Beflux *); void bfx_op2b(Beflux *); void bfx_op2c(Beflux *);
void bfx_op2d(Beflux *); void bfx_op2e(Beflux *); void bfx_op2f(Beflux *);

void bfx_op30(Beflux *);
void bfx_op31(Beflux *); void bfx_op32(Beflux *); void bfx_op33(Beflux *);
void bfx_op34(Beflux *); void bfx_op35(Beflux *); void bfx_op36(Beflux *);
void bfx_op37(Beflux *); void bfx_op38(Beflux *); void bfx_op39(Beflux *);
void bfx_op3a(Beflux *); void bfx_op3b(Beflux *); void bfx_op3c(Beflux *);
void bfx_op3d(Beflux *); void bfx_op3e(Beflux *); void bfx_op3f(Beflux *);

void bfx_op40(Beflux *);
void bfx_op41(Beflux *); void bfx_op42(Beflux *); void bfx_op43(Beflux *);
void bfx_op44(Beflux *); void bfx_op45(Beflux *); void bfx_op46(Beflux *);
void bfx_op47(Beflux *); void bfx_op48(Beflux *); void bfx_op49(Beflux *);
void bfx_op4a(Beflux *); void bfx_op4b(Beflux *); void bfx_op4c(Beflux *);
void bfx_op4d(Beflux *); void bfx_op4e(Beflux *); void bfx_op4f(Beflux *);

void bfx_op50(Beflux *);
void bfx_op51(Beflux *); void bfx_op52(Beflux *); void bfx_op53(Beflux *);
void bfx_op54(Beflux *); void bfx_op55(Beflux *); void bfx_op56(Beflux *);
void bfx_op57(Beflux *); void bfx_op58(Beflux *); void bfx_op59(Beflux *);
void bfx_op5a(Beflux *); void bfx_op5b(Beflux *); void bfx_op5c(Beflux *);
void bfx_op5d(Beflux *); void bfx_op5e(Beflux *); void bfx_op5f(Beflux *);

void bfx_op60(Beflux *);
void bfx_op61(Beflux *); void bfx_op62(Beflux *); void bfx_op63(Beflux *);
void bfx_op64(Beflux *); void bfx_op65(Beflux *); void bfx_op66(Beflux *);
void bfx_op67(Beflux *); void bfx_op68(Beflux *); void bfx_op69(Beflux *);
void bfx_op6a(Beflux *); void bfx_op6b(Beflux *); void bfx_op6c(Beflux *);
void bfx_op6d(Beflux *); void bfx_op6e(Beflux *); void bfx_op6f(Beflux *);

void bfx_op70(Beflux *);
void bfx_op71(Beflux *); void bfx_op72(Beflux *); void bfx_op73(Beflux *);
void bfx_op74(Beflux *); void bfx_op75(Beflux *); void bfx_op76(Beflux *);
void bfx_op77(Beflux *); void bfx_op78(Beflux *); void bfx_op79(Beflux *);
void bfx_op7a(Beflux *); void bfx_op7b(Beflux *); void bfx_op7c(Beflux *);
void bfx_op7d(Beflux *); void bfx_op7e(Beflux *); void bfx_op7f(Beflux *);

void (*bfx_default_op_bindings[BANK_SIZE])(Beflux *) = {
  NULL,     NULL,     NULL,     NULL,
  NULL,     NULL,     NULL,     NULL,
  NULL,     NULL,     NULL,     NULL,
  NULL,     NULL,     NULL,     NULL,

  NULL,     NULL,     NULL,     NULL,
  NULL,     NULL,     NULL,     NULL,
  NULL,     NULL,     NULL,     NULL,
  NULL,     NULL,     NULL,     NULL,

  bfx_op20, bfx_op21, bfx_op22, bfx_op23,
  bfx_op24, bfx_op25, bfx_op26, bfx_op27,
  bfx_op28, bfx_op29, bfx_op2a, bfx_op2b,
  bfx_op2c, bfx_op2d, bfx_op2e, bfx_op2f,

  bfx_op30, bfx_op31, bfx_op32, bfx_op33,
  bfx_op34, bfx_op35, bfx_op36, bfx_op37,
  bfx_op38, bfx_op39, bfx_op3a, bfx_op3b,
  bfx_op3c, bfx_op3d, bfx_op3e, bfx_op3f,

  bfx_op40, bfx_op41, bfx_op42, bfx_op43,
  bfx_op44, bfx_op45, bfx_op46, bfx_op47,
  bfx_op48, bfx_op49, bfx_op4a, bfx_op4b,
  bfx_op4c, bfx_op4d, bfx_op4e, bfx_op4f,

  bfx_op50, bfx_op51, bfx_op52, bfx_op53,
  bfx_op54, bfx_op55, bfx_op56, bfx_op57,
  bfx_op58, bfx_op59, bfx_op5a, bfx_op5b,
  bfx_op5c, bfx_op5d, bfx_op5e, bfx_op5f,

  bfx_op60, bfx_op61, bfx_op62, bfx_op63,
  bfx_op64, bfx_op65, bfx_op66, bfx_op67,
  bfx_op68, bfx_op69, bfx_op6a, bfx_op6b,
  bfx_op6c, bfx_op6d, bfx_op6e, bfx_op6f,

  bfx_op70, bfx_op71, bfx_op72, bfx_op73,
  bfx_op74, bfx_op75, bfx_op76, bfx_op77,
  bfx_op78, bfx_op79, bfx_op7a, bfx_op7b,
  bfx_op7c, bfx_op7d, bfx_op7e, bfx_op7f
};

/* Beflux Math Functions */
void bfx_m00(Beflux *);
void bfx_m01(Beflux *); void bfx_m02(Beflux *); void bfx_m03(Beflux *);
void bfx_m04(Beflux *); void bfx_m05(Beflux *); void bfx_m06(Beflux *);
void bfx_m07(Beflux *); void bfx_m08(Beflux *); void bfx_m09(Beflux *);
void bfx_m0a(Beflux *); void bfx_m0b(Beflux *); void bfx_m0c(Beflux *);
void bfx_m0d(Beflux *); void bfx_m0e(Beflux *); void bfx_m0f(Beflux *);

void (*bfx_default_m_bindings[BANK_SIZE])(Beflux *) = {
  bfx_m00, bfx_m01, bfx_m02, bfx_m03, bfx_m04, bfx_m05, bfx_m06, bfx_m07,
  bfx_m08, bfx_m09, bfx_m0a, bfx_m0b, bfx_m0c, bfx_m0d, bfx_m0e, bfx_m0f
};

#endif
