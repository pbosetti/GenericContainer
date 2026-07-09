/*--------------------------------------------------------------------------*\
 |  Exhaustive sweep of the promotion/copyto matrix: every copyto_vec_*,    |
 |  copyto_mat_* and promote_to_* function against a representative sample  |
 |  of every GC_type, checking it succeeds exactly where the switch in      |
 |  GenericContainerPromote.cc allows it and throws everywhere else.        |
\*--------------------------------------------------------------------------*/

#include "gc_test_utils.hh"

#include <catch2/catch_test_macros.hpp>

#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace GC_namespace;

namespace
{

  struct TypeSample
  {
    GC_type           type;
    std::string       name;
    GenericContainer  gc;
  };

  // One representative container per GC_type. Numeric values are small,
  // non-negative and integral so a single sample set doubles as valid input
  // for every "narrowing" target (int/uint/long/ulong) at once; range/sign
  // rejection paths are exercised separately in test_conversion_checks.cc
  // and test_promotion.cc.
  std::vector<TypeSample> all_type_samples()
  {
    std::vector<TypeSample> out;
    static int dummy_target{ 0 };

    {
      GenericContainer gc;
      out.push_back( { GC_type::NOTYPE, "NOTYPE", gc } );
    }
    {
      GenericContainer gc;
      gc.set_pointer( &dummy_target );
      out.push_back( { GC_type::POINTER, "POINTER", gc } );
    }
    {
      GenericContainer gc;
      gc = true;
      out.push_back( { GC_type::BOOL, "BOOL", gc } );
    }
    {
      GenericContainer gc;
      gc = int_type( 3 );
      out.push_back( { GC_type::INTEGER, "INTEGER", gc } );
    }
    {
      GenericContainer gc;
      gc = long_type( 3 );
      out.push_back( { GC_type::LONG, "LONG", gc } );
    }
    {
      GenericContainer gc;
      gc = real_type( 3.0 );
      out.push_back( { GC_type::REAL, "REAL", gc } );
    }
    {
      GenericContainer gc;
      gc = complex_type( 3.0, 0.0 );
      out.push_back( { GC_type::COMPLEX, "COMPLEX", gc } );
    }
    {
      GenericContainer gc;
      gc.set_string( "hi" );
      out.push_back( { GC_type::STRING, "STRING", gc } );
    }
    {
      GenericContainer gc;
      gc.set_vec_pointer( 2 );
      out.push_back( { GC_type::VEC_POINTER, "VEC_POINTER", gc } );
    }
    {
      GenericContainer gc;
      gc = vec_bool_type{ true, false };
      out.push_back( { GC_type::VEC_BOOL, "VEC_BOOL", gc } );
    }
    {
      GenericContainer gc;
      gc = vec_int_type{ 3, 4 };
      out.push_back( { GC_type::VEC_INTEGER, "VEC_INTEGER", gc } );
    }
    {
      GenericContainer gc;
      gc = vec_long_type{ 3, 4 };
      out.push_back( { GC_type::VEC_LONG, "VEC_LONG", gc } );
    }
    {
      GenericContainer gc;
      gc = vec_real_type{ 3.0, 4.0 };
      out.push_back( { GC_type::VEC_REAL, "VEC_REAL", gc } );
    }
    {
      GenericContainer gc;
      gc = vec_complex_type{ complex_type( 3, 0 ), complex_type( 4, 0 ) };
      out.push_back( { GC_type::VEC_COMPLEX, "VEC_COMPLEX", gc } );
    }
    {
      GenericContainer gc;
      gc = vec_string_type{ "a", "b" };
      out.push_back( { GC_type::VEC_STRING, "VEC_STRING", gc } );
    }
    {
      GenericContainer gc;
      auto & m = gc.set_mat_int( 2, 2 );
      m( 0, 0 ) = 3;
      m( 0, 1 ) = 4;
      m( 1, 0 ) = 5;
      m( 1, 1 ) = 6;
      out.push_back( { GC_type::MAT_INTEGER, "MAT_INTEGER", gc } );
    }
    {
      GenericContainer gc;
      auto & m = gc.set_mat_long( 2, 2 );
      m( 0, 0 ) = 3;
      m( 0, 1 ) = 4;
      m( 1, 0 ) = 5;
      m( 1, 1 ) = 6;
      out.push_back( { GC_type::MAT_LONG, "MAT_LONG", gc } );
    }
    {
      GenericContainer gc;
      auto & m = gc.set_mat_real( 2, 2 );
      m( 0, 0 ) = 3.0;
      m( 0, 1 ) = 4.0;
      m( 1, 0 ) = 5.0;
      m( 1, 1 ) = 6.0;
      out.push_back( { GC_type::MAT_REAL, "MAT_REAL", gc } );
    }
    {
      GenericContainer gc;
      auto & m = gc.set_mat_complex( 2, 2 );
      m( 0, 0 ) = complex_type( 3, 0 );
      m( 0, 1 ) = complex_type( 4, 0 );
      m( 1, 0 ) = complex_type( 5, 0 );
      m( 1, 1 ) = complex_type( 6, 0 );
      out.push_back( { GC_type::MAT_COMPLEX, "MAT_COMPLEX", gc } );
    }
    {
      // Scalar INTEGER elements: get_num_elements()==1 for each, so this
      // sample is simultaneously valid VECTOR input for the copyto_vec_*
      // (element-wise get_as_*) and copyto_mat_* (per-column copyto_vec_*)
      // paths, and for promote_to_vector's identity case.
      GenericContainer gc;
      auto & v = gc.set_vector( 2 );
      v[0]     = int_type( 3 );
      v[1]     = int_type( 4 );
      out.push_back( { GC_type::VECTOR, "VECTOR", gc } );
    }
    {
      GenericContainer gc;
      gc.set_map();
      gc["a"] = int_type( 1 );
      out.push_back( { GC_type::MAP, "MAP", gc } );
    }
    return out;
  }

  template <typename Convert>
  void check_matrix( std::set<GC_type> const & ok, Convert && convert )
  {
    for ( auto & s : all_type_samples() )
    {
      INFO( "source type: " << s.name );
      if ( ok.count( s.type ) > 0 )
      {
        CHECK_NOTHROW( convert( s.gc ) );
      }
      else
      {
        CHECK_THROWS_AS( convert( s.gc ), std::runtime_error );
      }
    }
  }

  // NOTYPE: get_num_elements() is 0, so the per-element conversion loop in
  // copyto_vec_* never runs and the switch's NOTYPE case (grouped with the
  // error branch) is never reached -- the call trivially succeeds with an
  // empty output vector rather than throwing.
  std::set<GC_type> const numeric_vec_ok{
    GC_type::NOTYPE,      GC_type::BOOL,        GC_type::INTEGER,    GC_type::LONG,
    GC_type::REAL,        GC_type::COMPLEX,     GC_type::VEC_BOOL,   GC_type::VEC_INTEGER,
    GC_type::VEC_LONG,    GC_type::VEC_REAL,    GC_type::VEC_COMPLEX, GC_type::MAT_INTEGER,
    GC_type::MAT_LONG,    GC_type::MAT_REAL,    GC_type::MAT_COMPLEX, GC_type::VECTOR
  };

  std::set<GC_type> const numeric_mat_ok{
    GC_type::NOTYPE,      GC_type::BOOL,        GC_type::INTEGER,     GC_type::VEC_BOOL,
    GC_type::VEC_INTEGER, GC_type::MAT_INTEGER, GC_type::VECTOR
  };

}  // namespace

TEST_CASE( "copyto_vec_int over every source type", "[promote][matrix]" )
{
  check_matrix( numeric_vec_ok, []( GenericContainer const & gc ) {
    vec_int_type v;
    gc.copyto_vec_int( v );
  } );
}

TEST_CASE( "copyto_vec_uint over every source type", "[promote][matrix]" )
{
  check_matrix( numeric_vec_ok, []( GenericContainer const & gc ) {
    vec_uint_type v;
    gc.copyto_vec_uint( v );
  } );
}

TEST_CASE( "copyto_vec_long over every source type", "[promote][matrix]" )
{
  check_matrix( numeric_vec_ok, []( GenericContainer const & gc ) {
    vec_long_type v;
    gc.copyto_vec_long( v );
  } );
}

TEST_CASE( "copyto_vec_ulong over every source type", "[promote][matrix]" )
{
  check_matrix( numeric_vec_ok, []( GenericContainer const & gc ) {
    vec_ulong_type v;
    gc.copyto_vec_ulong( v );
  } );
}

TEST_CASE( "copyto_vec_real over every source type", "[promote][matrix]" )
{
  check_matrix( numeric_vec_ok, []( GenericContainer const & gc ) {
    vec_real_type v;
    gc.copyto_vec_real( v );
  } );
}

TEST_CASE( "copyto_vec_complex over every source type", "[promote][matrix]" )
{
  // The VECTOR case dispatches element-wise through get_complex(), which
  // (unlike get_as_int/get_as_uint/get_number) requires an exact COMPLEX
  // element rather than auto-promoting -- our shared VECTOR sample holds
  // INTEGER elements, so it throws here instead of succeeding.
  std::set<GC_type> ok{ numeric_vec_ok };
  ok.erase( GC_type::VECTOR );
  check_matrix( ok, []( GenericContainer const & gc ) {
    vec_complex_type v;
    gc.copyto_vec_complex( v );
  } );
}

TEST_CASE( "copyto_vec_string over every source type", "[promote][matrix]" )
{
  std::set<GC_type> const ok{ GC_type::STRING, GC_type::VEC_STRING };
  check_matrix( ok, []( GenericContainer const & gc ) {
    vec_string_type v;
    gc.copyto_vec_string( v );
  } );

  // VECTOR of strings is also accepted, element-wise.
  GenericContainer gc;
  auto & v = gc.set_vector( 2 );
  v[0]     = std::string( "a" );
  v[1]     = std::string( "b" );
  vec_string_type out;
  CHECK_NOTHROW( gc.copyto_vec_string( out ) );
  CHECK( out == vec_string_type{ "a", "b" } );
}

TEST_CASE( "copyto_mat_int over every source type", "[promote][matrix]" )
{
  check_matrix( numeric_mat_ok, []( GenericContainer const & gc ) {
    mat_int_type m;
    gc.copyto_mat_int( m );
  } );
}

TEST_CASE( "copyto_mat_long over every source type", "[promote][matrix]" )
{
  std::set<GC_type> const ok{
    GC_type::NOTYPE, GC_type::BOOL,        GC_type::INTEGER,     GC_type::LONG,
    GC_type::VEC_BOOL, GC_type::VEC_INTEGER, GC_type::VEC_LONG,
    GC_type::MAT_INTEGER, GC_type::MAT_LONG, GC_type::VECTOR
  };
  check_matrix( ok, []( GenericContainer const & gc ) {
    mat_long_type m;
    gc.copyto_mat_long( m );
  } );
}

TEST_CASE( "copyto_mat_real over every source type", "[promote][matrix]" )
{
  std::set<GC_type> const ok{
    GC_type::NOTYPE,      GC_type::BOOL,        GC_type::INTEGER,     GC_type::LONG,
    GC_type::REAL,        GC_type::VEC_BOOL,    GC_type::VEC_INTEGER, GC_type::VEC_LONG,
    GC_type::VEC_REAL,    GC_type::MAT_INTEGER, GC_type::MAT_LONG,    GC_type::MAT_REAL,
    GC_type::VECTOR
  };
  check_matrix( ok, []( GenericContainer const & gc ) {
    mat_real_type m;
    gc.copyto_mat_real( m );
  } );
}

TEST_CASE( "copyto_mat_complex over every source type", "[promote][matrix]" )
{
  std::set<GC_type> const ok{
    GC_type::NOTYPE,      GC_type::BOOL,        GC_type::INTEGER,     GC_type::LONG,
    GC_type::REAL,        GC_type::COMPLEX,     GC_type::VEC_BOOL,    GC_type::VEC_INTEGER,
    GC_type::VEC_LONG,    GC_type::VEC_REAL,    GC_type::VEC_COMPLEX, GC_type::MAT_INTEGER,
    GC_type::MAT_LONG,    GC_type::MAT_REAL,    GC_type::MAT_COMPLEX, GC_type::VECTOR
  };
  check_matrix( ok, []( GenericContainer const & gc ) {
    mat_complex_type m;
    gc.copyto_mat_complex( m );
  } );
}

TEST_CASE( "promote_to_int over every source type", "[promote][matrix]" )
{
  std::set<GC_type> const ok{ GC_type::NOTYPE, GC_type::BOOL, GC_type::INTEGER };
  check_matrix( ok, []( GenericContainer & gc ) { gc.promote_to_int(); } );
}

TEST_CASE( "promote_to_long over every source type", "[promote][matrix]" )
{
  std::set<GC_type> const ok{ GC_type::NOTYPE, GC_type::BOOL, GC_type::INTEGER, GC_type::LONG };
  check_matrix( ok, []( GenericContainer & gc ) { gc.promote_to_long(); } );
}

TEST_CASE( "promote_to_real over every source type", "[promote][matrix]" )
{
  std::set<GC_type> const ok{ GC_type::NOTYPE, GC_type::BOOL, GC_type::INTEGER, GC_type::LONG, GC_type::REAL };
  check_matrix( ok, []( GenericContainer & gc ) { gc.promote_to_real(); } );
}

TEST_CASE( "promote_to_complex over every source type", "[promote][matrix]" )
{
  std::set<GC_type> const ok{
    GC_type::NOTYPE, GC_type::BOOL, GC_type::INTEGER, GC_type::LONG, GC_type::REAL, GC_type::COMPLEX,
    // promote_to_complex delegates VEC_* sources to promote_to_vec_complex()
    GC_type::VEC_BOOL, GC_type::VEC_INTEGER, GC_type::VEC_LONG, GC_type::VEC_REAL
  };
  check_matrix( ok, []( GenericContainer & gc ) { gc.promote_to_complex(); } );
}

TEST_CASE( "promote_to_vec_int over every source type", "[promote][matrix]" )
{
  std::set<GC_type> const ok{ GC_type::NOTYPE, GC_type::BOOL, GC_type::INTEGER, GC_type::VEC_BOOL, GC_type::VEC_INTEGER };
  check_matrix( ok, []( GenericContainer & gc ) { gc.promote_to_vec_int(); } );
}

TEST_CASE( "promote_to_vec_long over every source type", "[promote][matrix]" )
{
  std::set<GC_type> const ok{
    GC_type::NOTYPE,      GC_type::BOOL,        GC_type::INTEGER, GC_type::LONG,
    GC_type::VEC_BOOL,    GC_type::VEC_INTEGER, GC_type::VEC_LONG
  };
  check_matrix( ok, []( GenericContainer & gc ) { gc.promote_to_vec_long(); } );
}

TEST_CASE( "promote_to_vec_real over every source type", "[promote][matrix]" )
{
  std::set<GC_type> const ok{
    GC_type::NOTYPE,      GC_type::BOOL,        GC_type::INTEGER, GC_type::LONG,
    GC_type::REAL,        GC_type::VEC_BOOL,    GC_type::VEC_INTEGER,
    GC_type::VEC_LONG,    GC_type::VEC_REAL
  };
  check_matrix( ok, []( GenericContainer & gc ) { gc.promote_to_vec_real(); } );
}

TEST_CASE( "promote_to_vec_complex over every source type", "[promote][matrix]" )
{
  std::set<GC_type> const ok{
    GC_type::NOTYPE,      GC_type::BOOL,        GC_type::INTEGER, GC_type::LONG,
    GC_type::REAL,        GC_type::COMPLEX,     GC_type::VEC_BOOL,
    GC_type::VEC_INTEGER, GC_type::VEC_LONG,    GC_type::VEC_REAL, GC_type::VEC_COMPLEX
  };
  check_matrix( ok, []( GenericContainer & gc ) { gc.promote_to_vec_complex(); } );
}

TEST_CASE( "promote_to_mat_int over every source type", "[promote][matrix]" )
{
  std::set<GC_type> const ok{
    GC_type::NOTYPE, GC_type::BOOL, GC_type::INTEGER, GC_type::VEC_BOOL, GC_type::VEC_INTEGER, GC_type::MAT_INTEGER
  };
  check_matrix( ok, []( GenericContainer & gc ) { gc.promote_to_mat_int(); } );
}

TEST_CASE( "promote_to_mat_long over every source type", "[promote][matrix]" )
{
  // Quirk (matches test_promotion.cc): a LONG scalar does not promote to
  // mat_long_type -- only NOTYPE/BOOL/INTEGER singleton widen that way.
  std::set<GC_type> const ok{
    GC_type::NOTYPE,      GC_type::BOOL,        GC_type::INTEGER,
    GC_type::VEC_BOOL,    GC_type::VEC_INTEGER, GC_type::VEC_LONG,
    GC_type::MAT_INTEGER, GC_type::MAT_LONG
  };
  check_matrix( ok, []( GenericContainer & gc ) { gc.promote_to_mat_long(); } );
}

TEST_CASE( "promote_to_mat_real over every source type", "[promote][matrix]" )
{
  std::set<GC_type> const ok{
    GC_type::NOTYPE,      GC_type::BOOL,        GC_type::INTEGER,     GC_type::REAL,
    GC_type::VEC_BOOL,    GC_type::VEC_INTEGER, GC_type::VEC_LONG,    GC_type::VEC_REAL,
    GC_type::MAT_INTEGER, GC_type::MAT_LONG,    GC_type::MAT_REAL
  };
  check_matrix( ok, []( GenericContainer & gc ) { gc.promote_to_mat_real(); } );
}

TEST_CASE( "promote_to_mat_complex over every source type", "[promote][matrix]" )
{
  std::set<GC_type> const ok{
    GC_type::NOTYPE,      GC_type::BOOL,        GC_type::INTEGER,     GC_type::LONG,
    GC_type::REAL,        GC_type::VEC_BOOL,    GC_type::VEC_INTEGER, GC_type::VEC_LONG,
    GC_type::VEC_REAL,    GC_type::MAT_INTEGER, GC_type::MAT_LONG,    GC_type::MAT_REAL,
    GC_type::MAT_COMPLEX
  };
  check_matrix( ok, []( GenericContainer & gc ) { gc.promote_to_mat_complex(); } );
}

TEST_CASE( "promote_to_vector over every source type", "[promote][matrix]" )
{
  std::set<GC_type> const ok{
    GC_type::NOTYPE,      GC_type::POINTER,     GC_type::BOOL,        GC_type::INTEGER,
    GC_type::LONG,        GC_type::REAL,        GC_type::COMPLEX,     GC_type::STRING,
    GC_type::VEC_POINTER, GC_type::VEC_BOOL,    GC_type::VEC_INTEGER, GC_type::VEC_LONG,
    GC_type::VEC_REAL,    GC_type::VEC_COMPLEX, GC_type::VEC_STRING,  GC_type::VECTOR
  };
  check_matrix( ok, []( GenericContainer & gc ) { gc.promote_to_vector(); } );
}
