/**
 * epkill — procps utilties with environment constraints
 * 
 * Copyright © 2014, 2015  Mattias Andrée (m@maandree.se)
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
#ifndef EPKILL_ENVIRONMENT_H
#define EPKILL_ENVIRONMENT_H

#include <sys/types.h>


static char* const environment_synopsis =
  "    To restrict the found processes to those that have the environment\n"
  "    variable \033[4mVAR\033[m set to \033[4mVAL\033[m, add the option \033[4mVAR=VAL\033[m. \033[4mVAL\033[m can be empty,\n"
  "    but \033[4mVAR\033[m cannot be empty. This means the \033[4mVAR=\033[m will not pass if \033[4mVAR\033[m\n"
  "    is defined, it will only pass if the value of \033[4mVAR\033[m is an empty string.\n"
  "    To select process with the same value for \033[4mVAR\033[m as the current process,\n"
  "    you can use the shorthand \033[4m=VAR\033[m. The select processes that have \033[4mVAR\033[m\n"
  "    defined, possibly empty, use \033[4m+=VAR\033[m. You can prefix any of these\n"
  "    selectors with an exclamation point, to require the it does not hold\n"
  "    true. For example, \033[4m!VAR=\033[m will pass if \033[4mVAR\033[m is not defined or is \n"
  "    non-empty. You specify multiple constraints, a process will only be\n"
  "    selected if all constraints are met. For example, \033[4m+=VAR !VAR=\033[m will\n"
  "    pass only if \033[4mVAR\033[m is defined and is not empty. If required, you can\n"
  "    prefix the argument with an at-sign to force it to be parsed as an\n"
  "    environment constraint if it would otherwise start with a hyphen.";


extern size_t environment_count;
void environment_dispose(void);
void environment_parse(int* argc, char** argv);
int environment_test(pid_t pid, char* execname);


#endif

