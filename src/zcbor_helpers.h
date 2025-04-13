#ifndef ZCBOR_HELPERS_H
#define ZCBOR_HELPERS_H

#include <etl/string_view.h>
#include <zcbor_common.h>

inline etl::string_view to_string_view(const zcbor_string& zcbor) {
  return etl::string_view(reinterpret_cast<const char*>(zcbor.value), zcbor.len);
}

#define ZCBOR_REQUIRE_PRESENT(parent, field)           \
  if (!parent.field##_present) {                       \
    ULOG_ERROR("Attribute " #field " is not defined"); \
    return nullptr;                                    \
  }

#endif  // ZCBOR_HELPERS_H
