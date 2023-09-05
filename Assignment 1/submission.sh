#!/bin/bash

read -p "Enter your Roll No: " ROLL_NO
read -p "Enter your name: " NAME

rm -rf "$ROLL_NO"
mkdir "$ROLL_NO"
mkdir -p "$ROLL_NO"/Part1
mkdir -p "$ROLL_NO"/Part2
mkdir -p "$ROLL_NO"/Part3
cp Part1/*.c ./"$ROLL_NO"/Part1/
cp Part2/myDU.c ./"$ROLL_NO"/Part2/
cp Part3/mylib.c ./"$ROLL_NO"/Part3/

echo "I have read the CSE department’s anti-cheating policy available at https://www.cse.
iitk.ac.in/pages/AntiCheatingPolicy.html. I understand that plagiarism is a severe
offense. I have solved this assignment myself without indulging in any plagiarism.
If my submission is found to be plagiarized from the internet, fellow students, etc.,
then strict action can be taken against me. 

Name : $NAME
Roll No : $ROLL_NO

Resources Used : 
1. Linux Man Pages
2. GeeksForGeeks
3. Medium articles" > "$ROLL_NO"/declaration

zip -r "$ROLL_NO".zip ./"$ROLL_NO"