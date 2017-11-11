#ifndef BASE_EXCEPTIONS_TERMINAL_ERROR_H_
#define BASE_EXCEPTIONS_TERMINAL_ERROR_H_

#include <exception>
#include <string.h>

class TerminalError : public std::exception {
 public:
  TerminalError() {}
  TerminalError(const TerminalError& other) = default;

  TerminalError& operator=(const TerminalError& other) = default;

  ~TerminalError() override = default;
};

#endif  // BASE_EXCEPTIONS_TERMINAL_ERROR_H_
