/**
 * epidof – pidof with environment constraints
 * 
 * epidof under the epkill project:
 *   Copyright © 2014, 2015  Mattias Andrée (m@maandree.se)
 * 
 * pidof under the procps-ng project:
 *   Copyright © 2013  Jaromir Capik (jcapik@redhat.com)
 * 
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <libintl.h>
#define _  gettext

#include <proc/readproc.h>
#include <argparser.h>

#include "environment.h"


pid_t* procs = NULL;
static size_t proc_count = 0;

pid_t* omitted_procs = NULL;
static size_t omit_count = 0;

static char* program = NULL;

/* switch flags */
static int opt_single_shot = 0;  /* -s */
static int opt_scripts_too = 0;  /* -x */

static char* epidof_root = NULL;

static char** argv;
static void* new;


#define __grow(var)          (var = var * 5 / 4 + 1024)
#define basename(filename)   (strrchr(filename, '/') ? strrchr(filename, '/') + 1 : filename)
#define xrealloc(var, size)  (new = realloc(var, size), new ? new : (perror(*argv), free(var), exit(EXIT_FAILURE), NULL))
#define xfree(var)           (free(var), var = NULL)


static __attribute__((pure)) int is_omitted(pid_t pid)
{
  size_t i;
  for (i = 0; i < omit_count; i++)
    if (pid == omitted_procs[i])
      return 1;
  return 0;
}


static char* pid_link(pid_t pid, const char* base_name)
{
  size_t path_alloc_size = 0;
  ssize_t len = 0;
  char* result = NULL;
  char link[PROCPATHLEN];
  
  snprintf(link, sizeof(link) / sizeof(char), "/proc/%d/%s", pid, base_name);
  
  while ((size_t)len == path_alloc_size)
    {
      result = xrealloc(result, __grow(path_alloc_size));
      len = readlink(link, result, path_alloc_size - 1);
      if (len < 0)
	{
	  len = 0;
	  break;
	}
    }
  
  result[len] = '\0';
  return result;
}


static void select_procs(void)
{
  static size_t size = 0;
  PROCTAB* ptp;
  proc_t* task;
  size_t match;
  char* cmd_arg0;
  char* cmd_arg0_base;
  char* cmd_arg1;
  char* cmd_arg1_base;
  char* program_base;
  char* root_link;
  char* exe_link;
  char* exe_link_base;
  
  program_base = basename(program);  /* get the input base name */
  ptp = openproc(PROC_FILLCOM | PROC_FILLSTAT);
  
  while ((task = readproc(ptp, NULL)))
    {
      if (epidof_root)
	{
	  /* get the /proc/<pid>/root symlink value */
	  root_link = pid_link(task->XXXID, "root");
	  match = !strcmp(epidof_root, root_link);
	  xfree(root_link);
	  
	  if (!match) /* root check failed */
	    {
	      freeproc(task);
	      continue;
	    }
	}
      
      if (!is_omitted(task->XXXID) && task->cmdline)
	{
	  cmd_arg0 = task->cmdline[0];
	  
	  /* processes starting with '-' are login shells */
	  if (*cmd_arg0 == '-')
	    cmd_arg0++;
	  
	  cmd_arg0_base = basename(cmd_arg0);           /* get the argv0 basename */
	  exe_link      = pid_link(task->XXXID, "exe");  /* get the /proc/<pid>/exe symlink value */
	  exe_link_base = basename(exe_link);           /* get the exe_link basename */
	  match = 0;
	  
#define __test(p, c)  (!strcmp(p, c##_base) || !strcmp(p, c) || !strcmp(p##_base, c))
	  
	  if (__test(program, cmd_arg0) || __test(program, exe_link))
	    match = 1;
	  else if (opt_scripts_too && task->cmdline[1])
	    {
	      cmd_arg1 = task->cmdline[1];
	      cmd_arg1_base = basename(cmd_arg1);
	      
	      /* if script, then task->cmd = argv1, otherwise task->cmd = argv0 */
	      if (task->cmd &&
		  !strncmp(task->cmd, cmd_arg1_base, strlen(task->cmd)) &&
		  __test(program, cmd_arg1))
		match = 1;
	    }
	  
#undef __test
	  
	  xfree(exe_link);
	  
	  if (match && environment_test(task->XXXID, *argv))
	    {
	      if (proc_count == size)
		procs = xrealloc(procs, __grow(size) * sizeof(*procs));
	      procs[proc_count++] = task->XXXID;
	    }
	}
      freeproc(task);
    }
  closeproc(ptp);
}


static void add_to_omit_list(char* input_arg)
{
  static size_t omit_size = 0;
  char* omit_str;
  char* endptr;
  pid_t omit_pid;
  
  omit_str = strtok(input_arg, ",");
  while (omit_str)
    {
      omit_pid = (pid_t)strtoul(omit_str, &endptr, 10);
      
      if (*endptr == '\0')
	{
	  if (omit_count == omit_size)
	    omitted_procs = xrealloc(omitted_procs, __grow(omit_size) * sizeof(*omitted_procs));
	  omitted_procs[omit_count++] = omit_pid;
	}
      else
	{
	  fprintf(stderr, _("%s: Illegal omit PID value: %s!\n"), *argv, omit_str);
	  exit(EXIT_FAILURE);
	}
      
      omit_str = strtok(NULL, ",");
    }
}



int main(int argc, char** argv_)
{
  int found = 0;
  int first_pid = 1;
  char* usage_str;
  ssize_t i, n;
  
  argv = argv_;
  
  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);
  
  n = (ssize_t)(strlen(_(" [options] [program...]")) + strlen(*argv) + 1);
  usage_str = alloca((size_t)n * sizeof(char));
  sprintf(usage_str, "%s%s", *argv, _(" [options] [program...]"));
  
  args_init(!strcmp(argv[0], "dpidof")
	    ? _("epidof with display isolation")
	    : _("pidof with environment constraints"),
	    usage_str, NULL, 0, 1, 0, args_standard_abbreviations);
  
  args_add_option(args_new_argumentless(NULL,           0, "-c", "--check-root",  NULL), _("Restrict to processes running under the same root"));
  args_add_option(args_new_argumentless(NULL,           0, "-s", "--single-shot", NULL), _("Return only one process ID"));
  args_add_option(args_new_argumentless(NULL,           0, "-x", "--scripts",     NULL), _("Test the name of scripts"));
  args_add_option(args_new_argumented  (NULL, _("PID"), 0, "-o", "--omit-pid",    NULL), _("Do not return a specific process ID"));
  args_add_option(args_new_argumentless(NULL,           0, "-h", "--help",        NULL), _("Display this help information"));
  args_add_option(args_new_argumentless(NULL,           0, "-V", "--version",     NULL), _("Print the name and version of this program"));
  
  environment_parse(&argc, argv);
  args_parse(argc, argv);
  
  if (args_unrecognised_count || args_opts_used("-h"))  args_help(), fprintf(stderr, "%s\n\n", _(environment_synopsis));
  else if (args_opts_used("-V"))                        printf("%s " VERSION, !strcmp(argv[0], "dpidof") ? "dpidof" : "epidof");
  else                                                  goto cont;
  return args_unrecognised_count ? EXIT_FAILURE : EXIT_SUCCESS;
 cont:
  
  /* process command-line options */
  if (args_opts_used("-s"))  opt_single_shot = 1;
  if (args_opts_used("-x"))  opt_scripts_too = 1;
  if (args_opts_used("-c") && (geteuid() == 0))
    epidof_root = pid_link(getpid(), "root");
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
	      printf(first_pid ? "%ld" : " %ld", (long)(procs[i]));
	      first_pid = 0;
	      if (opt_single_shot)
		break;
	    }
	}
    }
  
  /* final line feed */
  if (found)
    printf("\n");
  
  xfree(procs);
  xfree(omitted_procs);
  xfree(epidof_root);
  args_dispose();
  environment_dispose();
  
  return !found;
}

