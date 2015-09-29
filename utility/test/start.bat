@echo off

rem set title and size of the command window
TITLE TwoWirePlus unit test shell
mode con:cols=130 lines=3000
color F1

rem Add compiler and make to system path
rem
PATH=%PATH%;C:\compiler\MinGW\bin

rem welcome and test perl
echo ----------------------------------------------
echo  Welcome to unit test framework
echo ----------------------------------------------
echo .
echo make -f Makefile help


rem leave command window open
cmd /K
