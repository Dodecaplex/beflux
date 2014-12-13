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
  case 0x00:
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

  case '@': { // Exit :: ( ) -> ( )
    ++t_minor_;
    prog.cursor_reset();
  } break;

  case 'Q': { // Quit :: (status) -> ( )
    active_ = false;
    status_ = mem.pop();
    ++t_major_;
    prog.cursor_reset();
  } break;

  case 'T': { // Push T Major :: ( ) -> (t_major_)
    mem.push(t_major_);
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

  case 'i': { // Load Input :: (0 "gnirts") -> ( )
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

  case 'j': { // Relative Jump :: (s) -> ( )
    byte ds = mem.pop();
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
    uint row_length = mem.row_wrap ? BLOCK_W : BLOCK_MAX;
    for (uint i = 0; i < row_length; ++i) {
      mem.set(0x00);
      mem.cursor_advance();
    }
    mem.cursor_set_position(0);
  } break;

  case 'o': { // Save Output :: (0 "gnirts") -> ( )
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

  case 't': { // Push T Minor :: ( ) -> (t_minor_)
    mem.push(t_minor_);
  } break;

  // TODO: Implement even more operators.
  }
}

} // namespace Dodecaplex
