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
#ifndef EPKILL_ENVIRONMENT_H
#define EPKILL_ENVIRONMENT_H

#include <sys/types.h>


static char* const environment_synopsis = 0;


extern size_t environment_count;
void environment_dispose(void);
void environment_parse(int* argc, char** argv);
int environment_test(pid_t pid, char* execname);


#endif

