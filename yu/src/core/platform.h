#ifndef YU_PLATFORM_H
#define YU_PLATFORM_H

//OS
#if defined(_WIN32) || defined(_WIN64)
	#define YU_OS_WIN32
#elif (defined(__APPLE__) && (defined(__GNUC__) ||\
	defined(__xlC__) || defined(__xlc__))) || defined(__MACOS__)
#  if (defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) || defined(__IPHONE_OS_VERSION_MIN_REQUIRED))
#    define YU_OS_IPHONE
#  else
#    define YU_OS_DARWIN
#    define YU_OS_MAC
#  endif
#endif

#if defined (YU_OS_WIN32)
#	if defined(_M_X64)
#		define YU_CPU_X86_64
#	elif defined (_M_IX86)
#		define YU_CPU_X86
#	else
#		error yu does not support the cpu
#	endif
#elif defined (YU_OS_MAC) || defined (YU_OS_DARWIN) || defined (YU_IPHONE)
#	if defined(__x86_64__)
#		define YU_CPU_X86_64
#	elif defined (__arm__)
#		define YU_CPU_ARM
#	else
#		error yu does not support the cpu
#	endif
#endif

//compiler
#if defined(_MSC_VER)
#	define YU_CC_MSVC
#elif defined(__GNUC__)
#  define YU_CC_GNU
#elif defined(__clang__)
#  define YU_CC_CLANG
#else
#  error yu does not support this compiler
#endif

//byte order
#define YU_BYTE_ORDER YU_LITTLE_ENDIAN

//force inline
#if defined(YU_CC_MSVC)
#	define YU_INLINE __forceinline
#elif defined(YU_CC_GNU)
#	define YU_INLINE inline __attribute__((always_inline))
#else 
#	define YU_INLINE inline
#endif

//no inline
#if defined(YU_CC_MSVC)
#	define YU_NO_INLINE __declspec(noinline)
#elif defined(YU_CC_GNU)
#	define YU_NO_INLINE __attribute__((noinline))
#else
#	define YU_NO_INLINE
#endif

//align
#if defined(YU_CC_MSVC)
#	define YU_ALIGN(n) __declspec(align(n))
#elif defined(YU_CC_GNU) || defined(YU_CC_CLANG)
#	define YU_ALIGN(n) __attribute__((aligned(n)))
#else 
#	error yu need to define align
#endif


//int type
namespace yu
{
	typedef unsigned char u8;
	typedef unsigned short u16;
	typedef unsigned int u32;
	typedef unsigned long long int u64;

	typedef char i8;
	typedef short i16;
	typedef int i32;
	typedef long long int i64;

	typedef float f32;
	typedef double f64;
}

#include <stddef.h>

#endif