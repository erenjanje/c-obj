#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include <unistd.h>
#include "object.h"
#include "objects/i64.c"
#include "objects/f64.c"
#include "objects/str.c"
#include "objects/ptr.c"
#include "objects/arr.c"
#include "objects/dic.c"



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
    sprintf(ret, "%lld" , self->int64);
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
    len = snprintf(NULL, 0, "<pointer at %p>", self->pointer.data);
    ret = calloc(len,sizeof(char));
    sprintf(ret, "<pointer at %p>", self->pointer.data);
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


static object* add_generic(object* o1, const object* o2) {
  massert(o1, "NullError: operand 1 is not a valid object");
  massert(o2, "NullError: operand 2 is not a valid object");
  massert(o1->type == o2->type, "TypeError: you cannot add apples and oranges");
  switch(o1->type) {
  case T_I64:
    return obj.i64(o1->int64 + o2->int64);
  break;

  case T_F64:
    return obj.f64(o1->float64 + o2->float64);
  break;

  case T_NIL:
    return NULL;
  break;

  case T_PTR:
    massert(1, "DON'T ADD POINTERS DIRECTLY");
  break;

  case T_ARR:
    obj.merge(o1,o2);
    return o1;
  break;
  
  }
}

static object* sub_generic(const object* o1, const object* o2) {
  return obj.i64(o1->int64 - o2->int64);
}

static object* mul_generic(const object* o1, const object* o2) {
  return obj.i64(o1->int64 * o2->int64);
}

static object* div_generic(const object* o1, const object* o2) {
  return obj.i64(o1->int64 / o2->int64);
}

static object* mod_generic(const object* o1, const object* o2) {
  return obj.i64(o1->int64 % o2->int64);
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

struct __object_module obj = {
  obj_type,
  new_int,
  new_num,
  new_str,
  new_ptr,
  new_arr,
  new_dict,
  insert_generic,
  str_obj,
  del_obj,
  remove_elem,
  merge_array
};

int main() {
  object* o = obj.array(5, obj.str("Hello"), obj.str("World!"), obj.i64(56), obj.f64(3.141592),\
    obj.dict(1, obj.str("Array"), obj.str("Ception")));
  obj.insert(o->array.data[4], obj.i64(4), obj.f64(2.71));
  obj.merge(o, obj.array(2,obj.i64(1),obj.str("haha")));
  printf("%s\n", obj.repr(o));
  return 0;
}
