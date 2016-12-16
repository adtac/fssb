/**
 * proxyfile.h - Proxy file operations.  Part of the FSSB project.
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

#ifndef _PROXYFILE_H
#define _PROXYFILE_H

#include <stdio.h>

typedef struct {
    void *next, *prev;
    char *file_path, *md5, *proxy_path;
    int fd;
} proxyfile;

typedef struct {
    proxyfile *head, *tail;
    int used;
    int PROXY_FILE_LEN;
    char *SANDBOX_DIR;
} proxyfile_list;

extern proxyfile_list *new_proxyfile_list();

extern proxyfile *new_proxyfile(proxyfile_list *list, char *file_path);

extern proxyfile *search_proxyfile(proxyfile_list *list, char *file_path);

extern void print_map(proxyfile_list *list, FILE *log_file);

#endif /* _PROXYFILE_H */
