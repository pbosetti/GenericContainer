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
 |      Universita` degli Studi di Trento                                   |
 |      email: enrico.bertolazzi@unitn.it                                   |
 |                                                                          |
\*--------------------------------------------------------------------------*/

//
// file: GenericContainer.cc
//

#include "GenericContainer.hh"
#include <iomanip>

// use pcre for pattern matching
#ifndef GENERIC_CONTAINER_NO_PCRE
  #include <pcre.h>
#endif

#define CHECK_RESIZE(pV,I) if ( pV->size() <= (I) ) pV->resize((I)+1)

namespace GenericContainerNamepace {

  std::ostream &
  operator << ( std::ostream & s, vec_bool_type const & v ) {
    if ( v.size() > 0 ) {
      s << (v[0]?"[ true":"[ false") ;
      for ( unsigned i = 1 ; i < v.size() ; ++i )
        s << (v[i]?" true":" false") ;
      s << " ]" ;
    } else {
      s << "[]" ;
    }
    return s ;
  }

  std::ostream &
  operator << ( std::ostream & s, vec_int_type const & v ) {
    if ( v.size() > 0 ) {
      s << "[ " << v[0] ;
      for ( unsigned i = 1 ; i < v.size() ; ++i ) s << " " << v[i] ;
      s << " ]" ;
    } else {
      s << "[]" ;
    }
    return s ;
  }

  std::ostream &
  operator << ( std::ostream & s, vec_real_type const & v ) {
    if ( v.size() > 0 ) {
      s << "[ " << v[0] ;
      for ( unsigned i = 1 ; i < v.size() ; ++i ) s << " " << v[i] ;
      s << " ]" ;
    } else {
      s << "[]" ;
    }
    return s ;
  }

  std::ostream &
  operator << ( std::ostream & s, vec_complex_type const & v ) {
    if ( v.size() > 0 ) {
      s << "[ (" << v[0].real() << ", " << v[0].imag() << " )"  ;
      for ( unsigned i = 1 ; i < v.size() ; ++i )
        s << " (" << v[i].real() << ", " << v[i].imag() << " )"  ;
      s << " ]" ;
    } else {
      s << "[]" ;
    }
    return s ;
  }

  real_type const &
  mat_real_type::operator () ( unsigned i, unsigned j ) const {
    GC_ASSERT( i < _numRows && j < _numCols,
               "mat_real_type::operator() (" << i << ", " << j <<
               ") index out of range: [0," << _numRows <<
               ") x [0," << _numCols << ")\n" ) ;
    return (*this)[size_type(i+j*_numRows)] ;
  }

  real_type &
  mat_real_type::operator () ( unsigned i, unsigned j ) {
    GC_ASSERT( i < _numRows && j < _numCols,
               "mat_real_type::operator() (" << i << ", " << j <<
               ") index out of range: [0," << _numRows <<
               ") x [0," << _numCols << ")\n" ) ;
    return (*this)[size_type(i+j*_numRows)] ;
  }

  complex_type const &
  mat_complex_type::operator () ( unsigned i, unsigned j ) const {
    GC_ASSERT( i < _numRows && j < _numCols,
               "mat_real_type::operator() (" << i << ", " << j <<
               ") index out of range: [0," << _numRows <<
               ") x [0," << _numCols << ")\n" ) ;
    return (*this)[size_type(i+j*_numRows)] ;
  }

  complex_type &
  mat_complex_type::operator () ( unsigned i, unsigned j ) {
    GC_ASSERT( i < _numRows && j < _numCols,
               "mat_real_type::operator() (" << i << ", " << j <<
               ") index out of range: [0," << _numRows <<
               ") x [0," << _numCols << ")\n" ) ;
    return (*this)[size_type(i+j*_numRows)] ;
  }

  std::ostream &
  operator << ( std::ostream & s, mat_real_type const & m ) {
    for ( unsigned i = 0 ; i < m.numRows() ; ++i ) {
      s << std::setw(8) << m(i,0) ;
      for ( unsigned j = 1 ; j < m.numCols() ; ++j )
        s << " " << std::setw(8) << m(i,j) ;
      s << '\n' ;
    }
    return s ;
  }

  std::ostream &
  operator << ( std::ostream & s, mat_complex_type const & m ) {
    for ( unsigned i = 0 ; i < m.numRows() ; ++i ) {
      s << std::setw(8) << m(i,0) ;
      for ( unsigned j = 1 ; j < m.numCols() ; ++j )
        s << " (" << std::setw(8) << m(i,j).real() << ", " << std::setw(8) << m(i,j).imag() << " )" ;
      s << '\n' ;
    }
    return s ;
  }

  #ifndef GENERIC_CONTAINER_NO_PCRE

  class GENERIC_CONTAINER_API_DLL Pcre_for_GC {

  private:

    pcre        * reCompiled      ; // pcre *
    pcre_extra  * pcreExtra       ; // pcre_extra *
    char const  * pcreErrorStr    ; // const char *
    int           pcreErrorOffset ;

  public:

    Pcre_for_GC() {
      reCompiled = pcre_compile("^\\s*\\d+\\s*(##?)(-|=|~|_|)\\s*(.*)$",
                                0,
                                &pcreErrorStr,
                                &pcreErrorOffset,
                                NULL);
      // pcre_compile returns NULL on error, and sets pcreErrorOffset & pcreErrorStr
      GC_ASSERT( reCompiled != nullptr,
                 "Cannot compile regex for GenericContainer\n" ) ;
      // Optimize the regex
      pcreExtra = pcre_study(reCompiled, 0, &pcreErrorStr);
      GC_ASSERT( pcreExtra != nullptr,
                 "Cannot optimize regex for GenericContainer\n" ) ;
    }

    int
    exec( char const str[],
          int  const len,
          int        imatch[30] ) {
      // num+"@"+"underline character"
      // Try to find the regex in aLineToMatch, and report results.
      return pcre_exec(reCompiled,
                       pcreExtra,
                       str,
                       len,         // length of string
                       0,           // Start looking at this point
                       0,           // OPTIONS
                       imatch,
                       30);         // Length of subStrVec
    }

  } ;

  static Pcre_for_GC pcre_for_GC ;

  #endif

  static char const *typeName[] = {

    "NOTYPE",

    "pointer",
    "bool_type",
    "int_type",
    "long_type",
    "real_type",
    "complex_type",
    "string_type",

    "vec_pointer_type",
    "vec_bool_type",
    "vec_int_type",
    "vec_real_type",
    "vec_complex_type",
    "vec_string_type",

    "mat_real_type",
    "mat_complex_type",

    "vector_type",
    "map_type"
  } ;

  // costruttore
  GenericContainer::GenericContainer()
  : _data_type(GC_NOTYPE)
  {
  }

  #ifdef GENERIC_CONTAINER_ON_WINDOWS
  bool
  GenericContainer::simple_data() const {
    return _data_type <= GC_STRING ;
  }
  bool
  GenericContainer::simple_vec_data() const {
    return _data_type <= GC_VEC_STRING ;
  }
  #endif

  // distruttore
  void
  GenericContainer::clear() {
    switch (_data_type) {
      case GC_POINTER:
        // removed annoying warning. To be re-thinked...
        //GC_WARNING( _data.p == nullptr, "find a pointer not deallocated!" ) ;
        break ;
      case GC_STRING:      delete _data.s   ; break ;
      case GC_COMPLEX:     delete _data.c   ; break ;

      case GC_VEC_POINTER: delete _data.v_p ; break ;
      case GC_VEC_BOOL:    delete _data.v_b ; break ;
      case GC_VEC_INTEGER: delete _data.v_i ; break ;
      case GC_VEC_REAL:    delete _data.v_r ; break ;
      case GC_VEC_COMPLEX: delete _data.v_c ; break ;
      case GC_MAT_REAL:    delete _data.m_r ; break ;
      case GC_MAT_COMPLEX: delete _data.m_c ; break ;
      case GC_VEC_STRING:  delete _data.v_s ; break ;

      case GC_VECTOR:      delete _data.v   ; break ;
      case GC_MAP:         delete _data.m   ; break ;
      default:
        break ;
    }
    _data_type = GC_NOTYPE ;
  }

  //! Return a string representing the type of data stored
  char const *
  GenericContainer::get_type_name() const {
    return typeName[_data_type] ;
  }

  unsigned
  GenericContainer::get_num_elements() const {
    switch (_data_type) {
      case GC_POINTER:
      case GC_BOOL:
      case GC_INTEGER:
      case GC_LONG:
      case GC_REAL:
      case GC_COMPLEX:
      case GC_STRING:      return 1 ;
      case GC_VEC_POINTER: return (unsigned)_data.v_p->size() ;
      case GC_VEC_BOOL:    return (unsigned)_data.v_b->size() ;
      case GC_VEC_INTEGER: return (unsigned)_data.v_i->size() ;
      case GC_VEC_REAL:    return (unsigned)_data.v_r->size() ;
      case GC_VEC_COMPLEX: return (unsigned)_data.v_c->size() ;
      case GC_MAT_REAL:    return (unsigned)_data.m_r->size() ;
      case GC_MAT_COMPLEX: return (unsigned)_data.m_c->size() ;
      case GC_VEC_STRING:  return (unsigned)_data.v_s->size() ;
      case GC_VECTOR:      return (unsigned)_data.v->size() ;
      case GC_MAP:         return (unsigned)_data.m->size() ;
      default:             return 0 ;
    }

  }

  unsigned
  GenericContainer::get_numRows() const {
    switch (_data_type) {
      case GC_POINTER:
      case GC_BOOL:
      case GC_INTEGER:
      case GC_LONG:
      case GC_REAL:
      case GC_COMPLEX:
      case GC_STRING:
      case GC_VEC_POINTER:
      case GC_VEC_BOOL:
      case GC_VEC_INTEGER:
      case GC_VEC_REAL:
      case GC_VEC_COMPLEX:
      case GC_VEC_STRING:
      case GC_VECTOR:      return 1 ;
      case GC_MAT_REAL:    return (unsigned)_data.m_r->numRows() ;
      case GC_MAT_COMPLEX: return (unsigned)_data.m_c->numRows() ;
      case GC_MAP:         return 1 ;
      default:             return 0 ;
    }
  }

  unsigned
  GenericContainer::get_numCols() const {
    switch (_data_type) {
      case GC_POINTER:
      case GC_BOOL:
      case GC_INTEGER:
      case GC_LONG:
      case GC_REAL:
      case GC_COMPLEX:
      case GC_STRING:      return 1 ;
      case GC_VEC_POINTER: return (unsigned)_data.v_p->size() ;
      case GC_VEC_BOOL:    return (unsigned)_data.v_b->size() ;
      case GC_VEC_INTEGER: return (unsigned)_data.v_i->size() ;
      case GC_VEC_REAL:    return (unsigned)_data.v_r->size() ;
      case GC_VEC_COMPLEX: return (unsigned)_data.v_c->size() ;
      case GC_MAT_REAL:    return (unsigned)_data.m_r->numCols() ;
      case GC_MAT_COMPLEX: return (unsigned)_data.m_c->numCols() ;
      case GC_VEC_STRING:  return (unsigned)_data.v_s->size() ;
      case GC_VECTOR:      return (unsigned)_data.v->size() ;
      case GC_MAP:         return (unsigned)_data.m->size() ;
      default:             return 0 ;
    }

  }

  //! Assign a generic container `a` to the generic container.
  void
  GenericContainer::load( GenericContainer const & gc ) {
    this -> clear() ;
    switch (gc._data_type) {
      case GC_NOTYPE:      break ;
      case GC_POINTER:     this -> set_pointer(gc._data.p)  ; break ;
      case GC_BOOL:        this -> set_bool(gc._data.b)     ; break ;
      case GC_INTEGER:     this -> set_int(gc._data.i)      ; break ;
      case GC_LONG:        this -> set_long(gc._data.l)      ; break ;
      case GC_REAL:        this -> set_real(gc._data.r)     ; break ;
      case GC_COMPLEX:     this -> set_complex(*gc._data.c) ; break ;
      case GC_STRING:      this -> set_string(*gc._data.s)  ; break ;

      case GC_VEC_POINTER: this -> set_vec_pointer(*gc._data.v_p) ; break ;
      case GC_VEC_BOOL:    this -> set_vec_bool(*gc._data.v_b)    ; break ;
      case GC_VEC_INTEGER: this -> set_vec_int(*gc._data.v_i)     ; break ;
      case GC_VEC_REAL:    this -> set_vec_real(*gc._data.v_r)    ; break ;
      case GC_VEC_COMPLEX: this -> set_vec_complex(*gc._data.v_c) ; break ;
      case GC_VEC_STRING:  this -> set_vec_string(*gc._data.v_s)  ; break ;

      case GC_MAT_REAL:    this -> set_mat_real(*gc._data.m_r)    ; break ;
      case GC_MAT_COMPLEX: this -> set_mat_complex(*gc._data.m_c) ; break ;

      case GC_VECTOR:
        { unsigned N = unsigned(gc._data.v->size()) ;
          allocate_vector( N ) ;
          std::copy( gc._data.v->begin(),
                     gc._data.v->end(),
                     this->_data.v->begin() ) ;
        }
        break ;
      case GC_MAP:
        { allocate_map() ;
          // this->_data.m->insert( gc._data.m->begin(), gc._data.m->end() ) ; !!!! DO NOT WORK ON CLANG
          for ( map_type::iterator it = gc._data.m->begin() ;
                it != gc._data.m->end() ; ++it )
            (*this->_data.m)[it->first] = it->second ;
        }
        break ;
      default:
        break ;
    }
  }

  int
  GenericContainer::ck( TypeAllowed tp ) const {
    if ( tp == _data_type ) return 0 ; // ok
    if ( tp == GC_NOTYPE  ) return 1 ; //
    return 2 ;
  }

  void
  GenericContainer::ck(char const who[], TypeAllowed tp) const {
    GC_ASSERT( tp == _data_type,
               who <<
               " bad data type\nexpect: " << typeName[tp] <<
               "\nbut data stored is of type: " << typeName[_data_type] ) ;
  }

  void
  GenericContainer::ck_or_set(char const who[], TypeAllowed tp) {
    if ( _data_type == GC_NOTYPE ) {
      _data_type = tp ;
    } else {
      GC_ASSERT( tp == _data_type,
                 who <<
                 " bad data type\nexpect: " << typeName[tp] <<
                 "\nbut data stored is of type: " << typeName[_data_type] ) ;
    }
  }

  /*
   //      _    _ _                 _
   //     / \  | | | ___   ___ __ _| |_ ___
   //    / _ \ | | |/ _ \ / __/ _` | __/ _ \
   //   / ___ \| | | (_) | (_| (_| | ||  __/
   //  /_/   \_\_|_|\___/ \___\__,_|\__\___|
   */
  void
  GenericContainer::allocate_string() {
    if ( _data_type != GC_STRING ) {
      clear() ;
      _data_type = GC_STRING ;
      _data.s    = new string_type ;
    }
  }

  void
  GenericContainer::allocate_complex() {
    if ( _data_type != GC_COMPLEX ) {
      clear() ;
      _data_type = GC_COMPLEX ;
      _data.c    = new complex_type ;
    }
  }

  void
  GenericContainer::allocate_vec_pointer( unsigned sz ) {
    if ( _data_type != GC_VEC_POINTER ) {
      clear() ;
      _data_type = GC_VEC_POINTER ;
      _data.v_p  = new vec_pointer_type() ;
    }
    if ( sz > 0 ) _data.v_p -> resize( sz ) ;
  }

  GenericContainer &
  GenericContainer::free_pointer() {
    GC_ASSERT( GC_POINTER == _data_type || GC_NOTYPE == _data_type,
               "free_pointer() bad data type\nexpect: " << typeName[GC_POINTER] <<
               "\nbut data stored is of type: " << typeName[_data_type] ) ;
    _data.p = nullptr ;
    _data_type = GC_NOTYPE ;
    return *this ;
  }

  void
  GenericContainer::allocate_vec_bool( unsigned sz ) {
    if ( _data_type != GC_VEC_BOOL ) {
      clear() ;
      _data_type = GC_VEC_BOOL ;
      _data.v_b  = new vec_bool_type() ;
    }
    if ( sz > 0 ) _data.v_b -> resize( sz ) ;
  }

  void
  GenericContainer::allocate_vec_int( unsigned sz ) {
    if ( _data_type != GC_VEC_INTEGER ) {
      clear() ;
      _data_type = GC_VEC_INTEGER ;
      _data.v_i  = new vec_int_type() ;
    }
    if ( sz > 0 ) _data.v_i -> resize( sz ) ;
  }

  void
  GenericContainer::allocate_vec_real( unsigned sz ) {
    if ( _data_type != GC_VEC_REAL ) {
      clear() ;
      _data_type = GC_VEC_REAL ;
      _data.v_r  = new vec_real_type() ;
    }
    if ( sz > 0 ) _data.v_r -> resize( sz ) ;
  }

  void
  GenericContainer::allocate_vec_complex( unsigned sz ) {
    if ( _data_type != GC_VEC_COMPLEX ) {
      clear() ;
      _data_type = GC_VEC_COMPLEX ;
      _data.v_c  = new vec_complex_type() ;
    }
    if ( sz > 0 ) _data.v_c -> resize( sz ) ;
  }

  void
  GenericContainer::allocate_mat_real( unsigned nr, unsigned nc ) {
    if ( _data_type != GC_MAT_REAL ) {
      clear() ;
      _data_type = GC_MAT_REAL ;
      _data.m_r  = new mat_real_type( nr, nc ) ;
    } else {
      _data.m_r -> resize( nr, nc ) ;
    }
  }

  void
  GenericContainer::allocate_mat_complex( unsigned nr, unsigned nc ) {
    if ( _data_type != GC_MAT_COMPLEX ) {
      clear() ;
      _data_type = GC_MAT_COMPLEX ;
      _data.m_c  = new mat_complex_type( nr, nc ) ;
    } else {
      _data.m_c -> resize( nr, nc ) ;
    }
  }

  void
  GenericContainer::allocate_vec_string( unsigned sz ) {
    if ( _data_type != GC_VEC_STRING ) {
      clear() ;
      _data_type = GC_VEC_STRING ;
      _data.v_s  = new vec_string_type() ;
    }
    if ( sz > 0 ) _data.v_s -> resize( sz ) ;
  }

  void
  GenericContainer::allocate_vector( unsigned sz ) {
    if ( _data_type != GC_VECTOR ) {
      clear() ;
      _data_type = GC_VECTOR ;
      _data.v    = new vector_type() ;
    }
    if ( sz > 0 ) _data.v -> resize( sz ) ;
  }

  void
  GenericContainer::allocate_map() {
    if ( _data_type != GC_MAP ) {
      clear() ;
      _data_type = GC_MAP ;
      _data.m    = new map_type() ;
    }
  }

  /*
  //   ____       _
  //  / ___|  ___| |_
  //  \___ \ / _ \ __|
  //   ___) |  __/ |_
  //  |____/ \___|\__|
  */

  pointer_type &
  GenericContainer::set_pointer( pointer_type value ) {
    clear() ;
    _data_type = GC_POINTER ;
    return (_data.p = value) ;
  }

  bool_type &
  GenericContainer::set_bool( bool_type value ) {
    clear() ;
    _data_type = GC_BOOL ;
    return (_data.b = value) ;
  }

  int_type &
  GenericContainer::set_int( int_type value ) {
    clear() ;
    _data_type = GC_INTEGER ;
    return (_data.i = value) ;
  }

  long_type &
  GenericContainer::set_long( long_type value ) {
    clear() ;
    _data_type = GC_LONG ;
    return (_data.l = value) ;
  }

  real_type &
  GenericContainer::set_real( real_type value ) {
    clear() ;
    _data_type = GC_REAL ;
    return (_data.r = value) ;
  }

  complex_type &
  GenericContainer::set_complex( complex_type & value ) {
    clear() ;
    _data_type = GC_COMPLEX ;
    _data.c    = new complex_type ;
    return (*_data.c=value) ;
  }

  complex_type &
  GenericContainer::set_complex( real_type re, real_type im ) {
    clear() ;
    _data_type = GC_COMPLEX ;
    _data.c    = new complex_type(re,im) ;
    return *_data.c ;
  }

  string_type &
  GenericContainer::set_string( string_type const & value ) {
    allocate_string() ;
    return (*_data.s = value) ;
  }

  vec_pointer_type &
  GenericContainer::set_vec_pointer( unsigned sz ) {
    allocate_vec_pointer( sz ) ;
    return *_data.v_p ;
  }

  vec_pointer_type &
  GenericContainer::set_vec_pointer( vec_pointer_type const & v ) {
    allocate_vec_pointer( unsigned(v.size()) ) ;
    std::copy( v.begin(), v.end(), _data.v_p->begin() ) ;
    return *_data.v_p ;
  }

  vec_bool_type &
  GenericContainer::set_vec_bool( unsigned sz ) {
    allocate_vec_bool( sz ) ; return *_data.v_b ;
  }

  vec_bool_type &
  GenericContainer::set_vec_bool( vec_bool_type const & v ) {
    allocate_vec_bool( unsigned(v.size()) ) ;
    std::copy( v.begin(), v.end(), _data.v_b->begin() ) ;
    return *_data.v_b ;
  }

  vec_int_type &
  GenericContainer::set_vec_int( unsigned sz ) {
    allocate_vec_int( sz ) ;
    return *_data.v_i ;
  }

  vec_int_type &
  GenericContainer::set_vec_int( vec_int_type const & v ) {
    allocate_vec_int( unsigned(v.size()) ) ;
    std::copy( v.begin(), v.end(), _data.v_i->begin() ) ;
    return *_data.v_i ;
  }

  vec_real_type &
  GenericContainer::set_vec_real( unsigned sz ) {
    allocate_vec_real( sz ) ;
    return *_data.v_r ;
  }

  vec_real_type &
  GenericContainer::set_vec_real( vec_real_type const & v ) {
    allocate_vec_real( unsigned(v.size()) ) ;
    std::copy( v.begin(), v.end(), _data.v_r->begin() ) ;
    return *_data.v_r ;
  }

  vec_complex_type &
  GenericContainer::set_vec_complex( unsigned sz ) {
    allocate_vec_complex( sz ) ;
    return *_data.v_c ;
  }

  vec_complex_type &
  GenericContainer::set_vec_complex( vec_complex_type const & v ) {
    allocate_vec_complex( unsigned(v.size()) ) ;
    std::copy( v.begin(), v.end(), _data.v_c->begin() ) ;
    return *_data.v_c ;
  }

  mat_real_type &
  GenericContainer::set_mat_real( unsigned nr, unsigned nc ) {
    allocate_mat_real( nr, nc ) ;
    return *_data.m_r ;
  }

  mat_real_type &
  GenericContainer::set_mat_real( mat_real_type const & m ) {
    allocate_mat_real( m.numRows(), m.numCols() ) ;
    std::copy( m.begin(), m.end(), _data.m_r->begin() ) ;
    return *_data.m_r ;
  }

  mat_complex_type &
  GenericContainer::set_mat_complex( unsigned nr, unsigned nc ) {
    allocate_mat_complex( nr, nc ) ;
    return *_data.m_c ;
  }

  mat_complex_type &
  GenericContainer::set_mat_complex( mat_complex_type const & m ) {
    allocate_mat_complex( m.numRows(), m.numCols() ) ;
    std::copy( m.begin(), m.end(), _data.m_c->begin() ) ;
    return *_data.m_c ;
  }

  vec_string_type &
  GenericContainer::set_vec_string( unsigned sz ) {
    allocate_vec_string( sz ) ;
    return *_data.v_s ;
  }

  vec_string_type &
  GenericContainer::set_vec_string( vec_string_type const & v ) {
    allocate_vec_string( unsigned(v.size()) ) ;
    std::copy( v.begin(), v.end(), _data.v_s->begin() ) ;
    return *_data.v_s ;
  }

  vector_type &
  GenericContainer::set_vector( unsigned sz ) {
    allocate_vector( sz ) ;
    return *_data.v ;
  }

  map_type &
  GenericContainer::set_map() {
    allocate_map() ;
    return *_data.m ;
  }
  
  /*
  //   ____            _
  //  |  _ \ _   _ ___| |__
  //  | |_) | | | / __| '_ \
  //  |  __/| |_| \__ \ | | |
  //  |_|    \__,_|___/_| |_|
  */
  void
  GenericContainer::push_bool( bool val ) {
    if ( _data_type == GC_VEC_BOOL ) {
      _data.v_b->push_back( val ) ;
    } else if ( _data_type == GC_VEC_INTEGER ) {
      _data.v_i->push_back( val ? 1 : 0 ) ;
    } else if ( _data_type == GC_VEC_REAL ) {
      _data.v_r->push_back( val ? 1 : 0 ) ;
    } else if ( _data_type == GC_VEC_COMPLEX ) {
      complex_type tmp( val ? 1 : 0, 0 ) ;
      _data.v_c->push_back( tmp ) ;
    } else if ( _data_type == GC_VECTOR   ) {
      _data.v->resize(_data.v->size()+1) ;
      _data.v->back().set_bool( val ) ;
    } else {
      GC_ASSERT( false, "push_bool, bad data stored: " << get_type_name() ) ;
    }
  }

  void
  GenericContainer::push_int( int_type val ) {
    if ( _data_type == GC_VEC_INTEGER ) {
      _data.v_i->push_back( val ) ;
    } else if ( _data_type == GC_VEC_REAL ) {
      _data.v_r->push_back( val ) ;
    } else if ( _data_type == GC_VEC_COMPLEX ) {
      complex_type tmp( val, 0 ) ;
      _data.v_c->push_back( tmp ) ;
    } else if ( _data_type == GC_VECTOR ) {
      _data.v->resize(_data.v->size()+1) ;
      _data.v->back().set_int( val ) ;
    } else {
      if ( _data_type != GC_VEC_INTEGER ) promote_to_vec_int() ;
      _data.v_i->push_back( val ) ;
    }
  }

  void
  GenericContainer::push_real( real_type val ) {
    if ( _data_type == GC_VEC_REAL ) {
      _data.v_r->push_back( val ) ;
    } else if ( _data_type == GC_VEC_COMPLEX ) {
      complex_type tmp( val, 0 ) ;
      _data.v_c->push_back( tmp ) ;
    } else if ( _data_type == GC_VECTOR ) {
      _data.v->resize(_data.v->size()+1) ;
      _data.v->back().set_real( val ) ;
    } else {
      if ( _data_type != GC_VEC_REAL ) promote_to_vec_real() ;
      _data.v_r->push_back( val ) ;
    }
  }

  void
  GenericContainer::push_complex( complex_type & val ) {
    if ( _data_type == GC_VECTOR ) {
      _data.v->resize(_data.v->size()+1) ;
      _data.v->back().set_complex( val ) ;
    } else {
      if ( _data_type != GC_VEC_COMPLEX ) promote_to_vec_complex() ;
      _data.v_c->push_back( val ) ;
    }
  }

  void
  GenericContainer::push_complex( real_type re, real_type im ) {
    complex_type tmp( re, im ) ;
    push_complex( tmp ) ;
  }

  void
  GenericContainer::push_string( string_type const & val ) {
    if ( _data_type != GC_VEC_STRING ) promote_to_vector() ;
    if ( _data_type == GC_VEC_STRING ) {
      _data.v_s->push_back( val ) ;
    } else {
      _data.v->resize(_data.v->size()+1) ;
      _data.v->back().set_string( val ) ;
    }
  }

  /*
  //    ____      _
  //   / ___| ___| |_
  //  | |  _ / _ \ __|
  //  | |_| |  __/ |_
  //   \____|\___|\__|
  */

#if defined(_WIN32) || defined(_WIN64)
  void *
  GenericContainer::get_pvoid( char const msg[] ) const {
    ck(msg,GC_POINTER) ;
    return _data.p ;
  }

  void **
  GenericContainer::get_ppvoid( char const msg[] ) const {
    ck(msg,GC_POINTER) ;
    return (void **)&_data.p ;
  }
#endif

  //! If data is boolean, integer or floating point return number, otherwise return `0`.
  real_type
  GenericContainer::get_number() const {
    switch (_data_type) {
      case GC_BOOL:    return (_data.b?1:0) ;
      case GC_INTEGER: return _data.i ;
      case GC_LONG:    return _data.l ;
      case GC_REAL:    return _data.r ;
      default:
        break ;
    }
    return 0 ;
  }

  //! If data is boolean, integer or floating point return number, otherwise return `0`.
  complex_type
  GenericContainer::get_complex_number() const {
    switch (_data_type) {
      case GC_BOOL:    return (_data.b?1:0) ;
      case GC_INTEGER: return _data.i ;
      case GC_LONG:    return _data.l ;
      case GC_REAL:    return _data.r ;
      case GC_COMPLEX: return *_data.c ;
      default:
      break ;
    }
    return 0 ;
  }

  void
  GenericContainer::get_complex_number( real_type & re, real_type & im ) const {
    complex_type tmp = get_complex_number() ;
    re = tmp.real() ;
    im = tmp.imag() ;
  }

  real_type
  GenericContainer::get_number_at( unsigned i ) const {
    switch (_data_type) {
      case GC_VEC_BOOL:    return (*_data.v_b)[i] ;
      case GC_VEC_INTEGER: return (*_data.v_i)[i] ;
      case GC_VEC_REAL:    return (*_data.v_r)[i] ;
      case GC_MAT_REAL:    return (*_data.m_r)[i] ;
      case GC_VECTOR:      return (*_data.v)[i].get_number() ;
      default: break ; // to quiet warnings
    }
    return 0 ;
  }

  complex_type
  GenericContainer::get_complex_number_at( unsigned i ) const {
    switch (_data_type) {
      case GC_VEC_BOOL:    return int((*_data.v_b)[i]) ;
      case GC_VEC_INTEGER: return (*_data.v_i)[i] ;
      case GC_VEC_REAL:    return (*_data.v_r)[i] ;
      case GC_VEC_COMPLEX: return (*_data.v_c)[i] ;
      case GC_MAT_REAL:    return (*_data.m_r)[i] ;
      case GC_MAT_COMPLEX: return (*_data.m_c)[i] ;
      case GC_VECTOR:      return (*_data.v)[i].get_complex_number() ;
      default: break ; // to quiet warnings
    }
    return 0 ;
  }

  void
  GenericContainer::get_complex_number_at( unsigned i, real_type & re, real_type & im ) const {
    complex_type tmp = get_complex_number_at(i) ;
    re = tmp.real() ;
    im = tmp.imag() ;
  }

  bool_type &
  GenericContainer::get_bool( char const msg[] ) {
    ck_or_set(msg,GC_BOOL) ;
    return _data.b ;
  }

  bool_type const &
  GenericContainer::get_bool( char const msg[] ) const {
    ck(msg,GC_BOOL) ;
    return _data.b ;
  }

  int_type &
  GenericContainer::get_int( char const msg[] ) {
    ck_or_set(msg,GC_INTEGER) ;
    return _data.i ;
  }

  int_type const &
  GenericContainer::get_int( char const msg[] ) const {
    ck(msg,GC_INTEGER) ;
    return _data.i ;
  }

  long_type &
  GenericContainer::get_long( char const msg[] ) {
    ck_or_set(msg,GC_LONG) ;
    return _data.l ;
  }

  long_type const &
  GenericContainer::get_long( char const msg[] ) const {
    ck(msg,GC_LONG) ;
    return _data.l ;
  }

  real_type &
  GenericContainer::get_real( char const msg[] ) {
    ck_or_set(msg,GC_REAL) ;
    return _data.r ;
  }

  real_type const &
  GenericContainer::get_real( char const msg[] ) const {
    ck(msg,GC_REAL) ;
    return _data.r ;
  }

  complex_type &
  GenericContainer::get_complex( char const msg[] ) {
    ck_or_set(msg,GC_COMPLEX) ;
    return *_data.c ;
  }

  complex_type const &
  GenericContainer::get_complex( char const msg[] ) const {
    ck(msg,GC_REAL) ;
    return *_data.c ;
  }

  string_type &
  GenericContainer::get_string( char const msg[] ) {
    ck_or_set(msg,GC_STRING) ;
    return *_data.s ;
  }

  string_type const &
  GenericContainer::get_string( char const msg[] ) const {
    ck(msg,GC_STRING) ;
    return *_data.s ;
  }

  vector_type &
  GenericContainer::get_vector( char const msg[] ) {
    ck(msg,GC_VECTOR) ;
    return *_data.v ;
  }

  vector_type const &
  GenericContainer::get_vector( char const msg[] ) const {
    ck(msg,GC_VECTOR) ;
    return *_data.v ;
  }

  vec_pointer_type &
  GenericContainer::get_vec_pointer( char const msg[] ) {
    ck(msg,GC_VEC_POINTER) ;
    return *_data.v_p ;
  }

  vec_pointer_type const &
  GenericContainer::get_vec_pointer( char const msg[] ) const {
    ck(msg,GC_VEC_POINTER) ;
    return *_data.v_p ;
  }

  vec_bool_type &
  GenericContainer::get_vec_bool( char const msg[] ) {
    ck(msg,GC_VEC_BOOL) ;
    return *_data.v_b ;
  }

  vec_bool_type const &
  GenericContainer::get_vec_bool( char const msg[] ) const {
    ck(msg,GC_VEC_BOOL) ;
    return *_data.v_b ;
  }

  vec_int_type &
  GenericContainer::get_vec_int( char const msg[] ) {
    if ( _data_type == GC_NOTYPE   ) set_vec_int() ;
    if ( _data_type == GC_VEC_BOOL ) promote_to_vec_int() ;
    ck(msg,GC_VEC_INTEGER) ;
    return *_data.v_i ;
  }

  vec_int_type const &
  GenericContainer::get_vec_int( char const msg[] ) const {
    ck(msg,GC_VEC_INTEGER) ;
    return *_data.v_i ;
  }

  vec_real_type &
  GenericContainer::get_vec_real( char const msg[] ) {
    if ( _data_type == GC_NOTYPE   ) set_vec_real() ;
    if ( _data_type == GC_VEC_BOOL ||
         _data_type == GC_VEC_INTEGER ) promote_to_vec_real() ;
    ck(msg,GC_VEC_REAL) ;
    return *_data.v_r ;
  }

  vec_real_type const &
  GenericContainer::get_vec_real( char const msg[] ) const {
    ck(msg,GC_VEC_REAL) ;
    return *_data.v_r ;
  }

  vec_complex_type &
  GenericContainer::get_vec_complex( char const msg[] ) {
    if ( _data_type == GC_NOTYPE ) set_vec_complex() ;
    if ( _data_type == GC_VEC_BOOL    ||
         _data_type == GC_VEC_INTEGER ||
         _data_type == GC_VEC_REAL ) promote_to_vec_complex() ;
    ck(msg,GC_VEC_COMPLEX) ;
    return *_data.v_c ;
  }

  vec_complex_type const &
  GenericContainer::get_vec_complex( char const msg[] ) const {
    ck(msg,GC_VEC_COMPLEX) ;
    return *_data.v_c ;
  }

  mat_real_type &
  GenericContainer::get_mat_real( char const msg[] ) {
    if ( _data_type == GC_NOTYPE ) set_mat_real() ;
    if ( _data_type == GC_VEC_BOOL    ||
         _data_type == GC_VEC_INTEGER ||
         _data_type == GC_VEC_REAL ) promote_to_mat_real() ;
    ck(msg,GC_MAT_REAL) ;
    return *_data.m_r ;
  }

  mat_real_type const &
  GenericContainer::get_mat_real( char const msg[] ) const {
    ck(msg,GC_MAT_REAL) ;
    return *_data.m_r ;
  }

  mat_complex_type &
  GenericContainer::get_mat_complex( char const msg[] ) {
    if ( _data_type == GC_NOTYPE ) set_mat_complex() ;
    if ( _data_type == GC_VEC_BOOL    ||
         _data_type == GC_VEC_INTEGER ||
         _data_type == GC_VEC_REAL    ||
         _data_type == GC_MAT_REAL    ||
         _data_type == GC_VEC_COMPLEX ) promote_to_mat_complex() ;
    ck(msg,GC_MAT_COMPLEX) ;
    return *_data.m_c ;
  }

  mat_complex_type const &
  GenericContainer::get_mat_complex( char const msg[] ) const {
    ck(msg,GC_MAT_COMPLEX) ;
    return *_data.m_c ;
  }

  vec_string_type &
  GenericContainer::get_vec_string( char const msg[] ) {
    ck(msg,GC_VEC_STRING) ;
    return *_data.v_s ;
  }

  vec_string_type const &
  GenericContainer::get_vec_string( char const msg[] ) const {
    ck(msg,GC_VEC_STRING) ;
    return *_data.v_s ;
  }

  map_type &
  GenericContainer::get_map( char const msg[] ) {
    ck(msg,GC_MAP) ;
    return *_data.m ;
  }

  map_type const &
  GenericContainer::get_map( char const msg[] ) const {
    ck(msg,GC_MAP) ;
    return *_data.m ;
  }

  bool
  GenericContainer::exists( std::string const & s ) const {
    if ( _data_type != GC_MAP ) return false ;
    map_type::iterator iv = (*_data.m).find(s) ;
    return iv != (*_data.m).end() ;
  }

  // --------------------------------------------------------------
  bool_type
  GenericContainer::get_bool_at( unsigned i ) {
    if ( _data_type == GC_NOTYPE   ) set_vec_bool() ;
    if ( _data_type == GC_VEC_BOOL ) {
      CHECK_RESIZE(_data.v_b,i) ; // correct type, check size
      return (*_data.v_b)[i] ;
    } else {
      if ( _data_type != GC_VECTOR ) promote_to_vector() ;
      CHECK_RESIZE(_data.v,i) ;
      return (*_data.v)[i].set_bool(false) ;
    }
  }

  bool_type
  GenericContainer::get_bool_at( unsigned i ) const {
    ck("get_bool()",GC_VEC_BOOL) ;
    GC_ASSERT( i < _data.v_b->size(), "get_bool_at( " << i << " ) const, out of range" ) ;
    return (*_data.v_b)[i] ;
  }

  int_type &
  GenericContainer::get_int_at( unsigned i ) {
    if      ( _data_type == GC_NOTYPE ) set_vec_int() ;
    else if ( _data_type == GC_VEC_BOOL ) promote_to_vec_int() ;
    if ( _data_type == GC_VEC_INTEGER ) {
      CHECK_RESIZE(_data.v_i,i) ; // correct type, check size
      return (*_data.v_i)[i] ;
    } else {
      if ( _data_type != GC_VECTOR ) promote_to_vector() ;
      CHECK_RESIZE(_data.v,i) ;
      return (*_data.v)[i].set_int(0) ;
    }
  }

  int_type const &
  GenericContainer::get_int_at( unsigned i, char const msg[] ) const {
    ck(msg,GC_VEC_INTEGER) ;
    GC_ASSERT( i < _data.v_i->size(), "get_int_at( " << i << " ) const, out of range" ) ;
    return (*_data.v_i)[i] ;
  }

  real_type &
  GenericContainer::get_real_at( unsigned i ) {
    if      ( _data_type == GC_NOTYPE ) set_vec_real() ;
    else if ( _data_type == GC_VEC_BOOL || _data_type == GC_VEC_INTEGER ) promote_to_vec_real() ;
    if ( _data_type == GC_VEC_REAL ) {
      CHECK_RESIZE(_data.v_r,i) ; // correct type, check size
      return (*_data.v_r)[i] ;
    } else {
      if ( _data_type != GC_VECTOR ) promote_to_vector() ;
      CHECK_RESIZE(_data.v,i) ;
      return (*_data.v)[i].set_real(0) ;
    }
  }

  real_type const &
  GenericContainer::get_real_at( unsigned i ) const  {
    GC_ASSERT( GC_VEC_REAL == _data_type,
               "get_real( " << i << " ) bad data type" <<
               "\nexpect: " << typeName[GC_VEC_REAL] <<
               "\nbut data stored is of type: " << typeName[_data_type] ) ;
    GC_ASSERT( i < _data.v_r->size(), "get_real_at( " << i << " ) const, out of range" ) ;
    return (*_data.v_r)[i] ;
  }

  real_type &
  GenericContainer::get_real_at( unsigned i, unsigned j ) {
    if      ( _data_type == GC_NOTYPE ) set_mat_real(i,j) ;
    else if ( _data_type == GC_VEC_BOOL    ||
              _data_type == GC_VEC_INTEGER ||
              _data_type == GC_VEC_REAL ) promote_to_mat_real() ;
    GC_ASSERT( GC_MAT_REAL == _data_type,
               "get_real_at( " << i << ", " << j << " ) bad data type" <<
               "\nexpect: " << typeName[GC_MAT_REAL] <<
               "\nbut data stored is of type: " << typeName[_data_type] ) ;
    return (*_data.m_r)(i,j) ;
  }

  real_type const &
  GenericContainer::get_real_at( unsigned i, unsigned j ) const  {
    GC_ASSERT( GC_MAT_REAL == _data_type,
               "get_real_at( " << i << ", " << j << " ) bad data type" <<
               "\nexpect: " << typeName[GC_MAT_REAL] <<
               "\nbut data stored is of type: " << typeName[_data_type] ) ;
    return (*_data.m_r)(i,j) ;
  }

  complex_type &
  GenericContainer::get_complex_at( unsigned i ) {
    if      ( _data_type == GC_NOTYPE ) set_vec_complex() ;
    else if ( _data_type == GC_VEC_BOOL    ||
              _data_type == GC_VEC_INTEGER ||
              _data_type == GC_VEC_REAL ) promote_to_vec_complex() ;
    if ( _data_type == GC_VEC_COMPLEX ) {
      CHECK_RESIZE(_data.v_c,i) ; // correct type, check size
      return (*_data.v_c)[i] ;
    } else {
      if ( _data_type != GC_VECTOR ) promote_to_vector() ;
      CHECK_RESIZE(_data.v,i) ;
      return (*_data.v)[i].set_complex(0,0) ;
    }
  }

  complex_type const &
  GenericContainer::get_complex_at( unsigned i ) const  {
    GC_ASSERT( GC_VEC_COMPLEX == _data_type,
               "get_complex( " << i << " ) bad data type" <<
               "\nexpect: " << typeName[GC_VEC_COMPLEX] <<
               "\nbut data stored is of type: " << typeName[_data_type] ) ;
    GC_ASSERT( i < _data.v_c->size(), "get_complex_at( " << i << " ) const, out of range" ) ;
    return (*_data.v_c)[i] ;
  }

  complex_type &
  GenericContainer::get_complex_at( unsigned i, unsigned j ) {
    if      ( _data_type == GC_NOTYPE ) set_mat_complex(i,j) ;
    else if ( _data_type == GC_VEC_BOOL    ||
              _data_type == GC_VEC_INTEGER ||
              _data_type == GC_VEC_REAL    ||
              _data_type == GC_VEC_COMPLEX ||
              _data_type == GC_MAT_REAL ) promote_to_mat_complex() ;
    GC_ASSERT( GC_MAT_COMPLEX == _data_type,
               "get_complex_at( " << i << ", " << j << " ) bad data type" <<
               "\nexpect: " << typeName[GC_MAT_COMPLEX] <<
               "\nbut data stored is of type: " << typeName[_data_type] ) ;
    return (*_data.m_c)(i,j) ;
  }

  complex_type const &
  GenericContainer::get_complex_at( unsigned i, unsigned j ) const  {
    GC_ASSERT( GC_MAT_COMPLEX == _data_type,
               "get_complex_at( " << i << ", " << j << " ) bad data type" <<
               "\nexpect: " << typeName[GC_MAT_COMPLEX] <<
               "\nbut data stored is of type: " << typeName[_data_type] ) ;
    return (*_data.m_c)(i,j) ;
  }

  string_type &
  GenericContainer::get_string_at( unsigned i ) {
    if ( _data_type == GC_NOTYPE ) set_vec_string() ;
    if ( _data_type == GC_VEC_STRING ) {
      CHECK_RESIZE(_data.v_s,i) ;
      return (*_data.v_s)[i] ;
    } else {
      promote_to_vector() ;
      return (*this)[i].set_string("") ;
    }
  }

  string_type const &
  GenericContainer::get_string_at( unsigned i ) const {
    GC_ASSERT( GC_VEC_STRING == _data_type,
               "get_string( " << i << " ) bad data type" <<
               "\nexpect: " << typeName[GC_VEC_STRING] <<
               "\nbut data stored is of type: " << typeName[_data_type] ) ;
    GC_ASSERT( i < _data.v_s->size(), "get_string_at( " << i << " ) const, out of range" ) ;
    return (*_data.v_s)[i] ;
  }

  /*
  //   _        __
  //  (_)_ __  / _| ___
  //  | | '_ \| |_ / _ \
  //  | | | | |  _| (_) |
  //  |_|_| |_|_|  \___/
  */

  //! Print to stream the kind of data stored
  GenericContainer const &
  GenericContainer::info( std::basic_ostream<char> & stream ) const {
    switch ( _data_type ) {
      case GC_NOTYPE:
        stream << "GenericContainer: No data stored\n" ;
        break ;
      case GC_POINTER:
        stream << "Generic pointer: " << _data.p << '\n' ;
        break ;
      case GC_BOOL:
        stream << "Boolean: " << (_data.b?"true":"false") << '\n' ;
        break ;
      case GC_INTEGER:
        stream << "Integer: " << _data.i << '\n' ;
        break ;
      case GC_LONG:
        stream << "Long: " << _data.l << '\n' ;
        break ;
      case GC_REAL:
        stream << "Floating Point: " << _data.r << '\n' ;
        break ;
      case GC_COMPLEX:
        stream << "Complex Floating Point: [" << _data.c->real() << ", " << _data.c->imag() << " ]\n" ;
        break ;
      case GC_STRING:
        stream << "String: " << *_data.s << '\n' ;
        break ;
      case GC_VEC_POINTER:
        stream << "Vector of generic pointer of size " << _data.v_p->size() << '\n' ;
        break ;
      case GC_VEC_BOOL:
        stream << "Vector of boolean of size " << _data.v_b->size() << '\n' ;
        break ;
      case GC_VEC_INTEGER:
        stream << "Vector of integer of size " << _data.v_i->size() << '\n' ;
        break ;
      case GC_VEC_REAL:
        stream << "Vector of floating point number of size " << _data.v_r->size() << '\n' ;
        break ;
      case GC_VEC_COMPLEX:
        stream << "Vector of complex floating point number of size " << _data.v_c->size() << '\n' ;
        break ;
      case GC_VEC_STRING:
        stream << "Vector of string of size " << _data.v_s->size() << '\n' ;
        break ;
      case GC_MAT_REAL:
        stream << "Matrix of floating point number of size " << _data.m_r->numRows() << " x " << _data.m_c->numCols() << '\n' ;
        break ;
      case GC_MAT_COMPLEX:
        stream << "Matrix of complex floating point number of size " << _data.m_c->numRows() << " x " << _data.m_c->numCols() << '\n' ;
        break ;
      case GC_VECTOR:
        stream << "Vector of generic data type of size " << _data.v->size() << '\n' ;
        break ;
      case GC_MAP:
        stream << "Map\n" ;
        break ;
      default:
        stream << "Type N. " << _data_type << " not recognized\n" ;
        break ;
    }
    return *this ;
  }

  /*
  //                              _               __ __
  //    ___  _ __   ___ _ __ __ _| |_ ___  _ __  | _|_ |
  //   / _ \| '_ \ / _ \ '__/ _` | __/ _ \| '__| | | | |
  //  | (_) | |_) |  __/ | | (_| | || (_) | |    | | | |
  //   \___/| .__/ \___|_|  \__,_|\__\___/|_|    | | | |
  //        |_|                                  |__|__|
  */

  GenericContainer &
  GenericContainer::operator [] ( unsigned i ) {
    switch ( ck( GC_VECTOR ) ) {
      case 0: break ; // data present
      default: set_vector() ; // data must be allocated ;
    }
    CHECK_RESIZE(_data.v,i) ;
    return (*_data.v)[i] ;
  }

  GenericContainer const &
  GenericContainer::operator [] ( unsigned i ) const {
    GC_ASSERT( GC_VECTOR == _data_type,
               "operator [] integer argument = " << i <<
               "\nexpect: " << typeName[GC_VECTOR] <<
               "\nbut data stored is of type: " << typeName[_data_type] ) ;
    GC_ASSERT( i < _data.v->size(), "operator [] const, index " << i << " out of range" ) ;
    return (*_data.v)[i] ;
  }

  /*
  //                              _                ____
  //    ___  _ __   ___ _ __ __ _| |_ ___  _ __   / /\ \
  //   / _ \| '_ \ / _ \ '__/ _` | __/ _ \| '__| | |  | |
  //  | (_) | |_) |  __/ | | (_| | || (_) | |    | |  | |
  //   \___/| .__/ \___|_|  \__,_|\__\___/|_|    | |  | |
  //        |_|                                   \_\/_/
  //
  */

  GenericContainer &
  GenericContainer::operator () ( unsigned i ) {
    GC_ASSERT( GC_VECTOR == _data_type,
               "operator () integer argument = " << i <<
               " bad data type\nexpect: " << typeName[GC_VECTOR] <<
               "\nbut data stored is of type: " << typeName[_data_type] ) ;
    return (*_data.v)[i] ;
  }

  GenericContainer const &
  GenericContainer::operator () ( unsigned i ) const {
    GC_ASSERT( GC_VECTOR == _data_type,
               "operator () integer argument = " << i <<
               " bad data type\nexpect: " << typeName[GC_VECTOR] <<
               "\nbut data stored is of type: " << typeName[_data_type] ) ;
    GC_ASSERT( i < _data.v->size(), "operator () const, index " << i << " out of range" ) ;
    return (*_data.v)[i] ;
  }

  /*
  //                              _               __ __
  //    ___  _ __   ___ _ __ __ _| |_ ___  _ __  | _|_ |
  //   / _ \| '_ \ / _ \ '__/ _` | __/ _ \| '__| | | | |
  //  | (_) | |_) |  __/ | | (_| | || (_) | |    | | | |
  //   \___/| .__/ \___|_|  \__,_|\__\___/|_|    | | | |
  //        |_|                                  |__|__|
  */

  GenericContainer &
  GenericContainer::operator [] ( std::string const & s ) {
    if ( ck( GC_MAP ) != 0 ) set_map() ; // if not data present allocate!
    return (*_data.m)[s] ;
  }

  GenericContainer const &
  GenericContainer::operator [] ( std::string const & s ) const {
    GC_ASSERT( GC_MAP == _data_type,
               "operator [] string argument ``" << s << "''"
               "\nexpect: " << typeName[GC_MAP] <<
               "\nbut data stored is of type: " << typeName[_data_type] ) ;
    return (*_data.m)[s] ;
  }

  GenericContainer &
  GenericContainer::operator () ( std::string const & s ) {
    GC_ASSERT( GC_MAP == _data_type,
               "operator () string argument = ``" << s <<
               "'' bad data type\nexpect: " << typeName[GC_MAP] <<
               "\nbut data stored is of type: " << typeName[_data_type] ) ;
    map_type::iterator iv = (*_data.m) . find(s) ;
    GC_ASSERT( iv != (*_data.m) . end(), "operator(): Cannot find key '" << s << "'!" ) ;
    return iv -> second ;
  }

  GenericContainer const &
  GenericContainer::operator () ( std::string const & s ) const {
    GC_ASSERT( GC_MAP == _data_type,
               "operator () string argument = ``" << s <<
               "'' bad data type\nexpect: " << typeName[GC_MAP] <<
               "\nbut data stored is of type: " << typeName[_data_type] ) ;
    map_type::const_iterator iv = (*_data.m) . find(s) ;
    GC_ASSERT( iv != (*_data.m) . end(), "operator(): Cannot find key '" << s << "'!" ) ;
    return iv -> second ;
  }

  /*
  //   ____                            _
  //  |  _ \ _ __ ___  _ __ ___   ___ | |_ ___
  //  | |_) | '__/ _ \| '_ ` _ \ / _ \| __/ _ \
  //  |  __/| | | (_) | | | | | | (_) | ||  __/
  //  |_|   |_|  \___/|_| |_| |_|\___/ \__\___|
  */

  GenericContainer const &
  GenericContainer::promote_to_int() {
    switch (_data_type) {
      case GC_NOTYPE:
        set_int(0) ;
        break ;
      case GC_BOOL:
        set_int(_data.b?1:0) ;
        break ;
      case GC_INTEGER:
        break ;
      default:
        GC_ASSERT( false, ":promote_to_int() cannot promote " << get_type_name() << " to int") ;
        break ;
    }
    return *this ;
  }

  GenericContainer const &
  GenericContainer::promote_to_long() {
    switch (_data_type) {
      case GC_NOTYPE:
        set_long(0) ;
        break ;
      case GC_BOOL:
        set_long(_data.b?1:0) ;
        break ;
      case GC_INTEGER:
        set_long(_data.i) ;
        break ;
      case GC_LONG:
        break ;
      default:
        GC_ASSERT( false, ":promote_to_long() cannot promote " << get_type_name() << " to long") ;
        break ;
    }
    return *this ;
  }

  GenericContainer const &
  GenericContainer::promote_to_real() {
    switch (_data_type) {
      case GC_NOTYPE:
        set_real(0) ;
        break ;
      case GC_BOOL:
        set_real(_data.b?1:0) ;
        break ;
      case GC_INTEGER:
        set_real(_data.i) ;
        break ;
      case GC_LONG:
        set_real(_data.l) ;
        break ;
      case GC_REAL:
        break ;
      default:
        GC_ASSERT( false, ":promote_to_real() cannot promote " << get_type_name() << " to real") ;
        break ;
    }
    return *this ;
  }

  GenericContainer const &
  GenericContainer::promote_to_vec_int() {
    switch (_data_type) {
      case GC_NOTYPE:
      { set_vec_int(1) ; get_int_at(0) = 0 ; }
        break ;
      case GC_BOOL:
      { int_type tmp = _data.b?1:0 ; set_vec_int(1) ; get_int_at(0) = tmp ; }
        break ;
      case GC_INTEGER:
      { int_type tmp = _data.i ; set_vec_int(1) ; get_int_at(0) = tmp ; }
        break ;
      case GC_VEC_BOOL:
        { vec_bool_type * v_b = _data.v_b ;
          _data_type = GC_NOTYPE ;
          set_vec_int(unsigned(v_b->size())) ;
          for ( unsigned i = 0 ; i < v_b->size() ; ++i ) (*_data.v_i)[i] = (*v_b)[i] ;
          delete v_b ;
        }
        break ;
      case GC_VEC_INTEGER:
        break ;
      default:
        GC_ASSERT( false, ":promote_to_vec_int() cannot promote " << get_type_name() << " to vec_int_type") ;
        break ;
    }
    return *this ;

  }

  //! If data contains vector of booleans or integer it is promoted to a vector of real.
  GenericContainer const &
  GenericContainer::promote_to_vec_real() {
    switch (_data_type) {
      case GC_NOTYPE:
        { set_vec_real(1) ; get_real_at(0) = 0 ; }
        break ;
      case GC_BOOL:
        { real_type tmp = _data.b?1:0 ; set_vec_real(1) ; get_real_at(0) = tmp ; }
        break ;
      case GC_INTEGER:
        { real_type tmp = _data.i ; set_vec_real(1) ; get_real_at(0) = tmp ; }
        break ;
      case GC_LONG:
        { real_type tmp = _data.l ; set_vec_real(1) ; get_real_at(0) = tmp ; }
        break ;
      case GC_REAL:
        { real_type tmp = _data.r ; set_vec_real(1) ; get_real_at(0) = tmp ; }
        break ;
      case GC_VEC_BOOL:
        { vec_bool_type * v_b = _data.v_b ;
          _data_type = GC_NOTYPE ;
          set_vec_real(unsigned(v_b->size())) ;
          for ( unsigned i = 0 ; i < v_b->size() ; ++i ) (*_data.v_r)[i] = (*v_b)[i] ;
          delete v_b ;
        }
        break ;
      case GC_VEC_INTEGER:
        { vec_int_type * v_i = _data.v_i ;
          _data_type = GC_NOTYPE ;
          set_vec_real(unsigned(v_i->size())) ;
          for ( unsigned i = 0 ; i < v_i->size() ; ++i ) (*_data.v_r)[i] = (*v_i)[i] ;
          delete v_i ;
        }
        break ;
      case GC_VEC_REAL:
        break ;
      default:
        GC_ASSERT( false, ":promote_to_vec_real() cannot promote " << get_type_name() << " vec_real_type") ;
        break ;
    }
    return *this ;
  }

  //! If data contains vector of booleans or integer it is promoted to a vector of real.
  GenericContainer const &
  GenericContainer::promote_to_vec_complex() {
    switch (_data_type) {
      case GC_NOTYPE:
        { set_vec_complex(1) ; get_complex_at(0) = 0 ; }
        break ;
      case GC_BOOL:
        { real_type tmp = _data.b?1:0 ; set_vec_complex(1) ; get_complex_at(0) = tmp ; }
        break ;
      case GC_INTEGER:
        { real_type tmp = _data.i ; set_vec_complex(1) ; get_complex_at(0) = tmp ; }
        break ;
      case GC_LONG:
        { real_type tmp = _data.l ; set_vec_complex(1) ; get_complex_at(0) = tmp ; }
        break ;
      case GC_REAL:
        { real_type tmp = _data.r ; set_vec_complex(1) ; get_complex_at(0) = tmp ; }
        break ;
      case GC_COMPLEX:
        { complex_type tmp = *_data.c ; set_vec_complex(1) ; get_complex_at(0) = tmp ; }
        break ;
      case GC_VEC_BOOL:
        { vec_bool_type * v_b = _data.v_b ;
          _data_type = GC_NOTYPE ;
          set_vec_complex(unsigned(v_b->size())) ;
          for ( unsigned i = 0 ; i < v_b->size() ; ++i ) (*_data.v_c)[i] = (*v_b)[i] ;
          delete v_b ;
        }
        break ;
      case GC_VEC_INTEGER:
        { vec_int_type * v_i = _data.v_i ;
          _data_type = GC_NOTYPE ;
          set_vec_complex(unsigned(v_i->size())) ;
          for ( unsigned i = 0 ; i < v_i->size() ; ++i ) (*_data.v_c)[i] = (*v_i)[i] ;
          delete v_i ;
        }
        break ;
      case GC_VEC_REAL:
        { vec_real_type * v_r = _data.v_r ;
          _data_type = GC_NOTYPE ;
          set_vec_complex(unsigned(v_r->size())) ;
          for ( unsigned i = 0 ; i < v_r->size() ; ++i ) (*_data.v_c)[i] = (*v_r)[i] ;
          delete v_r ;
        }
        break ;
      case GC_VEC_COMPLEX:
        break ;
      default:
        GC_ASSERT( false, ":promote_to_vec_real() cannot promote " << get_type_name() << " to vec_complex_type") ;
        break ;
    }
    return *this ;
  }

  //! If data contains vector of booleans, integer or real it is promoted to a vector of real.
  GenericContainer const &
  GenericContainer::promote_to_mat_real() {
    switch (_data_type) {
      case GC_NOTYPE:
        { set_mat_real(1,1) ; get_real_at(0,0) = 0 ; }
        break ;
      case GC_BOOL:
        { real_type tmp = _data.b?1:0 ; set_mat_real(1,1) ; get_real_at(0,0) = tmp ; }
        break ;
      case GC_INTEGER:
        { real_type tmp = _data.i ; set_mat_real(1,1) ; get_real_at(0,0) = tmp ; }
        break ;
      case GC_REAL:
        { real_type tmp = _data.r ; set_mat_real(1,1) ; get_real_at(0,0) = tmp ; }
        break ;
      case GC_VEC_BOOL:
        { vec_bool_type * v_b = _data.v_b ;
          _data_type = GC_NOTYPE ;
          set_mat_real(unsigned(v_b->size()),1) ;
          for ( unsigned i = 0 ; i < v_b->size() ; ++i ) (*_data.m_r)(i,0) = (*v_b)[i] ;
          delete v_b ;
        }
        break ;
      case GC_VEC_INTEGER:
        { vec_int_type * v_i = _data.v_i ;
          _data_type = GC_NOTYPE ;
          set_mat_real(unsigned(v_i->size()),1) ;
          for ( unsigned i = 0 ; i < v_i->size() ; ++i ) (*_data.m_r)(i,0) = (*v_i)[i] ;
          delete v_i ;
        }
        break ;
      case GC_VEC_REAL:
        { vec_real_type * v_r = _data.v_r ;
          _data_type = GC_NOTYPE ;
          set_mat_real(unsigned(v_r->size()),1) ;
          for ( unsigned i = 0 ; i < v_r->size() ; ++i ) (*_data.m_r)(i,0) = (*v_r)[i] ;
          delete v_r ;
        }
        break ;
      case GC_MAT_REAL:
        break ;
      default:
        GC_ASSERT( false, ":promote_to_mat_real() cannot promote " << get_type_name() << " to mat_real_type") ;
        break ;
    }
    return *this ;
  }

  //! If data contains vector of booleans, integer or real it is promoted to a vector of real.
  GenericContainer const &
  GenericContainer::promote_to_mat_complex() {
    switch (_data_type) {
      case GC_NOTYPE:
        { set_mat_complex(1,1) ; get_complex_at(0,0) = 0 ; }
        break ;
      case GC_BOOL:
        { real_type tmp = _data.b?1:0 ; set_mat_complex(1,1) ; get_complex_at(0,0) = tmp ; }
        break ;
      case GC_INTEGER:
        { real_type tmp = _data.i ; set_mat_complex(1,1) ; get_complex_at(0,0) = tmp ; }
        break ;
      case GC_LONG:
        { real_type tmp = _data.l ; set_mat_complex(1,1) ; get_complex_at(0,0) = tmp ; }
        break ;
      case GC_REAL:
        { real_type tmp = _data.r ; set_mat_complex(1,1) ; get_complex_at(0,0) = tmp ; }
        break ;
      case GC_VEC_BOOL:
        { vec_bool_type * v_b = _data.v_b ;
          _data_type = GC_NOTYPE ;
          set_mat_complex(unsigned(v_b->size()),1) ;
          for ( unsigned i = 0 ; i < v_b->size() ; ++i ) (*_data.m_r)(i,0) = (*v_b)[i] ;
          delete v_b ;
        }
        break ;
      case GC_VEC_INTEGER:
        { vec_int_type * v_i = _data.v_i ;
          _data_type = GC_NOTYPE ;
          set_mat_complex(unsigned(v_i->size()),1) ;
          for ( unsigned i = 0 ; i < v_i->size() ; ++i ) (*_data.m_r)(i,0) = (*v_i)[i] ;
          delete v_i ;
        }
        break ;
      case GC_VEC_REAL:
        { vec_real_type * v_r = _data.v_r ;
          _data_type = GC_NOTYPE ;
          set_mat_complex(unsigned(v_r->size()),1) ;
          for ( unsigned i = 0 ; i < v_r->size() ; ++i ) (*_data.m_r)(i,0) = (*v_r)[i] ;
          delete v_r ;
        }
        break ;
      case GC_MAT_REAL:
        { mat_real_type * m_r = _data.m_r ;
          _data_type = GC_NOTYPE ;
          set_mat_complex(m_r->numRows(),m_r->numCols()) ;
          for ( unsigned i = 0 ; i < m_r->numRows() ; ++i )
            for ( unsigned j = 0 ; j < m_r->numCols() ; ++j )
              (*_data.m_c)(i,j) = (*m_r)(i,j) ;
          delete m_r ;
        }
        break ;
      case GC_MAT_COMPLEX:
        break ;
      default:
        GC_ASSERT( false, ":promote_to_mat_real() cannot promote " << get_type_name() << " to mat_complex_type") ;
        break ;
    }
    return *this ;
  }


  //! If data contains vector of someting it is promoted to a vector of `GenericContainer`.
  GenericContainer const &
  GenericContainer::promote_to_vector() {
    switch (_data_type) {
      case GC_NOTYPE:
        { set_vector(1) ; (*this)[0].initialize() ; } // set data to no type
        break ;
      case GC_POINTER:
        { set_vector(1) ; (*this)[0] = _data.p ; }
        break ;
      case GC_BOOL:
        { set_vector(1) ; (*this)[0] = _data.b ; }
        break ;
      case GC_INTEGER:
        { set_vector(1) ; (*this)[0] = _data.i ; }
        break ;
      case GC_LONG:
        { set_vector(1) ; (*this)[0] = _data.l ; }
        break ;
      case GC_REAL:
        { set_vector(1) ; (*this)[0] = _data.r ; }
        break ;
      case GC_COMPLEX:
        { set_vector(1) ; (*this)[0] = *_data.c ; }
        break ;
      case GC_STRING:
        { set_vector(1) ; (*this)[0] = *_data.s ; }
        break ;
      case GC_VEC_POINTER:
        { vec_pointer_type * v_p = _data.v_p ;
          _data_type = GC_NOTYPE ;
          set_vector(unsigned(v_p->size())) ;
          for ( unsigned i = 0 ; i < v_p->size() ; ++i ) (*_data.v)[i] = (*v_p)[i] ;
          delete v_p ;
        }
        break ;
      case GC_VEC_BOOL:
        { vec_bool_type * v_b = _data.v_b ;
          _data_type = GC_NOTYPE ;
          set_vector(unsigned(v_b->size())) ;
          for ( unsigned i = 0 ; i < v_b->size() ; ++i ) (*_data.v)[i] = (*v_b)[i] ;
          delete v_b ;
        }
        break ;
      case GC_VEC_INTEGER:
        { vec_int_type * v_i = _data.v_i ;
          _data_type = GC_NOTYPE ;
          set_vector(unsigned(v_i->size())) ;
          for ( unsigned i = 0 ; i < v_i->size() ; ++i ) (*_data.v)[i] = (*v_i)[i] ;
          delete v_i ;
        }
        break ;
      case GC_VEC_REAL:
        { vec_real_type * v_r = _data.v_r ;
          _data_type = GC_NOTYPE ;
          set_vector(unsigned(v_r->size())) ;
          for ( unsigned i = 0 ; i < v_r->size() ; ++i ) (*_data.v)[i] = (*v_r)[i] ;
          delete v_r ;
        }
        break ;
      case GC_VEC_COMPLEX:
        { vec_complex_type * v_c = _data.v_c ;
          _data_type = GC_NOTYPE ;
          set_vector(unsigned(v_c->size())) ;
          for ( unsigned i = 0 ; i < v_c->size() ; ++i ) (*_data.v)[i] = (*v_c)[i] ;
          delete v_c ;
        }
        break ;
      case GC_VEC_STRING:
        { vec_string_type * v_s = _data.v_s ;
          _data_type = GC_NOTYPE ;
          set_vector(unsigned(v_s->size())) ;
          for ( unsigned i = 0 ; i < v_s->size() ; ++i ) (*_data.v)[i] = (*v_s)[i] ;
          delete v_s ;
        }
        break ;
      case GC_VECTOR:
        break ;
      default:
        GC_ASSERT( false, ":promote_to_vector() cannot promote " << get_type_name() << " to vector_type") ;
        break ;
    }
    return *this ;
  }

  /*
  //              _       _
  //   _ __  _ __(_)_ __ | |_
  //  | '_ \| '__| | '_ \| __|
  //  | |_) | |  | | | | | |_
  //  | .__/|_|  |_|_| |_|\__|
  //  |_|
  */

  void
  GenericContainer::print( std::basic_ostream<char> & stream,
                           std::string const        & prefix,
                           std::string const        & indent ) const {

    switch (_data_type) {

      case GC_NOTYPE:
        stream << prefix << "Empty!\n" ;
        break ;
      case GC_POINTER:
        stream << prefix << this -> get_pvoid() << '\n' ;
        break ;
      case GC_BOOL:
        stream << prefix << (this -> get_bool()?"true":"false") << '\n' ;
        break ;
      case GC_INTEGER:
        stream << prefix << this -> get_int() << '\n' ;
        break ;
      case GC_LONG:
        stream << prefix << this -> get_long() << '\n' ;
        break ;
      case GC_REAL:
        stream << prefix << this -> get_real() << '\n' ;
        break ;
      case GC_COMPLEX:
        stream << prefix << "( " << this -> get_complex().real() << ", " << this -> get_complex().imag() << " )\n" ;
        break ;
      case GC_STRING:
        stream << prefix << "\"" << this -> get_string() << "\"\n" ;
        break ;
      case GC_VEC_POINTER:
        { vec_pointer_type const & v = this -> get_vec_pointer() ;
          for ( vec_pointer_type::size_type i = 0 ; i < v.size() ; ++i )
            stream << prefix << "vec_pointer(" << i << "): " << (unsigned long)v[i] << '\n' ;
        }
        break ;
      case GC_VEC_BOOL:
        { vec_bool_type const & v = this -> get_vec_bool() ;
          stream << prefix << v << '\n' ; }
        break ;
      case GC_VEC_INTEGER:
        { vec_int_type const & v = this -> get_vec_int() ;
          stream << prefix << v << '\n' ; }
        break ;
      case GC_VEC_REAL:
        { vec_real_type const & v = this -> get_vec_real() ;
          stream << prefix << v << '\n' ; }
        break ;
      case GC_VEC_COMPLEX:
        { vec_complex_type const & v = this -> get_vec_complex() ;
          stream << prefix << v << '\n' ; }
        break ;
      case GC_MAT_REAL:
        { mat_real_type const & m = this -> get_mat_real() ;
          stream << m ; }
        break ;
      case GC_MAT_COMPLEX:
        { mat_complex_type const & m = this -> get_mat_complex() ;
          stream << m ; }
        break ;
      case GC_VEC_STRING:
        { vec_string_type const & v = this -> get_vec_string() ;
          stream << '\n';
          for ( vec_string_type::size_type i = 0 ; i < v.size() ; ++i )
            stream << prefix+indent << i << ": \"" << v[i] << "\"\n" ;
        }
        break ;

      case GC_VECTOR:
        { vector_type const & v = this -> get_vector() ;
          for ( vector_type::size_type i = 0 ; i < v.size() ; ++i ) {
            GenericContainer const & vi = v[i] ;
            if ( vi.simple_data() ||
                 ( vi.simple_vec_data() && vi.get_num_elements() <= 10 ) ) {
              stream << prefix << i << ": " ;
              vi.print(stream,"") ;
            } else {
              stream << prefix << i << ":\n" ;
              vi.print(stream,prefix+indent) ;
            }
          }
        }
        break ;
      case GC_MAP:
        { map_type const & m = this -> get_map() ;
          for ( map_type::const_iterator im = m.begin() ; im != m.end() ; ++im ) {
            // check formatting using pcre
            #ifdef GENERIC_CONTAINER_NO_PCRE
            if ( im->second.simple_data() ||
                 ( im->second.simple_vec_data() && im->second.get_num_elements() <= 10 ) ) {
              stream << prefix << im->first << ": " ;
              im->second.print(stream,"") ;
            } else {
              stream << prefix << im->first << ":\n" ;
              im->second.print(stream,prefix+indent) ;
            }
            #else
            // num+"@"+"underline character"
            // Try to find the regex in aLineToMatch, and report results.
            int imatch[30];
            int pcreExecRet = pcre_for_GC.exec( im->first.c_str(),
                                                int(im->first.length()),
                                                imatch ) ;
            if ( pcreExecRet == 4 ) {
              // extract match
              int m1 = imatch[3]-imatch[2] ; // # or ##
              int m2 = imatch[5]-imatch[4] ; // -,= etc
              int m3 = imatch[7]-imatch[6] ; // # or ##
              std::string header = im->first.substr((std::string::size_type)imatch[6],
                                                    (std::string::size_type)imatch[7]) ; // header
              // found formatting
              if ( im->second.simple_data() ) {
                stream << prefix << header << ": " ;
                im->second.print(stream,"") ;
              } else {
                if ( m1 > 1 ) stream << '\n' ; // double ## --> add nel line
                stream << prefix << header ;
                if ( m2 > 0 ) {
                  stream << '\n' << prefix ;
                  char fmt = im->first[(std::string::size_type)imatch[4]] ; // underline char
                  while ( m3-- > 0 ) stream << fmt ; // underline header
                } else {
                  stream << ':' ;
                }
                stream << '\n' ;
                im->second.print(stream,prefix+indent) ;
              }
            } else {
              std::string header = pcreExecRet == 3 ?
                                   im->first.substr((std::string::size_type)imatch[4],
                                                    (std::string::size_type)imatch[5]) :
                                   im->first ;
              if ( im->second.simple_data() ) {
                stream << prefix << header << ": " ;
                im->second.print(stream,"") ;
              } else {
                stream << prefix << header << ":\n" ;
                im->second.print(stream,prefix+indent) ;
              }
            }
            #endif
          }
        }
        break ;

      default:
        GC_ASSERT(false,"Error, print(...) unknown type!\n") ;
        break ;
    }
  }

  /*
  //   _                                 _
  //  | |_ ___     _   _  __ _ _ __ ___ | |
  //  | __/ _ \   | | | |/ _` | '_ ` _ \| |
  //  | || (_) |  | |_| | (_| | | | | | | |
  //   \__\___/____\__, |\__,_|_| |_| |_|_|
  //         |_____|___/
  */

  void
  GenericContainer::to_yaml( std::basic_ostream<char> & stream, std::string const & prefix ) const {
    switch (_data_type) {
        
      case GC_NOTYPE:
        stream << "Empty!\n" ;
        break ;
      case GC_BOOL:
        stream << (this -> get_bool()?"true":"false") << '\n' ;
        break ;
      case GC_INTEGER:
        stream << this -> get_int() << '\n' ;
        break ;
      case GC_LONG:
        stream << this -> get_long() << '\n' ;
        break ;
      case GC_REAL:
        stream << this -> get_real() << '\n' ;
        break ;
      case GC_STRING:
        stream << "'" << this -> get_string() << "'\n" ;
        break ;
        
      case GC_VEC_BOOL:
        { vec_bool_type const & v = this -> get_vec_bool() ;
          stream << "[ " << (v[0]?"true":"false") ;
          for ( vec_bool_type::size_type i = 1 ; i < v.size() ; ++i )
            stream << ", " << (v[i]?"true":"false") ;
          stream << " ]\n" ;
        }
        break ;
      case GC_VEC_INTEGER:
        { vec_int_type const & v = this -> get_vec_int() ;
          stream << "[ " << v[0] ;
          for ( vec_int_type::size_type i = 1 ; i < v.size() ; ++i )
            stream << ", " << v[i] ;
          stream << " ]\n" ;
        }
        break ;
      case GC_VEC_REAL:
        { vec_real_type const & v = this -> get_vec_real() ;
          stream << "[ " << v[0] ;
          for ( vec_real_type::size_type i = 1 ; i < v.size() ; ++i )
            stream << ", " << v[i] ;
          stream << " ]\n" ;
        }
        break ;
      case GC_MAT_REAL:
        { /* DA FARE */ }
        break ;
      case GC_VEC_STRING:
        { vec_string_type const & v = this -> get_vec_string() ;
          stream << "[ '" << v[0] << "'" ;
          for ( vec_string_type::size_type i = 1 ; i < v.size() ; ++i )
            stream << ", '" << v[i] << "'" ;
          stream << " ]\n" ;
        }
        break ;

      case GC_VECTOR:
        { vector_type const & v = this -> get_vector() ;
          stream << '\n' ;
          for ( vector_type::size_type i = 0 ; i < v.size() ; ++i ) {
            stream << prefix << "- " ;
            v[i].to_yaml(stream,prefix+"  ") ;
          }
        }
        break ;
      case GC_MAP:
        { map_type const & m = this -> get_map() ;
          stream << '\n' ;
          for ( map_type::const_iterator im = m.begin() ; im != m.end() ; ++im ) {
            stream << prefix << im->first << ": " ;
            im->second.to_yaml(stream,prefix+"  ") ;
          }
        }
        break ;
        
      default:
        GC_ASSERT( false, "Error, print(...) unknown type!\n" ) ;
        break ;
    }
  }

  void
  GenericContainer::exception( char const msg[] ) {
    throw std::runtime_error(msg) ;
  }
}

//
// eof: GenericContainer.cc
//
