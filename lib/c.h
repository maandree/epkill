/*
 * This header was copied from util-linux at fall 2011.
 */

/*
 * Fundamental C definitions.
 */

#ifndef EPKILL_C_H
#define EPKILL_C_H


#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <error.h>


/*
 * Program name.
 */
#ifndef HAVE_PROGRAM_INVOCATION_SHORT_NAME
# ifdef HAVE___PROGNAME
extern char *__progname;
#  define program_invocation_short_name  __progname
# else
#  ifdef HAVE_GETEXECNAME
#   define program_invocation_short_name  prog_inv_sh_nm_from_file(getexecname(), 0)
#  else
#   define program_invocation_short_name  prog_inv_sh_nm_from_file(__FILE__, 1)
#  endif
static char prog_inv_sh_nm_buf[256];
static inline char* prog_inv_sh_nm_from_file(char* f, char stripext)
{
  char* t;
  
  if ((t = strrchr(f, '/')) != NULL)
    t++;
  else
    t = f;
  
  strncpy(prog_inv_sh_nm_buf, t, sizeof(prog_inv_sh_nm_buf) - 1);
  prog_inv_sh_nm_buf[sizeof(prog_inv_sh_nm_buf) - 1] = '\0';
  
  if (stripext && (t = strrchr(prog_inv_sh_nm_buf, '.')) != NULL)
    *t = '\0';
  
  return prog_inv_sh_nm_buf;
}
# endif
#endif

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

#define EPKILL_VERSION        _("%s from %s\n"), program_invocation_short_name, PACKAGE_STRING


#endif

