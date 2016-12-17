#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <bits/types.h>
#include <openssl/md5.h>

#include "syscalls.h"
#include "proxyfile.h"
#include "arguments.h"
#include "utils.h"

#define RDONLY_MEM_WRITE_SIZE 256

long write_slots[6];

/* Hopefully we don't need a 90-digit number. */
char SANDBOX_DIR[100];
int PROXY_FILE_LEN;

proxyfile_list *list;

FILE *log_file, *debug_file;

int cleanup, print_list;

int finish_and_return(int child, int syscall, int *retval) {
    if(syscall == -1) {
        char buf[2];
        fgets(buf, sizeof(buf), stdin);
    }

    if(syscall_breakpoint(child) != 0)
        return 0;

    *retval = get_reg(child, eax);
    return 1;
}

int handle_syscalls(pid_t child) {
    if(syscall_breakpoint(child) != 0)
        return 0;

    int syscall;
    syscall = get_reg(child, orig_eax);

    if(syscall == SC_EXIT || syscall == SC_EXIT_GROUP) {
        int exit_code = get_syscall_arg(child, 0);
        fprintf(stderr, "fssb: child exited with %d\n", exit_code);
        fprintf(stderr, "fssb: sandbox directory: %s\n", SANDBOX_DIR);
    }

    if(syscall == SC_OPEN) {
        /* int open(const char *pathname, int flags); */

        long orig_word = get_syscall_arg(child, 0);
        char *pathname = get_string(child, orig_word);

        int flags = get_syscall_arg(child, 1);

        proxyfile *cur;

        if(flags & O_APPEND || flags & O_CREAT || flags & O_WRONLY) {
            cur = search_proxyfile(list, pathname);
            if(!cur)
                cur = new_proxyfile(list, pathname);

            write_string(child, write_slots[0], cur->proxy_path);
            set_syscall_arg(child, 0, write_slots[0]);
        }

        if(flags == O_RDONLY) {
            /* If this file has been written to, then we should hijack the arg
               with the proxyfile because we want the process to see its own
               changes.  If this file has never been opened with write
               permissions, we can just give the same file (this is more
               performant than copying the file). */
            cur = search_proxyfile(list, pathname);

            char *new_name;
            if(cur)
                new_name = cur->proxy_path;
            else
                new_name = pathname;

            write_string(child, write_slots[0], cur->proxy_path);
            set_syscall_arg(child, 0, write_slots[0]);
        }

        int retval;
        if(finish_and_return(child, syscall, &retval) == 0)
            return 0;

        set_syscall_arg(child, 0, orig_word);
    }

    if(syscall == SC_UNLINK) {
        /* int unlink(const char *pathname); */

        long orig_word = get_syscall_arg(child, 0);
        char *pathname = get_string(child, orig_word);

        proxyfile *cur = search_proxyfile(list, pathname);
        char *new_name;

        if(cur) /* it's a file we've previously written to */
            new_name = cur->proxy_path;
        else
            new_name = proxy_path(SANDBOX_DIR, pathname);

        write_string(child, write_slots[0], new_name);

        int retval;
        if(finish_and_return(child, syscall, &retval) == 0)
            return 0;

        set_syscall_arg(child, 0, orig_word);

        if(cur) /* let's take this off our records */
            delete_proxyfile(list, cur);
        else /* we malloc'd some memory, let's free it */
            free(new_name);
    }

    return 1;
}

void trace(pid_t child) {
    int status;
    waitpid(child, &status, 0);

    assert(WIFSTOPPED(status));
    ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACESYSGOOD);

    while(handle_syscalls(child));
}

int process_child(int argc, char **argv) {
    int i;
    char *args[argc+1];
    for(i=0; i < argc; i++)
        args[i] = argv[i];
    args[argc] = NULL;  /* execvp needs NULL terminated list */

    ptrace(PTRACE_TRACEME);
    kill(getpid(), SIGSTOP);
    return execvp(args[0], args);
}

void init(int child) {
    struct stat sb;

    int i;
    for(i = 1; ; i++) {
        sprintf(SANDBOX_DIR, "/tmp/fssb.%d/", i);
        if(stat(SANDBOX_DIR, &sb))
            break;
    }
    mkdir(SANDBOX_DIR, 0775);

    PROXY_FILE_LEN = strlen(SANDBOX_DIR) + 32;

    list = new_proxyfile_list();
    list->SANDBOX_DIR = SANDBOX_DIR;
    list->PROXY_FILE_LEN = PROXY_FILE_LEN;

    long start_pos = get_readonly_mem(child);
    for(int i = 0; i < 6; i++)
        write_slots[i] = start_pos + i*RDONLY_MEM_WRITE_SIZE;
}

int main(int argc, char **argv) {
    build_help();
    check_args_validity(argc, argv);
    if(help_requested(argc, argv)) {
        print_help();
        return 0;
    }

    /* everything else must run a program */
    int pos = get_child_args_start_pos(argc, argv);
    int child_argc = argc - pos;
    char **child_argv = argv + pos;

    set_parameters(pos - 1,
                   argv,
                   &cleanup,
                   &log_file,
                   &debug_file,
                   &print_list);

    pid_t child = fork();

    if(child > 0) {
        init(child);
        trace(child);
    }
    else if(child == 0)
        process_child(child_argc, child_argv);
    else {
        fprintf(stderr, "fssb: error: cannot fork\n");
        return 1;
    }

    if(print_list)
        print_map(list, log_file);

    if(cleanup) {
        remove_proxy_files(list);
        rmdir(SANDBOX_DIR);
    }

    return 0;
}
