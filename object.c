#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include "object.h"

static uint64_t hash(const object*);
static uint64_t hash_array(const object*);
static uint64_t hash_dict(const object*);
struct tostring_pattern {
  char is_printable[8];
  tostring_function tostring;
};

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

void add_pair(object* self, const object* key, const object* value) {
  ++self->dict.size;
  self->dict.data = realloc(self->dict.data, self->dict.size * sizeof(struct entry));
  self->dict.data[self->dict.size-1].hash = hash(key);
  self->dict.data[self->dict.size-1].key = key;
  self->dict.data[self->dict.size-1].value = value;
  merge_sort(self->dict.data, 0, self->dict.size-1);
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

char* str_obj(const object* self) {
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
    ; /* Unless there is an empty statement, due to the standard, the compiler throws
      "a label can only be part of a statement and a declaration is not a statement".
      Because "case" is a label and declaration of a new variable is not a statement. */
    struct tostring_pattern* pattern = self->pointer.data;
    puts(pattern->is_printable);
    char* tmp = NULL;
    if(!memcmp(pattern->is_printable, "strfunc", 8))
      tmp = pattern->tostring(self->pointer.data, self->pointer.size);
    if(tmp == NULL) {
      len = snprintf(NULL, 0, "<pointer to %p>", self->pointer.data) + 1;
      ret = calloc(len, sizeof(char));
      sprintf(ret, "<pointer to %p>", self->pointer.data);
      return ret;
    }
    return tmp;

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
    len = sizeof("{");
    ret = calloc(len,sizeof(char));
    sprintf(ret, "%s", "{");
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
    sprintf(ret, "%s%c", ret, '}');
    return ret;
  }

}

struct hah {
  char printable[8];
  tostring_function tostring;
  size_t num;
  int arr[];
};

char* string_hah(void* __self, size_t size) {
  struct hah* self = __self;
  char* ret = NULL;
  size_t len = 1;
  for(size_t i = 0; i < self->num; ++i) {
    len += snprintf(NULL, 0, "%d ", self->arr[i]);
    ret = realloc(ret, len * sizeof(char));
    sprintf(ret, "%s%d ", ret, self->arr[i]);
  }
  return ret;
}

int main() {
  struct hah* arr = calloc(1,sizeof(struct hah) + 6 * sizeof(int));
  printf("%d\n", arr->arr[4]);
  arr->num = 5;
  memcpy(arr->printable, "strfunc", 8);
  arr->tostring = string_hah;
  object* o = new_ptr(arr, sizeof(arr) + 6 * sizeof(int));
  puts(str_obj(o));
}