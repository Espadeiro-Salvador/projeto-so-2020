#!/bin/bash

# Verify number of arguments
if [ $# -ne 3 ] 
then
    echo "Wrong number of arguments" 
    exit
fi

# Check if input directory exists
if [ ! -d $1 ]
then
    echo "Input directory does not exist"
    exit
fi

# Check if output directory exists
if [ ! -d $2 ] 
then
    echo "Output directory does not exist"
    exit
fi

# Check if number of threads is more than 0
if [ $3 \< 1 ]
then 
    echo "Invalid number of threads" && exit
fi

for f in $1/*.*
do
    input=$(basename $f)
    name="${input%.*}"

    for i in $(seq 1 $3)
    do  
        echo InputFile=$input NumThreads=$i
        ./tecnicofs $f $2/$name-$i.txt $i | grep TecnicoFS
    done 
done
