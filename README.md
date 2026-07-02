# Generic Container

[![CI](https://github.com/pbosetti/GenericContainer/actions/workflows/ci.yml/badge.svg)](https://github.com/pbosetti/GenericContainer/actions/workflows/ci.yml)

`GenericContainer` is a `C++` class with permit to store eterogeneous
data:


- [Online documentation](http://ebertolazzi.github.io/GenericContainer)

## Supported types

- pointer
- boolean
- integer
- long integer
- floating point
- complex floating point
- string
- vector of pointer
- vector of boolean
- vector of integer
- vector of floating point
- vector of complex floating point
- vector of string
- matrix of floating point
- matrix of complex floating point

in addition to this data type the following two container data are avaiable

-  vector of `GenericContainer`
-  map of `GenericContainer`

this permits to build complex recursive data. The main usage of the
class is in interchange data with scripting language like `Ruby`,
`Lua`, `MATLAB`.

## Simple Usage

The usage is simple, for example it can be used as an associative array
with eterogenous data

```cpp
GenericContainer gc;
gc["one"]  = 1;             // store integer
gc["two"]  = true;          // store a boolean
gc["3"]    = 1.4;           // store floating point number
gc["four"] = "pippo";       // store a string
gc["five"].set_vec_int(10); // store a vector of integer of 10 elements
```

and to retrieve elements

``` cpp
cout << gc["one"].get_int()     << '\n';
cout << gc["two"].get_bool()    << '\n';
cout << gc["3"].get_real()      << '\n';
cout << gc["four"].get_string() << '\n';
GenericContainer::vec_int_type & v = gc["five"].get_vec_int();
cout << v[1] << '\n';
```

For more complex emxamples and recursive data see example test files in
the distribution.

## Compile and tests

Tou need rake installed in your system

```sh
rake
```

if the OS is not correctly detected try

```sh
rake build_osx   # compile for OSX
rake build_linux # compile for linux
rake build_win   # compile for WINDOWS
```

To run the test

```sh
rake run       # for linux/OSX
rake run_win   # for windows
```

## JSON

`GenericContainer` has a built-in interface to exchange data with JSON.
To use the interface include

```cpp
#include "GenericContainer/GenericContainerInterface_json.hh"
```

The interface is compiled as part of the library, so no extra linking step
is needed. It converts a `GenericContainer` to/from JSON text through
streams, strings or files:

```cpp
GenericContainer gc;
gc["one"]  = 1;
gc["two"]  = true;
gc["five"].set_vec_int(10);

std::string json_text;
GC_to_JSON( gc, json_text );  // GenericContainer -> JSON string

GenericContainer gc2;
JSON_to_GC( json_text, gc2 ); // JSON string -> GenericContainer
```

### `nlohmann::json` support

For projects already using [`nlohmann::json`](https://github.com/nlohmann/json),
an opt-in adapter is available as a single, standalone header:

```cpp
#include "GenericContainer/GenericContainerInterface_nlohmann.hh"
```

Including this header (and only this header) enables conversion through the
standard `adl_serializer` customization point, so a `GenericContainer`
converts to/from `nlohmann::json` like any other supported type:

```cpp
GenericContainer gc; gc["a"] = 42;
nlohmann::json j = gc;                            // to_json
GenericContainer gc2 = j.get<GenericContainer>();  // from_json
```

`nlohmann::json` is header-only, and so is this adapter: `GenericContainer`
itself has no hard dependency on it -- only translation units that include
this header need `nlohmann/json.hpp` on their include path (e.g. fetched
via CMake `FetchContent`). Homogeneous numeric JSON arrays are decoded
directly into `vec_int_type`/`vec_real_type` (and, when possible, matrices)
rather than a generic vector of `GenericContainer`, so downstream calls
like `copyto_vec_real` get the fast representation.

### Solving math expressions with Expressionist

JSON data may contain algebraic expressions stored as tagged strings, e.g.
`"$a + b"`. [`Expressionist`](https://github.com/pbosetti/Expressionist) is
a header-only C++20 library that evaluates such expressions in place on a
`nlohmann::json` object -- variables are simply other keys of the same JSON
tree, resolved in dependency order. There is no direct `GenericContainer`
API for this yet, but combined with the `nlohmann::json` adapter above it
is a three-step conversion:

```cpp
#include "GenericContainer/GenericContainerInterface_nlohmann.hh"
#include <expressionist.hpp>

std::string text = R"({
  "a": 1,
  "b": 2,
  "c": "$a + b",
  "g": "$sin(pi / 3) * b"
})";

Expressionist::Expressionist ex( text );
ex.evaluate();                                        // resolve the "$..." expressions in place
GenericContainer gc = ex.object().get<GenericContainer>(); // -> GenericContainer via adl_serializer

cout << gc["c"].get_int()  << '\n'; // 3
cout << gc["g"].get_real() << '\n'; // 1.732...
```

`Expressionist` is fetched via CMake `FetchContent` for this project's own
examples/tests (pinned to a commit, since upstream publishes no tags); it
requires a C++20 compiler, but only for translation units that link it --
it does not affect the C++17 standard used elsewhere in `GenericContainer`.
Consumers who want this combination need to `FetchContent` `Expressionist`
themselves, the same way they would for `nlohmann::json`.

## Lua Support

`GenericContainer` has an interfacing to exchange data with Lua.
To use the interface include

```cpp
#include "GenericContainerInterface_lua.hh"
```

compile and link with `GenericContainerInterface_lua.cc`.
The interface contains a set of functions to convert from `GenericContainer`
to Lua global variables and the other way around.

## Matlab Support

`GenericContainer` has an interfacing to exchange data with matlab.
To use the interface include

```cpp
#include "GenericContainerInterface_matlab.hh"
```

compile and link with `GenericContainerInterface_matlab.cc`.
The interface contains a set of functions to convert from `GenericContainer`
to `mxArray` and the other way around.

The following code stored in `mex_print_recursive.cc`

```cpp
#include "GenericContainer.hh"
#include "GenericContainerInterface_matlab.hh"
#include "mex.h"

#include <sstream>

namespace GC_namespace {

  extern "C"
  void
  mexFunction( int nlhs, mxArray       *plhs[],
               int nrhs, mxArray const *prhs[] ) {
    try {
      GenericContainer gc;
      mxArray_to_GenericContainer( prhs[0], gc );
      mexPrint(gc);
    }
    catch ( std::exception & exc ) {
      mexPrintf("Error: %s\n", exc.what() );
    }
    catch (...) {
      mexPrintf("Unknown error\n");
    }
  }
}
```

Implement a mex command that print a Matlab structure
recursively on the console after the conversion to a
`mxArray_to_GenericContainer`.
After the compilation

```text
  > mex mex_print_recursive.cc GenericContainerInterface_matlab.cc -output print_recursive
```

Produce the `Matlab` command `print_recursive`
so that the following MATLAB code:

```matlab
S         = [ 1 0 2 9; 0 0 2 3; 2 0 0 0; 1 0 -2 -2 ];
S1        = [ 1 0 2 9; 0 0 2 3; 2+1i 0 0 0; 1 0 -2 -2 ];
A.vector  = [1,2,3,4];
A.string  = 'pippo';
A.strings = { 'pippo', 'pluto', 'paperino' };
A.struct1  = { 'paperino', [1 2], [1 2; 3 5] };
A.struct2  = { B, sparse(S), sparse(S1) };

print_recursive( A );
```

has the following output:

```text

  string: "pippo"
  strings:
      0: "pippo"
      1: "pluto"
      2: "paperino"
  struct1:
      0: "paperino"
      1: [ 1 2 ]
      2:
         1        2
         3        5
  struct2:
      0:
          fieldA:
              0: 1
              1: 2
              2: 3
              3: "pippo"
          fieldB:
              [ 1 1 2 ]
          fieldC: "stringa"
      1:
          ir:
              [ 0 2 3 0 1 3 0 1 3 ]
          jc:
              [ 0 3 3 6 9 ]
          values:
              [ 1 2 1 2 2 -2 9 3 -2 ]
      2:
          ir:
              [ 0 2 3 0 1 3 0 1 3 ]
          jc:
              [ 0 3 3 6 9 ]
          values:
              [ (1, 0 ) (2, 1 ) (1, 0 ) (2, 0 ) (2, 0 ) (-2, 0 ) (9, 0 ) (3, 0 ) (-2, 0 ) ]
  vector:
      [ 1 2 3 4 ]
```
