/*
 * epgrep/epkill — Utilities to filter the process table
 * 
 * epgrep/epkill under the epkill project:
 *   Copyright © 2014        Mattias Andrée (maandree@member.fsf.org)
 * 
 * pgrep/pkill under the procps-ng project:
 *   Copyright © 2000        Kjetil Torgrim Homme (kjetilho@ifi.uio.no)
 *   Copyright © 2002, 2006  Albert Cahalan
 *   Copyright © 2012        Roberto Polli (rpolli@babel.it)
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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <regex.h>
#include <sys/file.h>
#include <locale.h>
#include <libintl.h>
#define _  gettext

#define CMDSTRSIZE  4096

#include <proc/sig.h>
#include <proc/devname.h>
#include <argparser.h>


static int i_am_epkill = 0;

struct el
{
  long num;
  char* str;
};

/* User supplied arguments */

static int opt_full = 0;
static int opt_long = 0;
static int opt_longlong = 0;
static int opt_oldest = 0;
static int opt_newest = 0;
static int opt_negate = 0;
static int opt_exact = 0;
static int opt_count = 0;
static int opt_signal = SIGTERM;
static int opt_lock = 0;
static int opt_case = 0;
static int opt_echo = 0;
static int opt_threads = 0;
static pid_t opt_ns_pid = 0;

static const char* opt_delim = "\n";
static struct el* opt_pgrp = NULL;
static struct el* opt_rgid = NULL;
static struct el* opt_pid = NULL;
static struct el* opt_ppid = NULL;
static struct el* opt_sid = NULL;
static struct el* opt_term = NULL;
static struct el* opt_euid = NULL;
static struct el* opt_ruid = NULL;
static struct el* opt_nslist = NULL;
static char* opt_pattern = NULL;
static char* opt_pidfile = NULL;

static char* execname;

/* by default, all namespaces will be checked */
static int ns_flags = 0x3f;

static void* new;


#define xstrdup(var)         (new =  strdup(var),       new ? new : (perror(execname),            exit(EXIT_FAILURE), NULL))
#define xmalloc(size)        (new =  malloc(size),      new ? new : (perror(execname),            exit(EXIT_FAILURE), NULL))
#define xrealloc(var, size)  (new = realloc(var, size), new ? new : (perror(execname), free(var), exit(EXIT_FAILURE), NULL))
#define xerror(string)       (fprintf(stderr, _("%s: %s\n"), execname, string), exit(EXIT_FAILURE))
#define xxerror(string)      (fprintf(stderr, _("%s: %s: %s\n"), execname, string, name), exit(EXIT_FAILURE))



/* we need to fill in only namespace information */
static int ns_read(pid_t pid, proc_t* ns_task)
{
  struct stat attr;
  char pathname[50];
  int i;
  
  for (i = 0; i < NUM_NS; i++)
    {
      sprintf(pathname, "/proc/%i/ns/%s", pid, get_ns_name(i));
      if (stat(pathname, &attr))
	{
	  if (errno != ENOENT)
	    return -1;
	  ns_task->ns[i] = 0;
	}
      else
	ns_task->ns[i] = (long int)attr.st_ino;
    }
  
  return 0;
}


static struct el* split_list(const char* restrict str, void (*convert)(const char*, struct el*))
{
  char* copy;
  char* ptr;
  char* sep_pos;
  size_t i = 0;
  size_t size = 0;
  struct el* list = NULL;
  
  if (str[0] == '\0')
    {
      args_help();
      exit(EXIT_FAILURE);
      return NULL;
    }
  
  copy = xstrdup(str);
  ptr = copy;
  
  do
    {
      if (i == size)
	{
	  size = size * 5 / 4 + 4;
	  /* add 1 because slot zero is a count */
	  list = xrealloc(list, 1 + size * sizeof(*list));
	}
      sep_pos = strchr(ptr, ',');
      if (sep_pos)
	*sep_pos = 0;
      /* Use ++i instead of i++ because slot zero is a count */
      convert(ptr, &list[++i]);
      if (sep_pos)
	ptr = sep_pos + 1;
    }
  while (sep_pos);
  
  free(copy);
  if (!i)
    {
      free(list);
      list = NULL;
      args_help();
      exit(EXIT_FAILURE);
    }
  else
    list[0].num = (long)i;
  return list;
}

/* strict_atol returns a Boolean: TRUE if the input string
 * contains a plain number, FALSE if there are any non-digits. */
static int strict_atol(const char* restrict str, long* restrict value)
{
  int res = 0;
  int sign = 1;
  
  if (*str == '+')
    str++;
  else if (*str == '-')
    {
      str++;
      sign = -1;
    }
  
  for (; *str; str++)
    {
      if (!isdigit(*str))
	return 0;
      res *= 10;
      res += *str - '0';
    }
  *value = sign * res;
  return 1;
}


/* We try a read lock. The daemon should have a write lock.
 * Seen using flock: FreeBSD code */
static int has_flock(int fd)
{
  return (flock(fd, LOCK_SH | LOCK_NB) == -1) && (errno == EWOULDBLOCK);
}

/* We try a read lock. The daemon should have a write lock.
 * Seen using fcntl: libslack */
static int has_fcntl(int fd)
{
  struct flock f;  /* seriously, struct flock is for a fnctl lock! */
  f.l_type = F_RDLCK;
  f.l_whence = SEEK_SET;
  f.l_start = 0;
  f.l_len = 0;
  return (fcntl(fd, F_SETLK, &f) == -1) && ((errno == EACCES) || (errno == EAGAIN));
}

static struct el* read_pidfile(void)
{
  char buf[12];
  int fd;
  struct stat sbuf;
  char* endp;
  ssize_t n;
  pid_t pid;
  struct el* list = NULL;
  
  fd = open(opt_pidfile, O_RDONLY | O_NOCTTY | O_NONBLOCK);
  if (fd < 0)
    goto just_ret;
  if (fstat(fd, &sbuf) || !S_ISREG(sbuf.st_mode) || (sbuf.st_size < 1))
    goto out;
  /* type of lock, if any, is not standardized on Linux */
  if(opt_lock && !has_flock(fd) && !has_fcntl(fd))
    goto out;
  memset(buf,'\0', sizeof(buf));
  n = read(fd,buf + 1, sizeof(buf) - 2);
  if (n < 1)
    goto out;
  pid = (pid_t)strtoul(buf + 1, &endp, 10);
  if ((endp <= buf + 1) || (pid < 1) || (pid > 0x7fffffff))
    goto out;
  if (*endp && !isspace(*endp))
    goto out;
  list = xmalloc(2 * sizeof(*list));
  list[0].num = 1;
  list[1].num = pid;
 out:
  close(fd);
 just_ret:
  return list;
}

static void conv_uid(const char* restrict name, struct el* restrict e)
{
  struct passwd* pwd;
  if (!strict_atol(name, &e->num))
    {
      if (pwd = getpwnam(name), pwd == NULL)
	xxerror(_("Invalid user name"));
      e->num = pwd->pw_uid;
    }
}


static void conv_gid(const char* restrict name, struct el* restrict e)
{
  struct group* grp;
  if (!strict_atol(name, &e->num))
    {
      if (grp = getgrnam(name), grp == NULL)
	xxerror(_("Invalid group name"));
      e->num = grp->gr_gid;
    }
}


static void conv_pgrp(const char* restrict name, struct el* restrict e)
{
  if (!strict_atol(name, &e->num))
    xxerror(_("Invalid process group"));
  if (e->num == 0)
    e->num = getpgrp();
}


static void conv_sid(const char* restrict name, struct el* restrict e)
{
  if (!strict_atol(name, &e->num))
    xxerror(_("Invalid session id"));
  if (e->num == 0)
    e->num = getsid(0);
}


static void conv_num(const char* restrict name, struct el* restrict e)
{
  if (!strict_atol(name, &e->num))
    xxerror(_("Not a number"));
}


static void conv_str(const char* restrict name, struct el* restrict e)
{
  e->str = xstrdup(name);
}


static void conv_ns(const char* restrict name, struct el* restrict e)
{
  int id;
  conv_str(name, e);
  ns_flags = 0;
  id = get_ns_id(name);
  if (id == -1)
    exit(EXIT_FAILURE);
  ns_flags |= 1 << id;
}

static int match_numlist(long value, const struct el* restrict list)
{
  int found = 0;
  long i;
  if (list == NULL)
    found = 0;
  else
    for (i = list[0].num; i > 0; i--)
      if (list[i].num == value)
	found = 1;
  return found;
}

static int match_strlist(const char* restrict value, const struct el* restrict list)
{
  int found = 0;
  long i;
  if (list == NULL)
    found = 0;
  else
    for (i = list[0].num; i > 0; i--)
      if (!strcmp(list[i].str, value))
	found = 1;
  return found;
}

static int match_ns(const proc_t* task, const proc_t* ns_task)
{
  int found = 1;
  long i;
  for (i = 0; i < NUM_NS; i++)
    if ((ns_flags & (1 << i)))
      if (task->ns[i] != ns_task->ns[i])
	found = 0;
  return found;
}

static void output_numlist(const struct el* restrict list, int num)
{
  const char* delim = opt_delim;
  long i;
  for (i = 0; i < num; i++)
    {
      if (i + 1 == num)
	delim = "\n";
      printf("%ld%s", list[i].num, delim);
    }
}

static void output_strlist(const struct el* restrict list, int num)
{
  /* FIXME: escape codes */
  const char* delim = opt_delim;
  long i;
  for (i = 0; i < num; i++)
    {
      if (i + 1 == num)
	delim = "\n";
      printf("%lu %s%s", list[i].num, list[i].str, delim);
    }
}

static PROCTAB* do_openproc(void)
{
  PROCTAB* ptp;
  int flags = 0;
  
  if (opt_pattern || opt_full || opt_longlong)                      flags |= PROC_FILLCOM;
  if (opt_ruid || opt_rgid)                                         flags |= PROC_FILLSTATUS;
  if (opt_oldest || opt_newest || opt_pgrp || opt_sid || opt_term)  flags |= PROC_FILLSTAT;
  if (!(flags & PROC_FILLSTAT))
    flags |= PROC_FILLSTATUS;  /* FIXME: need one, and PROC_FILLANY broken */
  if (opt_ns_pid)
    flags |= PROC_FILLNS;
  if (opt_euid && !opt_negate)
    {
      long num = opt_euid[0].num;
      long i = num;
      uid_t* uids = xmalloc((size_t)num * sizeof(uid_t));
      while (i-- > 0)
	uids[i] = (uid_t)(opt_euid[i+1].num);
      flags |= PROC_UID;
      ptp = openproc(flags, uids, num);
    }
  else
      ptp = openproc(flags);
  return ptp;
}

static regex_t* do_regcomp(void)
{
  regex_t* preg = NULL;
  
  if (opt_pattern)
    {
      char* re;
      char errbuf[256];
      int re_err;
      
      preg = xmalloc(sizeof(regex_t));
      if (opt_exact)
	{
	  re = xmalloc(strlen(opt_pattern) + 5);
	  sprintf(re, "^(%s)$", opt_pattern);
	}
      else
	re = opt_pattern;
    
      re_err = regcomp(preg, re, REG_EXTENDED | REG_NOSUB | opt_case);
      
      if (opt_exact)
	free(re);
      
      if (re_err)
	{
	  regerror(re_err, preg, errbuf, sizeof(errbuf));
	  fputs(errbuf, stderr);
	  exit(EXIT_FAILURE);
	}
    }
  return preg;
}

static struct el* select_procs(size_t* num)
{
  PROCTAB* ptp;
  proc_t task;
  unsigned long long saved_start_time; /* for new/old support */
  pid_t saved_pid = 0; /* for new/old support */
  size_t matches = 0;
  size_t size = 0;
  regex_t* preg;
  pid_t myself = getpid();
  struct el* list = NULL;
  char cmdline[CMDSTRSIZE];
  char cmdsearch[CMDSTRSIZE];
  char cmdoutput[CMDSTRSIZE];
  proc_t ns_task;
  
  ptp = do_openproc();
  preg = do_regcomp();
  
  saved_start_time = opt_newest ? 0ULL : ~0ULL;
  
  if (opt_newest)  saved_pid = 0;
  if (opt_oldest)  saved_pid = INT_MAX;
  if (opt_ns_pid && ns_read(opt_ns_pid, &ns_task))
    xerror(_("Error reading reference namespace information."));
  
  memset(&task, 0, sizeof(task));
  while (readproc(ptp, &task))
    {
      int match = 1;
      
      if (task.XXXID == myself)
	continue;
      else if (opt_newest && (task.start_time < saved_start_time))    match = 0;
      else if (opt_oldest && (task.start_time > saved_start_time))    match = 0;
      else if (opt_ppid   && !match_numlist(task.ppid,    opt_ppid))  match = 0;
      else if (opt_pid    && !match_numlist(task.tgid,    opt_pid))   match = 0;
      else if (opt_pgrp   && !match_numlist(task.pgrp,    opt_pgrp))  match = 0;
      else if (opt_euid   && !match_numlist(task.euid,    opt_euid))  match = 0;
      else if (opt_ruid   && !match_numlist(task.ruid,    opt_ruid))  match = 0;
      else if (opt_rgid   && !match_numlist(task.rgid,    opt_rgid))  match = 0;
      else if (opt_sid    && !match_numlist(task.session, opt_sid))   match = 0;
      else if (opt_ns_pid && !match_ns(&task, &ns_task))              match = 0;
      else if (opt_term)
	{
	  if (task.tty == 0)
	    match = 0;
	  else
	    {
	      char tty[256];
	      dev_to_tty(tty, sizeof(tty) - 1, (dev_t)(task.tty), task.XXXID, ABBREV_DEV);
	      match = match_strlist(tty, opt_term);
	    }
	}
      if (task.cmdline && (opt_longlong || opt_full))
	{
	  int i = 0;
	  size_t bytes = sizeof(cmdline) - 1;
	  
	  /* make sure it is always NUL-terminated */
	  cmdline[bytes] = 0;
	  /* make room for SPC in loop below */
	  bytes--;
	  
	  strncpy(cmdline, task.cmdline[i], bytes);
	  bytes -= strlen(task.cmdline[i++]);
	  while (task.cmdline[i] && (bytes > 0))
	    {
	      strncat(cmdline, " ", bytes);
	      strncat(cmdline, task.cmdline[i], bytes);
	      bytes -= strlen(task.cmdline[i++]) + 1;
	    }
	}
      
      if (opt_long || opt_longlong || (match && opt_pattern))
	{
	  if (opt_longlong && task.cmdline)
	    strncpy(cmdoutput, cmdline, CMDSTRSIZE);
	  else
	    strncpy(cmdoutput, task.cmd, CMDSTRSIZE);
	}
    
    if (match && opt_pattern)
      {
	if (opt_full && task.cmdline)
	  strncpy(cmdsearch, cmdline, CMDSTRSIZE);
	else
	  strncpy(cmdsearch, task.cmd, CMDSTRSIZE);
	
	if (regexec(preg, cmdsearch, 0, NULL, 0) != 0)
	  match = 0;
      }
    
    if (match ^ opt_negate)
      {
	if (opt_newest)
	  {
	    if (saved_start_time == task.start_time &&
		saved_pid > task.XXXID)
	      continue;
	    saved_start_time = task.start_time;
	    saved_pid = task.XXXID;
	    matches = 0;
	  }
	if (opt_oldest)
	  {
	    if (saved_start_time == task.start_time &&
		saved_pid < task.XXXID)
	      continue;
	    saved_start_time = task.start_time;
	    saved_pid = task.XXXID;
	    matches = 0;
	  }
	if (matches == size)
	  {
	    size = size * 5 / 4 + 4;
	    list = xrealloc(list, size * sizeof *list);
	  }
	if (opt_long || opt_longlong || opt_echo)
	  {
	    list[matches].num = task.XXXID;
	    list[matches++].str = xstrdup(cmdoutput);
	  }
	else
	  list[matches++].num = task.XXXID;
      
	/* epkill does not need subtasks!
	 * this control is still done at
	 * argparse time, but a further
	 * control is free */
	if (opt_threads && !i_am_epkill)
	  {
	    proc_t subtask;
	    memset(&subtask, 0, sizeof(subtask));
	    while (readtask(ptp, &task, &subtask))
	      {
		/* don't add redundand tasks */
		if (task.XXXID == subtask.XXXID)
		  continue;
		
		/* eventually grow output buffer */
		if (matches == size)
		  {
		    size = size * 5 / 4 + 4;
		    list = xrealloc(list, size * sizeof(*list));
		  }
		if (opt_long)
		  list[matches].str = xstrdup(cmdoutput);
		list[matches++].num = subtask.XXXID;
		memset(&subtask, 0, sizeof(subtask));
	      }
	  }
    }
    
    memset(&task, 0, sizeof(task));
  }
  closeproc(ptp);
  *num = matches;
  return list;
}

static int signal_option(int* argc, char** argv)
{
  int i, sig;
  for (i = 1; i < *argc; i++)
    if (argv[i][0] == '-')
      {
	sig = signal_name_to_number(argv[i] + 1);
	if (sig == -1 && isdigit(argv[i][1]))
	  sig = atoi(argv[i] + 1);
	if (-1 < sig)
	  {
	    memmove(argv + i, argv + i + 1, sizeof(char *) * (size_t)(*argc - i));
	    (*argc)--;
	    return sig;
	  }
      }
  return -1;
}

static void parse_opts(int argc, char** argv)
{
  int have_criterion = 0;
  size_t n = strlen(_(" [options] <pattern>")) + strlen(*argv) + 1;
  char* usage_str = alloca(n * sizeof(char));
  char* opt;
  
  sprintf(usage_str, "%s%s", *argv, _(" [options] <pattern>"));
  
  i_am_epkill = strstr(execname, "kill") != NULL;
  
  args_init(i_am_epkill ? _("pkill with environment constraints")
	                : _("pgrep with environment constraints"),
	    usage_str, NULL, 0, 1, 0, args_standard_abbreviations);
  
  if (i_am_epkill)
    {
      int sig;
      sig = signal_option(&argc, argv);
      if (sig > -1)
	opt_signal = sig;
      args_add_option(args_new_argumentless(NULL,             0, "-e", "--echo",   NULL), _("Display what is killed"));
      args_add_option(args_new_argumented  (NULL, _("SIG"),   0,       "--signal", NULL), _("Signal to send (either number or name)"));
    }
  else
    {
      args_add_option(args_new_argumented  (NULL, _("SYM"), 0, "-d", "--delimiter",   NULL), _("Specify output delimiter"));
      args_add_option(args_new_argumentless(NULL,           0, "-l", "--list-name",   NULL), _("List PID and process name"));
      args_add_option(args_new_argumentless(NULL,           0, "-a", "--list-full",   NULL), _("List the full command line as well as the process ID"));
      args_add_option(args_new_argumentless(NULL,           0, "-v", "--inverse",     NULL), _("Negates the matching"));
      args_add_option(args_new_argumentless(NULL,           0, "-w", "--lightweight", NULL), _("List all TID"));
    }
  args_add_option(args_new_argumentless(NULL,            0, "-c", "--count",      NULL), _("Count of matching processes"));
  args_add_option(args_new_argumentless(NULL,            0, "-f", "--full",       NULL), _("Use full process name to match"));
  args_add_option(args_new_argumented  (NULL, _("PGRP"), 0, "-g", "--pgroup",     NULL), _("Match listed process group ID:s"));
  args_add_option(args_new_argumented  (NULL, _("GID"),  0, "-G", "--group",      NULL), _("Match real group ID:s"));
  args_add_option(args_new_argumentless(NULL,            0, "-n", "--newest",     NULL), _("Select most recently started"));
  args_add_option(args_new_argumentless(NULL,            0, "-o", "--oldest",     NULL), _("Select least recently started"));
  args_add_option(args_new_argumented  (NULL, _("PPID"), 0, "-P", "--parent",     NULL), _("Match only child processes of the given parent"));
  args_add_option(args_new_argumented  (NULL, _("SID"),  0, "-s", "--session",    NULL), _("Match session ID:s"));
  args_add_option(args_new_argumented  (NULL, _("TERM"), 0, "-t", "--terminal",   NULL), _("Match by controlling terminal"));
  args_add_option(args_new_argumented  (NULL, _("EUID"), 0, "-u", "--euid",       NULL), _("Match by effective ID:s"));
  args_add_option(args_new_argumented  (NULL, _("UID"),  0, "-U", "--uid",        NULL), _("Match by real ID:s"));
  args_add_option(args_new_argumentless(NULL,            0, "-x", "--exact",      NULL), _("Match exactly with the command name"));
  args_add_option(args_new_argumented  (NULL, _("FILE"), 0, "-F", "--pidfile",    NULL), _("Read PID:s from file"));
  args_add_option(args_new_argumentless(NULL,            0, "-L", "--logpidfile", NULL), _("Fail if PID file is not locked"));
  args_add_option(args_new_argumented  (NULL, _("PID"),  0,       "--ns",         NULL), _("Match the processes that belong to the same namespace as <PID>"));
  args_add_option(args_new_argumented  (NULL, _("NAME"), 0,       "--nslist",     NULL), _("List which namespaces will be considered for the --ns option\n"
											   "Available namespaces: ipc, mnt, net, pid, user, uts"));
  args_add_option(args_new_argumentless(NULL,            0, "-h", "--help",       NULL), _("Display this help information"));
  args_add_option(args_new_argumentless(NULL,            0, "-V", "--version",    NULL), _("Print the name and version of this program"));
  
  args_parse(argc, argv);
  
  if (args_unrecognised_count || args_opts_used("-h"))  args_help();
  else if (args_opts_used("-V"))                        printf("%s %s", (i_am_epkill ? "epkill" : "epgrep"), VERSION);
  else                                                  goto cont;
  exit(args_unrecognised_count ? EXIT_FAILURE : EXIT_SUCCESS);
  return;
 cont:
  
#define univ_opts_used(ARG)  (opt = ARG, args_opts_used(ARG))
#define kill_opts_used(ARG)  ( i_am_epkill && univ_opts_used(ARG))
#define grep_opts_used(ARG)  (!i_am_epkill && univ_opts_used(ARG))
#define criteria             have_criterion = 1
#define optarg               (args_opts_get(opt)[0])
  
  if (kill_opts_used("-e"))  opt_echo = 1;
  if (univ_opts_used("-F"))  criteria, opt_pidfile = xstrdup(optarg);
  if (univ_opts_used("-G"))  criteria, opt_rgid = split_list(optarg, conv_gid);
  if (univ_opts_used("-L"))  opt_lock = 1;
  if (univ_opts_used("-P"))  criteria, opt_ppid = split_list(optarg, conv_num);
  if (univ_opts_used("-U"))  criteria, opt_ruid = split_list(optarg, conv_uid);
  if (univ_opts_used("-c"))  opt_count = 1;
  if (grep_opts_used("-d"))  opt_delim = xstrdup(optarg);
  if (univ_opts_used("-f"))  opt_full = 1;
  if (univ_opts_used("-g"))  criteria, opt_pgrp = split_list(optarg, conv_pgrp);
  if (grep_opts_used("-l"))  opt_long = 1;
  if (grep_opts_used("-a"))  opt_longlong = 1;
  if (univ_opts_used("-n"))  criteria, opt_newest = 1;
  if (univ_opts_used("-o"))  criteria, opt_oldest = 1;
  if (univ_opts_used("-s"))  criteria, opt_sid = split_list(optarg, conv_sid);
  if (univ_opts_used("-t"))  criteria, opt_term = split_list(optarg, conv_str);
  if (univ_opts_used("-u"))  criteria, opt_euid = split_list(optarg, conv_uid);
  if (grep_opts_used("-v"))  criteria, opt_negate = 1;
  if (grep_opts_used("-w"))  opt_threads = 1;
  if (univ_opts_used("-x"))  opt_exact = 1;
  
  if (univ_opts_used("--ns"))
    if (criteria, opt_ns_pid = atoi(optarg), opt_ns_pid == 0)
      {
	args_help();
	exit(EXIT_FAILURE);
      }
  
  if (univ_opts_used("--nslist"))
    opt_nslist = split_list(optarg, conv_ns);
  
  if (kill_opts_used("--signal"))
    {
      opt_signal = signal_name_to_number(optarg);
      if (opt_signal == -1 && isdigit(optarg[0]))
	opt_signal = atoi(optarg);
    }
  
#undef criteria
#undef grep_opts_used
#undef kill_opts_used
  
  if (opt_oldest + opt_negate + opt_newest > 1)
    xerror(_("-v, -n and -o are mutually exclusive options."));
  
  if (opt_lock && !opt_pidfile)
    xerror(_("%s: -L requires -F."));
  
  if (opt_pidfile)
    if (opt_pid = read_pidfile(), !opt_pid)
      xerror(_("%s: pidfile not valid."));
  
  
  if      (args_files_count == 1)  opt_pattern = *args_files;
  else if (args_files_count > 1)   xerror(_("Only one pattern can be provided."));
  else if (have_criterion == 0)    xerror(_("No matching criteria specified."));
}


static void cleanup(void)
{
  args_dispose();
}


int main(int argc, char** argv)
{
  struct el* procs;
  int num;
  
  execname = *argv;
  
  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);
  atexit(cleanup);
  
  parse_opts(argc, argv);
  
  procs = select_procs(&num);
  if (i_am_epkill)
    {
      int i;
      for (i = 0; i < num; i++)
	{
	  if (kill((pid_t)(procs[i].num), opt_signal) != -1)
	    {
	      if (opt_echo)
		printf(_("%s killed (pid %lu)\n"), procs[i].str, procs[i].num);
	      continue;
	    }
	  if (errno == ESRCH)
	    continue; /* gone now, which is OK */
	  fprintf(stderr, _("%s: Killing PID %ld failed."), execname, procs[i].num);
	}
      if (opt_count)
	fprintf(stdout, "%d\n", num);
    }
  else
    if (opt_count)
      fprintf(stdout, "%d\n", num);
    else
      if (opt_long || opt_longlong)
	output_strlist(procs,num);
      else
	output_numlist(procs,num);
  
  return !num;
}

