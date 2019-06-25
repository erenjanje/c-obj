#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include "object.h"

static uint64_t hash(const object*);
static uint64_t hash_array(const object*);
static uint64_t hash_dict(const object*);

typedef struct object {
  union {
    int64_t int64;
    double float64;
    struct {
      size_t len;
      char* data;
    } string;
    struct {
      size_t size;
      void* data;
    } pointer;
    struct {
      size_t size;
      object** data;
    } array;
    struct {
      size_t size;
      struct entry {
        uint64_t hash;
        object* key;
        object* value;
      }* data;
    } dict;
  };
  enum {
    T_NIL,
    T_I64,
    T_F64,
    T_STR,
    T_PTR,
    T_ARR,
    T_TAB
  } type;
} object;

const char* obj_type(const object* obj) {
  switch(obj->type) {
  case T_NIL:
    return "null";
  case T_I64:
    return "int64";
  case T_F64:
    return "float64";
  case T_STR:
    return "string";
  case T_PTR:
    return "pointer";
  case T_ARR:
    return "array";
  case T_TAB:
    return "dict";
  }
}

object* new_int(const int64_t i) {
  object* ret = (object*)calloc(1,sizeof(object));
  ret->int64 = i;
  ret->type = T_I64;
  return ret;
}

object* new_num(const double d) {
  object* ret = (object*)calloc(1,sizeof(object));
  ret->float64 = d;
  ret->type = T_F64;
  return ret;
}

object* new_str(const char* s) {
  object* ret = (object*)calloc(1,sizeof(object));
  ret->string.len = strlen(s);
  ret->string.data = (char*)calloc(ret->string.len, sizeof(char));
  memcpy(ret->string.data, s, ret->string.len);size_t size;
  ret->type = T_STR;
  return ret;
}

object* new_ptr(const void* data, const size_t size) {
  object* ret = (object*)calloc(1,sizeof(object));
  ret->pointer.size = size;
  ret->pointer.data = (void*)malloc(ret->pointer.size);
  memcpy(ret->pointer.data, data, ret->pointer.size);
  ret->type = T_PTR;
  return ret;
}

void add_elem(object* self, const object* elem) {
  if(self->type != T_ARR) {
    fprintf(stderr, "E_TYPE: Element cannot be added to a %s.\n", obj_type(self));
    exit(EXIT_FAILURE);
  }

  ++self->array.size;
  self->array.data = (object**)realloc(self->array.data, self->array.size * sizeof(object*));
  self->array.data[self->array.size-1] = elem;
}

object* new_arr(const size_t elem_num, ...) {
  object* ret = (object*)calloc(1,sizeof(object));
  ret->type = T_ARR;
  va_list args;
  va_start(args, elem_num);
  for(register size_t i = 0; i < elem_num; ++i) {
    const object* elem = va_arg(args, const object*);
    add_elem(ret, elem);
  }
  va_end(args);
  return ret;
}

static uint64_t hash_array(const object* self) {
  uint64_t ret = 0;
  size_t len = self->array.size;
  const object** ptr = self->array.data;

  for(register size_t i = 0; i < len; ++i) {
    ret += hash(ptr[i]);
    ret += ret << 21;
    ret ^= ret >> 13;
  }

  ret += ret << 7;
  ret ^= ret >> 19;
  ret += ret << 29;
  return ret;
}

static uint64_t hash_dict(const object* self) {
  uint64_t ret = 0;
  size_t len = self->dict.size;
  const struct entry* ptr = self->dict.data;

  for(register size_t i = 0; i < len; ++i) {
    ret += (hash(ptr[i].key) ^ hash(ptr[i].value)) * ptr[i].hash;
    ret += ret << 21;
    ret ^= ret >> 13;
  }

  ret += ret << 7;
  ret ^= ret >> 19;
  ret += ret << 29;
  return ret;
}

static uint64_t hash(const object* self) {
  uint64_t ret = 0;
  size_t len = 0;
  const char* ptr = NULL;

  switch(self->type) {
    case T_NIL:
      return 0;
    case T_I64:
      len = sizeof(int64_t);
      ptr = (const char*)&self->int64;
      break;
    case T_F64:
      len = sizeof(double);
      ptr = (const char*)&self->float64;
      break;
    case T_STR:
      len = self->string.len;
      ptr = self->string.data;
      break;
    case T_PTR:
      len = self->pointer.size;
      ptr = self->pointer.data;
      break;
    case T_ARR:
      return hash_array(self);
    case T_TAB:
      return hash_dict(self);
  }

  for(register size_t i = 0; i < len; ++i) {
    ret += ptr[i];
    ret += ret << 21;
    ret ^= ret >> 13;
  }

  ret += ret << 7;
  ret ^= ret >> 19;
  ret += ret << 29;

  return ret;
}

void add_pair(object* self, const object* key, const object* value) {
  ++self->dict.size;
  self->dict.data = realloc(self->dict.data, self->dict.size * sizeof(struct entry));
  register size_t size = self->dict.size;
  uint64_t hashed = hash(key);
  register size_t i;
  register struct entry* arr = self->dict.data;
  for(i = size-1; (i >= 0) && (arr[i].hash > hashed); --i)
    arr[i+1] = arr[i];
  self->dict.data[i].hash = hashed;
  self->dict.data[i].key = key;
  self->dict.data[i].value = value;
}

object* new_dict(const size_t pair_num, ...) {
  object* ret = (object*)calloc(1,sizeof(object));
  ret->type = T_TAB;
  va_list args;
  va_start(args, pair_num);
  for(register size_t i = 0; i < pair_num; ++i) {
    const object* key = va_arg(args, const object*);
    const object* value = va_arg(args, const object*);
    add_pair(ret, key, value);
  }
  return ret;
}

int main() {
  object* o = new_dict(1, new_str("hahaha"), new_int(5));
  printf("%#lx\n", o->dict.data[0].hash);
}