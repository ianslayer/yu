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
#elif defined(YU_CC_GNU) || defined(YU_CC_CLANG)
#	define YU_INLINE inline __attribute__((always_inline))
#else 
#	define YU_INLINE inline
#endif

//no inline
#if defined(YU_CC_MSVC)
#	define YU_NO_INLINE __declspec(noinline)
#elif defined(YU_CC_GNU) || defined(YU_CC_CLANG)
#	define YU_NO_INLINE __attribute__((noinline))
#else
#	define YU_NO_INLINE
#endif

//align
#if defined(YU_CC_MSVC)
#	define YU_ALIGN(n) __declspec(align(n))
#	define YU_PRE_ALIGN(n) __declspec(align(n))
#	define YU_POST_ALIGN(n)
#elif defined(YU_CC_GNU) || defined(YU_CC_CLANG)
#	define YU_ALIGN(n) __attribute__((aligned(n)))
#	define YU_PRE_ALIGN(n)
#	define YU_POST_ALIGN(n) __attribute__((aligned(n)))
#else 
#	error yu need to define align
#endif

//cache size
#if defined (YU_CPU_X86) || defined (YU_CPU_X86_64)
#	define CACHE_LINE 64
#else
#	error yu need to define CACHE_LINE	
#endif

#if defined YU_CC_MSVC
#	define COMPILER_BARRIER() _ReadWriteBarrier()
#elif defined(YU_CC_GNU) || defined(YU_CC_CLANG)
#	define COMPILER_BARRIER() asm volatile("" ::: "memory")
#endif

#if defined YU_CC_MSVC
#	if defined _DEBUG
#		define YU_DEBUG
#	endif
#endif

//graphics api TODO: should I place api selector here?
#if defined YU_OS_WIN32
#	define YU_DX11
#	define YU_GRAPHICS_API YU_DX11
#elif defined YU_OS_MAC
#	define YU_GL
#	define YU_GRAPHICS_API YU_GL
#endif

#include <stdint.h>

namespace yu
{
	typedef uint8_t u8;
	typedef uint16_t u16;
	typedef uint32_t u32;
	typedef uint64_t u64;

	typedef int8_t i8;
	typedef int16_t i16;
	typedef int32_t i32;
	typedef int64_t i64;

	typedef float f32;
	typedef double f64;
}

#include <stddef.h>

#endif