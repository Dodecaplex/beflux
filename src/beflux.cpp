// :: beflux.cpp :: 12/10/2014
// Tony Chiodo - http://dodecaplex.net
////////////////////////////////////////////////////////////////////////////////

#include "beflux.h"

namespace Dodecaplex {

Beflux::Beflux(void) {
  current_ = 0;
  x_ = 0;
  dx_ = 1;
  y_ = 0;
  dy_ = 0;
  in_ = &std::cin;
  out_ = &std::cout;
  active_ = status_ = mode_ = value_ = 0;
  value_ready = t_minor_ = t_major_ = loop_count_ = 0;
  tick_ = 0;
  pre_ = post_ = nullptr;
  for (auto &i: bindings_) i = nullptr;
}

Beflux::~Beflux(void) {
  for (auto &i: progs_) {
    i.clear();
  }
}

uint8_t Beflux::run(void) {
  active_ = true;
  while (active_) { // MAIN LOOP
    if (pre_ != nullptr) pre_(*this);
    update_();
    if (post_ != nullptr) post_(*this);
  }
  return status_;
}

void Beflux::io(std::istream *in, std::ostream *out) {
  in_ = in;
  out_ = out;
}

void Beflux::hook(void (*pre)(Beflux &), void (*post)(Beflux &)) {
  pre_ = pre;
  post_ = post;
}

void Beflux::bind(uint8_t index, void (*binding)(Beflux &)) {
  bindings_[index] = binding;
}

Beflux::Program::Program(void) {
  data_ = nullptr;
  width_ = height_ = 0;
}

void Beflux::Program::read(const uint8_t * const src, uint8_t width, uint8_t height) {
  if (width_ != width || height_ != height) resize_(width, height);
  for (uint16_t i = 0; i < size(); ++i) {
    data_[i] = src[i];
  }
}

void Beflux::Program::load(const char * const filename) {
  std::string str;
  std::ifstream fin(filename);
  if (fin.good()) {
    fin.ignore(256, '\n'); // Ignore first line (header)
    unsigned int w, h;
    fin >> w >> h;
    fin.ignore(256, '\n'); // Ignore remainder of second line
    resize_(w, h);
    for (uint8_t j = 0; j < height_; ++j) {
      uint16_t offset = j * width_;
      std::getline(fin, str);
      for (uint8_t i = 0; i < width_; ++i) {
        uint8_t c;
        if (i >= str.length()) { // Fill short lines with spaces
          c = ' ';  
        }
        else {
          c = str[i];
        }
        data_[offset + i] = c; // Otherwise, copy the character normally.
      }
    }
  }
  else {
    std::cerr << "Error loading program from " << filename << std::endl;
  }
}

void Beflux::Program::clear(void) {
  if (data_ != nullptr) {
    delete [] data_;
    data_ = nullptr;
    width_ = height_ = 0;
  }
}

uint8_t Beflux::Program::get(uint8_t x, uint8_t y) const {
  if (data_ == nullptr) return 0;
  return data_[x + y * width_];
}

void Beflux::Program::set(uint8_t x, uint8_t y, uint8_t value) {
  if (data_ == nullptr) return;
  data_[x + y * width_] = value;
}

uint8_t Beflux::Program::width(void) const { return width_; }
uint8_t Beflux::Program::height(void) const { return height_; }
uint16_t Beflux::Program::size(void) const { return width_ * height_; }

std::ostream &operator<<(std::ostream &os, const Beflux::Program &rhs) {
  for (uint8_t j = 0; j < rhs.height_; ++j) {
    for (uint8_t i = 0; i < rhs.width_; ++i) {
      os << rhs.get(i, j);
    }
    os << std::endl;
  }
  return os;
}

void Beflux::Program::resize_(uint8_t width, uint8_t height) {
  if (data_ != nullptr) delete [] data_;
  width_ = width;
  height_ = height;
  data_ = new uint8_t[width_ * height_];
}

Beflux::Program &Beflux::program(uint8_t index) {
  return progs_[index];
}

uint8_t &Beflux::current(void) {
  return current_;
}

void Beflux::update_(void) {
  eval_(progs_[current_].get(x_, y_));
  advance_();
  ++tick_;
}

void Beflux::advance_(void) {
  x_ = (x_ + dx_) % progs_[current_].width_;
  y_ = (y_ + dy_) % progs_[current_].height_;
}

void Beflux::push_(uint8_t value) {
  mem_.top().first[mem_.top().second++] = value;
}

uint8_t Beflux::pop_(void) {
  uint8_t value = mem_.top().first[--mem_.top().second];
  mem_.top().first[mem_.top().second] = 0;
  return value;
}

void Beflux::eval_(uint8_t op) {
  Program &prog = progs_[current_]; // Ref to current program

  switch (op) {
    default: break;
    case ' ': { // SKIP
      for (uint16_t i = 0; i < prog.size(); ++i) {
        advance_();
        if (prog.get(x_, y_) != ' ') break;
      }
    } break;
    case '!': {
    } break;
    case '"': { // STRING MODE
    } break;
    case '#': { // TRAMPOLINE
    } break;
    case '$': {
    } break;
    case '%': { // MOD
    } break;
    case '&': {
    } break;
    case '\'': {
    } break;
    case '(': {
    } break;
    case ')': {
    } break;
    case '*': { // MUL
    } break;
    case '+': { // ADD
    } break;
    case ',': {
    } break;
    case '-': { // SUB
    } break;
    case '.': {
    } break;
    case '/': { // DIV
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
    case '9': { // VALUE
    } break;
    case ':': { // DUP
    } break;
    case ';': { // COMMENT
    } break;
    case '<': { // GO WEST
    } break;
    case '=': { // EQ
    } break;
    case '>': { // GO EAST
    } break;
    case '?': { // GO AWAY
    } break;
    case '@': { // EXIT
    } break;
    case 'A': {
    } break;
    case 'B': {
    } break;
    case 'C': { // SUBCALL
    } break;
    case 'D': {
    } break;
    case 'E': {
    } break;
    case 'F': { // FUNCTION
    } break;
    case 'G': { // MEMGET
    } break;
    case 'H': {
    } break;
    case 'I': { // READIN
    } break;
    case 'J': { // ABSJUMP
    } break;
    case 'K': {
    } break;
    case 'L': { // LOOPCOUNT
    } break;
    case 'M': { // MATH
    } break;
    case 'N': { // MEMCLEAR
    } break;
    case 'O': { // WRITEOUT
    } break;
    case 'P': { // MEMPUT
    } break;
    case 'Q': { // KILL
    } break;
    case 'R': { // SUBRET
    } break;
    case 'S': {
    } break;
    case 'T': { // TMAJOR
    } break;
    case 'U': {
    } break;
    case 'V': {
    } break;
    case 'W': { // WRAP
    } break;
    case 'X': {
    } break;
    case 'Y': {
    } break;
    case 'Z': {
    } break;
    case '[': { // TURN LEFT
    } break;
    case '\\': { // SWAP
      } break;
    case ']': { // TURN RIGHT
    } break;
    case '^': { // GO NORTH
    } break;
    case '_': { // W/E IF
    } break;
    case '`': { // GREATERTHAN
    } break;
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f': { // VALUE
    } break;
    case 'g': { // PROGGET
    } break;
    case 'h': {
    } break;
    case 'i': { // READPROG
    } break;
    case 'j': { // RELJUMP
    } break;
    case 'k': { // ITER
    } break;
    case 'l': {
    } break;
    case 'm': {
    } break;
    case 'n': { // FRAMECLEAR
    } break;
    case 'o': { // WRITEPROG
    } break;
    case 'p': { // PROGPUT
    } break;
    case 'q': { // QUIT
    } break;
    case 'r': { // REFLECT
    } break;
    case 's': { // PROGSTORE
    } break;
    case 't': { // TMINOR
    } break;
    case 'u': {
    } break;
    case 'v': { // GO SOUTH
    } break;
    case 'w': {
    } break;
    case 'x': {
    } break;
    case 'y': {
    } break;
    case 'z': {
    } break;
    case '{': { // PUSHFRAME
    } break;
    case '|': { // N/S IF
    } break;
    case '}': { // POPFRAME
    } break;
    case '~': {
    } break;
  }
}

} // namespace Dodecaplex

// TEST
int main(void) {
  Dodecaplex::Beflux b;
  b.program(0).load("input.bfx");
  // std::cout << b.program(0) << std::endl;
  return 0;
}
