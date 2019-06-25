#ifndef OBJECT_H_DEFINED_
#define OBJECT_H_DEFINED_
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

typedef struct object object;

const char* obj_type(const object*);
object* new_int(const int64_t);
object* new_num(const double);
object* new_str(const char*);
object* new_ptr(const void*,const size_t);
object* new_arr(const size_t,...);
object* new_dict(const size_t,...);
void add_elem(object*,const object*);
void add_pair(object*,const object*,const object*);

#endif // OBJECT_H_DEFINED_