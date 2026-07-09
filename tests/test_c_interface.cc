/*--------------------------------------------------------------------------*\
 |  Characterization tests: the extern "C" interface. Its ABI is frozen     |
 |  through the modernization; these tests keep it honest.                  |
\*--------------------------------------------------------------------------*/

#include "GenericContainer/GenericContainerInterface_C.h"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <string>

namespace
{
  //! RAII for the global C-side container registry.
  struct CGuard
  {
    std::string id;
    explicit CGuard( char const * name ) : id( name ) { REQUIRE( GC_new( id.c_str() ) == 0 ); }
    ~CGuard() { GC_delete( id.c_str() ); }
  };
}  // namespace

TEST_CASE( "C API scalar set/get round trips", "[c_api]" )
{
  CGuard g( "c_scalars" );

  CHECK( GC_set_bool( 1 ) == 0 );
  CHECK( GC_get_bool() == 1 );

  CHECK( GC_set_int( -12 ) == 0 );
  CHECK( GC_get_int() == -12 );

  CHECK( GC_set_real( 2.5 ) == 0 );
  CHECK( GC_get_real() == 2.5 );

  CHECK( GC_set_complex2( 1.0, -2.0 ) == 0 );
  CHECK( GC_get_complex_re() == 1.0 );
  CHECK( GC_get_complex_im() == -2.0 );

  CHECK( GC_set_string( "from C" ) == 0 );
  CHECK( std::string( GC_get_string() ) == "from C" );
}

TEST_CASE( "C API vector set and element access", "[c_api]" )
{
  CGuard g( "c_vectors" );

  int const ivals[3]{ 4, 5, 6 };
  CHECK( GC_set_vector_of_int( ivals, 3 ) == 0 );
  CHECK( GC_get_vector_size() == 3 );
  CHECK( GC_get_int_at_pos( 1 ) == 5 );

  double const rvals[2]{ 0.5, 1.5 };
  CHECK( GC_set_vector_of_real( rvals, 2 ) == 0 );
  CHECK( GC_get_real_at_pos( 1 ) == 1.5 );

  char const * svals[2]{ "aa", "bb" };
  CHECK( GC_set_vector_of_string( svals, 2 ) == 0 );
  CHECK( std::string( GC_get_string_at_pos( 0 ) ) == "aa" );
}

TEST_CASE( "C API push into generic vector", "[c_api]" )
{
  CGuard g( "c_push" );

  CHECK( GC_set_empty_vector() == 0 );
  CHECK( GC_push_int( 3 ) == 0 );
  CHECK( GC_push_string( "el" ) == 0 );
  CHECK( GC_get_vector_size() == 2 );
}

TEST_CASE( "C API map navigation with keys", "[c_api]" )
{
  CGuard g( "c_map" );

  CHECK( GC_set_map() == 0 );
  CHECK( GC_push_map_position( "sub" ) == 0 );
  CHECK( GC_set_int( 9 ) == 0 );
  CHECK( GC_pop_head() == 0 );

  CHECK( GC_init_map_key() == 0 );
  char const * key = GC_get_next_key();
  REQUIRE( key != nullptr );
  CHECK( std::string( key ) == "sub" );
  // exhausted iteration yields an empty string, not NULL
  char const * done = GC_get_next_key();
  REQUIRE( done != nullptr );
  CHECK( std::string( done ).empty() );
}

TEST_CASE( "C API select switches between containers", "[c_api]" )
{
  CGuard a( "c_sel_a" );
  CGuard b( "c_sel_b" );

  REQUIRE( GC_select( "c_sel_a" ) == 0 );
  GC_set_int( 1 );
  REQUIRE( GC_select( "c_sel_b" ) == 0 );
  GC_set_int( 2 );

  REQUIRE( GC_select( "c_sel_a" ) == 0 );
  CHECK( GC_get_int() == 1 );
  REQUIRE( GC_select( "c_sel_b" ) == 0 );
  CHECK( GC_get_int() == 2 );
}

TEST_CASE( "C API error codes on misuse", "[c_api]" )
{
  // Characterization: GC_select auto-creates unknown ids (returns 0 and
  // registers a fresh empty container).
  CHECK( GC_select( "auto_created_by_select" ) == 0 );
  CHECK( GC_get_type() == 0 );  // NOTYPE
  GC_delete( "auto_created_by_select" );

  CGuard g( "c_err" );
  GC_set_int( 5 );
  // pushing into a scalar is refused with a nonzero code (C API maps
  // exceptions to return codes; it must not throw across the C boundary)
  CHECK( GC_push_map_position( "k" ) != 0 );
}

TEST_CASE( "C API vector element push for every scalar kind", "[c_api]" )
{
  CGuard g( "c_push_all" );

  CHECK( GC_set_empty_vector() == 0 );
  CHECK( GC_push_bool( 1 ) == 0 );
  CHECK( GC_push_real( 2.5 ) == 0 );
  c_complex_type const cc{ 1.0, -1.0 };
  CHECK( GC_push_complex( &cc ) == 0 );
  CHECK( GC_push_complex2( 3.0, 4.0 ) == 0 );
  CHECK( GC_get_vector_size() == 4 );
}

TEST_CASE( "C API typed vector setters for bool and complex", "[c_api]" )
{
  CGuard g( "c_typed_vectors" );

  int const bvals[2]{ 1, 0 };
  CHECK( GC_set_vector_of_bool( bvals, 2 ) == 0 );
  CHECK( GC_get_bool_at_pos( 0 ) == 1 );
  CHECK( GC_get_bool_at_pos( 1 ) == 0 );

  double const re[2]{ 1.0, 2.0 };
  double const im[2]{ -1.0, -2.0 };
  CHECK( GC_set_vector_of_complex( re, im, 2 ) == 0 );
  c_complex_type const c0{ GC_get_complex_at_pos( 0 ) };
  CHECK( c0.real == 1.0 );
  CHECK( c0.imag == -1.0 );
  CHECK( GC_get_complex_real_at_pos( 1 ) == 2.0 );
  CHECK( GC_get_complex_imag_at_pos( 1 ) == -2.0 );
}

TEST_CASE( "C API empty typed vectors", "[c_api]" )
{
  CGuard g( "c_empty_vectors" );

  CHECK( GC_set_empty_vector_of_bool() == 0 );
  CHECK( GC_set_empty_vector_of_int() == 0 );
  CHECK( GC_set_empty_vector_of_real() == 0 );
  CHECK( GC_set_empty_vector_of_complex() == 0 );
  CHECK( GC_set_empty_vector_of_string() == 0 );
  CHECK( GC_get_vector_size() == 0 );
}

TEST_CASE( "C API generic vector allocation with GC_set_vector", "[c_api]" )
{
  CGuard g( "c_set_vector" );

  CHECK( GC_set_vector( 5 ) == 0 );
  CHECK( GC_get_vector_size() == 5 );
  CHECK( GC_push_vector_position( 2 ) == 0 );
  CHECK( GC_set_int( 42 ) == 0 );
  CHECK( GC_pop_head() == 0 );
  CHECK( GC_get_int_at_pos( 2 ) == 42 );
}

TEST_CASE( "C API matrix coordinate access", "[c_api]" )
{
  CGuard g( "c_matrix" );

  REQUIRE( GC_fill_for_test( "c_matrix" ) == 0 );
  REQUIRE( GC_select( "c_matrix" ) == 0 );
  REQUIRE( GC_reset_head() == 0 );
  CHECK( GC_push_vector_position( 8 ) == 0 );  // element 8: mat_real 2x2
  CHECK( GC_get_matrix_num_rows() == 2 );
  CHECK( GC_get_matrix_num_cols() == 2 );
  CHECK( GC_get_real_at_coor( 1, 1 ) == 2 );
  CHECK( GC_get_real_at_coor( 0, 1 ) == 3 );
  CHECK( GC_pop_head() == 0 );

  CHECK( GC_push_vector_position( 9 ) == 0 );  // element 9: mat_complex 2x2
  c_complex_type const cc{ GC_get_complex_at_coor( 1, 1 ) };
  CHECK( cc.real == 2 );
  CHECK( cc.imag == 2 );
  CHECK( GC_get_complex_real_at_coor( 0, 1 ) == 1 );
  CHECK( GC_get_complex_imag_at_coor( 0, 1 ) == -1 );
}

TEST_CASE( "C API introspection: type name, dump, print, mem_ptr", "[c_api]" )
{
  CGuard g( "c_introspect" );

  CHECK( GC_set_real( 1.5 ) == 0 );
  CHECK( std::string( GC_get_type_name() ) == "real_type" );
  CHECK( GC_print_content_types() == 0 );
  CHECK( GC_dump() == 0 );
  CHECK( GC_mem_ptr( "c_introspect" ) != nullptr );

  CHECK( GC_reset_head() == 0 );
}

TEST_CASE( "C API is a safe no-op / returns sentinel values before any container is active", "[c_api]" )
{
  // Deleting the currently-active id nulls the active pointer without
  // creating a new one, exercising every `gc_active == nullptr` guard.
  REQUIRE( GC_new( "c_no_active" ) == 0 );
  REQUIRE( GC_delete( "c_no_active" ) == 0 );

  CHECK( GC_get_type() == -1 );
  CHECK( std::string( GC_get_type_name() ).empty() );
  CHECK( GC_mem_ptr( "c_no_active" ) != nullptr );  // GC_mem_ptr re-selects internally

  // re-null the active pointer for the remaining nullptr-guard checks
  REQUIRE( GC_delete( "c_no_active" ) == 0 );

  CHECK( GC_get_bool() == 0 );
  CHECK( GC_get_int() == 0 );
  CHECK( GC_get_long() == 0 );
  CHECK( GC_get_real() == 0 );
  c_complex_type const c{ GC_get_complex() };
  CHECK( c.real == 0 );
  CHECK( c.imag == 0 );
  CHECK( GC_get_complex_re() == 0 );
  CHECK( GC_get_complex_im() == 0 );
  CHECK( GC_get_string() == nullptr );

  CHECK( GC_get_bool_at_pos( 0 ) == 0 );
  CHECK( GC_get_int_at_pos( 0 ) == 0 );
  CHECK( GC_get_real_at_pos( 0 ) == 0 );
  c_complex_type const cp{ GC_get_complex_at_pos( 0 ) };
  CHECK( cp.real == 0 );
  CHECK( cp.imag == 0 );
  CHECK( GC_get_complex_real_at_pos( 0 ) == 0 );
  CHECK( GC_get_complex_imag_at_pos( 0 ) == 0 );
  CHECK( GC_get_string_at_pos( 0 ) == nullptr );

  CHECK( GC_get_real_at_coor( 0, 0 ) == 0 );
  c_complex_type const cc{ GC_get_complex_at_coor( 0, 0 ) };
  CHECK( cc.real == 0 );
  CHECK( cc.imag == 0 );
  CHECK( GC_get_complex_real_at_coor( 0, 0 ) == 0 );
  CHECK( GC_get_complex_imag_at_coor( 0, 0 ) == 0 );

  CHECK( GC_get_vector_size() == 0 );
  CHECK( GC_get_matrix_num_rows() == 0 );
  CHECK( GC_get_matrix_num_cols() == 0 );

  CHECK( GC_set_bool( 1 ) != 0 );
  CHECK( GC_set_int( 1 ) != 0 );
  CHECK( GC_set_real( 1.0 ) != 0 );
  c_complex_type const sc{ 1.0, 1.0 };
  CHECK( GC_set_complex( &sc ) != 0 );
  CHECK( GC_set_complex2( 1.0, 1.0 ) != 0 );
  CHECK( GC_set_string( "x" ) != 0 );
  CHECK( GC_push_bool( 1 ) != 0 );
  CHECK( GC_push_int( 1 ) != 0 );
  CHECK( GC_push_real( 1.0 ) != 0 );
  CHECK( GC_push_complex( &sc ) != 0 );
  CHECK( GC_push_complex2( 1.0, 1.0 ) != 0 );
  CHECK( GC_push_string( "x" ) != 0 );
  CHECK( GC_set_empty_vector_of_bool() != 0 );
  CHECK( GC_set_empty_vector_of_int() != 0 );
  CHECK( GC_set_empty_vector_of_real() != 0 );
  CHECK( GC_set_empty_vector_of_complex() != 0 );
  CHECK( GC_set_empty_vector_of_string() != 0 );
  CHECK( GC_set_vector_of_bool( nullptr, 0 ) != 0 );
  CHECK( GC_set_vector_of_int( nullptr, 0 ) != 0 );
  CHECK( GC_set_vector_of_real( nullptr, 0 ) != 0 );
  CHECK( GC_set_vector_of_complex( nullptr, nullptr, 0 ) != 0 );
  CHECK( GC_set_vector_of_string( nullptr, 0 ) != 0 );
  CHECK( GC_set_vector( 1 ) != 0 );
  CHECK( GC_set_empty_vector() != 0 );
  CHECK( GC_set_map() != 0 );
  CHECK( GC_push_vector_position( 0 ) != 0 );
  CHECK( GC_push_map_position( "k" ) != 0 );
  CHECK( GC_init_map_key() != 0 );
  CHECK( GC_get_next_key() == nullptr );
  CHECK( GC_pop_head() != 0 );
  CHECK( GC_reset_head() != 0 );
  CHECK( GC_print_content_types() != 0 );
  CHECK( GC_dump() != 0 );
}
