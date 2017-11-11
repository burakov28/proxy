#ifndef BASE_ID_GENERATOR_H_
#define BASE_ID_GENERATOR_H_

namespace base {

class IdGenerator {
public:
  IdGenerator() : id_(0) {}
  uint64_t GetNext() {
    return id_++;
  }

private:
  uint64_t id_;
};

}  // namespace base

#endif  // BASE_ID_GENERATOR_H_
