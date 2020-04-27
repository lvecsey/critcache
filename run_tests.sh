#!/bin/bash

testit () {

    testname="$1"

    if [ -e "$testname" ]; then
	"$testname" > /dev/null
	retval="$?"
	if [ "$retval" == 0 ]; then
	    echo -e "$testname"' '"\033[0;32mPASS.\033[0;00m"
	else
	    echo -e "$testname"' '"\033[0;31mFAIL.\033[0;00m"
	fi

    fi
    
    }

testit "./run_test-add.sh"
testit "./run_test-get.sh"
testit "./run_test-del.sh"

exit 0
