#ifndef BASE_MACROS_H_
#define BASE_MACROS_H_

#define PROHIBIT_COPY_AND_COPY_ASSIGN(class_name)           \
  class_name(const class_name& other) = delete;             \
  class_name& operator=(const class_name& other) = delete;

#endif  // BASE_MACROS_H_
