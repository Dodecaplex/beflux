#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "beflux.h"

/*******************************************************************************
 * BfxStack Functions
 */
void bfx_stack_init(BfxStack *s) {
  memset(s->data, 0, BANK_SIZE);
  s->size = 0;
}

void bfx_stack_push(BfxStack *s, bfx_word value) {
  s->data[s->size++] = value;
}

bfx_word bfx_stack_pop(BfxStack *s) {
  bfx_word result = s->data[s->size - 1];
  s->data[s->size-- - 1] = 0;
  return result;
}

bfx_word bfx_stack_top(BfxStack *s) {
  return s->data[s->size - 1];
}

void bfx_stack_clear(BfxStack *s) {
  while (s->size)
    bfx_stack_pop(s);
}


/*******************************************************************************
 * Beflux Functions
 */
void bfx_init(Beflux *bfx) {
  size_t i;

  bfx->programs = (bfx_word *)calloc(BANK_SIZE, PROGRAM_SIZE * sizeof(bfx_word));
  bfx->registers = (bfx_word *)calloc(BANK_SIZE, sizeof(bfx_word));

  bfx->op_bindings = (BfxFunc *)malloc(BANK_SIZE * sizeof(BfxFunc));
  memcpy(bfx->op_bindings, bfx_default_op_bindings, BANK_SIZE * sizeof(BfxFunc));

  bfx->f_bindings = (BfxFunc *)calloc(BANK_SIZE, sizeof(BfxFunc));

  bfx->m_bindings = (BfxFunc *)malloc(BANK_SIZE * sizeof(BfxFunc));
  memcpy(bfx->m_bindings, bfx_default_m_bindings, BANK_SIZE * sizeof(BfxFunc));

  bfx->pre_update = NULL;
  bfx->post_update = NULL;

  for (i = 0; i < BANK_SIZE; ++i) {
    bfx_stack_init(bfx->frames + i);
  }

  bfx_stack_init(&bfx->calls_x);
  bfx_stack_init(&bfx->calls_y);

  bfx->current_program = 0;
  bfx->current_frame = 0;

  bfx->mode = BFX_MODE_HALT;
  bfx->status = 0;
  bfx->value = 0;
  bfx->value_width = 0;

  bfx->t_minor = 0;
  bfx->t_major = 0;
  bfx->loop_count = 0;
  bfx->wrap_offset = 0;

  bfx->tick = 0;
  bfx->timeout = (size_t)(-1);

  bfx->in = stdin;
  bfx->out = stdout;
  bfx->err = stderr;

  bfx_ip_reset(bfx);

  srand(time(NULL));
}

void bfx_free(Beflux *bfx) {
  free(bfx->programs);
  free(bfx->registers);
  free(bfx->op_bindings);
  free(bfx->f_bindings);
  free(bfx->m_bindings);
  bfx->mode = BFX_MODE_FREED;
}


/* I/O */
void bfx_load(Beflux *bfx, bfx_word prog, const char *filename) {
  size_t linewidth = PROGRAM_WIDTH + 1;
  char filename_ext[BANK_SIZE];
  FILE *fin;

  sprintf(filename_ext, "%s.bfx", filename);
  fin = fopen(filename_ext, "r");

  if (fin != NULL) {
    char buffer[linewidth];
    size_t i, j;

    memset(bfx->programs + prog, ' ', PROGRAM_SIZE);

    if (fgets(buffer, linewidth, fin) != NULL) {
      for (j = 0; j < PROGRAM_HEIGHT; ++j) {
        size_t len;

        if (fgets(buffer, linewidth, fin) == NULL) break;
        len = strlen(buffer);

        for (i = 0; i < PROGRAM_WIDTH; ++i) {
          bfx_word c;
          if (i >= len) {
            c = ' ';
          }
          else {
            c = buffer[i];
            if (c == '\n')
              c = ' ';
          }
          bfx_program_set(bfx, prog, i, j, c);
        }
      }
    }
    fclose(fin);
  }
  else {
    fprintf(stderr, "Failed to load program from \"%s.bfx\"", filename);
  }
}

void bfx_save(Beflux *bfx, bfx_word prog, const char *filename) {

}

void bfx_read(Beflux *bfx, bfx_word prog, const bfx_word *src, size_t size) {
  memcpy(bfx->programs + prog, src, size);
}

void bfx_write(Beflux *bfx, bfx_word prog, bfx_word *dst, size_t size) {
  memcpy(dst, bfx->programs + prog, size);
}

void bfx_warning(Beflux *bfx, const char *message) {
  fprintf(bfx->err, "Warning: op%02x('%c') at %02x::%02x%02x\n  %s\n\n",
          bfx_ip_get_op(bfx), bfx_ip_get_op(bfx),
          bfx->current_program, bfx->ip.y, bfx->ip.x,
          message);
}

void bfx_error(Beflux *bfx, const char *message) {
  fprintf(bfx->err, "Error: op%02x('%c') at %02x::%02x%02x\n  %s\nExiting.\n\n",
          bfx_ip_get_op(bfx), bfx_ip_get_op(bfx),
          bfx->current_program, bfx->ip.y, bfx->ip.x,
          message);
  bfx->status = -1;
  bfx->mode = BFX_MODE_HALT;
}


/* Stack Manipulation */
void bfx_push(Beflux *bfx, bfx_word value) {
  bfx_stack_push(bfx->frames + bfx->current_frame, value);
}

bfx_word bfx_pop(Beflux *bfx) {
  return bfx_stack_pop(bfx->frames + bfx->current_frame);
}

bfx_word bfx_top(Beflux *bfx) {
  return bfx_stack_top(bfx->frames + bfx->current_frame);
}

void bfx_clear(Beflux *bfx) {
  bfx_stack_clear(bfx->frames + bfx->current_frame);
}

/* Execution */
bfx_word bfx_run(Beflux *bfx) {
  if (bfx->mode == BFX_MODE_HALT) {
    bfx->mode = BFX_MODE_NORMAL;
    while (bfx->mode) { /* MAIN LOOP */
      if (bfx->pre_update != NULL)
        bfx->pre_update(bfx);

      bfx_update(bfx);

      if (bfx->post_update != NULL)
        bfx->post_update(bfx);
    }
  }
  else if (bfx->mode == BFX_MODE_FREED) {
    bfx_error(bfx, "Interpreter has already been freed.");
  }
  else {
    bfx_error(bfx, "Initial interpreter mode BFX_MODE_HALT "
                   "is required to begin execution.");
  }
}

void bfx_update(Beflux *bfx) {
  bfx_eval(bfx, bfx_ip_get_op(bfx));
  bfx_ip_advance(bfx);
  ++bfx->tick;
  if (bfx->timeout && bfx->tick >= bfx->timeout) {
    bfx_error(bfx, "Program timeout.");
  }
}

void bfx_eval(Beflux *bfx, bfx_word op) {
  switch (bfx->mode) {
    default:
    case BFX_MODE_NORMAL: {
      BfxFunc func = bfx->op_bindings[op];
      if (func == NULL) {
        bfx_error(bfx, "Undefined opcode.");
      }
      else {
        func(bfx);
      }
    } break;
    case BFX_MODE_STRING:
      if (op == '"') {
        bfx->mode = BFX_MODE_NORMAL;
      }
      else if (op == '\\') {
        bfx->mode = BFX_MODE_STRING_ESC;
      }
      else {
        bfx_push(bfx, op);
      }
      break;
    case BFX_MODE_STRING_ESC: {
      switch (op) {
        case 'a': bfx_push(bfx, '\a'); break;
        case 'b': bfx_push(bfx, '\b'); break;
        case 'f': bfx_push(bfx, '\f'); break;
        case 'n': bfx_push(bfx, '\n'); break;
        case 'r': bfx_push(bfx, '\r'); break;
        case 't': bfx_push(bfx, '\t'); break;
        case 'v': bfx_push(bfx, '\v'); break;
        default: bfx_push(bfx, op); break;
      }
      bfx->mode = BFX_MODE_STRING;
      break;
    }
  }
}


/* Program Manipulatiion */
bfx_word bfx_program_get(Beflux *bfx, bfx_word prog, bfx_word x, bfx_word y) {
  return *(bfx->programs + x + PROGRAM_WIDTH * y + BANK_SIZE * prog);
}

void bfx_program_set(Beflux *bfx, bfx_word prog, bfx_word x, bfx_word y, bfx_word value) {
  *(bfx->programs + x + PROGRAM_WIDTH * y + BANK_SIZE * prog) = value;
}


/* IP Manipulatiion */
void bfx_ip_reset(Beflux *bfx) {
  bfx->ip.x = 0;
  bfx->ip.y = 0;
  bfx->ip.dir = BFX_IP_E;
}

void bfx_ip_advance(Beflux *bfx) {
  if (bfx->ip.wait) {
    --bfx->ip.wait;
  }
  else {
    if (bfx->wrap_offset) {
      switch (bfx->ip.dir) {
        case BFX_IP_E:
          bfx->ip.y += bfx->wrap_offset * (bfx->ip.x == 0xFF);
          ++bfx->ip.x; break;
        case BFX_IP_N: --bfx->ip.y; break;
        case BFX_IP_W:
          bfx->ip.y -= bfx->wrap_offset * (bfx->ip.x == 0x00);
          --bfx->ip.x; break;
        case BFX_IP_S: ++bfx->ip.y; break;
        default: break;
      }
    }
    else {
      switch (bfx->ip.dir) {
        case BFX_IP_E: ++bfx->ip.x; break;
        case BFX_IP_N: --bfx->ip.y; break;
        case BFX_IP_W: --bfx->ip.x; break;
        case BFX_IP_S: ++bfx->ip.y; break;
        default: break;
      }
    }
  }
}

bfx_word bfx_ip_get_op(Beflux *bfx) {
  return bfx_program_get(bfx, bfx->current_program, bfx->ip.x, bfx->ip.y);
}


/* Static Functions */
void bfx_get_digit(Beflux *bfx, bfx_word digit) {
  bfx->value <<= 4;
  bfx->value |= digit;
  if (bfx->value_width) {
    bfx_push(bfx, bfx->value);
    bfx->value = 0;
    bfx->value_width = 0;
  }
  else {
    ++bfx->value_width;
  }
}


/*******************************************************************************
 * Beflux Operators
 */
/* 0x20 */
void bfx_op20(Beflux *bfx) { /* ' ' - SKIP */
  size_t i = 0;
  size_t limit = bfx->wrap_offset == 0 ? PROGRAM_WIDTH : PROGRAM_SIZE;
  while (bfx_ip_get_op(bfx) == ' ') {
    bfx_ip_advance(bfx);
    if (i ++ > limit) {
      bfx_error(bfx, "Non-halting empty loop detected.");
      break;
    }
  }
  bfx->ip.wait = 1;
}

void bfx_op21(Beflux *bfx) { /* '!' - NOT */
  bfx_push(bfx, !bfx_pop(bfx));
}

void bfx_op22(Beflux *bfx) { /* '"' - STRING */
  bfx_push(bfx, '\0');
  bfx->mode = BFX_MODE_STRING;
}

void bfx_op23(Beflux *bfx) { /* '#' - TRAMPOLINE */
  bfx_ip_advance(bfx);
}

void bfx_op24(Beflux *bfx) { /* '$' - POP */
  bfx_pop(bfx);
}

void bfx_op25(Beflux *bfx) { /* '%' - MOD */
  bfx_push(bfx, bfx_pop(bfx) % bfx_pop(bfx));
}

void bfx_op26(Beflux *bfx) { /* '&' - SCAN HEX */
  if (bfx->in == NULL) {
    bfx_warning(bfx, "No input file.");
    bfx_push(bfx, 0);
  }
  else {
    bfx_word value;

    fprintf(bfx->err, "0x");

    long status = fscanf(bfx->in, "%02x", &value);
    if (status == 0) {
      bfx_error(bfx, "Invalid input.");
    }
    else if (status == EOF) {
      bfx_warning(bfx, "End of input stream.");
    }
    else {
      fprintf(bfx->err, "VALUE: %02x\n", value);
      bfx_push(bfx, value);
    }
  }
}

void bfx_op27(Beflux *bfx) { /* '\' - OVER */
  bfx_word a = bfx_pop(bfx);
  bfx_word b = bfx_top(bfx);
  bfx_push(bfx, a);
  bfx_push(bfx, b);
}

void bfx_op28(Beflux *bfx) { /* '(' - PUSH FRAME */
  ++bfx->current_frame;
}

void bfx_op29(Beflux *bfx) { /* ')' - POP FRAME */
  bfx_clear(bfx);
  --bfx->current_frame;
}

void bfx_op2a(Beflux *bfx) { /* '*' - MUL */
  bfx_push(bfx, bfx_pop(bfx) * bfx_pop(bfx));
}

void bfx_op2b(Beflux *bfx) { /* '+' - ADD */
  bfx_push(bfx, bfx_pop(bfx) + bfx_pop(bfx));
}

void bfx_op2c(Beflux *bfx) { /* ',' - PRINT ASCII */
  fputc(bfx_pop(bfx), bfx->out);
}

void bfx_op2d(Beflux *bfx) { /* '-' - SUB */
  bfx_push(bfx, bfx_pop(bfx) - bfx_pop(bfx));
}

void bfx_op2e(Beflux *bfx) { /* '.' - PRINT HEX */
  fprintf(bfx->out, "%02x", bfx_pop(bfx));
}

void bfx_op2f(Beflux *bfx) { /* '/' - DIV */
  bfx_push(bfx, bfx_pop(bfx) / bfx_pop(bfx));
}

/* 0x30 */
void bfx_op30(Beflux *bfx) { /* '0' */ bfx_get_digit(bfx, 0); }
void bfx_op31(Beflux *bfx) { /* '1' */ bfx_get_digit(bfx, 1); }
void bfx_op32(Beflux *bfx) { /* '2' */ bfx_get_digit(bfx, 2); }
void bfx_op33(Beflux *bfx) { /* '3' */ bfx_get_digit(bfx, 3); }
void bfx_op34(Beflux *bfx) { /* '4' */ bfx_get_digit(bfx, 4); }
void bfx_op35(Beflux *bfx) { /* '5' */ bfx_get_digit(bfx, 5); }
void bfx_op36(Beflux *bfx) { /* '6' */ bfx_get_digit(bfx, 6); }
void bfx_op37(Beflux *bfx) { /* '7' */ bfx_get_digit(bfx, 7); }
void bfx_op38(Beflux *bfx) { /* '8' */ bfx_get_digit(bfx, 8); }
void bfx_op39(Beflux *bfx) { /* '9' */ bfx_get_digit(bfx, 9); }

void bfx_op3a(Beflux *bfx) { /* ':' - DUP */
  bfx_push(bfx, bfx_top(bfx));
}

void bfx_op3b(Beflux *bfx) { /* ';' - COMMENT */
  size_t i = 0;
  size_t limit = bfx->wrap_offset == 0 ? PROGRAM_WIDTH - 3 : PROGRAM_SIZE;
  bfx_ip_advance(bfx);
  while (bfx_ip_get_op(bfx) != ';') {
    bfx_ip_advance(bfx);
    if (i ++ > limit) {
      bfx_error(bfx, "Non-halting comment loop detected.");
      break;
    }
  }
}

void bfx_op3c(Beflux *bfx) { /* '<' - WEST */
  bfx->ip.dir = BFX_IP_W;
}

void bfx_op3d(Beflux *bfx) { /* '=' - EQ */
  bfx_push(bfx, bfx_pop(bfx) == bfx_pop(bfx));
}

void bfx_op3e(Beflux *bfx) { /* '>' - EAST */
  bfx->ip.dir = BFX_IP_E;
}

void bfx_op3f(Beflux *bfx) { /* '?' - AWAY */
  bfx->ip.dir = (rand() % 4) << 7;
}

/* 0x40 */
void bfx_op40(Beflux *bfx) { /* '@' - RESET */
  bfx_ip_reset(bfx);
  bfx->ip.wait = 1;
}

void bfx_op41(Beflux *bfx) { /* 'A' TODO */
}

void bfx_op42(Beflux *bfx) { /* 'B' - BOUNCE */
  bfx->ip.dir += BFX_IP_TURN_B;
}

void bfx_op43(Beflux *bfx) { /* 'C' - CALL */
  bfx_stack_push(&bfx->calls_x, bfx->ip.x);
  bfx_stack_push(&bfx->calls_y, bfx->ip.y);
  bfx_op4a(bfx); /* 'J' */
}

void bfx_op44(Beflux *bfx) { /* 'D' - DICE */
  bfx_word max = bfx_pop(bfx);
  bfx_word min = bfx_pop(bfx);
  bfx_push(bfx, (rand() % (max - min)) + min);
}

void bfx_op45(Beflux *bfx) { /* 'E' - EOF */
  if (bfx->in == NULL) {
    bfx_push(bfx, 0xff);
  }
  else {
    bfx_push(bfx, !!feof(bfx->in));
  }
}

void bfx_op46(Beflux *bfx) { /* 'F' - FUNCTION */
  bfx->f_bindings[bfx_pop(bfx)](bfx);
}

void bfx_op47(Beflux *bfx) { /* 'G' - PROGRAM GET */
  bfx_word x = bfx_pop(bfx);
  bfx_word y = bfx_pop(bfx);
  bfx_program_get(bfx, bfx_pop(bfx), x, y);
}

void bfx_op48(Beflux *bfx) { /* 'H' TODO */
}

void bfx_op49(Beflux *bfx) { /* 'I' - INPUT FILE TODO */
  bfx_word c = bfx_top(bfx);
  if (c == 0x00) {
    bfx_pop(bfx);
    bfx->in = NULL;;
  }
  else if (c == 0xff) {
    bfx_pop(bfx);
    bfx->in = stdin;
  }
  else {
    if (bfx->in != NULL && bfx->in != stdin) {
      fclose(bfx->in);
    }
    /* OPEN INPUT */
  }
}

void bfx_op4a(Beflux *bfx) { /* 'J' - JUMP */
  bfx->ip.x = bfx_pop(bfx);
  bfx->ip.y = bfx_pop(bfx);
  bfx->ip.wait = 1;
}

void bfx_op4b(Beflux *bfx) { /* 'K' TODO */

}

void bfx_op4c(Beflux *bfx) { /* 'L' - LOOP COUNT */
  bfx_push(bfx, bfx->loop_count);
}

void bfx_op4d(Beflux *bfx) { /* 'M' - MATH */
  bfx->m_bindings[bfx_pop(bfx)](bfx);
}

void bfx_op4e(Beflux *bfx) { /* 'N' - MEMORY CLEAR */
  do {
    bfx_clear(bfx);
  } while (bfx->current_frame--);
}

void bfx_op4f(Beflux *bfx) { /* 'O' - OUTPUT FILE */
  bfx_word c = bfx_top(bfx);
  if (c == 0x00) {
    bfx_pop(bfx);
    bfx->out = NULL;;
  }
  else if (c == 0xff) {
    bfx_pop(bfx);
    bfx->out = stdout;
  }
  else {
    if (bfx->out != NULL && bfx->out != stdout) {
      fclose(bfx->in);
    }
    /* OPEN OUTPUT */
  }
}

/* 0x50 */
void bfx_op50(Beflux *bfx) { /* 'P' TODO */
}

void bfx_op51(Beflux *bfx) { /* 'Q' - QUIT */
  bfx_ip_reset(bfx);
  bfx->status = bfx->t_major = bfx->t_minor = 0;
  bfx->mode = BFX_MODE_HALT;
}

void bfx_op52(Beflux *bfx) { /* 'R' - RETURN */
  bfx_push(bfx, bfx_stack_pop(&bfx->calls_y));
  bfx_push(bfx, bfx_stack_pop(&bfx->calls_x));
  bfx_op4a(bfx); /* J */
  bfx_ip_advance(bfx);
}

void bfx_op53(Beflux *bfx) { /* 'S' - PROGRAM SET */
  bfx_word x = bfx_pop(bfx);
  bfx_word y = bfx_pop(bfx);
  bfx_program_set(bfx, bfx_pop(bfx), x, y, bfx_pop(bfx));
}

void bfx_op54(Beflux *bfx) { /* 'T' - MAJOR TIME */
  bfx_push(bfx, bfx->t_major);
}

void bfx_op55(Beflux *bfx) { /* 'U' CURRENT PROGRAM */
  bfx_push(bfx, bfx->current_program);
}

void bfx_op56(Beflux *bfx) { /* 'V' TODO */
}

void bfx_op57(Beflux *bfx) { /* 'W' - WRAP */
  bfx->wrap_offset = bfx_pop(bfx);
}

void bfx_op58(Beflux *bfx) { /* 'X' - PROGRAM EXEC */
  bfx->current_program = bfx_pop(bfx);
  bfx_op4a(bfx); /* 'J' */
}

void bfx_op59(Beflux *bfx) { /* 'Y' TODO */
}

void bfx_op5a(Beflux *bfx) { /* 'Z' RANDOM */
  bfx_push(bfx, rand());
}

void bfx_op5b(Beflux *bfx) { /* '[' - LEFT */
  bfx->ip.dir += BFX_IP_TURN_L;
}

void bfx_op5c(Beflux *bfx) { /* '\' - SWAP */
  bfx_word a = bfx_pop(bfx);
  bfx_word b = bfx_pop(bfx);
  bfx_push(bfx, a);
  bfx_push(bfx, b);
}

void bfx_op5d(Beflux *bfx) { /* ']' - RIGHT */
  bfx->ip.dir += BFX_IP_TURN_R;
}

void bfx_op5e(Beflux *bfx) { /* '^' - NORTH */
  bfx->ip.dir = BFX_IP_N;
}

void bfx_op5f(Beflux *bfx) { /* '_' - WEST / EAST IF */
  if (bfx_pop(bfx))
    bfx_op3c(bfx); /* '<' */
  else
    bfx_op3e(bfx); /* '>' */
}

/* 0x60 */
void bfx_op60(Beflux *bfx) { /* '`' - GREATER */
  bfx_push(bfx, bfx_pop(bfx) > bfx_pop(bfx));
}

void bfx_op61(Beflux *bfx) { /* 'a' */ bfx_get_digit(bfx, 10); }
void bfx_op62(Beflux *bfx) { /* 'b' */ bfx_get_digit(bfx, 11); }
void bfx_op63(Beflux *bfx) { /* 'c' */ bfx_get_digit(bfx, 12); }
void bfx_op64(Beflux *bfx) { /* 'd' */ bfx_get_digit(bfx, 13); }
void bfx_op65(Beflux *bfx) { /* 'e' */ bfx_get_digit(bfx, 14); }
void bfx_op66(Beflux *bfx) { /* 'f' */ bfx_get_digit(bfx, 15); }

void bfx_op67(Beflux *bfx) { /* 'g' - REGISTER GET */
  bfx_push(bfx, bfx->registers[bfx_pop(bfx)]);
}

void bfx_op68(Beflux *bfx) { /* 'h' TODO */
}

void bfx_op69(Beflux *bfx) { /* 'i' INPUT STRING */
}

void bfx_op6a(Beflux *bfx) { /* 'j' - RELATIVE JUMP */
  bfx_word dx = bfx_pop(bfx);
  bfx_word dy = bfx_pop(bfx);
  bfx_word dir = bfx->ip.dir;

  bfx->ip.dir = BFX_IP_E;
  while (dx--) {
    bfx_ip_advance(bfx);
  }

  bfx->ip.dir = BFX_IP_S;
  while (dy--) {
    bfx_ip_advance(bfx);
  }

  bfx->ip.dir = dir;
  bfx->ip.wait = 1;
}

void bfx_op6b(Beflux *bfx) { /* 'k' - ITER */
  bfx_ip_advance(bfx);
  bfx->ip.wait = bfx_pop(bfx);
}

void bfx_op6c(Beflux *bfx) { /* 'l' - RESET LOOP */
  bfx->loop_count = 0;
}

void bfx_op6d(Beflux *bfx) { /* 'm' - NORTH IF */
  if (bfx_pop(bfx))
    bfx_op5e(bfx); /* '^' */
}

void bfx_op6e(Beflux *bfx) { /* 'n' FRAME CLEAR */
  bfx_clear(bfx);
}

void bfx_op6f(Beflux *bfx) { /* 'o' - OUTPUT STRING */
  bfx_op72(bfx); /* 'r' */
  while (bfx_top(bfx)) {
    bfx_op2c(bfx); /* ',' */
  }
}

/* 0x70 */
void bfx_op70(Beflux *bfx) { /* 'p' TODO */
}

void bfx_op71(Beflux *bfx) { /* 'q' - STATUS QUIT */
  bfx_ip_reset(bfx);
  bfx->status = bfx_pop(bfx);
  bfx->t_major = bfx->t_minor = 0;
  bfx->mode = BFX_MODE_HALT;
}

void bfx_op72(Beflux *bfx) { /* 'r' - REVERSE STRING */
  bfx_word buffer[BANK_SIZE];

  bfx_word i;
  bfx_word count = 1;

  buffer[0] = '\0';
  while (bfx_top(bfx)) {
    buffer[count++] = bfx_pop(bfx);
  }

  for (i = 0; i < count; ++i) {
    bfx_push(bfx, buffer[i]);
  }
}

void bfx_op73(Beflux *bfx) { /* 's' - REGISTER SET */
  bfx->registers[bfx_pop(bfx)] = bfx_pop(bfx);
}

void bfx_op74(Beflux *bfx) { /* 't' - MINOR TIME */
  bfx_push(bfx, bfx->t_minor);
}

void bfx_op75(Beflux *bfx) { /* 'u' STRING JOIN */
  BfxStack tmp;

  while (bfx_top(bfx)) {
    bfx_stack_push(&tmp, bfx_pop(bfx));
  }
  bfx_pop(bfx);

  while (tmp.size) {
    bfx_push(bfx, bfx_stack_pop(&tmp));
  }

}

void bfx_op76(Beflux *bfx) { /* 'v' - SOUTH */
  bfx->ip.dir = BFX_IP_S;
}

void bfx_op77(Beflux *bfx) { /* 'w' - SOUTH IF */
  if (bfx_pop(bfx))
    bfx_op76(bfx); /* 'v' */
}

void bfx_op78(Beflux *bfx) { /* 'x' - EXEC OP */
  bfx_eval(bfx, bfx_pop(bfx));
}

void bfx_op79(Beflux *bfx) { /* 'y' TODO */
}

void bfx_op7a(Beflux *bfx) { /* 'z' TODO */
}

void bfx_op7b(Beflux *bfx) { /* '{' - BLOCK BEGIN */
  bfx_word depth = 1;
  if (!bfx_pop(bfx)) { /* Skip to matching BLOCK END */
    bfx_word c;
    size_t i = 0;
    size_t limit = bfx->wrap_offset == 0 ? PROGRAM_WIDTH - 3 : PROGRAM_SIZE;
    while (depth) {
      bfx_ip_advance(bfx);
      c = bfx_ip_get_op(bfx);
      if (c == '}') {
        --depth;
      }
      else if (c == '{') {
        ++depth;
      }

      if (i++ > limit || depth == -1) {
        bfx_error(bfx, "Non-halting block loop detected.");
      }
    }
  }
}

void bfx_op7c(Beflux *bfx) { /* '|' - NORTH / SOUTH IF */
  if (bfx_pop(bfx))
    bfx_op5e(bfx); /* '^' */
  else
    bfx_op76(bfx); /* 'v' */
}

void bfx_op7d(Beflux *bfx) { /* '}' - BLOCK END */
}

void bfx_op7e(Beflux *bfx) { /* '~' - SCAN ASCII */
  bfx_push(bfx, fgetc(bfx->in));
}

void bfx_op7f(Beflux *bfx) { /* NOP */
}

void bfx_m00(Beflux *bfx) { /* */
}
void bfx_m01(Beflux *bfx) { /* */
}
void bfx_m02(Beflux *bfx) { /* */
}
void bfx_m03(Beflux *bfx) { /* */
}
void bfx_m04(Beflux *bfx) { /* */
}
void bfx_m05(Beflux *bfx) { /* */
}
void bfx_m06(Beflux *bfx) { /* */
}
void bfx_m07(Beflux *bfx) { /* */
}
void bfx_m08(Beflux *bfx) { /* */
}
void bfx_m09(Beflux *bfx) { /* */
}
void bfx_m0a(Beflux *bfx) { /* */
}
void bfx_m0b(Beflux *bfx) { /* */
}
void bfx_m0c(Beflux *bfx) { /* */
}
void bfx_m0d(Beflux *bfx) { /* */
}
void bfx_m0e(Beflux *bfx) { /* */
}
void bfx_m0f(Beflux *bfx) { /* */
}

int main(int argc, char *argv[]) {
  int status = 0;
  if (argc == 1) {
    fprintf(stderr, ":: BEFLUX ::\nNo program specified.\n");
  }
  else {
    size_t i, j;
    Beflux b;
    bfx_init(&b);
    bfx_load(&b, 0, argv[1]);
    /*
    for (j = 0; j < 8; ++j) {
      for (i = 0; i < 16; ++i){
        printf("%02x ", bfx_program_get(&b, 0, i, j));
      }
      printf("\n");
    }
    */
    bfx_run(&b);
    bfx_free(&b);
  }
  return status;
}

