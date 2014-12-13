// :: beflux.h :: 12/10/2014
// Tony Chiodo - http://dodecaplex.net
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "util.h"

namespace Dodecaplex {

class Beflux {
public:
  static const uint BLOCK_W = 16;
  static const uint BLOCK_H = 16;
  static const uint BLOCK_MAX = BLOCK_W * BLOCK_H;

  static const int CURSOR_W = -1;
  static const int CURSOR_E = +1;
  static const int CURSOR_N = -BLOCK_W;
  static const int CURSOR_S = +BLOCK_W;
  static const int CURSOR_STOP = 0;

  static enum {
    PROGRAM = 0,
    MEMORY,
    INPUT,
    OUTPUT,
    SEGMENT_MAX
  } segment_id;

  static const uint BINDING_MAX = 256;

  class Block {
  public:
    Block(void);
    Block(const byte * const src, uint count);
    Block(const char * const filename);

    // TODO: read / write from rect in src
    void clear(void);
    void read(const byte * const src, uint count);
    void write(byte * const dst, uint count) const;
    void load(const char * const filename);
    void save(const char * const filename) const;

    void set(byte value);
    void set(uint s, byte value);
    void set(uint x, uint y, byte value);
    byte get(void) const;
    byte get(uint s) const;
    byte get(uint x, uint y) const;

    void push(byte x);
    byte pop(void);

    void cursor_set_position(uint s);
    void cursor_set_velocity(int ds);
    uint cursor_get_position(void) const;
    int  cursor_get_velocity(void) const;
    void cursor_advance(void);
    void cursor_backtrack(void);
    void cursor_reset(void);

    byte data[BLOCK_MAX];
    bool row_wrap;

  private:
    class Cursor_ {
    public:
      Cursor_(uint i_s=0, int i_ds=CURSOR_STOP) : s(i_s), ds(i_ds) {}
      uint s;
      int ds;
    } cursor_;

  };

  Beflux(void);

  Block *program(void);
  Block *memory(void);
  Block *input(void);
  Block *output(void);

  byte run(void);

  void bind(uint index, void (*binding)(Beflux *)=nullptr);
  void hook(void (*pre)(Beflux *)=nullptr, void (*post)(Beflux *)=nullptr);

private:
  Block segment_[SEGMENT_MAX];

  bool active_;
  byte status_;

  bool string_mode_;
  bool value_ready_;
  byte value_;
  byte t_major_;
  byte t_minor_;
  byte loop_counter_;

  void update_(void);
  void eval_(byte op);

  void (*bindings_[BINDING_MAX])(Beflux *);
  void (*pre_update_)(Beflux *);
  void (*post_update_)(Beflux *);
};

} // namespace Dodecaplex
