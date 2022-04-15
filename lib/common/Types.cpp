#include "common/Types.hpp"

namespace typeart {

const type_id_t type_id_t::unknown_type = {};

std::ostream& operator<<(std::ostream& os, const type_id_t& alloc_id) {
  os << "type_id(" << alloc_id.value() << ")";
  return os;
}

const alloc_id_t alloc_id_t::invalid = {};

std::ostream& operator<<(std::ostream& os, const alloc_id_t& alloc_id) {
  os << "alloc_id(" << alloc_id.value() << ")";
  return os;
}

}  // namespace typeart
