#include "../object.h"

static uint64_t hash(const object*);

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
