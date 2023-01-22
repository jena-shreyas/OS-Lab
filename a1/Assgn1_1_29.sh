#!/bin/bash
function gcd() {
	local gcd=$1
	local a=$1
	local b=$2
	if [ "$b" -lt "$gcd" ]
	then
		gcd=$b
	fi
	while [ "$gcd" -ne 0 ]
	do
		x=`expr $a % $gcd`
		y=`expr $b % $gcd`
		if [ $x -eq 0 -a $y -eq 0 ] 
		then
			break
		fi
		gcd=`expr $gcd - 1`
        done
	echo "$gcd"     
}
input=$1
answer=0
flag=0
while read -r line
	do
		line=$(echo $line | rev)
		if [ $flag -eq 1 ] 
		then
			g=$(gcd "$answer" "$line")
			answer=$(($answer*($line/$g)))
		fi
		if [ $flag -eq 0 ] 
		then
			answer=$line
			flag=1
		fi
	done < "$input"
echo "$answer"