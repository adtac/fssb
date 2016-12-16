/**
 * utils.c - Utility functions.  Part of the FSSB project.
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

#include <unistd.h>
#include <string.h>
#include <wait.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <openssl/md5.h>

#include "utils.h"

/**
 * syscall_breakpoint - break at the entry or exit of a syscall
 * @child: PID of the child process
 *
 * This will stop the process until called again.
 *
 * Return 0 if the child has been stopped, 1 if it has exited.
 */
int syscall_breakpoint(pid_t child)
{
    int status;

    while(1) {
        ptrace(PTRACE_SYSCALL, child, 0, 0);
        waitpid(child, &status, 0);

        if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80)
            return 0;
        if (WIFEXITED(status))
            return 1;
    }
}

/**
 * get_syscall_arg - get the nth argument of the syscall.
 * @child: PID of the child process
 * @n:     which argument (max 6 args for any syscall)
 *
 * Returns the long corresponding the argument.  If this is a string, a pointer
 * to it is returned.  If n is greater than 6, -1L is returned.
 */
long get_syscall_arg(pid_t child, int n)
{
    switch(n) {
#ifdef __amd64__
    /* x86_64 has {rdi, rsi, rdx, r10, r8, r9} */
    case 0: return get_reg(child, rdi);
    case 1: return get_reg(child, rsi);
    case 2: return get_reg(child, rdx);
    case 3: return get_reg(child, r10);
    case 4: return get_reg(child, r8);
    case 5: return get_reg(child, r9);
#else
    /* x86 has {ebx, ecx, edx, esi, edi, ebp} */
    case 0: return get_reg(child, ebx);
    case 1: return get_reg(child, ecx);
    case 2: return get_reg(child, edx);
    case 3: return get_reg(child, esi);
    case 4: return get_reg(child, edi);
    case 5: return get_reg(child, ebp);
#endif
    default: return -1L;
    }
}

/**
 * get_string - returns the string at the given address of the child process
 * @child: PID of the child process
 * @addr:  memory address location
 *
 * Note: the string has to be null-terminated.
 *
 * Returns a (char *) pointer.
 */
char *get_string(pid_t child, unsigned long addr)
{
    char *str = (char *)malloc(1024);
    int alloc = 1024, copied = 0;
    unsigned long word;

    while(1) {
        if(copied + sizeof(word) > alloc) {  /* too big (that's what she said) */
            alloc *= 2;
            str = (char *)realloc(str, alloc);
        }

        word = ptrace(PTRACE_PEEKDATA, child, addr + copied);
        if(errno) {
            str[copied] = 0;
            break;
        }
        memcpy(str + copied, &word, sizeof(word));

        /* If we've already encountered null, break and return */
        if(memchr(&word, 0, sizeof(word)) != NULL)
            break;

        copied += sizeof(word);
    }

    return str;
}

/**
 * write_string - writes a string to the given address of the child process
 * @child:            PID of the child process
 * @addr:             memory address location
 * @str:              string to be written
 * @overwritten_size: stores the number of bytes overwritten
 *
 * Returns a pointer to a memory address of bytes that needs to be restored.
 */
unsigned char *write_string(pid_t child,
                            unsigned long addr,
                            char *str,
                            int *overwritten_size)
{
    unsigned char *original_bytes = malloc(1024);
    int alloc = 1024;
    *overwritten_size = 0;

    while(1) {
        unsigned long word;

        word = ptrace(PTRACE_PEEKDATA, child, addr + *overwritten_size);
        if(*overwritten_size + sizeof(word) > alloc) {
            alloc *= 2;
            original_bytes = (unsigned char *)realloc(original_bytes, alloc);
        }
        memcpy(original_bytes + *overwritten_size, &word, sizeof(word));

        memcpy(&word, str + *overwritten_size, sizeof(word));
        ptrace(PTRACE_POKEDATA, child, addr + *overwritten_size, word);

        *overwritten_size += sizeof(word);

        if(memchr(&word, 0, sizeof(word)) != NULL)
            break;
    }

    return original_bytes;
}

/**
 * write_bytes - write n bytes to the given memory address
 * @child: PID of the child process
 * @addr:  memory address location
 * @bytes: bytes to be written
 * @n:     stores the number of bytes overwritten
 */
void write_bytes(pid_t child,
                 unsigned long addr,
                 unsigned char *bytes,
                 int n) {
    int cur;
    for(cur = 0; cur < n; ) {
        unsigned long word;
        memcpy(&word, bytes + cur, sizeof(word));
        ptrace(PTRACE_POKETEXT, child, addr + cur, word);
        cur += sizeof(word);
    }
}

/**
 * md5sum - return a MD5 hash of the given string
 * @str: the string
 *
 * Returns a 32-character string containing the MD5 hash.  Remember to free
 * this after use.
 */
char *md5sum(char *str)
{
    char *retval = (char *)malloc(33);

    unsigned char d[17];
    MD5(str, strlen(str), d);

    for(int i = 0; i < 16; i++) {
        unsigned char x = d[i] >> 4;
        if(x < 10)
            retval[2*i] = '0' + x;
        else
            retval[2*i] = 'a' + x - 10;

        x = d[i] & 0xf;
        if(x < 10)
            retval[2*i+1] = '0' + x;
        else
            retval[2*i+1] = 'a' + x - 10;
    }

    retval[32] = 0;

    return retval;
}

/**
 * proxy_path - returns a string containing the proxy path
 * @prefix:    prefix to the MD5 sum
 * @file_path: path of the original file
 *
 * Returns a (char *) pointer.  Remember to free this at the end.
 */
char *proxy_path(char *prefix, char *file_path)
{
    /* 32 for the MD5 hash, 1 for the null at the end */
    int malloc_len = strlen(prefix) + 32 + 1;
    char *retval = (char *)malloc(malloc_len*sizeof(char));
    *retval = 0;

    strcpy(retval, prefix);
    char *md5 = md5sum(file_path);
    strcat(retval, md5);
    free(md5);

    return retval;
}
