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
beflux *bfx_new(void) {
  beflux *bfx = (beflux *) malloc(sizeof(beflux));
  bfx_init(bfx);
  return bfx;
}

/**
 * \brief Allocates memory for the interpreter, and initializes its members.
 */
void bfx_init(beflux *bfx) {
  size_t i;

  bfx->programs = calloc(BFX_BANK_SIZE, BFX_PROGRAM_SIZE * sizeof(bfx_word));
  bfx->registers = calloc(BFX_BANK_SIZE, sizeof(bfx_word));

  bfx->op_bindings = bfx_default_op_bindings,
  bfx->f_bindings = calloc(BFX_BANK_SIZE, sizeof(bfx_func *));

  bfx->pre_update = NULL;
  bfx->post_update = NULL;

  for (i = 0; i < BFX_BANK_SIZE; ++i) {
    bfx_stack_init(bfx->frames + i);
  }

  bfx_stack_init(&bfx->calls_row);
  bfx_stack_init(&bfx->calls_col);

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
  time(&bfx->pre_timer);
  time(&bfx->post_timer);
  bfx->timeout = 0;
  bfx->sleep = 0;

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
  free(bfx->f_bindings);
  bfx->f_bindings = NULL;
  bfx->mode = BFX_MODE_FREED;
}

/**
 * \brief Frees the interpreter and its members.
 */
void bfx_del(beflux *bfx) {
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
    size_t row, col;

    memset(bfx->programs + BFX_PROGRAM_SIZE * prog, ' ', BFX_PROGRAM_SIZE);

    if (fgets(buffer, linewidth, fin) != NULL) {
      for (row = 0; row < BFX_PROGRAM_HEIGHT; ++row) {
        size_t len;

        memset(buffer, '\0', linewidth);
        if (
          fgets(buffer, linewidth, fin) == NULL ||
          !(len = strlen(buffer))
        ) break;

        for (col = 0; col < BFX_PROGRAM_WIDTH; ++col) {
          bfx_word c = ' ';
          if (col < len) {
            c = buffer[col];
            if (c == '\n') {
              break;
            }
          }
          bfx_program_set(bfx, prog, row, col, c);
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
  char filename_ext[BFX_BANK_SIZE];
  FILE *fout;

  sprintf(filename_ext, "%s.bfx", filename);
  fout = fopen(filename_ext, "w");
  if (fout != NULL) {
    size_t s, w;
    for (s = 0, w = 0; s < BFX_PROGRAM_SIZE; ++s, ++w) {
      if (w && w % BFX_PROGRAM_WIDTH == 0) {
        fputc('\n', fout);
        w = 0;
      }
      fputc(bfx->programs[BFX_PROGRAM_SIZE * prog + s], fout);
    }
    fclose(fout);
  }
  else {
    char msg[BFX_BANK_SIZE];
    sprintf(msg, "Failed to write program to \"%s\"", filename_ext);
    bfx_error(bfx, msg);
  }
}

/**
 * \brief Reads an array of words into the interpreter as a program.
 * \param prog The index of the program.
 * \param src A pointer to the source array.
 * \param size The number of words to read.
 */
void bfx_read(beflux *bfx, bfx_word prog, const bfx_word *src, size_t size) {
  memcpy(bfx->programs + BFX_PROGRAM_SIZE * prog, src, size);
}

/**
 * \brief Writes the contents of a program in interpreter memory to an array.
 * \param prog The index of the program.
 * \param dst A pointer to the destination array.
 * \param size The number of words to write.
 */
void bfx_write(beflux *bfx, bfx_word prog, bfx_word *dst, size_t size) {
  memcpy(dst, bfx->programs + BFX_PROGRAM_SIZE * prog, size);
}

/**
 * \brief Issues a notification through the interpreter's error file.
 * \param message C string containing the warning message.
 */
void bfx_note(beflux *bfx, const char *message) {
  bfx_word op = bfx_ip_get_op(bfx);
  fprintf(
    bfx->err,
    "Note: %s (op%02x='%c') at %02x%02x%02x\n  %s\n\n",
    bfx_opnames[op], op, op,
    bfx->current_program, bfx->ip.row, bfx->ip.col,
    message
  );
}

/**
 * \brief Issues a warning through the interpreter's error file.
 * \param message C string containing the warning message.
 */
void bfx_warning(beflux *bfx, const char *message) {
  bfx_word op = bfx_ip_get_op(bfx);
  fprintf(
    bfx->err,
    "Warning: %s (op%02x='%c') at %02x%02x%02x\n  %s\n\n",
    bfx_opnames[op], op, op,
    bfx->current_program, bfx->ip.row, bfx->ip.col,
    message
  );
}

/**
 * \brief Issues an error message through the interpreter's error file, and
 *        terminates the current program.
 * \param message C string containing the error message.
 */
void bfx_error(beflux *bfx, const char *message) {
  bfx_word op = bfx_ip_get_op(bfx);
  fprintf(
    bfx->err,
    "Error: %s (op%02x='%c') at %02x%02x%02x\n  %s\nExiting.\n\n",
    bfx_opnames[op], op, op,
    bfx->current_program, bfx->ip.row, bfx->ip.col,
    message
  );
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
 * \return The interpreter's exit status.
 */
bfx_word bfx_run(beflux *bfx) {
  time(&bfx->run_timer);

  switch (bfx->mode) {
    case BFX_MODE_HALT:
      bfx->mode = BFX_MODE_NORMAL;
      while (bfx->mode) { /* MAIN LOOP */
        time(&bfx->pre_timer);
        if (bfx->pre_update != NULL)
          bfx->pre_update(bfx);

        bfx_update(bfx);
        time(&bfx->post_timer);

        if (bfx->post_update != NULL)
          bfx->post_update(bfx);

        bfx_sleep(bfx);
      }
      break;

    case BFX_MODE_FREED:
      bfx_error(bfx, "Interpreter has already been freed.");
      break;

    default:
      bfx_error(bfx, "Bad interpreter mode.");
      break;
  }
  return bfx->status;
}

/**
 * \brief Updates the interpreter's internal state.
 */
void bfx_update(beflux *bfx) {
  bfx_eval(bfx, bfx_ip_get_op(bfx));
  bfx_ip_advance(bfx);
  ++bfx->tick;
}

/**
 * \brief Pauses or halts execution depending on the interpreter's timers.
 */
void bfx_sleep(beflux *bfx) {
  if (bfx->sleep) {
    time_t now;
    do {
      time(&now);
    } while (difftime(now, bfx->post_timer) <= bfx->sleep);
  }
  bfx->sleep = 0;

  if (bfx->timeout) {
    size_t dt = (size_t) difftime(bfx->post_timer, bfx->run_timer);
    if (dt >= bfx->timeout) {
      bfx_error(bfx, "Program timeout.");
    }
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
      bfx_func *func = bfx->op_bindings[op];
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
bfx_word bfx_program_get(beflux *bfx, bfx_word prog, bfx_word row, bfx_word col) {
  return bfx->programs[
    col + 
    BFX_PROGRAM_WIDTH * row +
    BFX_PROGRAM_SIZE  * prog
  ];
}

/**
 * \brief Writes a word to a program stored in the interpreter's memory.
 * \param prog The index of the program.
 */
void bfx_program_set(beflux *bfx, bfx_word prog, bfx_word row, bfx_word col, bfx_word value) {
  bfx->programs[
    col +
    BFX_PROGRAM_WIDTH * row +
    BFX_PROGRAM_SIZE  * prog
  ] = value;
}


/* IP Manipulatiion */
/**
 * \brief Resets the interpreter's instruction pointer to the default state.
 */
void bfx_ip_reset(beflux *bfx) {
  bfx->ip.row = bfx->ip.col = 0;
  bfx->ip.dir = BFX_IP_E;
  bfx->ip.wait = 0;
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
          if (bfx->ip.col == BFX_WORD_MAX) {
            bfx->ip.row += bfx->wrap_offset;
            bfx->ip.wait = 1;
          }
          ++bfx->ip.col; break;
        case BFX_IP_N: --bfx->ip.row; break;
        case BFX_IP_W:
          if (bfx->ip.col == 0x00) {
            bfx->ip.row -= bfx->wrap_offset;
            bfx->ip.wait = 1;
          }
          --bfx->ip.col; break;
        case BFX_IP_S: ++bfx->ip.row; break;
        default: break;
      }
    }
    else {
      switch (bfx->ip.dir) {
        case BFX_IP_E: ++bfx->ip.col; break;
        case BFX_IP_N: --bfx->ip.row; break;
        case BFX_IP_W: --bfx->ip.col; break;
        case BFX_IP_S: ++bfx->ip.row; break;
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
  return bfx_program_get(bfx, bfx->current_program, bfx->ip.row, bfx->ip.col);
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

/**
 * \brief Writes a string on the stack to an array.
 */
void bfx_get_string(beflux *bfx, char *dst) {
  bfx_op72(bfx); /* 'r' */
  bfx_word c;
  do {
    c = bfx_pop(bfx);
    *dst++ = c;
  } while (c != '\0');
}

/*******************************************************************************
 * Beflux Operators
 */
/* 0x20 */

/**
 * \brief ' ' - SKIP (0:0) - Skip to next non-space character.
 */
void bfx_op20(beflux *bfx) {
  size_t i = 0;
  size_t limit = bfx->wrap_offset == 0 ? BFX_PROGRAM_WIDTH : BFX_PROGRAM_SIZE;
  while (bfx_ip_get_op(bfx) == ' ') {
    bfx_ip_advance(bfx);
    if (i ++ > limit) {
      bfx_error(bfx, "Infinite empty loop detected.");
      break;
    }
  }
  bfx->ip.wait = 1;
}

/**
 * \brief '!' - NOT (1:1) - Boolean negation.
 */
void bfx_op21(beflux *bfx) {
  bfx_push(bfx, bfx_pop(bfx) == 0);
}

/**
 * \brief '"' - STR (0:0) - Toggle string mode.
 */
void bfx_op22(beflux *bfx) {
  bfx_push(bfx, '\0');
  bfx->mode = BFX_MODE_STRING;
}

/**
 * \brief '#' - HOP (0:0) - Skip the next character.
 */
void bfx_op23(beflux *bfx) {
  bfx_ip_advance(bfx);
}

/**
 * \brief '$' - POP (1:0) - Pop from the current stack.
 */
void bfx_op24(beflux *bfx) {
  bfx_pop(bfx);
}

/**
 * \brief '%' - MOD (2:1) - Calculate remainder.
 */
void bfx_op25(beflux *bfx) {
  bfx_word b = bfx_pop(bfx);
  if (b == 0) {
    bfx_error(bfx, "Zero modulus.");
  }
  bfx_push(bfx, bfx_pop(bfx) % b);
}

/**
 * \brief '&' - GETX (0:?) - Reads a single hex digit from input.
 */
void bfx_op26(beflux *bfx) {
  if (bfx->in == NULL) {
    bfx_error(bfx, "No input file.");
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

/**
 * \brief ''' - OVER (2:3) - Copies the value under the top of the stack.
 */
void bfx_op27(beflux *bfx) {
  bfx_word a = bfx_pop(bfx);
  bfx_word b = bfx_top(bfx);
  bfx_push(bfx, a);
  bfx_push(bfx, b);
}

/**
 * \brief '(' - PSHF (0:0) - Pushes a new stack frame.
 */
void bfx_op28(beflux *bfx) {
  ++bfx->current_frame;
}

/**
 * \brief ')' - POPF (0:0) - Pops the current stack frame.
 */
void bfx_op29(beflux *bfx) {
  --bfx->current_frame;
}

/**
 * \brief '*' - MUL (2:1) - Calculate product.
 */
void bfx_op2a(beflux *bfx) {
  bfx_push(bfx, bfx_pop(bfx) * bfx_pop(bfx));
}

/**
 * \brief '+' - ADD (2:1) - Calculate sum.
 */
void bfx_op2b(beflux *bfx) {
  bfx_push(bfx, bfx_pop(bfx) + bfx_pop(bfx));
}

/**
 * \brief ',' - PUTC (1:0) - Writes an ASCII character to output.
 */
void bfx_op2c(beflux *bfx) {
  if (bfx->out == NULL) {
    bfx_error(bfx, "No output file.");
  }
  fputc(bfx_pop(bfx), bfx->out);
}

/**
 * \brief '-' - SUB (2:1) - Calculate difference.
 */
void bfx_op2d(beflux *bfx) {
  bfx_word b = bfx_pop(bfx);
  bfx_push(bfx, bfx_pop(bfx) - b);
}

/**
 * \brief '.' - PUTX (1:0) - Writes a hexadecimal value to output.
 */
void bfx_op2e(beflux *bfx) {
  if (bfx->out == NULL) {
    bfx_error(bfx, "No output file.");
  }
  fprintf(bfx->out, "%02x", bfx_pop(bfx));
}

/**
 * \brief '/' - DIV (2:1) - Calculate quotient.
 */
void bfx_op2f(beflux *bfx) {
  bfx_word b = bfx_pop(bfx);
  if (b == 0) {
    bfx_error(bfx, "Zero denominator.");
  }
  bfx_push(bfx, bfx_pop(bfx) / b);
}

/* 0x30 */
/**
 * \brief '0' - V0 (0:?) - Hex digit 0.
 */
void bfx_op30(beflux *bfx) { bfx_get_digit(bfx, 0); }

/**
 * \brief '1' - V1 (0:?) - Hex digit 1.
 */
void bfx_op31(beflux *bfx) { bfx_get_digit(bfx, 1); }

/**
 * \brief '2' - V2 (0:?) - Hex digit 2.
 */
void bfx_op32(beflux *bfx) { bfx_get_digit(bfx, 2); }

/**
 * \brief '3' - V3 (0:?) - Hex digit 3.
 */
void bfx_op33(beflux *bfx) { bfx_get_digit(bfx, 3); }

/**
 * \brief '4' - V4 (0:?) - Hex digit 4.
 */
void bfx_op34(beflux *bfx) { bfx_get_digit(bfx, 4); }

/**
 * \brief '5' - V5 (0:?) - Hex digit 5.
 */
void bfx_op35(beflux *bfx) { bfx_get_digit(bfx, 5); }

/**
 * \brief '6' - V6 (0:?) - Hex digit 6.
 */
void bfx_op36(beflux *bfx) { bfx_get_digit(bfx, 6); }

/**
 * \brief '7' - V7 (0:?) - Hex digit 7.
 */
void bfx_op37(beflux *bfx) { bfx_get_digit(bfx, 7); }

/**
 * \brief '8' - V8 (0:?) - Hex digit 8.
 */
void bfx_op38(beflux *bfx) { bfx_get_digit(bfx, 8); }

/**
 * \brief '9' - V9 (0:?) - Hex digit 9.
 */
void bfx_op39(beflux *bfx) { bfx_get_digit(bfx, 9); }

/**
 * \brief ':' - DUP (1:2) - Duplicate current stack top.
 */
void bfx_op3a(beflux *bfx) {
  bfx_push(bfx, bfx_top(bfx));
}

/**
 * \brief ';' - COM (0:0) - Skip to next comment character.
 */
void bfx_op3b(beflux *bfx) {
  size_t i = 0;
  size_t limit = bfx->wrap_offset == 0 ? BFX_PROGRAM_WIDTH - 3 : BFX_PROGRAM_SIZE;
  bfx_ip_advance(bfx);
  while (bfx_ip_get_op(bfx) != ';') {
    bfx_ip_advance(bfx);
    if (i++ > limit) {
      bfx_error(bfx, "Infinite comment loop detected.");
      break;
    }
  }
}

/**
 * \brief '<' - MVW (0:0) - Change direction to West.
 */
void bfx_op3c(beflux *bfx) {
  bfx->ip.dir = BFX_IP_W;
}

/**
 * \brief '=' - EQ (2:1) - Test for equality.
 */
void bfx_op3d(beflux *bfx) {
  bfx_push(bfx, bfx_pop(bfx) == bfx_pop(bfx));
}

/**
 * \brief '>' - MVE (0:0) - Change direction to East.
 */
void bfx_op3e(beflux *bfx) {
  bfx->ip.dir = BFX_IP_E;
}

/**
 * \brief '?' - AWAY (0:0) - Change direction at random.
 */
void bfx_op3f(beflux *bfx) {
  bfx->ip.dir = (rand() % 4) << 7;
}

/* 0x40 */
/**
 * \brief '@' - REP (0:0) - Reset the IP and increment t_minor.
 */
void bfx_op40(beflux *bfx) {
  bfx_ip_reset(bfx);
  bfx->ip.wait = 1;
  ++bfx->t_minor;
}

/**
 * \brief 'A' - PRVP (0:0) - Decrement program index.
 */
void bfx_op41(beflux *bfx) {
  --bfx->current_program;
}

/**
 * \brief 'B' - REV (0:0) - Reverse IP direction.
 */
void bfx_op42(beflux *bfx) {
  bfx->ip.dir += BFX_IP_TURN_B;
}

/**
 * \brief 'C' - CALL (2:0) - Jump to position using call stack.
 */
void bfx_op43(beflux *bfx) {
  bfx_stack_push(&bfx->calls_row, bfx->ip.row);
  bfx_stack_push(&bfx->calls_col, bfx->ip.col);
  bfx_op4a(bfx); /* 'J' */
}

/**
 * \brief 'D' - DICE (2:1) - Push a random number in the given range.
 */
void bfx_op44(beflux *bfx) {
  bfx_word max = bfx_pop(bfx);
  bfx_word min = bfx_pop(bfx);
  bfx_push(bfx, (rand() % (max - min)) + min);
}

/**
 * \brief 'E' - EOF (0:1) - Return whether end of input has been reached.
 */
void bfx_op45(beflux *bfx) {
  if (bfx->in == NULL) {
    bfx_push(bfx, 0xff);
  }
  else {
    bfx_push(bfx, !!feof(bfx->in));
  }
}

/**
 * \brief 'F' - FUNC (1:?) - Call a user-defined function.
 */
void bfx_op46(beflux *bfx) {
  bfx->f_bindings[bfx_pop(bfx)](bfx);
}

/**
 * \brief 'G' - GETP (3:1) - Push character from a program.
 */
void bfx_op47(beflux *bfx) {
  bfx_word col = bfx_pop(bfx);
  bfx_word row = bfx_pop(bfx);
  bfx_push(bfx, bfx_program_get(bfx, bfx_pop(bfx), row, col));
}

/**
 * \brief 'H' - HOME (0:0) - Set program index to 0.
 */
void bfx_op48(beflux *bfx) {
  bfx->current_program = 0;
}

/**
 * \brief 'I' - FIN (str:0) - Open input file.
 */
void bfx_op49(beflux *bfx) {
  bfx_word c = bfx_top(bfx);
  if (c == 0x00) {
    bfx_pop(bfx);
    bfx->in = NULL;
  }
  else if (c == 0xff) {
    bfx_pop(bfx);
    bfx->in = stdin;
  }
  else {
    char fname[BFX_BANK_SIZE] = "";
    if (bfx->in != NULL && bfx->in != stdin) {
      fclose(bfx->in);
    }
    bfx_get_string(bfx, fname);
    bfx->in = fopen(fname, "rb");
    if (bfx->in == NULL) {
      char msg[BFX_BANK_SIZE + 32] = "";
      sprintf(msg, "Failed to open input file %s.", fname);
      bfx_error(bfx, msg);
    }
  }
}

/**
 * \brief 'J' - JMP (2:0) - Jump to specified position.
 */
void bfx_op4a(beflux *bfx) {
  bfx->ip.col = bfx_pop(bfx);
  bfx->ip.row = bfx_pop(bfx);
  bfx->ip.wait = 1;
}

/**
 * \brief 'K' - DUPF (0:?) - Pushes a copy of the current stack frame.
 */
void bfx_op4b(beflux *bfx) {
  bfx_op28(bfx); /* '(' */
  memcpy(
    &bfx->frames[bfx->current_frame],
    &bfx->frames[bfx->current_frame - 1],
    sizeof(bfx_stack)
  );
}

/**
 * \brief 'L' - LEND (0:0) - Resets the loop counter.
 */
void bfx_op4c(beflux *bfx) {
  bfx->loop_count = 0;
}

/**
 * \brief 'M' - CLRS (0:0) - Clears all stack frames.
 */
void bfx_op4d(beflux *bfx) {
  do {
    bfx_clear(bfx);
  } while (bfx->current_frame--);
}

/**
 * \brief 'N' - CLRF (0:0) - Clear the current stack frame.
 */
void bfx_op4e(beflux *bfx) {
  bfx_clear(bfx);
}

/**
 * \brief 'O' - FOUT (str:0) - Open output file.
 */
void bfx_op4f(beflux *bfx) {
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
    char fname[BFX_BANK_SIZE] = "";
    if (bfx->out != NULL && bfx->out != stdout) {
      fclose(bfx->out);
    }
    bfx_get_string(bfx, fname);
    bfx->out = fopen(fname, "wb");
    if (bfx->out == NULL) {
      char msg[BFX_BANK_SIZE + 32] = "";
      sprintf(msg, "Failed to open output file %s.", fname);
      bfx_error(bfx, msg);
    }
  }
}

/* 0x50 */
/**
 * \brief 'P' - LOAD (1,str:0) - Load program into specified index.
 */
void bfx_op50(beflux *bfx) {
  char fname[BFX_BANK_SIZE] = "";
  bfx_word prog = bfx_pop(bfx);
  bfx_get_string(bfx, fname);
  bfx_load(bfx, prog, fname);
}

/**
 * \brief 'Q' - QUIT (0:0) - End execution.
 */
void bfx_op51(beflux *bfx) {
  bfx_ip_reset(bfx);
  bfx->ip.wait = 1;
  bfx->status = bfx->t_minor = 0;
  ++bfx->t_major;
  bfx->mode = BFX_MODE_HALT;
}

/**
 * \brief 'R' - RET (0:0) - Return from a CALL.
 */
void bfx_op52(beflux *bfx) {
  bfx_push(bfx, bfx_stack_pop(&bfx->calls_row));
  bfx_push(bfx, bfx_stack_pop(&bfx->calls_col));
  bfx_op4a(bfx); /* J */
  bfx_ip_advance(bfx);
}

/**
 * \brief 'S' - SETP (4:0) - Set a specified program character.
 */
void bfx_op53(beflux *bfx) {
  bfx_word col = bfx_pop(bfx);
  bfx_word row = bfx_pop(bfx);
  bfx_word prog = bfx_pop(bfx);
  bfx_program_set(bfx, prog, row, col, bfx_pop(bfx));
}

/**
 * \brief 'T' - TMAJ (0:1) - Push the t_major timer.
 */
void bfx_op54(beflux *bfx) {
  bfx_push(bfx, bfx->t_major);
}

/**
 * \brief 'U' - CURP (0:1) - Push the current program index.
 */
void bfx_op55(beflux *bfx) {
  bfx_push(bfx, bfx->current_program);
}

/**
 * \brief 'V' - NXTP (0:0) - Incrememnt the program index.
 */
void bfx_op56(beflux *bfx) {
  ++bfx->current_program;
}

/**
 * \brief 'W' - WRAP (1:0) - Set wrapping offset.
 */
void bfx_op57(beflux *bfx) {
  bfx->wrap_offset = bfx_pop(bfx);
}

/**
 * \brief 'X' - EXEP (1:0) - Execute program.
 */
void bfx_op58(beflux *bfx) {
  bfx_op4a(bfx); /* 'J' */
  bfx->current_program = bfx_pop(bfx);
}

/**
 * \brief 'Y' - CLRR (0:0) - Clear all registers.
 */
void bfx_op59(beflux *bfx) {
  size_t i;
  for (i = 0; i < BFX_BANK_SIZE; ++i) {
    bfx->registers[i] = 0;
  }
}

/**
 * \brief 'Z' - RAND (0:1) - Push a random value.
 */
void bfx_op5a(beflux *bfx) {
  bfx_push(bfx, rand());
}

/**
 * \brief '[' - TRNL (0:0) - Turn IP left.
 */
void bfx_op5b(beflux *bfx) {
  bfx->ip.dir += BFX_IP_TURN_L;
}

/**
 * \brief '\' - SWP (2:2) - Swap the top two values on the current stack.
 */
void bfx_op5c(beflux *bfx) {
  bfx_word a = bfx_pop(bfx);
  bfx_word b = bfx_pop(bfx);
  bfx_push(bfx, a);
  bfx_push(bfx, b);
}

/**
 * \brief ']' - TRNR (0:0) - Turn IP right.
 */
void bfx_op5d(beflux *bfx) {
  bfx->ip.dir += BFX_IP_TURN_R;
}

/**
 * \brief '^' - MVN (0:0) - Change direction to North.
 */
void bfx_op5e(beflux *bfx) {
  bfx->ip.dir = BFX_IP_N;
}

/**
 * \brief '_' - WEIF (1:0) - West / East conditional branch.
 */
void bfx_op5f(beflux *bfx) {
  if (bfx_pop(bfx))
    bfx_op3c(bfx); /* '<' */
  else
    bfx_op3e(bfx); /* '>' */
}

/* 0x60 */
/**
 * \brief '`' - GT (2:1) - Compare values.
 */
void bfx_op60(beflux *bfx) {
  bfx_push(bfx, bfx_pop(bfx) > bfx_pop(bfx));
}

/**
 * \brief 'a' - VA (0:?) - Hex digit A.
 */
void bfx_op61(beflux *bfx) { bfx_get_digit(bfx, 10); }

/**
 * \brief 'b' - VB (0:?) - Hex digit B.
 */
void bfx_op62(beflux *bfx) { bfx_get_digit(bfx, 11); }

/**
 * \brief 'c' - VC (0:?) - Hex digit C.
 */
void bfx_op63(beflux *bfx) { bfx_get_digit(bfx, 12); }

/**
 * \brief 'd' - VD (0:?) - Hex digit D.
 */
void bfx_op64(beflux *bfx) { bfx_get_digit(bfx, 13); }

/**
 * \brief 'e' - VE (0:?) - Hex digit E.
 */
void bfx_op65(beflux *bfx) { bfx_get_digit(bfx, 14); }

/**
 * \brief 'f' - VF (0:?) - Hex digit F.
 */
void bfx_op66(beflux *bfx) { bfx_get_digit(bfx, 15); }

/**
 * \brief 'g' - GETR (1:1) - Push value from given register.
 */
void bfx_op67(beflux *bfx) {
  bfx_push(bfx, bfx->registers[bfx_pop(bfx)]);
}

/**
 * \brief 'h' - BMPN (0:0) - Bump IP North.
 */
void bfx_op68(beflux *bfx) {
  --bfx->ip.row;
  bfx->ip.wait = 1;
}

/**
 * \brief 'i' - GETS (0:str) - Read null-terminated string or line from input.
 */
void bfx_op69(beflux *bfx) {
  bfx_word top;
  do {
    bfx_op7e(bfx); /* '~' */
    top = bfx_top(bfx);
  } while (top && top != '\n');
}

/**
 * \brief 'j' - JREL (2:0) - Jump relative to current IP position.
 */
void bfx_op6a(beflux *bfx) {
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

/**
 * \brief 'k' - ITER (1:?) - Iterate next instruction.
 */
void bfx_op6b(beflux *bfx) {
  bfx_ip_advance(bfx);
  bfx->ip.wait = bfx_pop(bfx);
}

/**
 * \brief 'l' - LOOP (0:1) - Push and increment loop counter.
 */
void bfx_op6c(beflux *bfx) {
  bfx_push(bfx, bfx->loop_count++);
}

/**
 * \brief 'm' - NIF (1:0) - North / Continue conditional branch.
 */
void bfx_op6d(beflux *bfx) {
  if (bfx_pop(bfx))
    bfx_op5e(bfx); /* '^' */
}

/**
 * \brief 'n' - ENDL (0:0) - Write newline to output.
 */
void bfx_op6e(beflux *bfx) {
  bfx_push(bfx, '\n');
  bfx_op2c(bfx);
}

/**
 * \brief 'o' - PUTS (str:0) - Write null-terminated string to output.
 */
void bfx_op6f(beflux *bfx) {
  bfx_op72(bfx); /* 'r' */
  while (bfx_top(bfx)) {
    bfx_op2c(bfx); /* ',' */
  }
}

/* 0x70 */
/**
 * \brief 'p' - SWPR (2:1) - Swap top of current stack with value in register.
 */
void bfx_op70(beflux *bfx) {
  bfx_word i = bfx_pop(bfx);
  bfx_word tmp = bfx->registers[i];
  bfx->registers[i] = bfx_pop(bfx);
  bfx_push(bfx, tmp);
}

/**
 * \brief 'q' - EXIT (1:0) - End execution with status.
 */
void bfx_op71(beflux *bfx) {
  bfx->status = bfx_pop(bfx);
  if (bfx->status) {
    char msg[BFX_BANK_SIZE] = "";
    sprintf(msg, "Exited with status %02x.", bfx->status);
    bfx_warning(bfx, msg);
  }
  bfx_ip_reset(bfx);
  bfx->ip.wait = 1;
  bfx->t_minor = 0;
  ++bfx->t_major;
  bfx->mode = BFX_MODE_HALT;
}

/**
 * \brief 'r' - REVS (str:str) - Reverse string on stack.
 */
void bfx_op72(beflux *bfx) {
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

/**
 * \brief 's' - SETR (2:0) - Set given register.
 */
void bfx_op73(beflux *bfx) {
  bfx_word i = bfx_pop(bfx);
  bfx->registers[i] = bfx_pop(bfx);
}

/**
 * \brief 't' - TMIN (0:1) - Push the t_minor timer.
 */
void bfx_op74(beflux *bfx) {
  bfx_push(bfx, bfx->t_minor);
}

/**
 * \brief 'u' - JOIN (str,str:str) - Join two strings on the stack.
 */
void bfx_op75(beflux *bfx) {
  bfx_stack tmp;
  tmp.size = 0;

  while (bfx_top(bfx)) {
    bfx_stack_push(&tmp, bfx_pop(bfx));
  }
  bfx_pop(bfx);

  while (tmp.size) {
    bfx_push(bfx, bfx_stack_pop(&tmp));
  }
  bfx_pop(bfx);
}

/**
 * \brief 'v' - MVS (0:0) - Change direction to South.
 */
void bfx_op76(beflux *bfx) {
  bfx->ip.dir = BFX_IP_S;
}

/**
 * \brief 'w' - SIF (1:0) - South / Continue conditional branch.
 */
void bfx_op77(beflux *bfx) {
  if (bfx_pop(bfx))
    bfx_op76(bfx); /* 'v' */
}

/**
 * \brief 'x' - EXEC (1:?) - Execute operator.
 */
void bfx_op78(beflux *bfx) {
  bfx_eval(bfx, bfx_pop(bfx));
}

/**
 * \brief 'y' - BMPS (0:0) - Bump IP South.
 */
void bfx_op79(beflux *bfx) {
  ++bfx->ip.row;
  bfx->ip.wait = 1;
}

/**
 * \brief 'z' - WAIT (1:0) - Sleep for the given number of seconds.
 */
void bfx_op7a(beflux *bfx) {
  bfx->sleep = bfx_pop(bfx);
  fflush(bfx->out);
  fflush(bfx->err);
}

/**
 * \brief '{' - BLK (1:0) - Conditional block.
 */
void bfx_op7b(beflux *bfx) {
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
        bfx_error(bfx, "Infinite block loop detected.");
      }
    }
  }
}

/**
 * \brief '|' - NSIF (1:0) - North / South conditional branch.
 */
void bfx_op7c(beflux *bfx) {
  if (bfx_pop(bfx))
    bfx_op5e(bfx); /* '^' */
  else
    bfx_op76(bfx); /* 'v' */
}

/**
 * \brief '}' - BEND (0:0) - Block end.
 */
void bfx_op7d(beflux *bfx) {
  (void) bfx;
}

/**
 * \brief '~' - GETC (0:1) - Reads an ASCII character from input.
 */
void bfx_op7e(beflux *bfx) {
  if (bfx->in == NULL) {
    bfx_error(bfx, "No input file.");
  }
  bfx_push(bfx, fgetc(bfx->in));
}

/**
 * \brief 'DEL' - NOP (0:0) - Does nothing!
 */
void bfx_op7f(beflux *bfx) {
  (void) bfx;
}

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

#ifndef LIBBEFLUX
int main(int argc, char **argv) {
  int status = 0;
  if (argc == 1) {
    fprintf(stderr, ":: BEFLUX ::\nUsage: beflux [program.bfx]\n");
  }
  else {
    beflux *b = bfx_new();
    bfx_load(b, 0, argv[1]);
    status = bfx_run(b);
    bfx_del(b);
  }
  return status;
}
#endif

