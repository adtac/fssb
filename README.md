# FSSB - Filesystem Sandbox for Linux

## What is FSSB?

**FSSB** is a sandbox for your filesystem. With it, you can run any program
and be assured that none of your files are modified in any way. However, the
program will not know this - every change it attempts to make will be made
safely in a sandbox while still allowing it to read existing files. This
includes creating, modifying, renaming, and deleting files.

The applications are endless:

 * Run arbitrary binaries that you don't trust safely (maybe you downloaded it from the internet).
 * FSSB provides a safe, repeatable environment for programs. This can be useful for debugging programs.
 * Make dry runs to see how a program behaves keeping your files intact. You can see exactly what changes the program made.

Please note that FSSB is still in alpha. Check out the `Contributing` section
if you'd like to contribute.

## Installation

FSSB is a very lightweight application. It doesn't require too many
dependencies. On most systems, you only need to install the `openssl`
C library. And then, you can run:

```bash
$ make
```

to generate a binary `fssb`.

## Usage

FSSB is designed with simplicity in mind. Just do:

```bash
$ ./fssb -- <program> <args>
```

to run the program in a safe, dependable, sandboxed environment.

For example, say we have a Python program `program.py`:

```py
with open("new_file", "w") as f:  # create a new file in the current directory
    f.write("Hello world!")       # write something

with open("new_file", "r") as f:  # read the same file later on
    print(f.read())               # print the contents to console
```

Normally, running `python program.py` would result in a file:

```bash
$ python program.py
Hello world!
```

This, of course, would have created a file:

```
$ cat new_file
Hello world!
```

However, if you run it wrapped around FSSB:

```bash
$ ./fssb -m -- python program.py
Hello world!
fssb: child exited with 0
fssb: sandbox directory: /tmp/fssb-1
    + 25fa8325e4e0eb8180445e42558e60bd = new_file
```

you'll see that there's no file created:

```bash
$ cat new_file
cat: new_file: No such file or directory
```

Instead, the file is actually created in a sandbox:

```bash
$ cat /tmp/fssb-1/25fa8325e4e0eb8180445e42558e60bd
Hello world!
```

And the best part is, the running child program doesn't even know about it!

You can run `./fssb -h` to see more options.

## Neat. How does this work?

In Linux, every program's every operation (well, not every operation; most)
is actually made through something called a system call - or syscall for
short. The `open` command in Python is actually a `fopen` command written
in C a layer below, which is actually a syscall called `open`
(this is wrapped by `glibc`).

FSSB *intercepts* these syscalls before they are actually performed. For example
just before the `open` syscall is performed, the program is stopped. Now, each of
these syscalls have arguments. For example, a `fopen("new_file", "w")` in C
may actually look like:

```
open("new_file", O_WRONLY|O_CREAT|O_TRUNC, 0666) = 3
```

The first argument is the filename, the second are the flags, the third is
the mode. To learn more about these, run `man -s 2 open`.

Each syscall can have at most 6 arguments - these map to six CPU registers,
each of which hold a value. In the above example, just before the
`open` syscall is executed, the first register holds a memory address pointing
to a string `"new_file"`.

Anyway, just before the syscall is executed, FSSB *switches* the string to
something else - to a filename in the sandbox. And then lets the syscall go on.
To the CPU, you basically asked to create a file in the sandbox. So it does.

Then just after the syscall finishes (after the file is created), FSSB switches the
value to its original value. Now, syscalls are pretty low-level. And we're operating
on either sides of a syscall - just before and after its execution. So the program
has no way of knowing that the file was actually created in the sandbox. Every
subsequent operation is actually performed on the sandbox file.

## Contributing to the FSSB project

This is still in its alpha stage. There are so many syscalls - at the moment, there's
support for sandboxing of:

* creating new files
* modifying existing files
* deleting files
* renaming files
* reading files

There's still a lot of stuff to do. And I'd really appreciate help over here.
I've tried to make the code very readable with looots of comments and
documentation for what each thing does.

And of course, I've only implemented this for my x86_64 linux system. I'd
greatly appreciate any help if someone could make this portable to other archs
(please take a look at the `syscalls.h` file for this).

## License

```
    FSSB - Filesystem Sandbox for Linux
    Copyright (C) 2016 Adhityaa Chandrasekar

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
```

See the [LICENSE](LICENSE) file for more details.
