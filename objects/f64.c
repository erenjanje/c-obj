#include "../object.h"

static object* new_num(const double d) {
  object* ret = (object*)calloc(1,sizeof(object));
  massert(ret, "MemoryError: Cannot allocate space for object.");
  ret->float64 = d;
  ret->type = T_F64;
  return ret;
}