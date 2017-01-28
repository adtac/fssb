#!/bin/bash

SANDBOX="../fssb -- "
RUNNER="./tests.py"

testcases=(
	'test_no_syscalls'
	'test_save_empty_file'
)

echo "Removing all /tmp/fssb-*"
rm -rf -- /tmp/fssb-*

echo "Launching tests"

for testcase in ${testcases[@]}
do
	$SANDBOX $RUNNER test $testcase
	$RUNNER check $testcase
done

