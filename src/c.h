/*
 * This header was copied from util-linux at fall 2011.
 */

/*
 * Fundamental C definitions.
 */

#ifndef EPKILL_C_H
#define EPKILL_C_H


#include <error.h>


#define xwarn(...)          error(0, errno, __VA_ARGS__)
#define xwarnx(...)         error(0, 0, __VA_ARGS__)
#define xerrx(STATUS, ...)  error(STATUS, 0, __VA_ARGS__)

/*
 * Constant strings for usage() functions.
 */
#define USAGE_HEADER          _("\nUsage:\n")
#define USAGE_OPTIONS         _("\nOptions:\n")
#define USAGE_SEPARATOR       _("\n")
#define USAGE_HELP            _(" -h, --help     display this help and exit\n")
#define USAGE_VERSION         _(" -V, --version  output version information and exit\n")
#define USAGE_MAN_TAIL(_man)  _("\nFor more details see %s.\n"), _man


#endif

