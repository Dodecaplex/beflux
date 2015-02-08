// :: beflux.cpp :: 12/10/2014
// Tony Chiodo - http://dodecaplex.net
////////////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <cmath>
#include <fstream>
#include <string>

#include "beflux.h"

namespace Dodecaplex {

static const double PI = std::acos(-1.0);
static const double TAU = 2 * PI;

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
Beflux::Block::Block(const unsigned char * const src, unsigned int count) {
  Block();
  read(src, count);
}

// Initialize from File
Beflux::Block::Block(const char * const filename) {
  Block();
  load(filename);
}

void Beflux::Block::clear(void) {
  for (unsigned int i = 0; i < BLOCK_MAX; ++i) {
    data[i] = 0;
  }
  cursor_reset();
}

void Beflux::Block::read(const unsigned char * const src, unsigned int count) {
  for (unsigned int i = 0; i < count && i < BLOCK_MAX; ++i) {
    data[i] = src[i];
  }
}

void Beflux::Block::write(unsigned char * const dst, unsigned int count) const {
  for (unsigned int i = 0; i < count && i < BLOCK_MAX; ++i) {
    dst[i] = data[i];
  }
}

void Beflux::Block::load(const char * const filename) {
  std::ifstream fin(filename);
  for (unsigned int i = 0; i < BLOCK_MAX; ++i) {
    if (fin.good())
      data[i] = (unsigned char)(fin.get());
    else
      data[i] = 0;
  }
}

void Beflux::Block::save(const char * const filename) const {
  std::ofstream fout(filename);
  for (unsigned int i = 0; i < BLOCK_MAX; ++i) {
    fout << data[i];
  }
}

void Beflux::Block::set(unsigned char value) {
  data[cursor_.s] = value;
}

void Beflux::Block::set(unsigned int s, unsigned char value) {
  s %= BLOCK_MAX;
  data[s] = value;
}

void Beflux::Block::set(unsigned int x, unsigned int y, unsigned char value) {
  x %= BLOCK_W;
  y %= BLOCK_H;
  data[x + BLOCK_W * y] = value;
}

unsigned char Beflux::Block::get(void) const {
  return data[cursor_.s];
}

unsigned char Beflux::Block::get(unsigned int s) const {
  s %= BLOCK_MAX;
  return data[s];
}

unsigned char Beflux::Block::get(unsigned int x, unsigned int y) const {
  x %= BLOCK_W;
  y %= BLOCK_H;
  return data[x + BLOCK_W * y];
}

void Beflux::Block::push(unsigned char x) {
  set(x);
  cursor_advance();
}

unsigned char Beflux::Block::pop(void) {
  cursor_backtrack();
  auto x = get();
  set(0);
  return x;
}

void Beflux::Block::cursor_set_position(unsigned int s) {
  cursor_.s = s % BLOCK_MAX;
}

void Beflux::Block::cursor_set_velocity(unsigned int ds) {
  cursor_.ds = ds;
}

unsigned int Beflux::Block::cursor_get_position(void) const {
  return cursor_.s;
}

unsigned int Beflux::Block::cursor_get_velocity(void) const {
  return cursor_.ds;
}

void Beflux::Block::cursor_advance(void) {
  if (row_wrap) {
    unsigned int sx = (cursor_.s + cursor_.ds % BLOCK_W) % BLOCK_W;
    unsigned int sy = (cursor_.s + cursor_.ds / BLOCK_W) % BLOCK_H;
    cursor_.s = sx + BLOCK_W * sy;
  }
  else {
    cursor_.s = (cursor_.s + cursor_.ds) % BLOCK_MAX;
  }
}

void Beflux::Block::cursor_backtrack(void) {
  if (row_wrap) {
    unsigned int sx = (cursor_.s + cursor_.ds % BLOCK_W) % BLOCK_W;
    unsigned int sy = (cursor_.s + cursor_.ds / BLOCK_W) % BLOCK_H;
    cursor_.s = sx + BLOCK_W * sy;
  }
  else {
    cursor_.s = (cursor_.s + cursor_.ds) % BLOCK_MAX;
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

unsigned char Beflux::run(void) {
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

void Beflux::bind(unsigned int index, void (*binding)(Beflux *)) {
  index %= BINDING_MAX;
  bindings_[index] = binding;
}

void Beflux::hook(void (*pre)(Beflux *), void (*post)(Beflux *)) {
  pre_update_ = pre;
  post_update_ = post;
}

unsigned char Beflux::sin(unsigned char x) {
  return 0x80 + 0x7f * std::sin(bamtorad(x));
}

unsigned char Beflux::cos(unsigned char x) {
  return 0x80 + 0x7f * std::cos(bamtorad(x));
}

unsigned char Beflux::tan(unsigned char x) {
  return sin(x) / cos(x);
}

unsigned char Beflux::degtobam(double degs) {
  return 256 * degs / 360.0;
}

double Beflux::bamtodeg(unsigned char bams) {
  return 360.0 * bams / 256.0;
}

unsigned char Beflux::radtobam(double rads) {
  return 256 * rads / TAU;
}

double Beflux::bamtorad(unsigned char bams) {
  return TAU * bams / 256.0;
}

////////////////////////////////////////////////////////////////////////////////
// Beflux Private Methods
////////////////////////////////////////////////////////////////////////////////
void Beflux::update_(void) {
  eval_(segment_[PROGRAM].get());

  segment_[PROGRAM].cursor_advance();
}

void Beflux::eval_(unsigned char op) {
  Block &prog = segment_[PROGRAM];
  Block &mem  = segment_[MEMORY];
  Block &in   = segment_[INPUT];
  Block &out  = segment_[OUTPUT];

  switch(op) { // Operation :: (Stack Args) -> (Stack Returns)

    default: // NOP :: ( ) -> ( )
    break;

  case ' ': { // Skip Whitespace :: ( ) -> ( )
    for (unsigned int i = 0; i < BLOCK_MAX; ++i) {
      prog.cursor_advance();
      if (prog.get() != ' ')
        break;
    }
  } break;

  case '*': { // Multiply :: (a b) -> (a * b)
    auto b = mem.pop();
    auto a = mem.pop();
    mem.push(a * b);
  } break;

  // case ','

  case '+': { // Add :: (a b) -> (a + b)
    auto b = mem.pop();
    auto a = mem.pop();
    mem.push(a + b);
  } break;

  case '-': { // Subtract :: (a b) -> (a - b)
    auto b = mem.pop();
    auto a = mem.pop();
    mem.push(a - b);
  } break;

  case '.': { // Print :: (a) -> ( )
    out.push(mem.pop());
  } break;

  case '/': { // Divide :: (a b) -> (a / b)
    auto b = mem.pop();
    auto a = mem.pop();
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
    for (unsigned int i = 0; i < BLOCK_MAX; ++i) {
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
    auto b = mem.pop();
    auto a = mem.pop();
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
    auto s = mem.pop();
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
    auto ds = mem.pop();

    if (mem.row_wrap)
      ds %= BLOCK_W;

    if (mem.cursor_get_velocity())
      ds *= mem.cursor_get_velocity();

    mem.push(mem.get(mem.cursor_get_position() + ds));
  } break;

  case 'I': { // Load Input :: (0 "gnirts") -> ( )
    std::string filename;
    for (unsigned int i = 0; i < BLOCK_W; ++i) {
      auto c = mem.pop();
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

  case 'M': { // Call Math Function :: (n x) -> (M_n(x))
    auto n = mem.pop();
    switch (n) {
    case 0x00:
    default: mem.pop(); break;
    case 0x01: mem.push(sin(mem.pop())); break;
    case 0x02: mem.push(cos(mem.pop())); break;
    case 0x03: mem.push(tan(mem.pop())); break;
    }
  } break;

  case 'N': { // Clear Memory :: ( ) -> ( )
    mem.clear();
    mem.cursor_reset();
  } break;

  case 'O': { // Save Output :: (0 "gnirts") -> ( )
    std::string filename;
    for (unsigned int i = 0; i < BLOCK_W; ++i) {
      auto c = mem.pop();
      if (c == '\0')
        break;
      else if (std::isalnum(c))
        filename.push_back(c);
    }
    filename += ".bfx";
    out.save(filename.c_str());
  } break;

  case 'P': { // Put in Memory :: (c ds) -> ( )
    auto ds = mem.pop();
    auto c = mem.pop();

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
    auto n = mem.pop() % SEGMENT_MAX;
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
    auto b = mem.pop();
    auto a = mem.pop();
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
    auto ds = mem.pop();

    if (prog.row_wrap)
      ds %= BLOCK_W;

    if (prog.cursor_get_velocity())
      ds *= prog.cursor_get_velocity();

    mem.push(prog.get(prog.cursor_get_position() + ds));
  } break;

  case 'i': { // Load Program :: (0 "gnirts") -> ( )
    std::string filename;
    for (unsigned int i = 0; i < BLOCK_W; ++i) {
      auto c = mem.pop();
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
    auto ds = mem.pop();

    if (prog.row_wrap)
      ds %= BLOCK_W;

    if (prog.cursor_get_velocity())
      ds *= prog.cursor_get_velocity();

    if (ds)
      prog.cursor_set_position(prog.cursor_get_position() + ds);
  } break;

  case 'k': { // Iterate :: (k) -> ( )
    auto k = mem.pop();
    prog.cursor_advance();
    if (k) {
      for (unsigned int i = 0; i < k; ++i) {
        eval_(prog.get());
      }
    }
    prog.cursor_advance();
  } break;

  case 'n': { // Clear Row of Memory :: ( ) -> ( )
    if (mem.row_wrap) {
      for (unsigned int i = 0; i < BLOCK_W; ++i) {
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
    for (unsigned int i = 0; i < BLOCK_W; ++i) {
      auto c = mem.pop();
      if (c == '\0')
        break;
      else if (std::isalnum(c))
        filename.push_back(c);
    }
    filename += ".bfx";
    prog.save(filename.c_str());
  } break;

  case 'p': { // Put in Program :: (c ds) -> ( )
    auto ds = mem.pop();
    auto c = mem.pop();

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
    unsigned char ds = prog.cursor_get_position() + prog.cursor_get_velocity();

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
