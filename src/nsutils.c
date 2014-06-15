#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#include "nsutils.h"


/* we need to fill in only namespace information */
int ns_read(pid_t pid, proc_t* ns_task)
{
  struct stat attr;
  char buf[50];
  int i, rc = 0;
  
  for (i = 0; i < NUM_NS; i++)
    {
      snprintf(buf, sizeof(buf), "/proc/%i/ns/%s", pid, get_ns_name(i));
      if (stat(buf, &attr))
	{
	  if (errno != ENOENT)
	    rc = errno;
	  ns_task->ns[i] = 0;
	}
      else
	ns_task->ns[i] = (long int)attr.st_ino;
    }
  
  return rc;
}

