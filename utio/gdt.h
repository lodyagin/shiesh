// This file is part of the utio library, a terminal I/O library.
//
// Copyright (C) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.
//
// gdt.h
//

#ifndef GDT_H_10D99A7949ED8990743247ED3E6897DC
#define GDT_H_10D99A7949ED8990743247ED3E6897DC

#include "ticonst.h"

namespace utio {
/// Contains geometric primitive objects.
namespace gdt {

typedef int16_t			coord_t;	///< A geometric coordinate.
typedef uint16_t		dim_t;		///< A dimension.

struct Size2d
{
  dim_t w;
  dim_t h;
};

struct Point2d
{
  Point2d () : x (0), y (0) {}

  Point2d (coord_t _x, coord_t _y) 
    : x (_x), y (_y) 
  {}

  coord_t x;
  coord_t y;

  Point2d& operator += (const Size2d& p2)
  {
    x += p2.w;
    y += p2.h;
    return *this;
  }

  Point2d& operator -= (const Size2d& p2)
  {
    x -= p2.w;
    y -= p2.h;
    return *this;
  }

  Point2d& operator += (const Point2d& p2)
  {
    x += p2.x;
    y += p2.y;
    return *this;
  }

  Point2d& operator -= (const Point2d& p2)
  {
    x -= p2.x;
    y -= p2.y;
    return *this;
  }
};

inline Size2d operator - (const Point2d& p1, const Point2d& p2)
{
  Size2d size = { p1.x - p2.x, p1.y - p2.y };
  return size;
}




/// Represents a geometric rectangle.
class Rect {
public:
    Point2d a, b;

    inline		Rect (const Rect& r)
			    : a(r.a), b(r.b) {}
    inline		Rect (coord_t x = 0, coord_t y = 0, dim_t w = 0, dim_t h = 0)
			    { a.x = x; a.y = y;
			      b.x = x + w; b.y = y + h; }
    inline		Rect (const Point2d& tl, const Point2d& br)
			    { a = tl; b = br; }
    inline		Rect (const Point2d& tl, const Size2d& wh)
			    { a = b = tl; b += wh; }
    inline size_t	Width (void) const		{ return (b.x - a.x); }
    inline size_t	Height (void) const		{ return (b.y - a.y); }
    inline bool		Empty (void) const		{ return (!Width() || !Height()); }
    inline Size2d	Size (void) const		{ return (b - a); }
    inline const Rect&	operator+= (const Point2d& d)	{ a += d; b += d; return (*this); }
    inline const Rect&	operator-= (const Point2d& d)	{ a -= d; b -= d; return (*this); }
    inline const Rect&	operator+= (const Size2d& d)	{ a += d; b += d; return (*this); }
    inline const Rect&	operator-= (const Size2d& d)	{ a -= d; b -= d; return (*this); }
};

} // namespace gdt
} // namespace usio

#endif

