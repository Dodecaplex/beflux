/*!
 * @file beflux.h
 * @date 12/10/2014
 * @author Tony Chiodo (http://dodecaplex.net)
 */

#pragma once
#include <cstdint>
#include <iostream>
#include <fstream>
#include <stack>
#include <utility>
#include <string>

namespace Dodecaplex {

/*!
 * \brief An interpreter for a simple Befunge-like programming language.
 */
struct Beflux {
  Beflux(void);
  ~Beflux(void);

  uint8_t run(void);
  void io(std::istream *in, std::ostream *out);
  void hook(void (*pre)(Beflux &), void (*post)(Beflux &));
  void bind(uint8_t index, void (*binding)(Beflux &)=nullptr);

  struct Program {
    Program(void);

    void read(const uint8_t * const src, uint8_t width, uint8_t height);
    void load(const char * const filename);
    void clear(void);

    uint8_t get(uint8_t x, uint8_t y) const;
    void set(uint8_t x, uint8_t y, uint8_t value);

    uint8_t width(void) const;
    uint8_t height(void) const;
    uint16_t size(void) const;

    friend std::ostream &operator<<(std::ostream &os, const Program &rhs);

  private:
    uint8_t *data_;
    uint8_t width_;
    uint8_t height_;

    void resize_(uint8_t width, uint8_t height);

    friend class Beflux;
  };

  Program &program(uint8_t index);
  uint8_t &current(void);

  uint8_t getX(void) const;
  uint8_t getY(void) const;

private:
  void update_(void);
  void advance_(void);
  void ip_reset_(void);
  void push_frame_(void);
  void pop_frame_(void);
  void push_(uint8_t value);
  uint8_t pop_(void);
  void eval_(uint8_t op);

  Program progs_[UINT8_MAX];
  uint8_t current_;

  uint8_t x_;
  uint8_t dx_;
  uint8_t y_;
  uint8_t dy_;

  std::stack<std::pair<uint8_t[UINT8_MAX], uint8_t>> mem_;

  std::istream *in_;
  std::ostream *out_;

  uint8_t active_;
  uint8_t status_;
  uint8_t mode_;
  uint8_t value_;
  uint8_t value_ready_;
  uint8_t t_minor_;
  uint8_t t_major_;
  uint8_t loop_count_;
  uint16_t tick_;

  void (*pre_)(Beflux &);
  void (*post_)(Beflux &);
  void (*bindings_[UINT8_MAX])(Beflux &);
};

} // namespace Dodecaplex
