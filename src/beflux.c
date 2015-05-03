/**
 * @file beflux.c
 * @date 4/20/2015
 * @author Tony Chiodo (http://dodecaplex.net)
 */

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

#include "beflux.h"

/*******************************************************************************
 * bfx_stack Functions
 */

/**
 * \brief Prepares a Beflux Stack for use.
 */
void bfx_stack_init(bfx_stack *s) {
  memset(s->data, 0, BFX_BANK_SIZE);
  s->size = 0;
}

/**
 * \brief Pushes a word onto the stack.
 */
void bfx_stack_push(bfx_stack *s, bfx_word value) {
  s->data[s->size++] = value;
}

/**
 * \brief Pops a word from the stack.
 */
bfx_word bfx_stack_pop(bfx_stack *s) {
  bfx_word result = s->data[s->size - 1];
  s->data[s->size-- - 1] = 0;
  return result;
}

/**
 * \brief Reads a word from the top of the stack without popping it.
 */
bfx_word bfx_stack_top(bfx_stack *s) {
  return s->data[s->size - 1];
}

/**
 * \brief Pops words from the stack until it is empty.
 */
void bfx_stack_clear(bfx_stack *s) {
  while (s->size)
    bfx_stack_pop(s);
}


/*******************************************************************************
 * Beflux Functions
 */

/**
 * \brief Creates and initializes a new Beflux interpreter.
 * \return A pointer to the newly created interpreter.
 */
beflux *bfx_create(void) {
  beflux *bfx = (beflux *) malloc(sizeof(beflux));
  bfx_init(bfx);
  return bfx;
}

/**
 * \brief Allocates memory for the interpreter, and initializes its members.
 */
void bfx_init(beflux *bfx) {
  size_t i;

  bfx->programs = (bfx_word *)calloc(BFX_BANK_SIZE, BFX_PROGRAM_SIZE * sizeof(bfx_word));
  bfx->registers = (bfx_word *)calloc(BFX_BANK_SIZE, sizeof(bfx_word));

  bfx->op_bindings = (bfx_func *)malloc(BFX_BANK_SIZE * sizeof(bfx_func));
  memcpy(bfx->op_bindings, bfx_default_op_bindings, BFX_BANK_SIZE * sizeof(bfx_func));

  bfx->f_bindings = (bfx_func *)calloc(BFX_BANK_SIZE, sizeof(bfx_func));

  bfx->m_bindings = (bfx_func *)malloc(BFX_BANK_SIZE * sizeof(bfx_func));
  memcpy(bfx->m_bindings, bfx_default_m_bindings, BFX_BANK_SIZE * sizeof(bfx_func));

  bfx->pre_update = NULL;
  bfx->post_update = NULL;

  for (i = 0; i < BFX_BANK_SIZE; ++i) {
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

/**
 * \brief Frees the interpreters members, but not the interpreter itself.
 */
void bfx_free(beflux *bfx) {
  free(bfx->programs);
  bfx->programs = NULL;
  free(bfx->registers);
  bfx->registers = NULL;
  free(bfx->op_bindings);
  bfx->op_bindings = NULL;
  free(bfx->f_bindings);
  bfx->f_bindings = NULL;
  free(bfx->m_bindings);
  bfx->m_bindings = NULL;
  bfx->mode = BFX_MODE_FREED;
}

/**
 * \brief Frees the interpreter and its members.
 */
void bfx_destroy(beflux *bfx) {
  bfx_free(bfx);
  free(bfx);
}


/* I/O */
/**
 * \brief Loads a source file into the interpreter.
 * \param prog The index of the program.
 * \param filename C string containing a path to the source file.
 */
void bfx_load(beflux *bfx, bfx_word prog, const char *filename) {
  size_t linewidth = BFX_PROGRAM_WIDTH + 1;
  char filename_ext[BFX_BANK_SIZE];
  FILE *fin;

  sprintf(filename_ext, "%s.bfx", filename);
  fin = fopen(filename_ext, "r");

  if (fin != NULL) {
    char buffer[linewidth];
    size_t i, j;

    memset(bfx->programs + BFX_PROGRAM_SIZE * prog, 'Q', BFX_PROGRAM_SIZE);

    if (fgets(buffer, linewidth, fin) != NULL) {
      char msg[BFX_BANK_SIZE];
      sprintf(msg, "Loading program from \"%s\"", filename_ext);
      bfx_note(bfx, msg);

      for (j = 0; j < BFX_PROGRAM_HEIGHT; ++j) {
        size_t len;

        if (fgets(buffer, linewidth, fin) == NULL) break;
        len = strlen(buffer);

        for (i = 0; i < BFX_PROGRAM_WIDTH; ++i) {
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
    char msg[BFX_BANK_SIZE];
    sprintf(msg, "Failed to load program from \"%s\"", filename_ext);
    bfx_error(bfx, msg);
  }
}

/**
 * \brief Writes the contents of a program in interpreter memory to a file.
 * \param prog The index of the program.
 * \param filename C string containing a path to the destination file.
 */
void bfx_save(beflux *bfx, bfx_word prog, const char *filename) {
 // TODO
}

/**
 * \brief Reads an array of words into the interpreter as a program.
 * \param prog The index of the program.
 * \param src A pointer to the source array.
 * \param size The number of words to read.
 */
void bfx_read(beflux *bfx, bfx_word prog, const bfx_word *src, size_t size) {
  memcpy(bfx->programs + prog, src, size);
}

/**
 * \brief Writes the contents of a program in interpreter memory to an array.
 * \param prog The index of the program.
 * \param dst A pointer to the destination array.
 * \param size The number of words to write.
 */
void bfx_write(beflux *bfx, bfx_word prog, bfx_word *dst, size_t size) {
  memcpy(dst, bfx->programs + prog, size);
}

/**
 * \brief Issues a notification through the interpreter's error file.
 * \param message C string containing the warning message.
 */
void bfx_note(beflux *bfx, const char *message) {
  fprintf(bfx->err, "Note: op%02x('%c') at %02x::%02x%02x\n  %s\n\n",
          bfx_ip_get_op(bfx), bfx_ip_get_op(bfx),
          bfx->current_program, bfx->ip.y, bfx->ip.x,
          message);
}

/**
 * \brief Issues a warning through the interpreter's error file.
 * \param message C string containing the warning message.
 */
void bfx_warning(beflux *bfx, const char *message) {
  fprintf(bfx->err, "Warning: op%02x('%c') at %02x::%02x%02x\n  %s\n\n",
          bfx_ip_get_op(bfx), bfx_ip_get_op(bfx),
          bfx->current_program, bfx->ip.y, bfx->ip.x,
          message);
}

/**
 * \brief Issues an error message through the interpreter's error file, and
 *        terminates the current program.
 * \param message C string containing the error message.
 */
void bfx_error(beflux *bfx, const char *message) {
  fprintf(bfx->err, "Error: op%02x('%c') at %02x::%02x%02x\n  %s\nExiting.\n\n",
          bfx_ip_get_op(bfx), bfx_ip_get_op(bfx),
          bfx->current_program, bfx->ip.y, bfx->ip.x,
          message);
  bfx->status = BFX_WORD_MAX;
  bfx->mode = BFX_MODE_HALT;
}


/* Stack Manipulation */
/**
 * \brief Pushes a word onto the interpreter's current stack frame.
 */
void bfx_push(beflux *bfx, bfx_word value) {
  bfx_stack_push(bfx->frames + bfx->current_frame, value);
}

/**
 * \brief Pops a word from the interpreter's current stack frame.
 */
bfx_word bfx_pop(beflux *bfx) {
  return bfx_stack_pop(bfx->frames + bfx->current_frame);
}

/**
 * \brief Reads a word from the top of the interpreter's current stack frame.
 */
bfx_word bfx_top(beflux *bfx) {
  return bfx_stack_top(bfx->frames + bfx->current_frame);
}

/**
 * \brief Pops words from the interpreter's current stack frame
 *        until it is empty.
 */
void bfx_clear(beflux *bfx) {
  bfx_stack_clear(bfx->frames + bfx->current_frame);
}

/* Execution */
/**
 * \brief Enters the interpreter's main loop.
 * \return The interprete's exit status.
 */
bfx_word bfx_run(beflux *bfx) {
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

/**
 * \brief Updates the interpreter's internal state.
 */
void bfx_update(beflux *bfx) {
  bfx_eval(bfx, bfx_ip_get_op(bfx));
  bfx_ip_advance(bfx);
  ++bfx->tick;
  if (bfx->timeout && bfx->tick >= bfx->timeout) {
    bfx_error(bfx, "Program timeout.");
  }
}

/**
 * \brief Evaluates a word as a beflux opcode.
 * \param op The opcode to evaluate.
 */
void bfx_eval(beflux *bfx, bfx_word op) {
  switch (bfx->mode) {
    default:
    case BFX_MODE_NORMAL: {
      bfx_func func = bfx->op_bindings[op];
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
/**
 * \brief Reads a word from a program stored in the interpreter's memory.
 * \param prog The index of the program.
 */
bfx_word bfx_program_get(beflux *bfx, bfx_word prog, bfx_word x, bfx_word y) {
  return *(bfx->programs + x + BFX_PROGRAM_WIDTH * y + BFX_BANK_SIZE * prog);
}

/**
 * \brief Writes a word to a program stored in the interpreter's memory.
 * \param prog The index of the program.
 */
void bfx_program_set(beflux *bfx, bfx_word prog, bfx_word x, bfx_word y, bfx_word value) {
  *(bfx->programs + x + BFX_PROGRAM_WIDTH * y + BFX_BANK_SIZE * prog) = value;
}


/* IP Manipulatiion */
/**
 * \brief Resets the interpreter's instruction pointer to the default state.
 */
void bfx_ip_reset(beflux *bfx) {
  bfx->ip.x = 0;
  bfx->ip.y = 0;
  bfx->ip.dir = BFX_IP_E;
}

/**
 * \brief Moves the interpreter's instruction pointer forward one step.
 */
void bfx_ip_advance(beflux *bfx) {
  if (bfx->ip.wait) {
    --bfx->ip.wait;
  }
  else {
    if (bfx->wrap_offset) {
      switch (bfx->ip.dir) {
        case BFX_IP_E:
          bfx->ip.y += bfx->wrap_offset * (bfx->ip.x == BFX_WORD_MAX);
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

/**
 * \brief Reads the word at the position of the interpreter's
 *        instruction pointer.
 */
bfx_word bfx_ip_get_op(beflux *bfx) {
  return bfx_program_get(bfx, bfx->current_program, bfx->ip.x, bfx->ip.y);
}


/* Utility Functions */
/**
 * \brief Constructs a literal word value one digit at a time.
 */
void bfx_get_digit(beflux *bfx, bfx_word digit) {
  bfx->value <<= 4;
  bfx->value |= digit;
  if (bfx->value_width == sizeof(bfx_word)) {
    bfx_push(bfx, bfx->value);
    bfx->value = 0;
    bfx->value_width = 0;
  }
  else {
    ++bfx->value_width;
  }
}

bfx_word bfx_degtobam(double degrees) {
  return (bfx_word) round(0x80 * degrees / 180.0);
}

bfx_word bfx_radtobam(double radians) {
  return (bfx_word) round(0x80 * radians / M_PI);
}

double bfx_bamtorad(bfx_word bams) {
  return M_PI * bams / 0x80;
}

double bfx_bamtodeg(bfx_word bams) {
  return 180.0 * bams / 0x80;
}

void bfx_sign_encode(beflux *bfx, double y) {
  bfx_push(bfx, y > 0 ?  y : 0);
  bfx_push(bfx, y < 0 ? -y : 0);
}

void bfx_trig_encode(beflux *bfx, double y) {
  bfx_push(bfx, y > 0 ? BFX_WORD_MAX *  y : 0);
  bfx_push(bfx, y < 0 ? BFX_WORD_MAX * -y : 0);
}

double bfx_sign_decode(beflux *bfx) {
  bfx_word neg = bfx_pop(bfx);
  bfx_word pos = bfx_pop(bfx);
  return pos - neg;
}

double bfx_trig_decode(beflux *bfx) {
  return bfx_sign_decode(bfx) / BFX_WORD_MAX;
}

double bfx_table_lookup(double *table, bfx_word index) {
  double result;
  if (index < BFX_TABLE_SIZE) {
    result = table[index];
  }
  else {
    result = -table[index % BFX_TABLE_SIZE];
  }
  return result;
}



/*******************************************************************************
 * Beflux Operators
 */
/* 0x20 */
void bfx_op20(beflux *bfx) { /* ' ' - SKIP */
  size_t i = 0;
  size_t limit = bfx->wrap_offset == 0 ? BFX_PROGRAM_WIDTH : BFX_PROGRAM_SIZE;
  while (bfx_ip_get_op(bfx) == ' ') {
    bfx_ip_advance(bfx);
    if (i ++ > limit) {
      bfx_error(bfx, "Non-halting empty loop detected.");
      break;
    }
  }
  bfx->ip.wait = 1;
}

void bfx_op21(beflux *bfx) { /* '!' - NOT */
  bfx_push(bfx, bfx_pop(bfx) == 0);
}

void bfx_op22(beflux *bfx) { /* '"' - STRING */
  bfx_push(bfx, '\0');
  bfx->mode = BFX_MODE_STRING;
}

void bfx_op23(beflux *bfx) { /* '#' - TRAMPOLINE */
  bfx_ip_advance(bfx);
}

void bfx_op24(beflux *bfx) { /* '$' - POP */
  bfx_pop(bfx);
}

void bfx_op25(beflux *bfx) { /* '%' - MOD */
  bfx_word b = bfx_pop(bfx);
  if (b == 0) {
    bfx_error(bfx, "Zero modulus.");
  }
  bfx_push(bfx, bfx_pop(bfx) % b);
}

void bfx_op26(beflux *bfx) { /* '&' - SCAN HEX */
  if (bfx->in == NULL) {
    bfx_warning(bfx, "No input file.");
    bfx_push(bfx, 0);
  }
  else {
    int c = fgetc(bfx->in);
    switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        bfx_get_digit(bfx, c - '0');
        break;
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
        bfx_get_digit(bfx, c - 'A' + 10);
        break;
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
        bfx_get_digit(bfx, c - 'a' + 10);
        break;
      case EOF:
        bfx_error(bfx, "End of input stream.");
      default: break;
    };
  }
}

void bfx_op27(beflux *bfx) { /* '\' - OVER */
  bfx_word a = bfx_pop(bfx);
  bfx_word b = bfx_top(bfx);
  bfx_push(bfx, a);
  bfx_push(bfx, b);
}

void bfx_op28(beflux *bfx) { /* '(' - PUSH FRAME */
  ++bfx->current_frame;
}

void bfx_op29(beflux *bfx) { /* ')' - POP FRAME */
  --bfx->current_frame;
}

void bfx_op2a(beflux *bfx) { /* '*' - MUL */
  bfx_push(bfx, bfx_pop(bfx) * bfx_pop(bfx));
}

void bfx_op2b(beflux *bfx) { /* '+' - ADD */
  bfx_push(bfx, bfx_pop(bfx) + bfx_pop(bfx));
}

void bfx_op2c(beflux *bfx) { /* ',' - PRINT ASCII */
  fputc(bfx_pop(bfx), bfx->out);
}

void bfx_op2d(beflux *bfx) { /* '-' - SUB */
  bfx_push(bfx, bfx_pop(bfx) - bfx_pop(bfx));
}

void bfx_op2e(beflux *bfx) { /* '.' - PRINT HEX */
  fprintf(bfx->out, "%02x", bfx_pop(bfx));
}

void bfx_op2f(beflux *bfx) { /* '/' - DIV */
  bfx_word b = bfx_pop(bfx);
  if (b == 0) {
    bfx_error(bfx, "Zero denominator.");
  }
  bfx_push(bfx, bfx_pop(bfx) / b);
}

/* 0x30 */
void bfx_op30(beflux *bfx) { /* '0' */ bfx_get_digit(bfx, 0); }
void bfx_op31(beflux *bfx) { /* '1' */ bfx_get_digit(bfx, 1); }
void bfx_op32(beflux *bfx) { /* '2' */ bfx_get_digit(bfx, 2); }
void bfx_op33(beflux *bfx) { /* '3' */ bfx_get_digit(bfx, 3); }
void bfx_op34(beflux *bfx) { /* '4' */ bfx_get_digit(bfx, 4); }
void bfx_op35(beflux *bfx) { /* '5' */ bfx_get_digit(bfx, 5); }
void bfx_op36(beflux *bfx) { /* '6' */ bfx_get_digit(bfx, 6); }
void bfx_op37(beflux *bfx) { /* '7' */ bfx_get_digit(bfx, 7); }
void bfx_op38(beflux *bfx) { /* '8' */ bfx_get_digit(bfx, 8); }
void bfx_op39(beflux *bfx) { /* '9' */ bfx_get_digit(bfx, 9); }

void bfx_op3a(beflux *bfx) { /* ':' - DUP */
  bfx_push(bfx, bfx_top(bfx));
}

void bfx_op3b(beflux *bfx) { /* ';' - COMMENT */
  size_t i = 0;
  size_t limit = bfx->wrap_offset == 0 ? BFX_PROGRAM_WIDTH - 3 : BFX_PROGRAM_SIZE;
  bfx_ip_advance(bfx);
  while (bfx_ip_get_op(bfx) != ';') {
    bfx_ip_advance(bfx);
    if (i ++ > limit) {
      bfx_error(bfx, "Non-halting comment loop detected.");
      break;
    }
  }
}

void bfx_op3c(beflux *bfx) { /* '<' - WEST */
  bfx->ip.dir = BFX_IP_W;
}

void bfx_op3d(beflux *bfx) { /* '=' - EQ */
  bfx_push(bfx, bfx_pop(bfx) == bfx_pop(bfx));
}

void bfx_op3e(beflux *bfx) { /* '>' - EAST */
  bfx->ip.dir = BFX_IP_E;
}

void bfx_op3f(beflux *bfx) { /* '?' - AWAY */
  bfx->ip.dir = (rand() % 4) << 7;
}

/* 0x40 */
void bfx_op40(beflux *bfx) { /* '@' - RESET */
  bfx_ip_reset(bfx);
  bfx->ip.wait = 1;
  ++bfx->t_minor;
}

void bfx_op41(beflux *bfx) { /* 'A' TODO */
}

void bfx_op42(beflux *bfx) { /* 'B' - BOUNCE */
  bfx->ip.dir += BFX_IP_TURN_B;
}

void bfx_op43(beflux *bfx) { /* 'C' - CALL */
  bfx_stack_push(&bfx->calls_x, bfx->ip.x);
  bfx_stack_push(&bfx->calls_y, bfx->ip.y);
  bfx_op4a(bfx); /* 'J' */
}

void bfx_op44(beflux *bfx) { /* 'D' - DICE */
  bfx_word max = bfx_pop(bfx);
  bfx_word min = bfx_pop(bfx);
  bfx_push(bfx, (rand() % (max - min)) + min);
}

void bfx_op45(beflux *bfx) { /* 'E' - EOF */
  if (bfx->in == NULL) {
    bfx_push(bfx, 0xff);
  }
  else {
    bfx_push(bfx, !!feof(bfx->in));
  }
}

void bfx_op46(beflux *bfx) { /* 'F' - FUNCTION */
  bfx->f_bindings[bfx_pop(bfx)](bfx);
}

void bfx_op47(beflux *bfx) { /* 'G' - PROGRAM GET */
  bfx_word x = bfx_pop(bfx);
  bfx_word y = bfx_pop(bfx);
  bfx_program_get(bfx, bfx_pop(bfx), x, y);
}

void bfx_op48(beflux *bfx) { /* 'H' TODO */
}

void bfx_op49(beflux *bfx) { /* 'I' - INPUT FILE TODO */
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

void bfx_op4a(beflux *bfx) { /* 'J' - JUMP */
  bfx->ip.x = bfx_pop(bfx);
  bfx->ip.y = bfx_pop(bfx);
  bfx->ip.wait = 1;
}

void bfx_op4b(beflux *bfx) { /* 'K' COPY FRAME */
  bfx_op28(bfx); /* '(' */
  memcpy(&bfx->frames[bfx->current_frame],
         &bfx->frames[bfx->current_frame - 1], sizeof(bfx_stack));
}

void bfx_op4c(beflux *bfx) { /* 'L' - LOOP RESET */
  bfx->loop_count = 0;
}

void bfx_op4d(beflux *bfx) { /* 'M' - MATH */
  bfx->m_bindings[bfx_pop(bfx)](bfx);
}

void bfx_op4e(beflux *bfx) { /* 'N' - MEMORY CLEAR */
  do {
    bfx_clear(bfx);
  } while (bfx->current_frame--);
}

void bfx_op4f(beflux *bfx) { /* 'O' - OUTPUT FILE */
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
void bfx_op50(beflux *bfx) { /* 'P' PROGRAM LOAD */
  bfx_stack tmp;
  bfx_word prog = bfx_pop(bfx);
  bfx_op72(bfx); /* 'r' */
  while (bfx_top(bfx)) {
    bfx_stack_push(&tmp, bfx_pop(bfx));
  }
  bfx_stack_push(&tmp, '\0');
  bfx_load(bfx, prog, tmp.data);
}

void bfx_op51(beflux *bfx) { /* 'Q' - QUIT */
  bfx_ip_reset(bfx);
  bfx->ip.wait = 1;
  bfx->status = bfx->t_minor = 0;
  ++bfx->t_major;
  bfx->mode = BFX_MODE_HALT;
}

void bfx_op52(beflux *bfx) { /* 'R' - RETURN */
  bfx_push(bfx, bfx_stack_pop(&bfx->calls_y));
  bfx_push(bfx, bfx_stack_pop(&bfx->calls_x));
  bfx_op4a(bfx); /* J */
  bfx_ip_advance(bfx);
}

void bfx_op53(beflux *bfx) { /* 'S' - PROGRAM SET */
  bfx_word x = bfx_pop(bfx);
  bfx_word y = bfx_pop(bfx);
  bfx_program_set(bfx, bfx_pop(bfx), x, y, bfx_pop(bfx));
}

void bfx_op54(beflux *bfx) { /* 'T' - MAJOR TIME */
  bfx_push(bfx, bfx->t_major);
}

void bfx_op55(beflux *bfx) { /* 'U' CURRENT PROGRAM */
  bfx_push(bfx, bfx->current_program);
}

void bfx_op56(beflux *bfx) { /* 'V' TODO */
}

void bfx_op57(beflux *bfx) { /* 'W' - WRAP */
  bfx->wrap_offset = bfx_pop(bfx);
}

void bfx_op58(beflux *bfx) { /* 'X' - PROGRAM EXEC */
  bfx_op4a(bfx); /* 'J' */
  bfx->current_program = bfx_pop(bfx);
}

void bfx_op59(beflux *bfx) { /* 'Y' TODO */
}

void bfx_op5a(beflux *bfx) { /* 'Z' RANDOM */
  bfx_push(bfx, rand());
}

void bfx_op5b(beflux *bfx) { /* '[' - LEFT */
  bfx->ip.dir += BFX_IP_TURN_L;
}

void bfx_op5c(beflux *bfx) { /* '\' - SWAP */
  bfx_word a = bfx_pop(bfx);
  bfx_word b = bfx_pop(bfx);
  bfx_push(bfx, a);
  bfx_push(bfx, b);
}

void bfx_op5d(beflux *bfx) { /* ']' - RIGHT */
  bfx->ip.dir += BFX_IP_TURN_R;
}

void bfx_op5e(beflux *bfx) { /* '^' - NORTH */
  bfx->ip.dir = BFX_IP_N;
}

void bfx_op5f(beflux *bfx) { /* '_' - WEST / EAST IF */
  if (bfx_pop(bfx))
    bfx_op3c(bfx); /* '<' */
  else
    bfx_op3e(bfx); /* '>' */
}

/* 0x60 */
void bfx_op60(beflux *bfx) { /* '`' - GREATER */
  bfx_push(bfx, bfx_pop(bfx) > bfx_pop(bfx));
}

void bfx_op61(beflux *bfx) { /* 'a' */ bfx_get_digit(bfx, 10); }
void bfx_op62(beflux *bfx) { /* 'b' */ bfx_get_digit(bfx, 11); }
void bfx_op63(beflux *bfx) { /* 'c' */ bfx_get_digit(bfx, 12); }
void bfx_op64(beflux *bfx) { /* 'd' */ bfx_get_digit(bfx, 13); }
void bfx_op65(beflux *bfx) { /* 'e' */ bfx_get_digit(bfx, 14); }
void bfx_op66(beflux *bfx) { /* 'f' */ bfx_get_digit(bfx, 15); }

void bfx_op67(beflux *bfx) { /* 'g' - REGISTER GET */
  bfx_push(bfx, bfx->registers[bfx_pop(bfx)]);
}

void bfx_op68(beflux *bfx) { /* 'h' TODO */
}

void bfx_op69(beflux *bfx) { /* 'i' INPUT STRING */
}

void bfx_op6a(beflux *bfx) { /* 'j' - RELATIVE JUMP */
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

void bfx_op6b(beflux *bfx) { /* 'k' - ITER */
  bfx_ip_advance(bfx);
  bfx->ip.wait = bfx_pop(bfx);
}

void bfx_op6c(beflux *bfx) { /* 'l' - LOOP COUNT */
  bfx_push(bfx, bfx->loop_count++);
}

void bfx_op6d(beflux *bfx) { /* 'm' - NORTH IF */
  if (bfx_pop(bfx))
    bfx_op5e(bfx); /* '^' */
}

void bfx_op6e(beflux *bfx) { /* 'n' FRAME CLEAR */
  bfx_clear(bfx);
}

void bfx_op6f(beflux *bfx) { /* 'o' - OUTPUT STRING */
  bfx_op72(bfx); /* 'r' */
  while (bfx_top(bfx)) {
    bfx_op2c(bfx); /* ',' */
  }
}

/* 0x70 */
void bfx_op70(beflux *bfx) { /* 'p' TODO */
}

void bfx_op71(beflux *bfx) { /* 'q' - STATUS QUIT */
  bfx_ip_reset(bfx);
  bfx->ip.wait = 1;
  bfx->status = bfx_pop(bfx);
  bfx->t_minor = 0;
  ++bfx->t_major;
  bfx->mode = BFX_MODE_HALT;
}

void bfx_op72(beflux *bfx) { /* 'r' - REVERSE STRING */
  bfx_word buffer[BFX_BANK_SIZE];

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

void bfx_op73(beflux *bfx) { /* 's' - REGISTER SET */
  bfx_word i = bfx_pop(bfx);
  bfx->registers[i] = bfx_pop(bfx);
}

void bfx_op74(beflux *bfx) { /* 't' - MINOR TIME */
  bfx_push(bfx, bfx->t_minor);
}

void bfx_op75(beflux *bfx) { /* 'u' STRING JOIN */
  bfx_stack tmp;

  while (bfx_top(bfx)) {
    bfx_stack_push(&tmp, bfx_pop(bfx));
  }
  bfx_pop(bfx);

  while (tmp.size) {
    bfx_push(bfx, bfx_stack_pop(&tmp));
  }

}

void bfx_op76(beflux *bfx) { /* 'v' - SOUTH */
  bfx->ip.dir = BFX_IP_S;
}

void bfx_op77(beflux *bfx) { /* 'w' - SOUTH IF */
  if (bfx_pop(bfx))
    bfx_op76(bfx); /* 'v' */
}

void bfx_op78(beflux *bfx) { /* 'x' - EXEC OP */
  bfx_eval(bfx, bfx_pop(bfx));
}

void bfx_op79(beflux *bfx) { /* 'y' TODO */
}

void bfx_op7a(beflux *bfx) { /* 'z' TODO */
}

void bfx_op7b(beflux *bfx) { /* '{' - BLOCK BEGIN */
  bfx_word depth = 1;
  if (!bfx_pop(bfx)) { /* Skip to matching BLOCK END */
    bfx_word c;
    size_t i = 0;
    size_t limit = bfx->wrap_offset == 0 ? BFX_PROGRAM_WIDTH - 3 : BFX_PROGRAM_SIZE;
    while (depth) {
      bfx_ip_advance(bfx);
      c = bfx_ip_get_op(bfx);
      if (c == '}') {
        --depth;
      }
      else if (c == '{') {
        ++depth;
      }

      if (i++ > limit || depth == BFX_WORD_MAX) {
        bfx_error(bfx, "Non-halting block loop detected.");
      }
    }
  }
}

void bfx_op7c(beflux *bfx) { /* '|' - NORTH / SOUTH IF */
  if (bfx_pop(bfx))
    bfx_op5e(bfx); /* '^' */
  else
    bfx_op76(bfx); /* 'v' */
}

void bfx_op7d(beflux *bfx) { /* '}' - BLOCK END */
}

void bfx_op7e(beflux *bfx) { /* '~' - SCAN ASCII */
  bfx_push(bfx, fgetc(bfx->in));
}

void bfx_op7f(beflux *bfx) { /* NOP */
}

/* Math Functions */
/* Note: The SIN and COS functions push two words onto the stack representing
 *       positive and negative parts of the result, scaled to the word width.
 *
 *       0xff 0x00 == +1.0; 0x00 0xff == -1.0;
 *
 *       Adding these words will yield a scaled absolute value.
 *
 *       To retrieve the actual floating point result for use in a bound
 *       function, use the bfx_trig_decode utility function.
 */
void bfx_m00(beflux *bfx) { /* ID */
}

void bfx_m01(beflux *bfx) { /* SIN */
  bfx_trig_encode(bfx, bfx_table_lookup(bfx_sin_table, bfx_pop(bfx)));
}

void bfx_m02(beflux *bfx) { /* COS */
  bfx_trig_encode(bfx, bfx_table_lookup(bfx_sin_table, bfx_pop(bfx) - 0x40));
}

void pre(beflux *this) {
  bfx_note(this, "!");
}

int main(int argc, char *argv[]) {
  int status = 0;
  if (argc == 1) {
    fprintf(stderr, ":: BEFLUX ::\nNo program specified.\n");
  }
  else {
    size_t i, j;
    beflux *b = bfx_create();
    bfx_load(b, 0, argv[1]);
    // b->pre_update = pre;
    /*
    for (i = 0; i < 256; ++i) {
      bfx_run(b);
      printf("%f ", bfx_trig_decode(b));
    }
    */
    bfx_run(b);
    bfx_destroy(b);
  }
  return status;
}

