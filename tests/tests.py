#!/usr/bin/env python
"""
As the sandbox may change paths used by the program,
the tests have to be performed in two seperately run phases:

1. Test phase - this does all the things that *are tested*
2. Check & cleanup phase - this asserts whether the 1st phase did proper stuff

The phases has to be run seperately.
Only the 1st phase is run under the sandbox.

To implement a test, simply write a test function that will return
a tuple of two functions corresponding to the phases.

If a phase is not required, simply make it to just pass, e.g.:
    def check_save_empty_file():
        pass

NOTE: The `check` function name should describe the testcase
as if the `_assert` fails, it will inspect `check` name and print it.
"""

from __future__ import print_function
import os
import sys
import glob
import shutil
import inspect
import hashlib
import operator


# via http://stackoverflow.com/questions/287871/print-in-terminal-with-colors-using-python
HEADER = '\033[95m'
OKBLUE = '\033[94m'
OKGREEN = '\033[92m'
WARNING = '\033[93m'
FAIL = '\033[91m'
ENDC = '\033[0m'
BOLD = '\033[1m'
UNDERLINE = '\033[4m'


def colored(color, msg):
    return color + msg + ENDC


def sandbox_paths():
    sandbox_dirs = glob.glob('/tmp/fssb-*')
    
    # sorting by number after 'fssb-'
    sandbox_dirs.sort(key=lambda d: int(d[d.rindex('-')+1:]), reverse=True)

    sandbox_dir = sandbox_dirs[0]
    filemap_path = os.path.join(sandbox_dir, 'file-map')

    return sandbox_dir, filemap_path


def _assert(fnc, *args, **kwargs):
    curr_frame = inspect.currentframe()
    upper_fn_frame = inspect.getouterframes(curr_frame)[1]

    assert_line_no = upper_fn_frame[2]
    caller_name = upper_fn_frame[3]

    if fnc(*args, **kwargs):
        print(colored(OKGREEN, 'Assert in line {} passed: {}'.format(assert_line_no, caller_name)))

    else:
        assert_lines = upper_fn_frame[4]
        print(colored(FAIL, 'Assert in line {} failed: {}'.format(assert_line_no, caller_name)))
        print(colored(WARNING, 'args = {}'.format(args)))
        print(colored(WARNING, 'kwargs = {}'.format(kwargs)))
        print(colored(WARNING, ''.join(assert_lines)))


def test_no_syscalls():
    def test():
        pass

    def check_no_syscalls():
        _sandbox_dir, filemap_path = sandbox_paths()

        _assert(operator.eq, open(filemap_path).read(), '')

    return test, check_no_syscalls


def test_save_empty_file():
    empty_file_name = 'save_empty_file'
    hashed_name = hashlib.md5(empty_file_name).hexdigest()

    def test():
        with open(empty_file_name, 'wb') as f:
            pass

    def check_save_empty_file():
        sandbox_dir, filemap_path = sandbox_paths()
        
        empty_file_path = os.path.join(sandbox_dir, hashed_name)

        # lets assume we made a bad testcase (or we've changed the fssb
        # to skip newline at the end of file-map file)
        ### saving, quiting
        _assert(operator.eq, open(filemap_path).read(), '{} = {}\n'.format(empty_file_path, empty_file_name))
        _assert(operator.eq, open(empty_file_path).read(), '')

    return test, check_save_empty_file


if __name__ == '__main__':
    if len(sys.argv) < 3:
        self_name = sys.argv[0]

        print('Usage: %s (test|check) <test_name>')
        print('    test    - launches test phase for given test')
        print('    check   - launches check&clean phase for given test')
        sys.exit(-1)

    phase = sys.argv[1]
    test_name = sys.argv[2]

    phases = ('test', 'check')

    if phase not in phases:
        print('Wrong phase name. Valid phases: {}'.format(phases))
        sys.exit(-1)

    test_function = globals()[test_name]
    
    test_exec, test_check = test_function()

    print(colored(HEADER, 'Launching {} on {}'.format(phase, test_name)))

    if phase == 'test':
        test_exec()
    else:
        test_check()

