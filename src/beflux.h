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

typedef uint8_t bfx_word;
#define BFX_WORD_MAX  UINT8_MAX
#define BFX_BANK_SIZE BFX_WORD_MAX + 1

#define BFX_PROGRAM_WIDTH  BFX_WORD_MAX
#define BFX_PROGRAM_HEIGHT BFX_WORD_MAX
#define BFX_PROGRAM_SIZE   BFX_PROGRAM_WIDTH * BFX_PROGRAM_HEIGHT

#define BFX_IP_E      0x00
#define BFX_IP_N      0x40
#define BFX_IP_W      0x80
#define BFX_IP_S      0xC0
#define BFX_IP_TURN_L 0x40
#define BFX_IP_TURN_B 0x80
#define BFX_IP_TURN_R 0xC0

#define BFX_MODE_HALT       0
#define BFX_MODE_NORMAL     1
#define BFX_MODE_STRING     2
#define BFX_MODE_STRING_ESC 3
#define BFX_MODE_FREED      BFX_WORD_MAX

typedef struct beflux beflux;

typedef void bfx_func(struct beflux *bfx);

typedef struct bfx_stack {
  bfx_word size;
  bfx_word data[BFX_BANK_SIZE];
} bfx_stack;

struct beflux {
  bfx_word *programs;
  bfx_word *registers;

  bfx_func **op_bindings;
  bfx_func **f_bindings;
  bfx_func *pre_update;
  bfx_func *post_update;

  bfx_stack frames[BFX_BANK_SIZE];
  bfx_stack calls_row;
  bfx_stack calls_col;

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
  time_t run_timer;
  time_t pre_timer;
  time_t post_timer;
  size_t timeout;
  bfx_word sleep;

  FILE *in;
  FILE *out;
  FILE *err;

  struct {
    bfx_word row;
    bfx_word col;
    bfx_word dir;
    bfx_word wait;
  } ip;
};

/*******************************************************************************
 * Beflux Functions
 */
beflux *bfx_new(void);
void bfx_init(beflux *bfx);
void bfx_free(beflux *bfx);
void bfx_del(beflux *bfx);

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
void bfx_sleep(beflux *bfx);
void bfx_eval(beflux *bfx, bfx_word op);

/* Program Manipulation */
bfx_word bfx_program_get(
  beflux *bfx,
  bfx_word prog,
  bfx_word row,
  bfx_word col
);

void bfx_program_set(
  beflux *bfx,
  bfx_word prog,
  bfx_word row,
  bfx_word col,
  bfx_word value
);

/* IP Manipulation */
void bfx_ip_reset(beflux *bfx);
void bfx_ip_advance(beflux *bfx);
bfx_word bfx_ip_get_op(beflux *bfx);

/* Beflux Operators */
bfx_func bfx_op20; bfx_func bfx_op21; bfx_func bfx_op22; bfx_func bfx_op23;
bfx_func bfx_op24; bfx_func bfx_op25; bfx_func bfx_op26; bfx_func bfx_op27;
bfx_func bfx_op28; bfx_func bfx_op29; bfx_func bfx_op2a; bfx_func bfx_op2b;
bfx_func bfx_op2c; bfx_func bfx_op2d; bfx_func bfx_op2e; bfx_func bfx_op2f;

bfx_func bfx_op30; bfx_func bfx_op31; bfx_func bfx_op32; bfx_func bfx_op33;
bfx_func bfx_op34; bfx_func bfx_op35; bfx_func bfx_op36; bfx_func bfx_op37;
bfx_func bfx_op38; bfx_func bfx_op39; bfx_func bfx_op3a; bfx_func bfx_op3b;
bfx_func bfx_op3c; bfx_func bfx_op3d; bfx_func bfx_op3e; bfx_func bfx_op3f;

bfx_func bfx_op40; bfx_func bfx_op41; bfx_func bfx_op42; bfx_func bfx_op43;
bfx_func bfx_op44; bfx_func bfx_op45; bfx_func bfx_op46; bfx_func bfx_op47;
bfx_func bfx_op48; bfx_func bfx_op49; bfx_func bfx_op4a; bfx_func bfx_op4b;
bfx_func bfx_op4c; bfx_func bfx_op4d; bfx_func bfx_op4e; bfx_func bfx_op4f;

bfx_func bfx_op50; bfx_func bfx_op51; bfx_func bfx_op52; bfx_func bfx_op53;
bfx_func bfx_op54; bfx_func bfx_op55; bfx_func bfx_op56; bfx_func bfx_op57;
bfx_func bfx_op58; bfx_func bfx_op59; bfx_func bfx_op5a; bfx_func bfx_op5b;
bfx_func bfx_op5c; bfx_func bfx_op5d; bfx_func bfx_op5e; bfx_func bfx_op5f;

bfx_func bfx_op60; bfx_func bfx_op61; bfx_func bfx_op62; bfx_func bfx_op63;
bfx_func bfx_op64; bfx_func bfx_op65; bfx_func bfx_op66; bfx_func bfx_op67;
bfx_func bfx_op68; bfx_func bfx_op69; bfx_func bfx_op6a; bfx_func bfx_op6b;
bfx_func bfx_op6c; bfx_func bfx_op6d; bfx_func bfx_op6e; bfx_func bfx_op6f;

bfx_func bfx_op70; bfx_func bfx_op71; bfx_func bfx_op72; bfx_func bfx_op73;
bfx_func bfx_op74; bfx_func bfx_op75; bfx_func bfx_op76; bfx_func bfx_op77;
bfx_func bfx_op78; bfx_func bfx_op79; bfx_func bfx_op7a; bfx_func bfx_op7b;
bfx_func bfx_op7c; bfx_func bfx_op7d; bfx_func bfx_op7e; bfx_func bfx_op7f;

extern bfx_func *bfx_default_op_bindings[BFX_BANK_SIZE];
extern const char *bfx_opnames[BFX_BANK_SIZE];

#endif

#ifdef __cplusplus
}
#endif
