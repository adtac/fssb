/**
 * utils.c - Defines for various utlities.  Part of the FSSB project.
 *
 * Copyright (C) 2016 Adhityaa Chandrasekar
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _UTILS_H
#define _UTILS_H

#include <sys/types.h>
#include <sys/user.h>
#include <sys/ptrace.h>

/* A hack for x86_64 systems. */
#ifdef __amd64__
#define eax rax
#define orig_eax orig_rax
#endif

/* Get the offset of `field` in struct `str`. */
#ifndef offsetof
#define offsetof(str, field) __builtin_offsetof(str, field)
#endif

/* Return the register `reg` of `child`. */
#ifndef get_reg
#define get_reg(child, reg) ptrace(PTRACE_PEEKUSER, \
                                   child, \
                                   offsetof(struct user, regs.reg))
#endif

extern int syscall_breakpoint(pid_t child);

extern long get_syscall_arg(pid_t child, int n);

extern char *get_string(pid_t child, unsigned long addr);

extern unsigned char *write_string(pid_t child,
                                   unsigned long addr,
                                   char *str,
                                   int *overwritten_bytes);

extern void write_bytes(pid_t child,
                        unsigned long addr,
                        unsigned char *bytes,
                        int n);

extern char *md5sum(char *str);

extern char *proxy_path(char *prefix, char *file_path);

#endif /* _UTILS_H */
