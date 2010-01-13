#pragma once

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

#define roundup(x, y)   ((((x)+((y)-1))/(y))*(y))

namespace coressh {

typedef unsigned __int8  u_int8_t;
typedef unsigned __int16 u_int16_t;
typedef unsigned __int32 u_int32_t;
typedef unsigned __int64 u_int64_t;

typedef int sa_family_t;

}
