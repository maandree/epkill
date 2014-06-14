#ifndef EPKILL_NSUTILS_H
#define EPKILL_NSUTILS_H


#include <proc/readproc.h>
#include <sys/types.h>

int ns_read(pid_t pid, proc_t* ns_task);


#endif

