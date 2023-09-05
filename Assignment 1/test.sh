#!/bin/bash

read -p "Enter your roll no: " ROLL_NO


mkdir test
unzip "$ROLL_NO".zip -d test/ > trash.txt
cp -r Part* test/
cp -r test/"$ROLL_NO"/Part1/* test/Part1/ 
cp -r test/"$ROLL_NO"/Part2/* test/Part2/ 
cp -r test/"$ROLL_NO"/Part3/* test/Part3/ 

echo "####### Part1 #######"
cd test/Part1
# pwd
./run_tests.sh

echo "####### Part2 #######"
cd ../Part2
# pwd
./test.sh
./test4.sh

echo "####### Part3 #######"
cd ../Part3
make > trash.txt
./Testcases/test1
./Testcases/test2
./Testcases/test3
./Testcases/test4
./Testcases/test5

cd ../..
rm -r test
rm trash.txt