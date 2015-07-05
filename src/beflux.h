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

struct beflux;

typedef void bfx_func(struct beflux *bfx);

typedef struct bfx_stack {
  bfx_word data[BFX_BANK_SIZE];
  bfx_word size;
} bfx_stack;

typedef struct beflux {
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

bfx_func *bfx_default_op_bindings[BFX_BANK_SIZE] = {
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

  /* NULL ... */
};

const char *bfx_opnames[BFX_BANK_SIZE] = {
/* CONTROL */
/* NUL     SOH     STX     ETX     EOT     ENQ     ACK     BEL */
  "OP00", "OP01", "OP02", "OP03", "OP04", "OP05", "OP06", "OP07",
/* BS      TAB     LF      VT      FF      CR      SO      SI  */
  "OP08", "OP09", "OP0A", "OP0B", "OP0C", "OP0D", "OP0E", "OP0F",

/* DLE     DC1     DC2     DC3     DC4     NAK     SYN     ETB */
  "OP10", "OP11", "OP12", "OP13", "OP14", "OP15", "OP16", "OP17",
/* CAN     EM      SUB     ESC     FS      GS      RS      US  */
  "OP18", "OP19", "OP1A", "OP1B", "OP1C", "OP1D", "OP1E", "OP1F",

/* PRINTABLE */
/*         !       "       #       $       %       &       '   */
  "SKIP", "NOT" , "STR" , "HOP" , "POP" , "MOD",  "GETX", "OVER",
/* (       )       *       +       ,       -       .       /   */
  "PSHF", "POPF", "MUL" , "ADD" , "PUTC", "SUB",  "PUTX", "DIV" ,

/* 0       1       2       3       4       5       6       7   */
  "V0"  , "V1"  , "V2"  , "V3"  , "V4"  , "V5"  , "V6"  , "V7"  ,
/* 8       9       :       ;       <       =       >       ?   */
  "V8"  , "V9"  , "DUP" , "COM" , "MVW" , "EQ"  , "MVE" , "AWAY",

/* @       A       B       C       D       E       F       G   */
  "REP" , "PRVP", "REV" , "CALL", "DICE", "EOF" , "FUNC", "GETP",
/* H       I       J       K       L       M       N       O   */
  "HOME", "FIN" , "JMP" , "DUPF", "LEND", "CLRS", "CLRF", "FOUT",

/* P       Q       R       S       T       U       V       W   */
  "LOAD", "QUIT", "RET" , "SETP", "TMAJ", "CURP", "NXTP", "WRAP",
/* X       Y       Z       [       \       ]       ^       _   */
  "EXEP", "CLRR", "RAND", "TRNL", "SWP" , "TRNR", "MVN" , "WEIF",

/* `       a       b       c       d       e       f       g   */
  "GT"  , "VA"  , "VB"  , "VC"  , "VD"  , "VE"  , "VF"  , "GETR",
/* h       i       j       k       l       m       n       o   */
  "BMPN", "GETS", "JREL", "ITER", "LOOP", "NIF" , "ENDL", "PUTS",

/* p       q       r       s       t       u       v       w    */
  "SWPR", "EXIT", "REVS", "SETR", "TMIN", "JOIN", "MVS" , "SIF" ,
/* x       y       z       {       |       }       ~       DEL  */
  "EXEC", "BMPS", "WAIT", "BLK" , "NSIF", "BEND", "GETC", "NOP" ,

/* EXTENDED */
  "OP80", "OP81", "OP82", "OP83", "OP84", "OP85", "OP86", "OP87",
  "OP88", "OP89", "OP8A", "OP8B", "OP8C", "OP8D", "OP8E", "OP8F",

  "OP90", "OP91", "OP92", "OP93", "OP94", "OP95", "OP96", "OP97",
  "OP98", "OP99", "OP9A", "OP9B", "OP9C", "OP9D", "OP9E", "OP9F",

  "OPA0", "OPA1", "OPA2", "OPA3", "OPA4", "OPA5", "OPA6", "OPA7",
  "OPA8", "OPA9", "OPAA", "OPAB", "OPAC", "OPAD", "OPAE", "OPAF",

  "OPB0", "OPB1", "OPB2", "OPB3", "OPB4", "OPB5", "OPB6", "OPB7",
  "OPB8", "OPB9", "OPBA", "OPBB", "OPBC", "OPBD", "OPBE", "OPBF",

  "OPC0", "OPC1", "OPC2", "OPC3", "OPC4", "OPC5", "OPC6", "OPC7",
  "OPC8", "OPC9", "OPCA", "OPCB", "OPCC", "OPCD", "OPCE", "OPCF",

  "OPD0", "OPD1", "OPD2", "OPD3", "OPD4", "OPD5", "OPD6", "OPD7",
  "OPD8", "OPD9", "OPDA", "OPDB", "OPDC", "OPDD", "OPDE", "OPDF",

  "OPE0", "OPE1", "OPE2", "OPE3", "OPE4", "OPE5", "OPE6", "OPE7",
  "OPE8", "OPE9", "OPEA", "OPEB", "OPEC", "OPED", "OPEE", "OPEF",

  "OPF0", "OPF1", "OPF2", "OPF3", "OPF4", "OPF5", "OPF6", "OPF7",
  "OPF8", "OPF9", "OPFA", "OPFB", "OPFC", "OPFD", "OPFE", "OPFF"
};

#endif

#ifdef __cplusplus
}
#endif
