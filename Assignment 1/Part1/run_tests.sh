#!/bin/bash

test () {
    # echo "$($2)"
    # Execute the given ops pattern.
    RESULT=`echo "$($2)"`
    # If produced result and expected result is equal then test is passed.
    if [[ $RESULT -eq $3 ]] 
    then
        echo "TEST $1 PASSED"
    else
        echo "TEST $1 FAILED"
    fi

}


# Cleanup old executable 
[ -f sqroot ] && rm sqroot
[ -f double ] && rm double
[ -f square ] && rm square

# Compile
gcc -o sqroot sqroot.c -lm
gcc -o double double.c -lm
gcc -o square square.c -lm

# Tests
test 1 "./sqroot 5" 2
test 2 "./double square 2" 16
test 3 "./sqroot square 4" 4
test 4 "./double square sqroot 4" 8
test 5 "./double square sqroot double square 4" 256
test 6 "./square square square square double double double double double double double double double double 10" 10240000000000000000
test 7 "./sqroot sqroot sqroot 4" 1
test 8 "./sqroot 7" 3
test 9 "./square sqroot double sqroot 8" 4
test 10 "./sqroot square 5" 4
test 11 "./sqroot square 7" 9
