/**
 * epkill — procps utilties with environment constraints
 * 
 * Copyright © 2014  Mattias Andrée (maandree@member.fsf.org)
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
#include "environment.h"

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>


typedef struct constraint
{
  int not;
  char* name;
  char* value;
  size_t met;
} constraint_t;



size_t environment_count = 0;
static constraint_t* constraints = NULL;
static size_t test_id = 0;



void environment_dispose(void)
{
  free(constraints);
}


void environment_parse(int* argc, char** argv)
{
  /*
   * =ENV      same as here
   * !=ENV     different then from here
   * ENV=VAL   is VAL
   * !ENV=VAL  is not VAL
   * +=ENV     have, possiblity empty
   * !+=ENV    do not have, fails if empty
   */
  
  int i, n;
  for (i = 1, n = *argc; i < n; i++)
    {
      char* arg = argv[i];
      int not   = 0;
      int have  = 0;
      int eq    = 0;
      char* var;
      constraint_t* old;
      constraint_t constraint;
      
      if (!strcmp(arg, "--"))
	break;
      if (!strcmp(arg, "++"))
	{
	  i++;
	  continue;
	}
      
      if (strchr(arg, '=') == NULL)
	continue;
      
      if (*arg == '!')  not  = 1, arg++;
      if (*arg == '+')  have = 1, arg++;
      if (*arg == '=')  eq   = 1, arg++;
      
      if (have && !eq)
	continue;
      
      var = arg;
      if (eq)
	arg = have ? NULL : getenv(var);
      else
	{
	  arg = strchr(arg, '=');
	  *arg = '\0';
	  arg++;
	}
      
      old = constraints;
      constraints = realloc(old, ++environment_count * sizeof(constraint_t));
      if (constraints == NULL)
	{
	  perror(*argv);
	  free(old);
	  exit(EXIT_FAILURE);
	  return;
	}
      
      constraint.met = 0;
      constraint.not = not;
      constraint.name = var;
      constraint.value = arg;
      constraints[environment_count - 1] = constraint;
      
      memmove(argv + i, argv + i + 1, (size_t)(--n - i) * sizeof(char*));
    }
  *argc = n;
}


int environment_test(pid_t pid, char* execname)
{
  char pathname[50];
  char* buf = NULL;
  size_t size = 0;
  size_t have = 0;
  size_t got;
  char* old;
  FILE* f;
  size_t i;
  
  test_id++;
  
  sprintf(pathname, "/proc/%ld/environ", (long)pid);
  f = fopen(pathname, "r");
  
  if (f == NULL)
    {
      if ((errno == EACCES) || (errno == EEXIST))
	return 0;
      perror(execname);
      exit(EXIT_FAILURE);
    }
  
  for (;;)
    {
      if (size - have < 4096)
	if ((buf = realloc(old = buf, (size += 8192) * sizeof(char))) == NULL)
	  {
	    perror(execname);
	    fclose(f);
	    free(old);
	    exit(EXIT_FAILURE);
	    return 0;
	  }
      got = fread(buf, sizeof(char), size - have, f);
      if (got < size - have)
	{
	  have += got;
	  if (feof(f))
	    break;
	  else if (errno != EINTR)
	    {
	      perror(execname);
	      fclose(f);
	      free(buf);
	      exit(EXIT_FAILURE);
	      return 0;
	    }
	}
      else
	have += got;
    }
  fclose(f);
  
  old = buf;
  while (have > 0)
    {
      size_t len = strlen(buf);
      char* value = strchr(buf, '=');
      
      if (value == NULL)
	goto corrupt_environent;
      
      *value++ = '\0';
      
      for (i = 0; i < environment_count; i++)
	{
	  if (strcmp(constraints[i].name, buf))
	    continue;
	  
	  if (constraints[i].value)
	    if (strcmp(constraints[i].value, value))
	      continue;
	  
	  constraints[i].met = test_id;
	}
      
    corrupt_environent:
      have -= len + 1;
      buf += len + 1;
    }
  
  free(old);
  
  for (i = 0; i < environment_count; i++)
    if ((constraints[i].met == test_id) == constraints[i].not)
      return 0;
  
  return 1;
}

