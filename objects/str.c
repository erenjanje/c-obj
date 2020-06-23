#include "../object.h"

static object* new_str(const char* s) {
  object* ret = (object*)calloc(1,sizeof(object));
  massert(ret, "MemoryError: Cannot allocate space for object.");
  ret->string.len = strlen(s);
  ret->string.data = (char*)calloc(ret->string.len, sizeof(char));
  memcpy(ret->string.data, s, ret->string.len);
  ret->type = T_STR;
  return ret;
}