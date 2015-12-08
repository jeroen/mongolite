#ifndef BSON_CONFIG_H
#define BSON_CONFIG_H

/* stuff for solaris */
#if (defined(__sun) && defined(__SVR4))
#define BSON_HAVE_STRNLEN 0
#define BSON_HAVE_ATOMIC_32_ADD_AND_FETCH 0
#define BSON_HAVE_ATOMIC_64_ADD_AND_FETCH 0
/* sparc is big endian */
#include <sys/byteorder.h>
#ifdef _BIG_ENDIAN
#define BSON_BYTE_ORDER 4321
#else
#define BSON_BYTE_ORDER 1234
#endif
#else
/* for everyone else */
#define BSON_BYTE_ORDER 1234
#define BSON_HAVE_STRNLEN 1
#define BSON_HAVE_ATOMIC_32_ADD_AND_FETCH 1
#define BSON_HAVE_ATOMIC_64_ADD_AND_FETCH 1
#endif

/*
 * Define to 1 if you have stdbool.h
 */
#define BSON_HAVE_STDBOOL_H 1
#if BSON_HAVE_STDBOOL_H != 1
# undef BSON_HAVE_STDBOOL_H
#endif


/*
 * Define to 1 for POSIX-like systems, 2 for Windows.
 */
#ifdef _WIN32
#define BSON_OS 2
#else
#define BSON_OS 1
#define BSON_HAVE_CLOCK_GETTIME 1
#endif

/*
 * Define to 1 if we have access to GCC 32-bit atomic builtins.
 * While this requires GCC 4.1+ in most cases, it is also architecture
 * dependent. For example, some PPC or ARM systems may not have it even
 * if it is a recent GCC version.
 */
#if BSON_HAVE_ATOMIC_32_ADD_AND_FETCH != 1
# undef BSON_HAVE_ATOMIC_32_ADD_AND_FETCH
#endif

/*
 * Similarly, define to 1 if we have access to GCC 64-bit atomic builtins.
 */
#if BSON_HAVE_ATOMIC_64_ADD_AND_FETCH != 1
# undef BSON_HAVE_ATOMIC_64_ADD_AND_FETCH
#endif


/*
 * Define to 1 if your system requires {} around PTHREAD_ONCE_INIT.
 * This is typically just Solaris 8-10.
 */
#define BSON_PTHREAD_ONCE_INIT_NEEDS_BRACES 0
#if BSON_PTHREAD_ONCE_INIT_NEEDS_BRACES != 1
# undef BSON_PTHREAD_ONCE_INIT_NEEDS_BRACES
#endif


/*
 * Define to 1 if you have clock_gettime() available.
 */
#if BSON_HAVE_CLOCK_GETTIME != 1
# undef BSON_HAVE_CLOCK_GETTIME
#endif


/*
 * Define to 1 if you have strnlen available on your platform.
 */
#if BSON_HAVE_STRNLEN != 1
# undef BSON_HAVE_STRNLEN
#endif


/*
 * Define to 1 if you have snprintf available on your platform.
 */
#define BSON_HAVE_SNPRINTF 1
#if BSON_HAVE_SNPRINTF != 1
# undef BSON_HAVE_SNPRINTF
#endif


/*
* Define to 1 if you have _set_output_format (VS2013 and older).
*/
#define BSON_NEEDS_SET_OUTPUT_FORMAT 0
#if BSON_NEEDS_SET_OUTPUT_FORMAT != 1
# undef BSON_NEEDS_SET_OUTPUT_FORMAT
#endif

/*
* Define to 1 if you have struct timespec available on your platform.
*/
#define BSON_HAVE_TIMESPEC 1
#if BSON_HAVE_TIMESPEC != 1
# undef BSON_HAVE_TIMESPEC
#endif


/*
* Define to 1 if you want extra aligned types in libbson
* Moved to this in Makevars because it doesn't work well with mingw
* #if BSON_EXTRA_ALIGN != 1
* # undef BSON_EXTRA_ALIGN
* #endif
*/



#endif /* BSON_CONFIG_H */
