#include "../object.h"

static object* new_int(const int64_t i) {
  object* ret = (object*)calloc(1,sizeof(object));
  massert(ret, "MemoryError: Cannot allocate space for object.");
  ret->int64 = i;
  ret->type = T_I64;
  return ret;
}
