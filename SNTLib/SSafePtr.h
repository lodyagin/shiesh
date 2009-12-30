#ifndef __SSAFEPTR_H
#define __SSAFEPTR_H

#include <SCommon.h>


// Safe pointer to base type

template<class T>
class SSafePtr
{
public:

  explicit SSafePtr( T * ap = 0 ) : p(ap) {}
  ~SSafePtr() { if ( p ) delete p; }

  SSafePtr & operator = ( T * ap ) { if ( p ) delete p; p = ap; return *this; }
  T & operator * () const { return *p; }
  T * operator -> () const { return p; }
  T * get() const { return p; }
  bool operator ! () const { return get() == 0; }
  operator bool () const { return get() != 0; }

  void free() { if ( p ) delete p; p = 0; }
  T * release() { T * t = p; p = 0; return t; }

  friend bool operator == ( const SSafePtr<T> &, const SSafePtr<T> & );
  friend bool operator < ( const SSafePtr<T> &, const SSafePtr<T> & );

protected:

  SSafePtr( const SSafePtr<T> & );        // prohibited
  void operator = ( const SSafePtr<T> & );

  T * p;

};


// Safe pointer to class

template<class T>
class SClassPtr : public SSafePtr<T>
{
public:

  typedef SSafePtr<T> Parent;

  explicit SClassPtr( T * ap = 0, bool o = true ) : Parent(ap), own(o) {}
  ~SClassPtr() { free(); } 

  SClassPtr & operator = ( T * ap ) { free(); p = ap; return *this; }

  void free() { if ( p && own ) delete p; p = 0; }

  bool ownsElements() const { return own; }
  bool ownsElements( bool o ) { bool old = own; own = o; return old; }

private:

  SClassPtr( const SClassPtr<T> & );        // prohibited
  void operator = ( const SClassPtr<T> & );


  bool own;

};


// Safe pointer to array (vector)

template<class T>
class SVSafePtr
{
public:

  explicit SVSafePtr( T * ap = 0 ) : p(ap) {}
  ~SVSafePtr() { if ( p ) delete [] p; }

  SVSafePtr & operator = ( T * ap ) { if ( p ) delete [] p; p = ap; return *this; }
  T & operator * () const { return *p; }
  T & operator [] ( size_t i ) const { return p[i]; }
  T * operator -> () const { return p; }
  T * operator + ( size_t i ) { return p + i; }
  T * get() const { return p; }
  bool operator ! () { return get() == 0; }
  operator bool () { return get() != 0; }

  T * release() { T * t = p; p = 0; return t; }

  friend bool operator == ( const SVSafePtr<T> &, const SVSafePtr<T> & );
  friend bool operator < ( const SVSafePtr<T> &, const SVSafePtr<T> & );

private:

  SVSafePtr( const SVSafePtr<T> & );        // prohibited
  void operator = ( const SVSafePtr<T> & );

  T * p;

};


template<class T>
T * ptr2ptr( const SSafePtr<T> & sp )
{
  return sp.get();
}


template<class T>
T * ptr2ptr( const SVSafePtr<T> & sp )
{
  return sp.get();
}


template<class T>
inline bool operator == ( const SSafePtr<T> & x, const SSafePtr<T> & y )
{
  return x.p == y.p;
}

template<class T>
inline bool operator < ( const SSafePtr<T> & x, const SSafePtr<T> & y )
{
  return x.p < y.p;
}


template<class T>
inline bool operator == ( const SVSafePtr<T> & x, const SVSafePtr<T> & y )
{
  return x.p == y.p;
}

template<class T>
inline bool operator < ( const SVSafePtr<T> & x, const SVSafePtr<T> & y )
{
  return x.p < y.p;
}


#endif  // __SSAFEPTR_H
