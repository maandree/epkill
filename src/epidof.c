/*
 * epidof.c - Utility for listing pids of running processes
 *
 * Copyright (C) 2013  Jaromir Capik <jcapik@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>

#include "c.h"
#include "fileutils.h"
#include "nls.h"

#include <proc/readproc.h>
#include <argparser.h>


pid_t* procs = NULL;
static size_t proc_count = 0;

pid_t* omitted_procs = NULL;
static size_t omit_count = 0;

static char* program = NULL;

/* switch flags */
static int opt_single_shot    = 0;  /* -s */
static int opt_scripts_too    = 0;  /* -x */
static int opt_rootdir_check  = 0;  /* -c */

static char* epidof_root = NULL;


static int is_omitted(pid_t pid)
{
  size_t i;
  for (i = 0; i < omit_count; i++)
    if (pid == omitted_procs[i])
      return 1;
  return 0;
}


#define basename(filename)  (strrchr(filename, '/') ? strrchr(filename, '/') + 1 : filename)


static char* pid_link(pid_t pid, const char* base_name)
{
  size_t path_alloc_size = 0;
  ssize_t len = 0;
  char* result = NULL;
  char link[PROCPATHLEN];
  
  snprintf(link, sizeof(link), "/proc/%d/%s", pid, base_name);
  
  do
    {
      if (len == (ssize_t)path_alloc_size)
	result = (char*)realloc(result, path_alloc_size <<= 1);
      
      if ((len = readlink(link, result, path_alloc_size - 1)) < 0)
	{
	  len = 0;
	  break;
	}
    }
  while ((size_t)len == path_alloc_size);
  
  result[len] = '\0';
  return result;
}


static void select_procs(void)
{
  PROCTAB* ptp;
  proc_t task;
  size_t match;
  static size_t size = 0;
  char* cmd_arg0;
  char* cmd_arg0base;
  char* cmd_arg1;
  char* cmd_arg1base;
  char* pos;
  char* program_base;
  char* root_link;
  char* exe_link;
  char* exe_link_base;
  
  /* get the input base name */
  program_base = basename(program);
  
  ptp = openproc(PROC_FILLCOM | PROC_FILLSTAT);
  
  memset(&task, 0, sizeof(task));
  while (readproc(ptp, &task))
    {
      if (opt_rootdir_check)
	{
	  /* get the /proc/<pid>/root symlink value */
	  root_link = pid_link(task.XXXID, "root");
	  match = !strcmp(epidof_root, root_link);
	  free(root_link);
	  
	  if (!match) /* root check failed */
	    {
	      memset(&task, 0, sizeof(task));
	      continue;
	    }
	}
      
      if (!is_omitted(task.XXXID) && task.cmdline)
	{
	  cmd_arg0 = *task.cmdline;
	  
	  /* processes starting with '-' are login shells */
	  if (*cmd_arg0 == '-')
	    cmd_arg0++;
	  
	  cmd_arg0base  = basename(cmd_arg0);           /* get the argv0 basename */
	  exe_link      = pid_link(task.XXXID, "exe");  /* get the /proc/<pid>/exe symlink value */
	  exe_link_base = basename(exe_link);           /* get the exe_link basename */
	  
	  match = 0;
	  
	  if (!strcmp(program, cmd_arg0base) ||
	      !strcmp(program_base, cmd_arg0) ||
	      !strcmp(program, cmd_arg0) ||
	      
	      !strcmp(program, exe_link_base) ||
	      !strcmp(program, exe_link))
	    {
	      match = 1;
	    }
	  else if (opt_scripts_too && *(task.cmdline+1))
	    {
	      pos = cmd_arg1base = cmd_arg1 = *(task.cmdline+1);
	      
	      /* get the arg1 base name */
	      while (*pos != '\0')
		if (*(pos++) == '/')
		  cmd_arg1base = pos;
	      
	      /* if script, then task.cmd = argv1, otherwise task.cmd = argv0 */
	      if (task.cmd &&
		  !strncmp(task.cmd, cmd_arg1base, strlen(task.cmd)) &&
		  (!strcmp(program, cmd_arg1base) ||
		   !strcmp(program_base, cmd_arg1) ||
		   !strcmp(program, cmd_arg1)))
		{
		  match = 1;
		}
	    }
	  
	  free(exe_link);
	  
	  if (match)
	    {
	      if (proc_count == size)
		procs = realloc(procs, (size <<= 1) * sizeof(*procs));
	      if (procs)
		procs[proc_count++] = task.XXXID;
	      else
		xerrx(EXIT_FAILURE, _("internal error"));
	    }
	  
	}
      
      memset(&task, 0, sizeof(task));
    }
  
  closeproc(ptp);
}


static void add_to_omit_list(char* input_arg)
{
  static size_t omit_size = 0;
  char* omit_str;
  char* endptr;
  pid_t omit_pid;
  
  omit_str = strtok(input_arg, ",;:");
  while (omit_str)
    {
      if (!strcmp(omit_str, "%PPID")) /* keeping this %PPID garbage for backward compatibility only */
	{
	  omit_pid = getppid(); /* ... as it can be replaced with $$ in common shells */
	  endptr = omit_str + sizeof("%PPID") - 1;
	}
      else
	omit_pid = (pid_t)strtoul(omit_str, &endptr, 10);
      
      if (*endptr == '\0')
	{
	  if (omit_count == omit_size)
	    omitted_procs = realloc(omitted_procs, (omit_size <<= 1) * sizeof(*omitted_procs));
	  if (omitted_procs)
	    omitted_procs[omit_count++] = omit_pid;
	  else
	    xerrx(EXIT_FAILURE, _("internal error"));
	}
      else
	xwarnx(_("illegal omit pid value (%s)!\n"), omit_str);
      
      omit_str = strtok(NULL, ",;:");
    }
}



int main(int argc, char** argv)
{
  int found = 0;
  int first_pid = 1;
  char* usage_str;
  ssize_t i, n;
  
  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);
  atexit(close_stdout);
  
  n = (ssize_t)(strlen(_(" [options] [program...]")) + strlen(*argv) + 1);
  usage_str = alloca((size_t)n * sizeof(char));
  sprintf(usage_str, "%s %s", *argv, _("[options] [program...]"));
  
  args_init(_("Find the PID for a process based on environment"),
	    usage_str, NULL, 0, 1, 0, args_standard_abbreviations);
  
  args_add_option(args_new_argumentless(NULL,           0, "-c", "--check-root",  NULL), _("Restrict to processes running under the same root"));
  args_add_option(args_new_argumentless(NULL,           0, "-s", "--single-shot", NULL), _("Return only one process ID"));
  args_add_option(args_new_argumentless(NULL,           0, "-x", "--scripts",     NULL), _("Test the name of scripts"));
  args_add_option(args_new_argumented  (NULL, _("PID"), 0, "-o", "--omit-pid",    NULL), _("Do not return a specific process ID"));
  args_add_option(args_new_argumentless(NULL,           0, "-h", "--help",        NULL), _("Display this help information"));
  args_add_option(args_new_argumentless(NULL,           0, "-V", "--version",     NULL), _("Print the name and version of this program"));
  
  args_parse(argc, argv);
  
  if (args_unrecognised_count || args_opts_used("-h"))  args_help();
  else if (args_opts_used("-V"))                        printf("epidof " VERSION);
  else                                                  goto cont;
  args_dispose();
  return args_unrecognised_count ? EXIT_FAILURE : EXIT_SUCCESS;
 cont:
  
  /* process command-line options */
  if (args_opts_used("-s"))  opt_single_shot = 1;
  if (args_opts_used("-x"))  opt_scripts_too = 1;
  if (args_opts_used("-c") && (geteuid() == 0))
    opt_rootdir_check = 1, epidof_root = pid_link(getpid(), "root");
  if (args_opts_used("-o"))
    {
      char** arr = args_opts_get("-o");
      for (i = 0, n = (ssize_t)args_opts_get_count("-o"); i < n; i++)
	add_to_omit_list(arr[i]);
    }
  
  /* main loop */
  for (n = 0; n < args_files_count; n++) /* for each program */
    {
      program = args_files[n];
      proc_count = 0;
      select_procs(); /* get the list of matching processes */
      
      if (proc_count > 0)
	{
	  found = 1;
	  for (i = (ssize_t)proc_count - 1; i >= 0; i--) /* and display their PIDs */
	    {
	      printf(first_pid ? "%ld" : " %ld", (long)procs[i]);
	      first_pid = 0;
	      if (opt_single_shot)
		break;
	    }
	}
    }
  
  /* final line feed */
  if (found)
    printf("\n");
  
  /* some cleaning */
  free(procs);
  free(omitted_procs);
  free(epidof_root);
  args_dispose();
  
  return !found;
}
