/*
 *  matiec - a compiler for the programming languages defined in IEC 61131-3
 *  Copyright (C) 2003-2011  Mario de Sousa (msousa@fe.up.pt)
 *  Copyright (C) 2007-2011  Laurent Bessard and Edouard Tisserant
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * This code is made available on the understanding that it will not be
 * used in safety-critical situations without a full and competent review.
 */

/*
 * An IEC 61131-3 compiler.
 *
 * Based on the
 * FINAL DRAFT - IEC 61131-3, 2nd Ed. (2001-12-10)
 *
 */



#ifndef _MAIN_HH
#define _MAIN_HH



 /* Function used throughout the code --> used to report failed assertions (i.e. internal compiler errors)! */
#include <stddef.h>  /* required for NULL */
 
#define ERROR               error_exit(__FILE__,__LINE__)
#define ERROR_MSG(msg, ...) error_exit(__FILE__,__LINE__, msg, ## __VA_ARGS__)

extern void error_exit(const char *file_name, int line_no, const char *errmsg = NULL, ...);




 /* Get the definition of INT16_MAX, INT16_MIN, UINT64_MAX, INT64_MAX, INT64_MIN, ... */
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS /* required to have UINTxx_MAX defined when including stdint.h from C++ source code. */
#endif
#include <stdint.h>         
#include <limits>

#ifndef   UINT64_MAX 
  #define UINT64_MAX (std::numeric_limits< uint64_t >::max())
#endif
#ifndef    INT64_MAX 
  #define  INT64_MAX (std::numeric_limits<  int64_t >::max())
#endif
#ifndef    INT64_MIN
  #define  INT64_MIN (std::numeric_limits<  int64_t >::min()) 
#endif



/* Determine, for the current platform, which datas types (float, double or long double) use 64 and 32 bits. */
/* NOTE: We cant use sizeof() in pre-processor directives, so we have to do it another way... */
/* CURIOSITY: We can use sizeof() and offsetof() inside static_assert() but:
 *          - this only allows us to make assertions, and not #define new macros
 *          - is only available in the C standard [ISO/IEC 9899:2011] and the C++ 0X draft standard [Becker 2008]. It is not available in C99.
 *          https://www.securecoding.cert.org/confluence/display/seccode/DCL03-C.+Use+a+static+assertion+to+test+the+value+of+a+constant+expression
 *         struct {int a, b, c, d} header_t;
 *  e.g.:  static_assert(offsetof(struct header_t, c) == 8, "Compile time error message.");
 */

#include <float.h>
#if    (LDBL_MANT_DIG == 53) /* NOTE: 64 bit IEC559 real has 53 bits for mantissa! */
  #define long_double long double
  #define real64_t long_double /* so we can later use #if (real64_t == long_double) directives in the code! */
  #define REAL64_MAX  LDBL_MAX
#elif  ( DBL_MANT_DIG == 53) /* NOTE: 64 bit IEC559 real has 53 bits for mantissa! */
  #define real64_t double
  #define REAL64_MAX  DBL_MAX
#elif  ( FLT_MANT_DIG == 53) /* NOTE: 64 bit IEC559 real has 53 bits for mantissa! */
  #define real64_t float
  #define REAL64_MAX  FLT_MAX
#else 
  #error Could not find a 64 bit floating point data type on this platform. Aborting...
#endif


#if    (LDBL_MANT_DIG == 24) /* NOTE: 32 bit IEC559 real has 24 bits for mantissa! */
  #ifndef long_double 
    #define long_double long double
  #endif  
  #define real32_t long_double /* so we can later use #if (real32_t == long_double) directives in the code! */
  #define REAL32_MAX  LDBL_MAX
#elif  ( DBL_MANT_DIG == 24) /* NOTE: 32 bit IEC559 real has 24 bits for mantissa! */
  #define real32_t double
  #define REAL32_MAX  DBL_MAX
#elif  ( FLT_MANT_DIG == 24) /* NOTE: 32 bit IEC559 real has 24 bits for mantissa! */
  #define real32_t float
  #define REAL32_MAX  FLT_MAX
#else 
  #error Could not find a 32 bit floating point data type on this platform. Aborting...
#endif



#include <math.h>
#ifndef INFINITY
  #error Could not find the macro that defines the value for INFINITY in the current platform.
#endif
#ifndef NAN
  #error Could not find the macro that defines the value for NAN in the current platform.
#endif



/* get the printf format macros for printing variables of fixed data size
 * e.g.  int64_t v; printf("value=%"PRId64" !!\n", v);
 * e.g. uint64_t v; printf("value=%"PRIu64" !!\n", v);
 * e.g. uint64_t v; printf("value=%"PRIx64" !!\n", v);  // hexadecimal format
 */
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>


#endif // #ifndef _MAIN_HH
