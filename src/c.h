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


#endif

