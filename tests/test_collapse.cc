/*--------------------------------------------------------------------------*\
 |  Characterization tests: collapse() of VECTOR-of-uniform content.        |
\*--------------------------------------------------------------------------*/

#include "gc_test_utils.hh"

#include <catch2/catch_test_macros.hpp>

#include <utility>

using namespace GC_namespace;

TEST_CASE( "vector of uniform scalars collapses to typed vector", "[collapse]" )
{
  GenericContainer gc;
  auto &           v = gc.set_vector( 3 );
  v[0] = 1;
  v[1] = 2;
  v[2] = 3;
  gc.collapse();
  CHECK( gc.get_type() == GC_type::VEC_INTEGER );
  CHECK( std::as_const( gc ).get_vec_int() == vec_int_type{ 1, 2, 3 } );
}

TEST_CASE( "vector of mixed int/real collapses to vec_real", "[collapse]" )
{
  GenericContainer gc;
  auto &           v = gc.set_vector( 3 );
  v[0] = 1;
  v[1] = 2.5;
  v[2] = 3;
  gc.collapse();
  CHECK( gc.get_type() == GC_type::VEC_REAL );
  CHECK( std::as_const( gc ).get_vec_real() == vec_real_type{ 1.0, 2.5, 3.0 } );
}

TEST_CASE( "vector of bools collapses to vec_bool", "[collapse]" )
{
  GenericContainer gc;
  auto &           v = gc.set_vector( 2 );
  v[0] = true;
  v[1] = false;
  gc.collapse();
  CHECK( gc.get_type() == GC_type::VEC_BOOL );
  CHECK( std::as_const( gc ).get_vec_bool() == vec_bool_type{ true, false } );
}

TEST_CASE( "vector of equal-length numeric vectors collapses to matrix", "[collapse]" )
{
  GenericContainer gc;
  auto &           v = gc.set_vector( 2 );
  v[0] = vec_real_type{ 1.0, 2.0, 3.0 };
  v[1] = vec_real_type{ 4.0, 5.0, 6.0 };
  gc.collapse();
  CHECK( gc.get_type() == GC_type::MAT_REAL );
  CHECK( gc.get_num_elements() == 6 );
  // pin the orientation: each source vector becomes a column
  auto const & m = std::as_const( gc ).get_mat_real();
  REQUIRE( gc.num_rows() == 3 );
  REQUIRE( gc.num_cols() == 2 );
  CHECK( m( 0, 0 ) == 1.0 );
  CHECK( m( 1, 0 ) == 2.0 );
  CHECK( m( 2, 0 ) == 3.0 );
  CHECK( m( 0, 1 ) == 4.0 );
  CHECK( m( 1, 1 ) == 5.0 );
  CHECK( m( 2, 1 ) == 6.0 );
}

TEST_CASE( "heterogeneous vector stays VECTOR", "[collapse]" )
{
  GenericContainer gc;
  auto &           v = gc.set_vector( 2 );
  v[0] = 1;
  v[1] = "str";
  gc.collapse();
  CHECK( gc.get_type() == GC_type::VECTOR );
}

TEST_CASE( "collapse recurses into map values", "[collapse]" )
{
  GenericContainer gc;
  auto &           v = gc["nums"].set_vector( 2 );
  v[0] = 5;
  v[1] = 6;
  gc["other"] = "text";
  gc.collapse();
  CHECK( gc.get_type() == GC_type::MAP );
  CHECK( std::as_const( gc )( "nums" ).get_type() == GC_type::VEC_INTEGER );
  CHECK( std::as_const( gc )( "nums" ).get_vec_int() == vec_int_type{ 5, 6 } );
}

TEST_CASE( "vector of strings collapses to vec_string", "[collapse]" )
{
  GenericContainer gc;
  auto &           v = gc.set_vector( 2 );
  v[0] = std::string( "a" );
  v[1] = std::string( "b" );
  gc.collapse();
  CHECK( gc.get_type() == GC_type::VEC_STRING );
  CHECK( std::as_const( gc ).get_vec_string() == vec_string_type{ "a", "b" } );
}

TEST_CASE( "vector of longs collapses to vec_long", "[collapse]" )
{
  GenericContainer gc;
  auto &           v = gc.set_vector( 2 );
  v[0] = long_type( 1LL << 40 );
  v[1] = long_type( 1LL << 41 );
  gc.collapse();
  CHECK( gc.get_type() == GC_type::VEC_LONG );
  CHECK( std::as_const( gc ).get_vec_long() == vec_long_type{ 1LL << 40, 1LL << 41 } );
}

TEST_CASE( "vector of complex collapses to vec_complex", "[collapse]" )
{
  GenericContainer gc;
  auto &           v = gc.set_vector( 2 );
  v[0] = complex_type( 1, 2 );
  v[1] = complex_type( 3, 4 );
  gc.collapse();
  CHECK( gc.get_type() == GC_type::VEC_COMPLEX );
  CHECK(
    std::as_const( gc ).get_vec_complex() == vec_complex_type{ complex_type( 1, 2 ), complex_type( 3, 4 ) } );
}

TEST_CASE( "vector of equal-length long vectors collapses to mat_long", "[collapse]" )
{
  GenericContainer gc;
  auto &           v = gc.set_vector( 2 );
  v[0] = vec_long_type{ 1, 2 };
  v[1] = vec_long_type{ 3, 4 };
  gc.collapse();
  CHECK( gc.get_type() == GC_type::MAT_LONG );
  auto const & m = std::as_const( gc ).get_mat_long();
  CHECK( m( 0, 0 ) == 1 );
  CHECK( m( 1, 1 ) == 4 );
}

TEST_CASE( "vector of equal-length complex vectors collapses to mat_complex", "[collapse]" )
{
  GenericContainer gc;
  auto &           v = gc.set_vector( 2 );
  v[0] = vec_complex_type{ complex_type( 1, 0 ), complex_type( 2, 0 ) };
  v[1] = vec_complex_type{ complex_type( 3, 0 ), complex_type( 4, 0 ) };
  gc.collapse();
  CHECK( gc.get_type() == GC_type::MAT_COMPLEX );
  auto const & m = std::as_const( gc ).get_mat_complex();
  CHECK( m( 0, 0 ) == complex_type( 1, 0 ) );
  CHECK( m( 1, 1 ) == complex_type( 4, 0 ) );
}

TEST_CASE( "empty vector collapse is a no-op", "[collapse]" )
{
  GenericContainer gc;
  gc.set_vector( 0 );
  gc.collapse();
  CHECK( gc.get_type() == GC_type::VECTOR );
  CHECK( gc.get_num_elements() == 0 );
}

TEST_CASE( "unequal-length sub-vectors prevent matrix collapse", "[collapse]" )
{
  GenericContainer gc;
  auto &           v = gc.set_vector( 2 );
  v[0] = vec_real_type{ 1.0, 2.0 };
  v[1] = vec_real_type{ 3.0, 4.0, 5.0 };
  gc.collapse();
  CHECK( gc.get_type() == GC_type::VECTOR );
}

TEST_CASE( "collapse on a plain scalar is a no-op", "[collapse]" )
{
  GenericContainer gc;
  gc.set_int( 42 );
  gc.collapse();
  CHECK( gc.get_type() == GC_type::INTEGER );
  CHECK( std::as_const( gc ).get_int() == 42 );
}
