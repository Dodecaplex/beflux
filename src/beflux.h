// :: beflux.h :: 12/10/2014
// Tony Chiodo - http://dodecaplex.net
////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace Dodecaplex {

class Beflux {
public:
  static const unsigned int BLOCK_W = 16;
  static const unsigned int BLOCK_H = 16;
  static const unsigned int BLOCK_MAX = BLOCK_W * BLOCK_H;

  static const unsigned int CURSOR_W = -1;
  static const unsigned int CURSOR_E = +1;
  static const unsigned int CURSOR_N = -BLOCK_W;
  static const unsigned int CURSOR_S = +BLOCK_W;
  static const unsigned int CURSOR_STOP = 0;

  static enum {
    PROGRAM = 0,
    MEMORY,
    INPUT,
    OUTPUT,
    SEGMENT_MAX
  } segment_id;

  static const unsigned int BINDING_MAX = 256;

  class Block {
  public:

    class Cursor {
    public:
      Cursor(unsigned int i_s=0, int i_ds=CURSOR_STOP) : s(i_s), ds(i_ds) {}
      unsigned int s;
      unsigned int ds;
    };

    Block(void);
    Block(const unsigned char * const src, unsigned int count);
    Block(const char * const filename);

    // TODO: read / write from rect in src
    void clear(void);
    void read(const unsigned char * const src, unsigned int count);
    void write(unsigned char * const dst, unsigned int count) const;
    void load(const char * const filename);
    void save(const char * const filename) const;

    void set(unsigned char value);
    void set(unsigned int s, unsigned char value);
    void set(unsigned int x, unsigned int y, unsigned char value);
    unsigned char get(void) const;
    unsigned char get(unsigned int s) const;
    unsigned char get(unsigned int x, unsigned int y) const;

    void push(unsigned char x);
    unsigned char pop(void);

    void cursor_set_position(unsigned int s);
    void cursor_set_velocity(unsigned int ds);
    unsigned int cursor_get_position(void) const;
    unsigned int cursor_get_velocity(void) const;
    void cursor_advance(void);
    void cursor_backtrack(void);
    void cursor_reset(void);

    unsigned char data[BLOCK_MAX];
    bool row_wrap;

  private:
    Cursor cursor_;
  };

  Beflux(void);

  Block *program(void);
  Block *memory(void);
  Block *input(void);
  Block *output(void);

  unsigned char run(void);

  void bind(unsigned int index, void (*binding)(Beflux *)=nullptr);
  void hook(void (*pre)(Beflux *)=nullptr, void (*post)(Beflux *)=nullptr);

  // Math Functions
  static unsigned char sin(unsigned char x); // 0x01
  static unsigned char cos(unsigned char x); // 0x02
  static unsigned char tan(unsigned char x); // 0x03

  static unsigned char degtobam(double degs);
  static double bamtodeg(unsigned char bams);
  static unsigned char radtobam(double rads);
  static double bamtorad(unsigned char bams);

private:
  Block segment_[SEGMENT_MAX];

  bool active_;
  unsigned char status_;

  bool string_mode_;
  bool value_ready_;
  unsigned char value_;
  unsigned char t_major_;
  unsigned char t_minor_;
  unsigned char loop_counter_;

  void update_(void);
  void eval_(unsigned char op);

  void (*bindings_[BINDING_MAX])(Beflux *);
  void (*pre_update_)(Beflux *);
  void (*post_update_)(Beflux *);
};

} // namespace Dodecaplex
