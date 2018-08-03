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
#if !defined (__FreeBSD__) && !defined (__OpenBSD__)
#define BSON_HAVE_SYSCALL_TID 1
#endif
#endif

/* Fix for snow leopard */
#ifdef __APPLE__
#define BSON_HAVE_REALLOCF 1
#include <Availability.h>
#ifndef MAC_OS_X_VERSION_10_8
#undef BSON_HAVE_STRNLEN
#define BSON_HAVE_STRNLEN 0
#endif
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
#undef BSON_HAVE_SYSCALL_TID
#else
#define BSON_OS 1
#define BSON_HAVE_CLOCK_GETTIME 1
#define BSON_HAVE_RAND_R 1
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
 * Define to 1 if you have strings.h available on your platform.
 */
#define BSON_HAVE_STRINGS_H 1
#if BSON_HAVE_STRINGS_H != 1
# undef BSON_HAVE_STRINGS_H
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
 * Define to 1 if you have gmtime_r available on your platform.
 */
#ifndef _WIN32
#define BSON_HAVE_GMTIME_R 1
#endif
#if BSON_HAVE_GMTIME_R != 1
# undef BSON_HAVE_GMTIME_R
#endif

/*
 * Define to 1 if you have struct timespec available on your platform.
 */
#define BSON_HAVE_TIMESPEC 1
#if BSON_HAVE_TIMESPEC != 1
# undef BSON_HAVE_TIMESPEC
#endif

#endif /* BSON_CONFIG_H */
