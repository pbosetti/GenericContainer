/*--------------------------------------------------------------------------*\
 |                                                                          |
 |  Copyright (C) 2013                                                      |
 |                                                                          |
 |         , __                 , __                                        |
 |        /|/  \               /|/  \                                       |
 |         | __/ _   ,_         | __/ _   ,_                                |
 |         |   \|/  /  |  |   | |   \|/  /  |  |   |                        |
 |         |(__/|__/   |_/ \_/|/|(__/|__/   |_/ \_/|/                       |
 |                           /|                   /|                        |
 |                           \|                   \|                        |
 |                                                                          |
 |      Enrico Bertolazzi                                                   |
 |      Dipartimento di Ingegneria Industriale                              |
 |      Università degli Studi di Trento                                    |
 |      email: enrico.bertolazzi@unitn.it                                   |
 |                                                                          |
\*--------------------------------------------------------------------------*/

//
// file: GenericContainer_ffi.cc
//
// Implementation is C++ (it constructs/destroys GenericContainer instances,
// calls its methods and catches its exceptions), but nothing but the plain
// C types declared in GenericContainer_ffi.h ever crosses the exported ABI.
//

#include "GenericContainer/GenericContainer_ffi.h"
#include "GenericContainer/GenericContainer.hh"

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

using GC_namespace::GenericContainer;

namespace
{

  // thread_local: each thread sees only errors from calls it made itself.
  thread_local std::string g_last_error;

  inline void
  clear_error()
  {
    g_last_error.clear();
  }

  inline void
  set_error( std::string const & msg )
  {
    g_last_error = msg;
  }

  inline GenericContainer *
  as_gc( gc_handle_t h )
  {
    return static_cast<GenericContainer *>( h );
  }

}  // namespace

extern "C" {

// -----------------------------------------------------------------------------
//  Lifecycle
// -----------------------------------------------------------------------------

gc_handle_t
gc_new( void )
{
  clear_error();
  try
  {
    return new GenericContainer();
  }
  catch ( std::exception const & e )
  {
    set_error( e.what() );
  }
  catch ( ... )
  {
    set_error( "gc_new: unknown error" );
  }
  return nullptr;
}

void
gc_free( gc_handle_t h )
{
  clear_error();
  delete as_gc( h );
}

// -----------------------------------------------------------------------------
//  JSON bridge
// -----------------------------------------------------------------------------

// GenericContainer::from_json/to_json are implemented in
// src_json_interface/, which is not part of this build yet (see the
// top-level CMakeLists.txt header comment). Calling through to them
// unconditionally would leave this object with an unresolved external
// reference, which a shared-library build must resolve at link time.
// Revert gc_from_json/gc_to_json to the commented-out bodies below once
// that backend is back in the build.

int
gc_from_json( gc_handle_t h, char const * json_text )
{
  clear_error();
  if ( !h ) { set_error( "gc_from_json: null handle" ); return 1; }
  if ( !json_text ) { set_error( "gc_from_json: null json_text" ); return 1; }
  set_error( "gc_from_json: JSON support is not built into this library" );
  return 1;
#if 0
  try
  {
    std::istringstream stream( json_text );
    if ( as_gc( h )->from_json( stream ) ) return 0;
    set_error( "gc_from_json: JSON parse error" );
  }
  catch ( std::exception const & e )
  {
    set_error( e.what() );
  }
  catch ( ... )
  {
    set_error( "gc_from_json: unknown error" );
  }
  return 1;
#endif
}

char *
gc_to_json( gc_handle_t h )
{
  clear_error();
  if ( !h ) { set_error( "gc_to_json: null handle" ); return nullptr; }
  set_error( "gc_to_json: JSON support is not built into this library" );
  return nullptr;
#if 0
  try
  {
    std::ostringstream stream;
    as_gc( h )->to_json( stream );
    std::string const s{ stream.str() };
    char * out{ static_cast<char *>( std::malloc( s.size() + 1 ) ) };
    if ( !out )
    {
      set_error( "gc_to_json: out of memory" );
      return nullptr;
    }
    std::memcpy( out, s.data(), s.size() );
    out[s.size()] = '\0';
    return out;
  }
  catch ( std::exception const & e )
  {
    set_error( e.what() );
  }
  catch ( ... )
  {
    set_error( "gc_to_json: unknown error" );
  }
  return nullptr;
#endif
}

void
gc_free_string( char * s )
{
  std::free( s );
}

// -----------------------------------------------------------------------------
//  Scalar convenience accessors
// -----------------------------------------------------------------------------

#define GC_FFI_SET( NAME, CTYPE, EXPR )                                \
  void NAME( gc_handle_t h, CTYPE v )                                  \
  {                                                                     \
    clear_error();                                                     \
    if ( !h ) { set_error( #NAME ": null handle" ); return; }          \
    try { EXPR; }                                                      \
    catch ( std::exception const & e ) { set_error( e.what() ); }      \
    catch ( ... ) { set_error( #NAME ": unknown error" ); }            \
  }

GC_FFI_SET( gc_set_bool, int, as_gc( h )->set_bool( v != 0 ) )
GC_FFI_SET( gc_set_int32, int32_t, as_gc( h )->set_int( v ) )
GC_FFI_SET( gc_set_int64, int64_t, as_gc( h )->set_long( v ) )
GC_FFI_SET( gc_set_real, double, as_gc( h )->set_real( v ) )
GC_FFI_SET( gc_set_string, char const *, as_gc( h )->set_string( v ? v : "" ) )

#undef GC_FFI_SET

#define GC_FFI_GET( NAME, RTYPE, SENTINEL, EXPR )                       \
  RTYPE NAME( gc_handle_t h )                                           \
  {                                                                      \
    clear_error();                                                      \
    if ( !h ) { set_error( #NAME ": null handle" ); return SENTINEL; }  \
    try { return EXPR; }                                                \
    catch ( std::exception const & e ) { set_error( e.what() ); }       \
    catch ( ... ) { set_error( #NAME ": unknown error" ); }             \
    return SENTINEL;                                                    \
  }

GC_FFI_GET( gc_get_bool, int, 0, as_gc( h )->get_bool() ? 1 : 0 )
GC_FFI_GET( gc_get_int32, int32_t, 0, as_gc( h )->get_int() )
GC_FFI_GET( gc_get_int64, int64_t, 0, as_gc( h )->get_long() )
GC_FFI_GET( gc_get_real, double, 0.0, as_gc( h )->get_real() )
GC_FFI_GET( gc_get_string, char const *, "", as_gc( h )->get_string().c_str() )

#undef GC_FFI_GET

// -----------------------------------------------------------------------------
//  Introspection
// -----------------------------------------------------------------------------

int
gc_type( gc_handle_t h )
{
  clear_error();
  if ( !h )
  {
    set_error( "gc_type: null handle" );
    return -1;
  }
  return static_cast<int>( as_gc( h )->get_type() );
}

char const *
gc_type_name( gc_handle_t h )
{
  clear_error();
  if ( !h )
  {
    set_error( "gc_type_name: null handle" );
    return "";
  }
  return GC_namespace::to_string( as_gc( h )->get_type() ).data();
}

// -----------------------------------------------------------------------------
//  Error handling
// -----------------------------------------------------------------------------

char const *
gc_last_error( void )
{
  return g_last_error.c_str();
}

}  // extern "C"

//
// eof: GenericContainer_ffi.cc
//
