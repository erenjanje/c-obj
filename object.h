#ifndef OBJECT_H_DEFINED_
#define OBJECT_H_DEFINED_
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

typedef struct object object;
typedef char* (*tostring_function)(void*,size_t);


#endif // OBJECT_H_DEFINED_
