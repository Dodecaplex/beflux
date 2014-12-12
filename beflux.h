// :: beflux.h :: 12/10/2014
// Tony Chiodo - http://dodecaplex.net
////////////////////////////////////////////////////////////////////////////////

#pragma once

static const uint BLOCK_W = 16;
static const uint BLOCK_H = 16;
static const uint BLOCK_MAX = BLOCK_W * BLOCK_H;

static const uint BINDING_MAX = 256;

namespace Dodecaplex {

class Beflux {
public:
  class Block {
  public:
    Block(void);
    Block(const byte *src);
    Block(const char * const filename);

    load(const char * const filename);
    save(const char * const filename) const;
    clear(void);

  private:
    byte data_[BLOCK_MAX];
  };

  void set_program(const Block &src);
  void set_memory(const Block &src);
  void set_input(const Block &src);
  void set_output(const Block &src);

  Block &program(void) const;
  Block &memory(void) const;
  Block &input(void) const;
  Block &output(void) const;

  byte run(void);

  void bind(uint index, void (*binding)(Beflux *));
  void unbind(uint index);

private:
  Block program_;
  Block memory_;
  Block input_;
  Block output_;

  struct Cursor_ {
    ulong x, y;
    ulong dx, dy;
  };

  Cursor program_cursor_;
  Cursor memory_cursor_;
  Cursor input_cursor_;
  Cursor output_cursor_;

  bool active_;
  byte status_;

  bool string_mode_;
  bool value_ready_;
  byte value_;
  byte t_major_;
  byte t_minor_;

  void update_(void);
  void eval_(byte op);

  void (*bindings_[BINDING_MAX])(Beflux *);
}

} // namespace Dodecaplex
