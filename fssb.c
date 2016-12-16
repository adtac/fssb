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
        /* get open(...) args */
        long word = get_syscall_arg(child, 0);
        char *file = get_string(child, word);
        int flags = get_syscall_arg(child, 1);

        /* log message */
        fprintf(debug_file, "open(\"%s\", %d)\n", file, flags);

        /* name switch */
        char *new_name;
        char *original_bytes;
        int overwritten_size;
        int switch_name = 0;
        proxyfile *cur = NULL;

        if(flags & O_APPEND || flags & O_CREAT || flags & O_WRONLY) {
            fprintf(debug_file, "opening with write access\n");

            /* if the file already exists, use it */
            cur = search_proxyfile(list, file);
            if(cur == NULL)
                cur = new_proxyfile(list, file);

            new_name = cur->proxy_path;
            switch_name = 1;
        }
        if(flags == O_RDONLY) {
            fprintf(debug_file, "opening with read access\n");

            proxyfile *cur = search_proxyfile(list, file);
            if(cur != NULL) {
                new_name = cur->proxy_path;
                switch_name = 1;
            }
        }

        if(switch_name) {
            original_bytes = write_string(child,
                                          word,
                                          new_name,
                                          &overwritten_size);
        }

        int retval;
        if(finish_and_return(child, syscall, &retval) == 0)
            return 0;

        /* restore the memory */
        if(switch_name)
            write_bytes(child, word, original_bytes, overwritten_size);

        if(cur == NULL || cur->file_path != file)
            free(file);
    }

    if(syscall == SC_UNLINK) {
        /* get unlink(...) args */
        long word = get_syscall_arg(child, 0);
        char *file = get_string(child, word);

        /* name switch */
        char *new_name;
        char *original_bytes;
        int overwritten_size;
        int switch_name = 0;

        proxyfile *cur = search_proxyfile(list, file);
        if(cur)
            new_name = cur->proxy_path;
        else
            new_name = proxy_path(SANDBOX_DIR, file);

        /* switch the name */
        original_bytes = write_string(child,
                                      word,
                                      new_name,
                                      &overwritten_size);

        int retval;
        if(finish_and_return(child, syscall, &retval) == 0)
            return 0;

        /* restore the memory */
        write_bytes(child, word, original_bytes, overwritten_size);

        if(cur)
            delete_proxyfile(list, cur);
        else
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

void init() {
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

    init();

    pid_t child = fork();
    if(child > 0)
        trace(child);
    else if(child == 0)
        process_child(child_argc, child_argv);
    else {
        fprintf(stderr, "fssb: error: cannot fork\n");
        return 1;
    }

    if(cleanup)
        rmdir(SANDBOX_DIR);
    else if(print_map)
        print_map(list, log_file);

    return 0;
}
