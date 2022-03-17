
#ifndef __MSYS_TYPEDEFS_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79
#define __MSYS_TYPEDEFS_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79

#include <limits.h>



#if defined( MSYS_WIN32 )
#if defined( _CHAR_UNSIGNED )
#define MSYS_CHAR_UNSIGNED
#endif
#elif defined( MSYS_LINUX )
#if defined( __CHAR_UNSIGNED__ )
#define MSYS_CHAR_UNSIGNED
#endif
#endif

#if !defined( MSYS_NO_NAMESPACE_MSYS )
# define MSYS_NAMESPACE_BEGIN     namespace msys {
# define MSYS_NAMESPACE_END       }
#else
# define MSYS_NO_DIRECTIVE_USING_NAMESPACE_MSYS
# define MSYS_NAMESPACE_BEGIN
# define MSYS_NAMESPACE_END
#endif

// begin namespace msys

#if !(defined MSYS_TYPE_VOID)
  #define MSYS_TYPE_VOID void
#endif
#if !(defined MSYS_NO_TYPE_VOID )
  typedef MSYS_TYPE_VOID Void;
#endif

#if !(defined MSYS_TYPE_BOOL)
  #define MSYS_TYPE_BOOL bool
#endif
#if !(defined MSYS_NO_TYPE_BOOL )
  typedef MSYS_TYPE_BOOL Bool;
#endif

#if !(defined MSYS_TYPE_SCHAR)
  #define MSYS_TYPE_SCHAR signed char
#endif
#if !(defined MSYS_NO_TYPE_SCHAR )
  typedef MSYS_TYPE_SCHAR SChar;
#endif

#if !(defined MSYS_TYPE_CHAR)
  #define MSYS_TYPE_CHAR char
#endif
#if !(defined MSYS_NO_TYPE_CHAR )
  typedef MSYS_TYPE_CHAR Char;
#endif

#if !(defined MSYS_TYPE_UCHAR)
  #define MSYS_TYPE_UCHAR unsigned char
#endif
#if !(defined MSYS_NO_TYPE_UCHAR )
  typedef MSYS_TYPE_UCHAR UChar;
#endif

#if !(defined MSYS_TYPE_INT8)
  #define MSYS_TYPE_INT8 signed char
#endif
#if !(defined MSYS_NO_TYPE_INT8 )
  typedef MSYS_TYPE_INT8 Int8;
#endif

#if !(defined MSYS_TYPE_UINT8)
  #define MSYS_TYPE_UINT8 unsigned char
#endif
#if !(defined MSYS_NO_TYPE_UINT8 )
  typedef MSYS_TYPE_UINT8 UInt8;
#endif

#if !(defined MSYS_TYPE_SHORT)
  #define MSYS_TYPE_SHORT short
#endif
#if !(defined MSYS_NO_TYPE_SHORT )
  typedef MSYS_TYPE_SHORT Short;
#endif

#if !(defined MSYS_TYPE_USHORT)
  #define MSYS_TYPE_USHORT unsigned short
#endif
#if !(defined MSYS_NO_TYPE_USHORT )
  typedef MSYS_TYPE_USHORT UShort;
#endif

#if !(defined MSYS_TYPE_INT16)
  #define MSYS_TYPE_INT16 short
#endif
#if !(defined MSYS_NO_TYPE_INT16 )
  typedef MSYS_TYPE_INT16 Int16;
#endif

#if !(defined MSYS_TYPE_UINT16)
  #define MSYS_TYPE_UINT16 unsigned short
#endif
#if !(defined MSYS_NO_TYPE_UINT16 )
  typedef MSYS_TYPE_UINT16 UInt16;
#endif

#if !(defined MSYS_TYPE_INT)
  #define MSYS_TYPE_INT int
#endif
#if !(defined MSYS_NO_TYPE_INT )
  typedef MSYS_TYPE_INT Int;
#endif

#if !(defined MSYS_TYPE_UINT)
  #define MSYS_TYPE_UINT unsigned int
#endif
#if !(defined MSYS_NO_TYPE_UINT )
  typedef MSYS_TYPE_UINT UInt;
#endif

#if !(defined MSYS_TYPE_INT32)
  #define MSYS_TYPE_INT32 int
#endif
#if !(defined MSYS_NO_TYPE_INT32 )
  typedef MSYS_TYPE_INT32 Int32;
#endif

#if !(defined MSYS_TYPE_UINT32)
  #define MSYS_TYPE_UINT32 unsigned int
#endif
#if !(defined MSYS_NO_TYPE_UINT32 )
  typedef MSYS_TYPE_UINT32 UInt32;
#endif

#if !(defined MSYS_TYPE_LONG)
  #define MSYS_TYPE_LONG long
#endif
#if !(defined MSYS_NO_TYPE_LONG )
  typedef MSYS_TYPE_LONG Long;
#endif

#if !(defined MSYS_TYPE_ULONG)
  #define MSYS_TYPE_ULONG unsigned long
#endif
#if !(defined MSYS_NO_TYPE_ULONG )
  typedef MSYS_TYPE_ULONG ULong;
#endif


#if defined( MSYS_WIN32 ) || defined( WIN32 )
  #if !(defined MSYS_TYPE_INT64)
    #define MSYS_TYPE_INT64 __int64
  #endif
  #if !(defined MSYS_NO_TYPE_INT64 )
    typedef MSYS_TYPE_INT64 Int64;
  #endif

  #if !(defined MSYS_TYPE_UINT64)
    #define MSYS_TYPE_UINT64 unsigned __int64
  #endif
  #if !(defined MSYS_NO_TYPE_UINT64 )
    typedef MSYS_TYPE_UINT64 UInt64;
  #endif
#elif defined( MSYS_LINUX )
  #if !(defined MSYS_TYPE_INT64)
    #define MSYS_TYPE_INT64 long long int
  #endif
  #if !(defined MSYS_NO_TYPE_INT64 )
    typedef MSYS_TYPE_INT64 Int64;
  #endif

  #if !(defined MSYS_TYPE_UINT64)
    #define MSYS_TYPE_UINT64 unsigned long long int
  #endif
  #if !(defined MSYS_NO_TYPE_UINT64 )
    typedef MSYS_TYPE_UINT64 UInt64;
  #endif
#endif


#if !(defined MSYS_TYPE_FLOAT)
  #define MSYS_TYPE_FLOAT float
#endif
#if !(defined MSYS_NO_TYPE_FLOAT )
  typedef MSYS_TYPE_FLOAT Float;
#endif

#if !(defined MSYS_TYPE_DOUBLE)
  #define MSYS_TYPE_DOUBLE double
#endif
#if !(defined MSYS_NO_TYPE_DOUBLE )
  typedef MSYS_TYPE_DOUBLE Double;
#endif


#define MSYS_CHAR_BIT          8

#define MSYS_SCHAR_MIN        (-128)
#define MSYS_SCHAR_MAX        127
#define MSYS_UCHAR_MAX        0xFF
#define MSYS_INT8_MIN          MSYS_SCHAR_MIN
#define MSYS_INT8_MAX          MSYS_SCHAR_MAX
#define MSYS_UINT8_MAX        MSYS_UCHAR_MAX

#if defined( MSYS_CHAR_UNSIGNED )
#define MSYS_CHAR_MIN          0
#define MSYS_CHAR_MAX          MSYS_UCHAR_MAX
#else
#define MSYS_CHAR_MIN          MSYS_SCHAR_MIN
#define MSYS_CHAR_MAX          MSYS_SCHAR_MAX
#endif

#define MSYS_SHORT_MIN        (-32768)
#define MSYS_SHORT_MAX        32767
#define MSYS_USHORT_MAX        0xFFFF
#define MSYS_INT16_MIN        MSYS_SHORT_MIN
#define MSYS_INT16_MAX        MSYS_SHORT_MAX
#define MSYS_UINT16_MAX        MSYS_USHORT_MAX

#define MSYS_INT_MIN          (-2147483647 - 1)
#define MSYS_INT_MAX          2147483647
#define MSYS_UINT_MAX          0xFFFFFFFFU
#define MSYS_INT32_MIN        MSYS_INT_MIN
#define MSYS_INT32_MAX        MSYS_INT_MAX
#define MSYS_UINT32_MAX        MSYS_UINT_MAX

#define MSYS_LONG_MIN          (-2147483647L - 1L)
#define MSYS_LONG_MAX          2147483647L
#define MSYS_ULONG_MAX        0xFFFFFFFFUL

#if defined( MSYS_WIN32 ) || defined( WIN32 )
#define MSYS_INT64_MIN        (-9223372036854775807i64 - 1i64)
#define MSYS_INT64_MAX        9223372036854775807i64
#define MSYS_UINT64_MAX        0xFFFFFFFFFFFFFFFFui64
#elif defined( MSYS_LINUX )
#define MSYS_INT64_MIN        (-9223372036854775807LL - 1LL)
#define MSYS_INT64_MAX        9223372036854775807LL
#define MSYS_UINT64_MAX        0xFFFFFFFFFFFFFFFFULL
#endif


#if !(defined MSYS_TYPE_ERRVAL)
  #define MSYS_TYPE_ERRVAL int
#endif
#if !(defined MSYS_NO_TYPE_ERRVAL )
  typedef MSYS_TYPE_ERRVAL ErrVal;
#endif


// define status of lifecycle of an object instance
#define INST_STATE_UNINITIALIZED  (0)
#define INST_STATE_INITIALIZING   (1)
#define INST_STATE_INITIALIZED    (2)
#define INST_STATE_UNINITIALIZING (3)

#if !(defined MSYS_TYPE_INSTSTATE)
  #define MSYS_TYPE_INSTSTATE unsigned int
#endif
#if !(defined MSYS_NO_TYPE_INSTSTATE )
  typedef MSYS_TYPE_INSTSTATE InstState;
#endif


#endif //__TYPEDEFS_H_D64BE9B4_A8DA_11D3_AFE7_005004464B79
