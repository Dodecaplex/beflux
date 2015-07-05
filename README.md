:: beflux ::
============

Beflux is an embeddable interpreter for a Befunge-like programming language.

Unlike more traditional languages that execute linearly, control flow in a
Beflux program relies on the movement of an instruction pointer in a
two-dimensional grid of single-character operators.

To compile and run unit tests: make && beflux examples/test

Overview
--------
  * Unsigned 8-bit integer arithmetic
  * 256 x 256 character program space
  * 256 registers
  * 128 built-in operators
  * 256 user-definable functions
  * Dynamic program loading (up to 256 programs stored in memory)
  * Simple string operations
  * File I/O
  * Easy to embed in C projects

Opcodes
-------

Operator - NAME (In : Out) - Description

 * ' ' - SKIP (0:0) - Skip to next non-space character.
 * '!' - NOT (1:1) - Boolean negation.
 * '"' - STR (0:0) - Toggle string mode.
 * '#' - HOP (0:0) - Skip the next character.
 * '$' - POP (1:0) - Pop from the current stack.
 * '%' - MOD (2:1) - Calculate remainder.
 * '&' - GETX (0:?) - Reads a single hex digit from input.
 * ''' - OVER (2:3) - Copies the value under the top of the stack.
 * '(' - PSHF (0:0) - Pushes a new stack frame.
 * ')' - POPF (0:0) - Pops the current stack frame.
 * '\*' - MUL (2:1) - Calculate product.
 * '+' - ADD (2:1) - Calculate sum.
 * ',' - PUTC (1:0) - Writes an ASCII character to output.
 * '-' - SUB (2:1) - Calculate difference.
 * '.' - PUTX (1:0) - Writes a hexadecimal value to output.
 * '/' - DIV (2:1) - Calculate quotient.
 * '0' - V0 (0:?) - Hex digit 0.
 * '1' - V1 (0:?) - Hex digit 1.
 * '2' - V2 (0:?) - Hex digit 2.
 * '3' - V3 (0:?) - Hex digit 3.
 * '4' - V4 (0:?) - Hex digit 4.
 * '5' - V5 (0:?) - Hex digit 5.
 * '6' - V6 (0:?) - Hex digit 6.
 * '7' - V7 (0:?) - Hex digit 7.
 * '8' - V8 (0:?) - Hex digit 8.
 * '9' - V9 (0:?) - Hex digit 9.
 * ':' - DUP (1:2) - Duplicate current stack top.
 * ';' - COM (0:0) - Skip to next comment character.
 * '<' - MVW (0:0) - Change direction to West.
 * '=' - EQ (2:1) - Test for equality.
 * '>' - MVE (0:0) - Change direction to East.
 * '?' - AWAY (0:0) - Change direction at random.
 * '@' - REP (0:0) - Reset the IP and increment t_minor.
 * 'A' - PRVP (0:0) - Decrement program index.
 * 'B' - REV (0:0) - Reverse IP direction.
 * 'C' - CALL (2:0) - Jump to position using call stack.
 * 'D' - DICE (2:1) - Push a random number in the given range.
 * 'E' - EOF (0:1) - Return whether end of input has been reached.
 * 'F' - FUNC (1:?) - Call a user-defined function.
 * 'G' - GETP (3:1) - Push character from a program.
 * 'H' - HOME (0:0) - Set program index to 0.
 * 'I' - FIN (str:0) - Open input file.
 * 'J' - JMP (2:0) - Jump to specified position.
 * 'K' - DUPF (0:?) - Pushes a copy of the current stack frame.
 * 'L' - LEND (0:0) - Resets the loop counter.
 * 'M' - CLRS (0:0) - Clears all stack frames.
 * 'N' - CLRF (0:0) - Clear the current stack frame.
 * 'O' - FOUT (str:0) - Open output file.
 * 'P' - LOAD (1,str:0) - Load program into specified index.
 * 'Q' - QUIT (0:0) - End execution.
 * 'R' - RET (0:0) - Return from a CALL.
 * 'S' - SETP (4:0) - Set a specified program character.
 * 'T' - TMAJ (0:1) - Push the major timer.
 * 'U' - CURP (0:1) - Push the current program index.
 * 'V' - NXTP (0:0) - Incrememnt the program index.
 * 'W' - WRAP (1:0) - Set wrapping offset.
 * 'X' - EXEP (1:0) - Execute program.
 * 'Y' - CLRR (0:0) - Clear all registers.
 * 'Z' - RAND (0:1) - Push a random value.
 * '[' - TRNL (0:0) - Turn IP left.
 * '\' - SWP (2:2) - Swap the top two values on the current stack.
 * ']' - TRNR (0:0) - Turn IP right.
 * '^' - MVN (0:0) - Change direction to North.
 * '\_' - WEIF (1:0) - West / East conditional branch.
 * '\`' - GT (2:1) - Compare values.
 * 'a' - VA (0:?) - Hex digit A.
 * 'b' - VB (0:?) - Hex digit B.
 * 'c' - VC (0:?) - Hex digit C.
 * 'd' - VD (0:?) - Hex digit D.
 * 'e' - VE (0:?) - Hex digit E.
 * 'f' - VF (0:?) - Hex digit F.
 * 'g' - GETR (1:1) - Push value from given register.
 * 'h' - BMPN (0:0) - Bump IP North.
 * 'i' - GETS (0:str) - Read null-terminated string or line from input.
 * 'j' - JREL (2:0) - Jump relative to current IP position.
 * 'k' - ITER (1:?) - Iterate next instruction.
 * 'l' - LOOP (0:1) - Push and increment loop counter.
 * 'm' - NIF (1:0) - North / Continue conditional branch.
 * 'n' - ENDL (0:0) - Write newline to output.
 * 'o' - PUTS (str:0) - Write null-terminated string to output.
 * 'p' - SWPR (2:1) - Swap top of current stack with value in register.
 * 'q' - EXIT (1:0) - End execution with status.
 * 'r' - REVS (str:str) - Reverse string on stack.
 * 's' - SETR (2:0) - Set given register.
 * 't' - TMIN (0:1) - Push the minor timer.
 * 'u' - JOIN (str,str:str) - Join two strings on the stack.
 * 'v' - MVS (0:0) - Change direction to South.
 * 'w' - SIF (1:0) - South / Continue conditional branch.
 * 'x' - EXEC (1:?) - Execute operator.
 * 'y' - BMPS (0:0) - Bump IP South.
 * 'z' - WAIT (1:0) - Sleep for the given number of seconds.
 * '{' - BLK (1:0) - Conditional block.
 * '|' - NSIF (1:0) - North / South conditional branch.
 * '}' - BEND (0:0) - Block end.
 * '~' - GETC (0:1) - Reads an ASCII character from input.
 * 'DEL' - NOP (0:0) - Does nothing!
