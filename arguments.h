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

#ifndef _ARGUMENT_H
#define _ARGUMENT_H

typedef struct {
    char arg[3], desc[128];
    int num_vals;
} help;

extern void build_help();

extern void check_args_validity(int argc, char **argv);

extern int help_requested(int argc, char **argv);

extern void print_help();

extern void set_parameters(int argc,
                           char **argv,
                           int *cleanup,
                           FILE **log_file,
                           FILE **debug_file,
                           int *print_map);

extern int get_child_args_start_pos(int argc, char **argv);

help *help_list;
int help_list_count, help_list_allocated;

#endif /* _ARGUMENT_H */
