#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include <unistd.h>
#include "object.h"
#define massert(cond, msg, args...) \
    if(!(cond)) { \
      fprintf(stderr, "Assertment failed in file %s in function %s at %d:\n--", __FILE__, __func__, __LINE__); \
      fprintf(stderr, msg, ##args); \
      exit(1); \
    }

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

static const char* obj_type(const object* obj) {
  massert(obj, "NullError: obj is not a valid object.");
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
  default:
    return NULL;
  }
}

static object* new_int(const int64_t i) {
  object* ret = (object*)calloc(1,sizeof(object));
  massert(ret, "MemoryError: Cannot allocate space for object.");
  ret->int64 = i;
  ret->type = T_I64;
  return ret;
}

static object* new_num(const double d) {
  object* ret = (object*)calloc(1,sizeof(object));
  massert(ret, "MemoryError: Cannot allocate space for object.");
  ret->float64 = d;
  ret->type = T_F64;
  return ret;
}

static object* new_str(const char* s) {
  object* ret = (object*)calloc(1,sizeof(object));
  massert(ret, "MemoryError: Cannot allocate space for object.");
  ret->string.len = strlen(s);
  ret->string.data = (char*)calloc(ret->string.len, sizeof(char));
  memcpy(ret->string.data, s, ret->string.len);
  ret->type = T_STR;
  return ret;
}

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

static void add_elem(object* self, object* elem) {
  massert(self, "NullError: self is not a valid object.");
  massert(elem, "NullError: element is not a valid object.");

  ++self->array.size;
  self->array.data = (object**)realloc(self->array.data, self->array.size * sizeof(object*));
  massert(self->array.data, "MemoryError: Cannot expand the element array.");
  self->array.data[self->array.size-1] = elem;
}

static object* new_arr(const size_t elem_num, ...) {
  object* ret = (object*)calloc(1,sizeof(object));
  massert(ret, "MemoryError: Cannot allocate space for object.");
  ret->type = T_ARR;
  va_list args;
  va_start(args, elem_num);
  for(register size_t i = 0; i < elem_num; ++i) {
    object* elem = va_arg(args, object*);
    add_elem(ret, elem);
  }
  va_end(args);
  return ret;
}

static uint64_t hash_array(const object* self) {
  massert(self, "NullError: self is not a valid object.");
  uint64_t ret = 0;
  size_t len = self->array.size;
  const object** ptr = (const object**)self->array.data;

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
  massert(self, "NullError: self is not a valid object.");
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
  massert(self, "NullError: self is not a valid object.");
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

static void merge(struct entry* arr, size_t l, size_t m, size_t r) {
  register size_t i, j, k;
  size_t n1 = m - l + 1;
  size_t n2 = r - m;

  struct entry* L = (struct entry*)malloc(n1*sizeof(struct entry));
  struct entry* R = (struct entry*)malloc(n2*sizeof(struct entry));

  for(i = 0; i < n1; ++i)
    L[i] = arr[l + i];
  for(j = 0; j < n2; ++j)
    R[j] = arr[m + 1 + j];
  
  i = 0, j = 0, k = 0;
  while(i < n1 && j < n2) {
    if(L[i].hash <= R[j].hash)
      arr[k++] = L[i++];
    else
      arr[k++] = R[j++];
  }

  while(i < n1)
    arr[k++] = L[i++];
  while(j < n2)
    arr[k++] = R[j++];
}

static void merge_sort(struct entry* arr, size_t l, size_t r) {
  if(l < r) {
    size_t m = l+(r-l)/2;
    merge_sort(arr, l, m);
    merge_sort(arr, m+1, r);

    merge(arr, l, m, r);
  }
}

static void add_pair(object* self, object* key, object* value) {
  massert(self, "NullError: self is not a valid object.");
  massert(key, "NullError: key is not a valid object.");
  massert(value, "NullError: value is not a valid object.");
  ++self->dict.size;
  self->dict.data = (struct entry*)realloc(self->dict.data, self->dict.size * sizeof(struct entry));
  massert(self->dict.data, "MemoryError: Cannot expand the dictionary.");
  self->dict.data[self->dict.size-1].hash = hash(key);
  self->dict.data[self->dict.size-1].key = key;
  self->dict.data[self->dict.size-1].value = value;
  merge_sort(self->dict.data, 0, self->dict.size-1);
}

static object* new_dict(const size_t pair_num, ...) {
  object* ret = (object*)calloc(1,sizeof(object));
  massert(ret, "MemoryError: Cannot allocate space for object.");
  ret->type = T_TAB;
  va_list args;
  va_start(args, pair_num);
  for(register size_t i = 0; i < pair_num; ++i) {
    object* key = va_arg(args, object*);
    object* value = va_arg(args, object*);
    add_pair(ret, key, value);
  }
  return ret;
}

static char* str_obj(const object* self) {
  massert(self, "NullError: self is not a valid object.");
  char* ret = NULL;
  size_t len = 0;
  switch(self->type) {
  case T_NIL:
    len = sizeof("null");
    ret = calloc(len,sizeof(char));
    sprintf(ret, "%s", "null");
    return ret;
  case T_I64:
    len = snprintf(NULL, 0, "%lu", self->int64) + 1;
    ret = calloc(len,sizeof(char));
    sprintf(ret, "%lu" , self->int64);
    return ret;
  case T_F64:
    len = snprintf(NULL, 0, "%g", self->float64) + 1;
    ret = calloc(len,sizeof(char));
    sprintf(ret, "%g" , self->float64);
    return ret;
  case T_STR:
    len = self->string.len + 3;
    ret = calloc(len,sizeof(char));
    sprintf(ret, "\"%s\"", self->string.data);
    return ret;
  case T_PTR:
    len = sprintf(NULL, 0, "<pointer at %p>", self->pointer);
    ret = calloc(len,sizeof(char));
    sprintf(ret, "<pointer at %p>", self->pointer);
    return ret;
  case T_ARR:
    len = sizeof("[");
    ret = calloc(len,sizeof(char));
    sprintf(ret, "%s", "[");
    for(size_t i = 0; i < self->array.size; ++i) {
      char* tmp = str_obj(self->array.data[i]);
      len += strlen(tmp);
      ret = realloc(ret, len * sizeof(char));
      sprintf(ret, "%s%s", ret, tmp);
      if(i != self->array.size-1) {
        len += 2;
        ret = realloc(ret, len * sizeof(char));
        sprintf(ret, "%s%s", ret, ", ");
      }
      free(tmp);
    }
    ++len;
    ret = realloc(ret, len * sizeof(char));
    sprintf(ret, "%s%c", ret, ']');
    return ret;
  case T_TAB:
    len = sizeof("[");
    ret = calloc(len,sizeof(char));
    sprintf(ret, "%s", "[");
    for(size_t i = 0; i < self->dict.size; ++i) {
      char* tmp_key = str_obj(self->dict.data[i].key);
      char* tmp_val = str_obj(self->dict.data[i].value);
      len += strlen(tmp_key) + strlen(tmp_val) + sizeof(":") - 1;
      ret = realloc(ret, len * sizeof(char));
      sprintf(ret, "%s%s:%s", ret, tmp_key, tmp_val);
      if(i != self->dict.size-1) {
        len += 2;
        ret = realloc(ret, len * sizeof(char));
        sprintf(ret, "%s%s", ret, ", ");
      }
      free(tmp_key);
      free(tmp_val);
    }
    ++len;
    ret = realloc(ret, len * sizeof(char));
    sprintf(ret, "%s%c", ret, ']');
    return ret;
  default:
    return NULL;
  }

}

static void insert_generic(object* self, ...) {
  massert(self, "NullError: self is not a valid object.");
  va_list args;
  switch(self->type) {
  case T_ARR:
    va_start(args,self);
    object* tmp = va_arg(args, object*);
    add_elem(self, tmp);
  break;

  case T_TAB:
    va_start(args,self);
    object* tmp1 = va_arg(args, object*);
    object* tmp2 = va_arg(args, object*);
    add_pair(self, tmp1, tmp2);
  break;
  
  default:
    massert(0, "TypeError: Cannot add something to a%c %s", ((self->type == T_I64) || (self->type == T_ARR)) ? 'n' : '\0' , obj_type(self));
  break;
  }
}

static void del_obj(object* self) {
  massert(self, "NullError: self is not a valid object.");
  switch(self->type) {
  case T_NIL:
  case T_I64:
  case T_F64:
    free(self);
  break;
  case T_STR:
    free(self->string.data);
    free(self);
  break;
  case T_PTR:
    free(self->pointer.data);
    free(self);
  break;
  case T_ARR:
    for(size_t i = 0; i < self->array.size; ++i)
      del_obj(self->array.data[i]);
    free(self);
  break;
  case T_TAB:
    for(size_t i = 0; i < self->dict.size; ++i) {
      del_obj(self->dict.data[i].key);
      del_obj(self->dict.data[i].value);
    }
    free(self);
  break;
  default:
    massert(0,"TypeError: Object type is not valid.");
  break;
  }
}

struct {
  const char* (*type)(const object*);
  object* (*i64)(const int64_t);
  object* (*f64)(const double);
  object* (*str)(const char*);
  object* (*ptr)(const void*, const size_t);
  object* (*array)(const size_t, ...);
  object* (*dict)(const size_t, ...);
  void (*insert)(object*, ...);
  char* (*repr)(const object*);
  void (*delete)(object*);
} obj = {
  obj_type,
  new_int,
  new_num,
  new_str,
  new_ptr,
  new_arr,
  new_dict,
  insert_generic,
  str_obj,
  del_obj
};

int main() {
  object* o = obj.array(5, obj.str("Hello"), obj.str("World!"), obj.i64(56), obj.f64(3.141592),\
    obj.dict(1, obj.str("Array"), obj.str("Ception")));
  obj.insert(o->array.data[4], obj.i64(4), obj.f64(2.71));
  printf("%s\n", obj.repr(o));
  return 0;
}

#undef massert
