#include "../object.h"

static void add_elem(object* self, const object* elem) {
  massert(self, "NullError: self is not a valid object.");
  massert(elem, "NullError: element is not a valid object.");

  ++self->array.size;
  self->array.data = (object**)realloc(self->array.data, self->array.size * sizeof(object*));
  massert(self->array.data, "MemoryError: Cannot expand the element array.");
  self->array.data[self->array.size-1] = elem;
}

static void remove_elem(object* self, object* idx) {
  massert(self, "NullError: self is not a valid object.");
  massert(idx, "NullError: index is not a valid object.");
  massert(idx->int64 < self->array.size, "RangeError: Index out of range.");
  massert(self->type == T_ARR, "TypeError: self must be an array");
  massert(idx->type == T_I64, "TypeError: index must be an integer");

  obj.delete(self->array.data[idx->int64]);
  for(size_t i = idx->int64; i < self->array.size-1; i++)
    self->array.data[i] = self->array.data[i+1];
  self->array.size--;
  self->array.data = (object**)realloc(self->array.data, sizeof(object*)*self->array.size);
  obj.delete(idx);
}

static object* merge_array(const object* self, const object* other) {
  massert(self, "NullError: self is not a valid object.");
  massert(other, "NullError: other object to be merged is not a valid object");
  massert(self->type == T_ARR, "TypeError: self must be an array");
  massert(other->type == T_ARR, "TypeError: other object to be merged must be an array");

  size_t total_len = self->array.size + other->array.size;
  object* ret = obj.array(total_len, NULL);
  for(size_t i = 0; i < self->array.size; i++)
    ret->array.data[i] = self->array.data[i];
  for(size_t i = self->array.size; i < total_len; i++)
    ret->array.data[i] = other->array.data[i-self->array.size];
}

static object* new_arr(const size_t elem_num, ...) {
  object* ret = (object*)calloc(1,sizeof(object));
  massert(ret, "MemoryError: Cannot allocate space for object.");
  ret->type = T_ARR;
  va_list args;
  va_start(args, elem_num);
  for(register size_t i = 0; i < elem_num; ++i) {
    object* elem = va_arg(args, object*);
    if(elem == NULL) {
      va_end(args);
      return ret;
    }
    add_elem(ret, elem);
  }
  va_end(args);
  return ret;
}