/**
 * @file beflux.h
 * @date 4/20/2015
 * @author Tony Chiodo (http://dodecaplex.net)
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BEFLUX_H
#define BEFLUX_H

#include <stdio.h>
#include <stdint.h>

#define bfx_word uint8_t
#define BFX_WORD_MAX UINT8_MAX
#define BFX_BANK_SIZE BFX_WORD_MAX

#define BFX_PROGRAM_WIDTH  BFX_BANK_SIZE
#define BFX_PROGRAM_HEIGHT BFX_BANK_SIZE
#define BFX_PROGRAM_SIZE   BFX_BANK_SIZE * BFX_BANK_SIZE

#define BFX_TABLE_SIZE 0x80

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
#define BFX_MODE_FREED BFX_WORD_MAX

struct beflux;

typedef void (*bfx_func)(struct beflux *);

typedef struct bfx_stack {
  bfx_word data[BFX_BANK_SIZE];
  bfx_word size;
} bfx_stack;

typedef struct beflux {
  bfx_word *programs;
  bfx_word *registers;

  bfx_func *op_bindings;
  bfx_func *f_bindings;
  bfx_func *m_bindings;
  bfx_func pre_update;
  bfx_func post_update;

  bfx_stack frames[BFX_BANK_SIZE];
  bfx_stack calls_x;
  bfx_stack calls_y;

  bfx_word current_program;
  bfx_word program_count;
  bfx_word current_frame;
  bfx_word frame_count;

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
} beflux;

/*******************************************************************************
 * bfx_stack Functions
 */
void bfx_stack_init(bfx_stack *s);
void bfx_stack_push(bfx_stack *s, bfx_word value);
bfx_word bfx_stack_pop(bfx_stack *s);
bfx_word bfx_stack_top(bfx_stack *s);
void bfx_stack_clear(bfx_stack *s);

/*******************************************************************************
 * Beflux Functions
 */
beflux *bfx_create(void);
void bfx_init(beflux *bfx);
void bfx_free(beflux *bfx);
void bfx_destroy(beflux *bfx);

/* I/O */
void bfx_load(beflux *bfx, bfx_word prog, const char *filename);
void bfx_save(beflux *bfx, bfx_word prog, const char *filename);
void bfx_read(beflux *bfx, bfx_word prog, const bfx_word *src, size_t size);
void bfx_write(beflux *bfx, bfx_word prog, bfx_word *dst, size_t size);

void bfx_note(beflux *bfx, const char *message);
void bfx_warning(beflux *bfx, const char *message);
void bfx_error(beflux *bfx, const char *message);

/* Stack Manipulation */
void bfx_push(beflux *bfx, bfx_word value);
bfx_word bfx_pop(beflux *bfx);
bfx_word bfx_top(beflux *bfx);
void bfx_clear(beflux *bfx);

/* Execution */
bfx_word bfx_run(beflux *bfx);
void bfx_update(beflux *bfx);
void bfx_eval(beflux *bfx, bfx_word op);

/* Program Manipulation */
bfx_word bfx_program_get(beflux *bfx, bfx_word prog, bfx_word x, bfx_word y);
void bfx_program_set(beflux *bfx, bfx_word prog, bfx_word x, bfx_word y, bfx_word value);

/* IP Manipulation */
void bfx_ip_reset(beflux *bfx);
void bfx_ip_advance(beflux *bfx);
bfx_word bfx_ip_get_op(beflux *bfx);

/* Beflux Utilities */
bfx_word bfx_degtobam(double degrees);
bfx_word bfx_radtobam(double radians);

double bfx_bamtodeg(bfx_word bams);
double bfx_bamtorad(bfx_word bams);

void bfx_sign_encode(beflux *bfx, double y);
void bfx_trig_encode(beflux *bfx, double y);

double bfx_sign_decode(beflux *bfx);
double bfx_trig_decode(beflux *bfx);

double bfx_table_lookup(double *table, bfx_word index);


/* Beflux Operators */
void bfx_op20(beflux *);
void bfx_op21(beflux *); void bfx_op22(beflux *); void bfx_op23(beflux *);
void bfx_op24(beflux *); void bfx_op25(beflux *); void bfx_op26(beflux *);
void bfx_op27(beflux *); void bfx_op28(beflux *); void bfx_op29(beflux *);
void bfx_op2a(beflux *); void bfx_op2b(beflux *); void bfx_op2c(beflux *);
void bfx_op2d(beflux *); void bfx_op2e(beflux *); void bfx_op2f(beflux *);

void bfx_op30(beflux *);
void bfx_op31(beflux *); void bfx_op32(beflux *); void bfx_op33(beflux *);
void bfx_op34(beflux *); void bfx_op35(beflux *); void bfx_op36(beflux *);
void bfx_op37(beflux *); void bfx_op38(beflux *); void bfx_op39(beflux *);
void bfx_op3a(beflux *); void bfx_op3b(beflux *); void bfx_op3c(beflux *);
void bfx_op3d(beflux *); void bfx_op3e(beflux *); void bfx_op3f(beflux *);

void bfx_op40(beflux *);
void bfx_op41(beflux *); void bfx_op42(beflux *); void bfx_op43(beflux *);
void bfx_op44(beflux *); void bfx_op45(beflux *); void bfx_op46(beflux *);
void bfx_op47(beflux *); void bfx_op48(beflux *); void bfx_op49(beflux *);
void bfx_op4a(beflux *); void bfx_op4b(beflux *); void bfx_op4c(beflux *);
void bfx_op4d(beflux *); void bfx_op4e(beflux *); void bfx_op4f(beflux *);

void bfx_op50(beflux *);
void bfx_op51(beflux *); void bfx_op52(beflux *); void bfx_op53(beflux *);
void bfx_op54(beflux *); void bfx_op55(beflux *); void bfx_op56(beflux *);
void bfx_op57(beflux *); void bfx_op58(beflux *); void bfx_op59(beflux *);
void bfx_op5a(beflux *); void bfx_op5b(beflux *); void bfx_op5c(beflux *);
void bfx_op5d(beflux *); void bfx_op5e(beflux *); void bfx_op5f(beflux *);

void bfx_op60(beflux *);
void bfx_op61(beflux *); void bfx_op62(beflux *); void bfx_op63(beflux *);
void bfx_op64(beflux *); void bfx_op65(beflux *); void bfx_op66(beflux *);
void bfx_op67(beflux *); void bfx_op68(beflux *); void bfx_op69(beflux *);
void bfx_op6a(beflux *); void bfx_op6b(beflux *); void bfx_op6c(beflux *);
void bfx_op6d(beflux *); void bfx_op6e(beflux *); void bfx_op6f(beflux *);

void bfx_op70(beflux *);
void bfx_op71(beflux *); void bfx_op72(beflux *); void bfx_op73(beflux *);
void bfx_op74(beflux *); void bfx_op75(beflux *); void bfx_op76(beflux *);
void bfx_op77(beflux *); void bfx_op78(beflux *); void bfx_op79(beflux *);
void bfx_op7a(beflux *); void bfx_op7b(beflux *); void bfx_op7c(beflux *);
void bfx_op7d(beflux *); void bfx_op7e(beflux *); void bfx_op7f(beflux *);

void (*bfx_default_op_bindings[BFX_BANK_SIZE])(beflux *) = {
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
void bfx_m00(beflux *); void bfx_m01(beflux *); void bfx_m02(beflux *);

void (*bfx_default_m_bindings[BFX_BANK_SIZE])(beflux *) = {
  bfx_m00, bfx_m01, bfx_m02
};

/* Sine Table */
double bfx_sin_table[BFX_TABLE_SIZE] = {
  0x0.000000p+0, 0x1.92155fp-6, 0x1.91f65fp-5, 0x1.2d5209p-4,
  0x1.917a6cp-4, 0x1.f564e5p-4, 0x1.2c8107p-3, 0x1.5e2144p-3,
  0x1.8f8b84p-3, 0x1.c0b827p-3, 0x1.f19f98p-3, 0x1.111d26p-2,
  0x1.294063p-2, 0x1.4135c9p-2, 0x1.58f9a7p-2, 0x1.708853p-2,

  0x1.87de2ap-2, 0x1.9ef794p-2, 0x1.b5d101p-2, 0x1.cc66eap-2,
  0x1.e2b5d3p-2, 0x1.f8ba4ep-2, 0x1.07387ap-1, 0x1.11eb35p-1,
  0x1.1c73b4p-1, 0x1.26d055p-1, 0x1.30ff80p-1, 0x1.3affa3p-1,
  0x1.44cf32p-1, 0x1.4e6cacp-1, 0x1.57d693p-1, 0x1.610b75p-1,

  0x1.6a09e6p-1, 0x1.72d083p-1, 0x1.7b5df2p-1, 0x1.83b0e1p-1,
  0x1.8bc807p-1, 0x1.93a225p-1, 0x1.9b3e04p-1, 0x1.a29a7ap-1,
  0x1.a9b663p-1, 0x1.b090a5p-1, 0x1.b72834p-1, 0x1.bd7c0bp-1,
  0x1.c38b2fp-1, 0x1.c954b2p-1, 0x1.ced7afp-1, 0x1.d4134dp-1,

  0x1.d906bdp-1, 0x1.ddb13bp-1, 0x1.e21210p-1, 0x1.e6288fp-1,
  0x1.e9f415p-1, 0x1.ed740ep-1, 0x1.f0a7f0p-1, 0x1.f38f3bp-1,
  0x1.f6297dp-1, 0x1.f87650p-1, 0x1.fa7558p-1, 0x1.fc2647p-1,
  0x1.fd88dap-1, 0x1.fe9cdbp-1, 0x1.ff621ep-1, 0x1.ffd886p-1,

  0x1.000000p+0, 0x1.ffd886p-1, 0x1.ff621ep-1, 0x1.fe9cdbp-1,
  0x1.fd88dap-1, 0x1.fc2647p-1, 0x1.fa7558p-1, 0x1.f87650p-1,
  0x1.f6297dp-1, 0x1.f38f3bp-1, 0x1.f0a7f0p-1, 0x1.ed740ep-1,
  0x1.e9f415p-1, 0x1.e6288fp-1, 0x1.e21210p-1, 0x1.ddb13bp-1,

  0x1.d906bdp-1, 0x1.d4134dp-1, 0x1.ced7afp-1, 0x1.c954b2p-1,
  0x1.c38b2fp-1, 0x1.bd7c0bp-1, 0x1.b72834p-1, 0x1.b090a5p-1,
  0x1.a9b663p-1, 0x1.a29a7ap-1, 0x1.9b3e04p-1, 0x1.93a225p-1,
  0x1.8bc807p-1, 0x1.83b0e1p-1, 0x1.7b5df2p-1, 0x1.72d083p-1,

  0x1.6a09e6p-1, 0x1.610b75p-1, 0x1.57d693p-1, 0x1.4e6cacp-1,
  0x1.44cf32p-1, 0x1.3affa3p-1, 0x1.30ff80p-1, 0x1.26d055p-1,
  0x1.1c73b4p-1, 0x1.11eb35p-1, 0x1.07387ap-1, 0x1.f8ba4ep-2,
  0x1.e2b5d3p-2, 0x1.cc66eap-2, 0x1.b5d101p-2, 0x1.9ef794p-2,

  0x1.87de2ap-2, 0x1.708853p-2, 0x1.58f9a7p-2, 0x1.4135c9p-2,
  0x1.294063p-2, 0x1.111d26p-2, 0x1.f19f98p-3, 0x1.c0b827p-3,
  0x1.8f8b84p-3, 0x1.5e2144p-3, 0x1.2c8107p-3, 0x1.f564e5p-4,
  0x1.917a6cp-4, 0x1.2d5209p-4, 0x1.91f65fp-5, 0x1.92155fp-6
};

#endif

#ifdef __cplusplus
}
#endif
