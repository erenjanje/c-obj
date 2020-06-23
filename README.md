# Basic C Dynamic Object Implementation

This is a basic c library for dynamic object handling.

## Types

**Nil:** A type for non-existent variables. It's like NULL in C, None in Python, nil in Lua etc.  
**Integer:** A 64 bit integer for basic integer holding.  
**Float:** A double for basic floating-point number holding.  
**String:** A string with size. So it can hold strings with embedded nulls (aka \0)  
**Pointer:** A pointer holder. It is implemented with void*. So it have to be casted before most of the usage cases.  
**Array:** A minimal heterogenous array implementation with basic functions.  
**Dict:** A minimal associative array implemented with Jenkins hash function, merge sort and binary search.  

## Basic Usage

You can create any type of object with obj.[type] functions. All of them returns same type of pointer, an "object*" typed one. The type information is stored in an enum. An user should not worry about the internal handling of types. User can get the type as a string literal with `obj.type` function.

~~~c
// NOTE: For the sake of demonstration, deleting an object is not showed here. But an user must free the object with obj.delete if they don't want any dangling pointers around the program
object* o = obj.i64(2); // o has type I64 and value 2
o = obj.f64(3.141592); // o has type F64 and value 3.141592
o = obj.str("Hello World")
~~~

## Extra functions

User can add elements to a composite type (i.e. Array or Dict) with `obj.insert` function. It inserts an element to an array or creates a pair in a dict.

The string representation of an object can be obtained with `obj.repr`. It creates a string with contents of the given object. The problem is that this string have to be freed after it is used, since it is created in heap.