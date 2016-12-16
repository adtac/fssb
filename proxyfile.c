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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "proxyfile.h"
#include "utils.h"

#define PROXYFILE_LIST_ALLOC 128

proxyfile_list *new_proxyfile_list()
{
    proxyfile_list *retval = (proxyfile_list *)malloc(sizeof(proxyfile_list));
    retval->list = (proxyfile *)malloc(PROXYFILE_LIST_ALLOC*sizeof(proxyfile));
    retval->alloc = PROXYFILE_LIST_ALLOC;
    retval->used = 0;

    return retval;
}

proxyfile *new_proxyfile(proxyfile_list *list, char *file_path)
{
    if(list->used + 1 > list->alloc) {
        list->alloc *= 2;
        list->list = (proxyfile *)realloc(list->list,
                                          list->alloc*sizeof(proxyfile));
    }

    proxyfile *cur = list->list + list->used;

    cur->file_path = file_path; /* no need to copy char-by-char */

    cur->md5 = md5sum(file_path); /* just assign the already assigned addr */

    cur->proxy_path = (char *)malloc((list->PROXY_FILE_LEN + 1)*sizeof(char));
    strcpy(cur->proxy_path, list->SANDBOX_DIR);
    strcat(cur->proxy_path, cur->md5);

    list->used++;

    return cur;
}

proxyfile *search_proxyfile(proxyfile_list *list, char *file_path) {
    char *md5 = md5sum(file_path);
    
    int i;
    for(i = 0; i < list->used; i++) {
        if(strcmp((list->list + i)->md5, md5) == 0)
            return list->list + i;
    }

    return NULL;
}

void print_map(proxyfile_list *list, FILE *log_file) {
    fprintf(log_file, "\n");
    fprintf(log_file, "==============\n");
    fprintf(log_file, " File mapping \n");
    fprintf(log_file, "==============\n");
    fprintf(log_file, "\n");

    int i;
    for(i = 0; i < list->used; i++) {
        proxyfile *cur = list->list + i;
        fprintf(log_file, "%s = %s\n", cur->md5, cur->file_path);
    }
}
