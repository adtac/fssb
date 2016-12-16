/**
 * arguments.h - Argument handling.  Part of the FSSB project.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "arguments.h"

#define INIT_HELP_ALLOC 8

/**
 * insert_help - inserts a line of help into the list
 * @arg:  the argument
 * @desc: help text that's supposed to accompany the argument
 */
void insert_help(char *arg, char *desc, int num_vals) {
    if(help_list_count >= help_list_allocated) {
        help_list_allocated *= 2;
        help_list = (help *)realloc(help_list, help_list_allocated);
    }
    strcpy(help_list[help_list_count].arg, arg);
    strcpy(help_list[help_list_count].desc, desc);
    help_list[help_list_count].num_vals = num_vals;
    help_list_count++;
}

/**
 * build_help - builds the list of arguments and descriptions.
 */
void build_help() {
    help_list_count = 0;

    help_list = (help *)malloc(sizeof(help)*INIT_HELP_ALLOC);
    help_list_allocated = INIT_HELP_ALLOC;

    insert_help("-h", "show this help and exit", 0);
    insert_help("-r", "remove all temporary files at the end", 0);
    insert_help("-o", "logging output file (stderr by default)", 1);
    insert_help("-d", "debug output file (off by default)", 1);
    insert_help("-m", "print file to proxyfile map at the end", 0);
}

/**
 * check_args_validity - ensure all args are recognized
 * @argc: number of args given to the tracer
 * @argv: argument list
 *
 * Exits with 1 if there is/are unrecognized argument(s).
 */
void check_args_validity(int argc, char **argv)
{
    int i, j;

    for(i = 1; i < argc; i++) {
        /* it's the child's arguments from here on */
        if(strcmp(argv[i], "--") == 0)
            break;

        int recognized = 0;
        for(j = 0; j < help_list_count; j++) {
            if(strcmp(argv[i], help_list[j].arg) == 0) {
                recognized = 1;
                i += help_list[j].num_vals;
            }
        }
        if(!recognized) {
            fprintf(stderr, "fssb: error: invalid option '%s'\n\n", argv[i]);
            print_help();
            exit(1);
        }
    }
}

/**
 * help_requested - determine if the help text is requested by the user
 * @argc: number of args given to the tracer
 * @argv: argument list
 *
 * Returns 1 if help is requested, 0 otherwise.
 */
int help_requested(int argc, char **argv)
{
    int i, other_args = 0, show_help = 0;

    for(i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-h") == 0)
            show_help = 1;
        else
            other_args = 1;
    }

    if(!show_help)
        return 0;

    if(other_args)
        fprintf(stderr, "`-h` must be the only argument if it is used.\n\n");

    return 1;
}

/* comp function for qsort */
int comp(const void *a, const void *b) {
    return strcmp(((help *)a)->arg, ((help *)b)->arg);
}

/**
 * print_help - print the help manual
 */
void print_help() {
    fprintf(stdout, "Usage: fssb [OPTIONS] -- COMMAND\n");
    fprintf(stdout, "\n\
FSSB is a filesystem sandbox for Linux. It's useful if you want to run a\n\
program but also protect your files and directories from modification.\n\n");

    /* sort help_list by argument - heavily helps readability */
    qsort(help_list, help_list_count, sizeof(help), comp);

    int i;
    for(i = 0; i < help_list_count; i++) {
        fprintf(stdout, "  %s", help_list[i].arg);
        for(int j = 0; j < help_list[i].num_vals; j++)
            fprintf(stdout, " ARG");
        for(int j = 0; j < 15 - 4*help_list[i].num_vals; j++)
            fprintf(stdout, " ");
        fprintf(stdout, "%s\n", help_list[i].desc);
    }

    fprintf(stdout, "\n\
You can find a more complete at https://github.com/adtac/fssb\n");
}

/**
 * get_log_file_obj - returns a file object to write to
 * @argc: number of arguments
 * @argv: argument list
 * @i:    index of the flag
 *
 * Note: this logs to stderr and exits with an error code 1 if the given
 * file does not exist or if there are insufficient number of arguments.
 *
 * Returns a (FILE *) object that can be used.
 */
FILE *get_log_file_obj(int argc, char **argv, int i)
{
    FILE *retval;

    struct stat sb;
    if(i == argc - 1) {
        fprintf(stderr, "fssb: error: no logging file specified\n");
        exit(1);
    }

    retval = fopen(argv[i + 1], "w");
    if(retval == NULL) {
        fprintf(stderr, "fssb: error: cannot create log file %s\n",
                        argv[i + 1]);
        exit(1);
    }

    return retval;
}

/**
 * set_parameters - reads the command line arguments and sets the values
 * @argc:     number of args given to the tracer
 * @argv:     argument list
 * @cleanup:  whether to cleanup all temp files at exit
 * @log_file: file to log all output to
 */
void set_parameters(int argc,
                    char **argv,
                    int *cleanup,
                    FILE **log_file,
                    FILE **debug_file,
                    int *print_map)
{
    /* default values */
    *cleanup = 0;
    *log_file = stdout;
    *debug_file = fopen("/dev/null", "w");
    *print_map = 0;

    int i;
    for(i = 0; i < argc; i++) {
        if(strcmp(argv[i], "-r") == 0)
            *cleanup = 1;

        if(strcmp(argv[i], "-m") == 0)
            *print_map = 1;

        if(strcmp(argv[i], "-d") == 0) {
            fclose(*debug_file);
            *debug_file = get_log_file_obj(argc, argv, i);
            i++;
        }

        if(strcmp(argv[i], "-o") == 0) {
            *log_file = get_log_file_obj(argc, argv, i);
            i++;
        }
    }
}

/**
 * get_child_args_start_pos - get the position where the child args start
 * @argc: number of args given to the tracer
 * @argv: argument list
 *
 * Returns the point in the array where the child arguments begin.
 */
int get_child_args_start_pos(int argc, char **argv)
{
    int pos = 0;

    while(pos < argc) {
        if(strcmp(argv[pos], "--") == 0)
            break;
        pos++;
    }

    if(pos == argc) {
        fprintf(stderr, "fssb: error: no `--` found in arguments\n");
        fprintf(stderr, "usage: fssb -- <program> <args>\n");
        exit(1);
    }

    if(pos == argc - 1) {
        fprintf(stderr, "fssb: error: nothing found after `--`\n");
        fprintf(stderr, "usage: fssb -- <program> <args>\n");
        exit(1);
    }

    return pos + 1;
}
