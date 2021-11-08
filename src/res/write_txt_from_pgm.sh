#!/bin/bash

function askForContinue {
  local answer=""
  while [[ "$answer" != "y" ]]; do
    echo "Finished? [y]"
    read answer
  done
}

filename='FontExtraLarge.pgm'
outputname='FontExtraLarge.txt'
lineIdx=0
charIdx=0
row=0
col=0
while read line; do
  lineIdx=$((lineIdx+1))
  if [[ $lineIdx -eq 1 || $lineIdx -eq 2 ]]; then
    continue
  fi
  if [[ $lineIdx -eq 3 ]]; then
    IFS=' ' read -r v1 v2 <<<"$line"
    row=$v1
	col=$v2
	echo "=$row;" > $outputname
	echo "=$col;" >> $outputname
	continue	
  fi
  if [[ $lineIdx -ge 5 ]]; then
    if [[ $lineIdx%36 -eq 5 ]]; then
	  echo "" >> $outputname
	fi
    echo -n "$line," >> $outputname 
  fi
  if [[ $lineIdx -eq 4 ]]; then
	echo -n "={" >> $outputname
  fi
  if [[ $lineIdx -eq $row*$col+4 ]]; then
  	echo "" >> $outputname
	echo "};" >> $outputname
  fi
  if [[ $lineIdx%$col -eq 5 ]]; then
    echo -n "."
  fi
done < $filename
echo "====="
askForContinue
