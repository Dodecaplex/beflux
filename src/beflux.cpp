// :: beflux.cpp :: 12/10/2014
// Tony Chiodo - http://dodecaplex.net
////////////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <fstream>
#include <string>

#include "beflux.h"

namespace Dodecaplex {

////////////////////////////////////////////////////////////////////////////////
// Beflux::Block Public Methods
////////////////////////////////////////////////////////////////////////////////
// Zero-Initialize Block
Beflux::Block::Block(void) {
  row_wrap = false;
  cursor_reset();
  clear();
}

// Initialize from Array
Beflux::Block::Block(const byte * const src, uint count) {
  Block();
  read(src, count);
}

// Initialize from File
Beflux::Block::Block(const char * const filename) {
  Block();
  load(filename);
}

void Beflux::Block::clear(void) {
  for (uint i = 0; i < BLOCK_MAX; ++i) {
    data[i] = 0;
  }
  cursor_reset();
}

void Beflux::Block::read(const byte * const src, uint count) {
  for (uint i = 0; i < count && i < BLOCK_MAX; ++i) {
    data[i] = src[i];
  }
}

void Beflux::Block::write(byte * const dst, uint count) const {
  for (uint i = 0; i < count && i < BLOCK_MAX; ++i) {
    dst[i] = data[i];
  }
}

void Beflux::Block::load(const char * const filename) {
  std::ifstream fin(filename);
  for (uint i = 0; i < BLOCK_MAX; ++i) {
    if (fin.good())
      data[i] = byte(fin.get());
    else
      data[i] = 0;
  }
}

void Beflux::Block::save(const char * const filename) const {
  std::ofstream fout(filename);
  for (uint i = 0; i < BLOCK_MAX; ++i) {
    fout << data[i];
  }
}

void Beflux::Block::set(byte value) {
  data[cursor_.s] = value;
}

void Beflux::Block::set(uint s, byte value) {
  s = wrap(s, 0, BLOCK_MAX);
  data[s] = value;
}

void Beflux::Block::set(uint x, uint y, byte value) {
  x = wrap(x, 0, BLOCK_W);
  y = wrap(y, 0, BLOCK_H);
  data[x + BLOCK_W * y] = value;
}

byte Beflux::Block::get(void) const {
  return data[cursor_.s];
}

byte Beflux::Block::get(uint s) const {
  s = wrap(s, 0, BLOCK_MAX);
  return data[s];
}

byte Beflux::Block::get(uint x, uint y) const {
  x = wrap(x, 0, BLOCK_W);
  y = wrap(y, 0, BLOCK_H);
  return data[x + BLOCK_W * y];
}

void Beflux::Block::push(byte x) {
  set(x);
  cursor_advance();
}

byte Beflux::Block::pop(void) {
  cursor_backtrack();
  byte x = get();
  set(0);
  return x;
}

void Beflux::Block::cursor_set_position(uint s) {
  cursor_.s = wrap(s, 0, BLOCK_MAX);
}

void Beflux::Block::cursor_set_velocity(int ds) {
  cursor_.ds = wrap(ds, 0, BLOCK_MAX);
}

uint Beflux::Block::cursor_get_position(void) const {
  return cursor_.s;
}

int Beflux::Block::cursor_get_velocity(void) const {
  return cursor_.ds;
}

void Beflux::Block::cursor_advance(void) {
  if (row_wrap) {
    uint sx = wrap(cursor_.s + cursor_.ds % BLOCK_W, 0, BLOCK_W);
    uint sy = wrap(cursor_.s + cursor_.ds / BLOCK_W, 0, BLOCK_H);
    cursor_.s = sx + BLOCK_W * sy;
  }
  else {
    cursor_.s = wrap(cursor_.s + cursor_.ds, 0, BLOCK_MAX);
  }
}

void Beflux::Block::cursor_backtrack(void) {
  if (row_wrap) {
    uint sx = wrap(cursor_.s - cursor_.ds % BLOCK_W, 0, BLOCK_W);
    uint sy = wrap(cursor_.s - cursor_.ds / BLOCK_W, 0, BLOCK_H);
    cursor_.s = sx + BLOCK_W * sy;
  }
  else {
    cursor_.s = wrap(cursor_.s - cursor_.ds, 0, BLOCK_MAX);
  }
}

void Beflux::Block::cursor_reset(void) {
  cursor_.s = 0;
  cursor_.ds = CURSOR_E;
}

////////////////////////////////////////////////////////////////////////////////
// Beflux Public Methods
////////////////////////////////////////////////////////////////////////////////
Beflux::Beflux(void) {
  active_ = false;
  status_ = 0x00;
  string_mode_ = false;
  value_ready_ = false;
  value_ = 0x00;
  t_major_ = t_minor_ = loop_counter_ = 0x00;
  for (auto &i: bindings_) {
    i = nullptr;
  }
  pre_update_ = post_update_ = nullptr;
}

Beflux::Block *Beflux::program(void) {
  return &segment_[PROGRAM];
}

Beflux::Block *Beflux::memory(void) {
  return &segment_[MEMORY];
}

Beflux::Block *Beflux::input(void) {
  return &segment_[INPUT];
}

Beflux::Block *Beflux::output(void) {
  return &segment_[OUTPUT];
}

byte Beflux::run(void) {
  active_ = true;
  while (active_) {
    if (pre_update_ != nullptr)
      pre_update_(this);

    update_();

    if (post_update_ != nullptr)
      post_update_(this);
  }
  return status_;
}

void Beflux::bind(uint index, void (*binding)(Beflux *)) {
  index = wrap(index, 0, BINDING_MAX);
  bindings_[index] = binding;
}

void Beflux::hook(void (*pre)(Beflux *), void (*post)(Beflux *)) {
  pre_update_ = pre;
  post_update_ = post;
}

////////////////////////////////////////////////////////////////////////////////
// Beflux Private Methods
////////////////////////////////////////////////////////////////////////////////
void Beflux::update_(void) {
  eval_(segment_[PROGRAM].get());

  segment_[PROGRAM].cursor_advance();
}

void Beflux::eval_(byte op) {
  Block &prog = segment_[PROGRAM];
  Block &mem  = segment_[MEMORY];
  Block &in   = segment_[INPUT];
  Block &out  = segment_[OUTPUT];

  switch(op) { // Operation :: (Stack Args) -> (Stack Returns)

  default:
    break;

  case ' ': { // Skip Whitespace :: ( ) -> ( )
    for (uint i = 0; i < BLOCK_MAX; ++i) {
      prog.cursor_advance();
      if (prog.get() != ' ')
        break;
    }
  } break;

  case '*': { // Multiply :: (a b) -> (a * b)
    byte b = mem.pop();
    byte a = mem.pop();
    mem.push(a * b);
  } break;

  // case ','

  case '+': { // Add :: (a b) -> (a + b)
    byte b = mem.pop();
    byte a = mem.pop();
    mem.push(a + b);
  } break;

  case '-': { // Subtract :: (a b) -> (a - b)
    byte b = mem.pop();
    byte a = mem.pop();
    mem.push(a - b);
  } break;

  case '.': { // Print :: (a) -> ( )
    out.push(mem.pop());
  } break;

  case '/': { // Divide :: (a b) -> (a / b)
    byte b = mem.pop();
    byte a = mem.pop();
    if (b)
      mem.push(a / b);
    else
      mem.push(0xFF);
  } break;

  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9': { // Construct Byte :: ( ) -> (value_)
    value_ <<= 4;
    value_ |= op - '0';
    if (value_ready_) {
      mem.push(value_);
      value_ = 0;
      value_ready_ = false;
    }
    else {
      value_ready_ = true;
    }
  } break;

  case ':': { // Duplicate :: (a) -> (a a)
    mem.push(mem.get());
  } break;

  case ';': { // Comment :: ( ) -> ( )
    for (uint i = 0; i < BLOCK_MAX; ++i) {
      if (prog.get() != ';')
        prog.cursor_advance();
      else
        break;
    }
  } break;

  case '<': { // Move West :: ( ) -> ( )
    prog.cursor_set_velocity(CURSOR_W);
  } break;

  case '=': { // Equality :: (a b) -> (a == b)
    byte b = mem.pop();
    byte a = mem.pop();
    mem.push(a == b);
  } break;

  case '>': { // Move East :: ( ) -> ( )
    prog.cursor_set_velocity(CURSOR_E);
  } break;

  case '@': { // Exit :: ( ) -> ( )
    ++t_minor_;
    prog.cursor_reset();
  } break;

  case 'C': { // Subroutine Call :: (s) -> ( )
    byte s = mem.pop();
    mem.push(prog.cursor_get_position());
    mem.push(prog.cursor_get_velocity());
    mem.push(s);
    eval_('\\');
    eval_('J');
    eval_('{');
  } break;

  case 'F': { // Call Bound Function :: (n) -> (F_n(this))
    bindings_[mem.pop()](this);
  } break;

  case 'G': { // Get from Memory :: (ds) -> (mem.get(mem.cursor_.s + ds))
    byte ds = mem.pop();

    if (mem.row_wrap)
      ds %= BLOCK_W;

    if (mem.cursor_get_velocity())
      ds *= mem.cursor_get_velocity();

    mem.push(mem.get(mem.cursor_get_position() + ds));
  } break;

  case 'I': { // Load Input :: (0 "gnirts") -> ( )
    std::string filename;
    for (uint i = 0; i < BLOCK_W; ++i) {
      byte c = mem.pop();
      if (c == '\0')
        break;
      else if (std::isalnum(c))
        filename.push_back(c);
    }
    filename += ".bfx";
    in.load(filename.c_str());
    eval_('@');
  } break;

  case 'J': { // Absolute Jump :: (s) -> ( )
    prog.cursor_set_position(mem.pop());
  } break;

  case 'L': { // Loop Counter :: ( ) -> (loop_counter_++)
    mem.push(loop_counter_++);
  } break;

  case 'M': { // Call Math Function :: (n) -> (M_n(this))
    byte n = mem.pop();
    switch (n) {
    case 0x00: // NOP
    default:
      mem.push(n);
      break;
    // TODO: Implement math functions.
    case 0x01: // Sine
      break;
    case 0x02: // Cosine
      break;
    case 0x03: // Tangent
      break;
    case 0x04: // Cosecant
      break;
    case 0x05: // Secant
      break;
    case 0x06: // Cotangent
      break;
    case 0x07: // Chord
      break;
    case 0x08: // Hyperbolic Sine
      break;
    case 0x09: // Hyperbolic Cosine
      break;
    case 0x0a: // Hyperbolic Tangent
      break;
    case 0x0b: // Hyperbolic Cosecant
      break;
    case 0x0c: // Hyperbolic Secant
      break;
    case 0x0d: // Hyperbolic Cotangent
      break;
    case 0x0e: // Hyperbolic Chord
      break;
    case 0x0f: // TODO: Some cool trig-related function...
      break;
    }
  } break;

  case 'N': { // Clear Memory :: ( ) -> ( )
    mem.clear();
    mem.cursor_reset();
  } break;

  case 'O': { // Save Output :: (0 "gnirts") -> ( )
    std::string filename;
    for (uint i = 0; i < BLOCK_W; ++i) {
      byte c = mem.pop();
      if (c == '\0')
        break;
      else if (std::isalnum(c))
        filename.push_back(c);
    }
    filename += ".bfx";
    out.save(filename.c_str());
  } break;

  case 'P': { // Put in Memory :: (c ds) -> ( )
    byte ds = mem.pop();
    byte c = mem.pop();

    if (mem.row_wrap)
      ds %= BLOCK_W;

    if (mem.cursor_get_velocity())
      ds *= mem.cursor_get_velocity();

    mem.set(mem.cursor_get_position() + ds, c);
  } break;

  case 'Q': { // Hard Quit :: (status) -> ( )
    eval_('q');
    eval_('N');
  } break;

  case 'R': { // Subroutine Return :: ( ) -> ( )
    eval_('}');
    eval_('J');
    prog.cursor_set_velocity(mem.pop());
    prog.cursor_set_position(mem.pop());
  } break;

  case 'T': { // Push T Major :: ( ) -> (t_major_)
    mem.push(t_major_);
  } break;

  case 'W': { // Toggle Row Wrapping :: (n) -> ( )
    byte n = mem.pop() % SEGMENT_MAX;
    segment_[n].row_wrap = !segment_[n].row_wrap;
  } break;

  case '[': { // Turn Left :: ( ) -> ( )
    switch (prog.cursor_get_velocity()) {
    case CURSOR_W:
      prog.cursor_set_velocity(CURSOR_S);
      break;
    case CURSOR_E:
      prog.cursor_set_velocity(CURSOR_N);
      break;
    case CURSOR_N:
      prog.cursor_set_velocity(CURSOR_W);
      break;
    case CURSOR_S:
      prog.cursor_set_velocity(CURSOR_E);
      break;
    default:
      prog.cursor_set_velocity(CURSOR_W);
    }
  } break;

  case '\\': { // Swap :: (a b) -> (b a)
    byte b = mem.pop();
    byte a = mem.pop();
    mem.push(b);
    mem.push(a);
  } break;

  case ']': { // Turn Right :: ( ) -> ( )
    switch (prog.cursor_get_velocity()) {
    case CURSOR_W:
      prog.cursor_set_velocity(CURSOR_N);
      break;
    case CURSOR_E:
      prog.cursor_set_velocity(CURSOR_S);
      break;
    case CURSOR_N:
      prog.cursor_set_velocity(CURSOR_E);
      break;
    case CURSOR_S:
      prog.cursor_set_velocity(CURSOR_W);
      break;
    default:
      prog.cursor_set_velocity(CURSOR_E);
    }
  } break;

  case '^': { // Move North :: ( ) -> ( )
    prog.cursor_set_velocity(CURSOR_N);
  } break;

  case '_': { // West / East Conditional :: (a) -> ( )
    if (mem.pop())
      prog.cursor_set_velocity(CURSOR_W);
    else
      prog.cursor_set_velocity(CURSOR_E);
  } break;

  case '`': { // Greater Than :: (a b) -> (a > b)

  } break;

  case 'a':
  case 'b':
  case 'c':
  case 'd':
  case 'e':
  case 'f': { // Construct Byte :: ( ) -> (value_)
    value_ <<= 4;
    value_ |= op - 'a' + 10;
    if (value_ready_) {
      mem.push(value_);
      value_ = 0;
      value_ready_ = false;
    }
    else {
      value_ready_ = true;
    }
  } break;

  case 'g': { // Get from Program :: (ds) -> (prog.get(prog.cursor_.s + ds))
    byte ds = mem.pop();

    if (prog.row_wrap)
      ds %= BLOCK_W;

    if (prog.cursor_get_velocity())
      ds *= prog.cursor_get_velocity();

    mem.push(prog.get(prog.cursor_get_position() + ds));
  } break;

  case 'i': { // Load Program :: (0 "gnirts") -> ( )
    std::string filename;
    for (uint i = 0; i < BLOCK_W; ++i) {
      byte c = mem.pop();
      if (c == '\0')
        break;
      else if (std::isalnum(c))
        filename.push_back(c);
    }
    filename += ".bfx";
    prog.load(filename.c_str());
    eval_('@');
  } break;

  case 'j': { // Relative Jump :: (ds) -> ( )
    byte ds = mem.pop();

    if (prog.row_wrap)
      ds %= BLOCK_W;

    if (prog.cursor_get_velocity())
      ds *= prog.cursor_get_velocity();

    if (ds)
      prog.cursor_set_position(prog.cursor_get_position() + ds);
  } break;

  case 'k': { // Iterate :: (k) -> ( )
    byte k = mem.pop();
    prog.cursor_advance();
    if (k) {
      for (uint i = 0; i < k; ++i) {
        eval_(prog.get());
      }
    }
    prog.cursor_advance();
  } break;

  case 'n': { // Clear Row of Memory :: ( ) -> ( )
    if (mem.row_wrap) {
      for (uint i = 0; i < BLOCK_W; ++i) {
        mem.set(0x00);
        mem.cursor_advance();
      }
    }
    else {
      mem.clear();
    }
    mem.cursor_reset();
  } break;

  case 'o': { // Save Program :: (0 "gnirts") -> ( )
    std::string filename;
    for (uint i = 0; i < BLOCK_W; ++i) {
      byte c = mem.pop();
      if (c == '\0')
        break;
      else if (std::isalnum(c))
        filename.push_back(c);
    }
    filename += ".bfx";
    prog.save(filename.c_str());
  } break;

  case 'p': { // Put in Program :: (c ds) -> ( )
    byte ds = mem.pop();
    byte c = mem.pop();

    if (prog.row_wrap)
      ds %= BLOCK_W;

    if (prog.cursor_get_velocity())
      ds *= prog.cursor_get_velocity();

    prog.set(prog.cursor_get_position() + ds, c);
  } break;

  case 'q': { // Quit :: (status) -> ( )
    active_ = false;
    status_ = mem.pop();
    ++t_major_;
    prog.cursor_reset();
  } break;

  case 'r': { // Reflect :: ( ) -> ( )
    prog.cursor_set_velocity(prog.cursor_get_velocity() * -1);
  } break;

  case 's': { // Store in Program :: (c) -> ( )
    byte ds = prog.cursor_get_position() + prog.cursor_get_velocity();

    if (prog.row_wrap)
      ds %= BLOCK_W;

    prog.set(ds, mem.pop());
  } break;

  case 't': { // Push T Minor :: ( ) -> (t_minor_)
    mem.push(t_minor_);
  } break;

  case 'v': { // Move South :: ( ) - > ( )
    prog.cursor_set_velocity(CURSOR_S);
  } break;

  case '{': { // Next Memory Stack Frame :: ( ) -> ( )
    mem.cursor_set_position(mem.cursor_get_position() + BLOCK_W);
  } break;

  case '|': { // North / South Conditional :: (a) -> ( )
    if (mem.pop())
      prog.cursor_set_velocity(CURSOR_N);
    else
      prog.cursor_set_velocity(CURSOR_S);
  } break;

  case '}': { // Previous Memory Stack Frame :: ( ) -> ( )
    mem.cursor_set_position(mem.cursor_get_position() - BLOCK_W);
  } break;

  // TODO: Implement even more operators.
  }
}

} // namespace Dodecaplex
