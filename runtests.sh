#!/bin/bash
# This file is inteded to be run by makefile after all test executables got
# built. Don't run it manually.
echo "======= TESTS ======="
passed=0 
failed=0
green="\\e[1;32m"
red="\\e[1;31m"
default="\\e[0m"
for t in $(find build -name "*.test") ; do
	test_source=`echo $t | sed -e "s/build\/\(.*\).test/\1.c/"`
	if $t ; then
		echo -e $green "PASS" $default $test_source
		((++passed))
	else
		echo -e $red "FAIL" $default $test_source
		((++failed))
	fi
done
((total = passed + failed))
echo "SUMMARY"
if ((0 == total)) ; then
	echo "    THERE'S NO TESTS"
else
	echo "    passed: $passed ($((passed * 100 / total))%)"
	echo "    failed: $failed ($((failed * 100 / total))%)"
	echo "     total:" $total 
fi
echo "======= TESTS ======="

