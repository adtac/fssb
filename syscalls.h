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

#define SC_READ       0
#define SC_WRITE      1
#define SC_OPEN       2
#define SC_CLOSE      3
#define SC_STAT       4
#define SC_LSTAT      6
#define SC_ACCESS     21
#define SC_EXECVE     59
#define SC_EXIT       60
#define SC_RENAME     82
#define SC_CREAT      85
#define SC_UNLINK     87
#define SC_EXIT_GROUP 231
#define SC_UNLINKAT   263
