#include <errno.h>
#include <error.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <unistd.h>

#include "nls.h"
#include "fileutils.h"
#ifndef HAVE_ERROR_H
# include "c.h" /* for error() emulation */
#endif

int close_stream(FILE * stream)
{
  const int some_pending = (__fpending(stream) != 0);
  const int prev_fail = (ferror(stream) != 0);
  const int fclose_fail = (fclose(stream) != 0);
  if (prev_fail || (fclose_fail && (some_pending || errno != EBADF)))
    {
      if (!fclose_fail && errno != EPIPE)
	errno = 0;
      return EOF;
    }
  return 0;
}

/* Use atexit(); */
void close_stdout(void)
{
  if (close_stream(stdout) != 0 && !(errno == EPIPE))
    {
      char const *write_error = _("write error");
      error(0, errno, "%s", write_error);
      _exit(EXIT_FAILURE);
    }

  if (close_stream(stderr) != 0)
    _exit(EXIT_FAILURE);
}

