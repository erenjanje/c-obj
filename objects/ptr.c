#include "../object.h"

static object* new_ptr(const void* data, const size_t size) {
  massert(data, "NullError: Given data is not valid.");
  object* ret = (object*)calloc(1,sizeof(object));
  massert(ret, "MemoryError: Cannot allocate space for object.");
  ret->pointer.size = size;
  ret->pointer.data = (void*)malloc(ret->pointer.size);
  massert(ret->pointer.data, "MemoryError: Cannot allocate space for pointer.");
  memcpy(ret->pointer.data, data, ret->pointer.size);
  ret->type = T_PTR;
  return ret;
}