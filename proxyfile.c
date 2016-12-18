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

/**
 * new_proxyfile_list - creates a new proxyfile list
 *
 * Returns a (proxyfile_list *) pointer pointing to the newly created
 * proxyfile_list object.
 */
proxyfile_list *new_proxyfile_list()
{
    proxyfile_list *retval = (proxyfile_list *)malloc(sizeof(proxyfile_list));

    retval->head = NULL;
    retval->tail = NULL;
    retval->used = 0;

    return retval;
}

/**
 * new_proxyfile - create a new proxyfile and append it to the list
 * @list:      the proxyfile_list
 * @file_path: path to the file to be added
 *
 * Returns a (proxyfile *) pointer pointing to the newly created proxyfile
 * object.
 */
proxyfile *new_proxyfile(proxyfile_list *list, char *file_path)
{
    if(list->head == NULL) { /* first proxyfile */
        list->head = (proxyfile *)malloc(sizeof(proxyfile));
        list->tail = list->head;
        list->tail->next = NULL;
        list->tail->prev = NULL;
    }
    else { /* append to existing linked list */
        proxyfile *next = (proxyfile *)malloc(sizeof(proxyfile));
        next->prev = list->tail;
        next->next = NULL;
        list->tail = next;
    }

    proxyfile *cur = list->tail;
    cur->file_path = file_path; /* no need to copy char-by-char */

    cur->md5 = md5sum(file_path); /* just assign the already assigned addr */

    cur->proxy_path = (char *)malloc((list->PROXY_FILE_LEN + 1)*sizeof(char));
    strcpy(cur->proxy_path, list->SANDBOX_DIR);
    strcat(cur->proxy_path, cur->md5);

    list->used++;

    return cur;
}

/**
 * search_proxyfile - search for a proxyfile
 * @list:      the proxyfile_list
 * @file_path: the file path
 *
 * Returns a (proxyfile *) pointer.
 */
proxyfile *search_proxyfile(proxyfile_list *list, char *file_path) {
    char *md5 = md5sum(file_path);
    
    proxyfile *cur = list->head;
    while(cur != NULL) {
        if(strcmp(cur->md5, md5) == 0)
            return cur;
        cur = cur->next;
    }

    return NULL;
}

/**
 * delete_proxyfile - remove a proxyfile from the proxyfile_list
 * @list: the proxyfile_list
 * @pf:   the proxyfile
 */
void delete_proxyfile(proxyfile_list *list, proxyfile *pf) {
    if(pf->prev)
        ((proxyfile *)pf->prev)->next = pf->next;
    else /* only head has this property */
        list->head = pf->next;

    if(pf->next)
        ((proxyfile *)pf->next)->prev = pf->prev;

    free(pf->file_path);
    free(pf->md5);
    free(pf->proxy_path);
    free(pf);
}

/**
 * print_map - print the internal file map
 * @list:     the proxyfile_list
 * @log_file: a (FILE *) pointer to write to
 *
 * Prints each MD5 and its real file path.  This is done only when the -m
 * arg is passed to FSSB.
 */
void print_map(proxyfile_list *list, FILE *log_file) {
    proxyfile *cur = list->head;
    while(cur != NULL) {
        fprintf(log_file, "    + %s = %s\n", cur->md5, cur->file_path);
        cur = cur->next;
    }
}

/**
 * write_map - writes the proxy file map list to a file
 * @list:        the proxfile_list
 * @SANDBOX_DIR: the sandbox directory
 *
 * Writes the list of proxy files and their actual file maps.
 */
extern void write_map(proxyfile_list *list, char *SANDBOX_DIR)
{
    char proxyfile_map[256];
    strcpy(proxyfile_map, SANDBOX_DIR);
    strcat(proxyfile_map, "file-map");
    FILE *pfm = fopen(proxyfile_map, "w");

    proxyfile *cur = list->head;
    while(cur != NULL) {
        fprintf(pfm, "%s%s = %s\n", SANDBOX_DIR, cur->md5, cur->file_path);
        cur = cur->next;
    }

    fclose(pfm);
}

/**
 * remove_proxy_files - delete all proxy files in the sandbox directory
 * @list: the proxyfile_list
 */
void remove_proxy_files(proxyfile_list *list)
{
    proxyfile *cur = list->head;
    while(cur != NULL) {
        remove(cur->proxy_path);
        cur = cur->next;
    }
}
