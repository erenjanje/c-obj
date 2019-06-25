#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "object.h"

typedef struct object {
  union {
    int64_t i64;
    double f64;
    struct {
      size_t size;
      char* data;
    } str;
    struct {
      size_t size;
      void* data;
    } ptr;
    struct {
      size_t size;
      object** data;
    } arr;
    struct {
      size_t size;
      uint64_t* hashes;
      object** keys;
      object** vals;
    } tab;
  } dat;
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

int main() {
  object a;
  sizeof(a.dat);
}