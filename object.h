#ifndef OBJECT_H_DEFINED_
#define OBJECT_H_DEFINED_
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define massert(cond, msg, args...) \
    if(!(cond)) { \
      fprintf(stderr, "Assertment failed in file %s in function %s at %d:\n  ", __FILE__, __func__, __LINE__); \
      fprintf(stderr, msg, ##args); \
      exit(1); \
    }

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
      struct object** data;
    } array;
    struct {
      size_t size;
      struct entry {
        uint64_t hash;
        struct object* key;
        struct object* value;
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

struct __object_module {
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
  void (*remove)(object*,object*);
  object* (*merge)(const object*,const object*);
  object* (*add)(object*,const object*);
} obj;

typedef char* (*tostring_function)(void*,size_t);


#endif // OBJECT_H_DEFINED_
