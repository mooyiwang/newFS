#!/bin/bash

./build/newfs --device=/home/students/200210231/ddriver -f -d -s ./tests/mnt/ &

mkdir ./tests/mnt/dir0
mkdir ./tests/mnt/dir0/dir0
mkdir ./tests/mnt/dir0/dir0/dir0
mkdir ./tests/mnt/dir1

touch ./tests/mnt/file0
touch ./tests/mnt/dir0/file0
touch ./tests/mnt/dir0/dir0/file0
touch ./tests/mnt/dir0/dir0/dir0/file0
touch ./tests/mnt/dir1/file0

mkdir ./tests/mnt/dir0
touch ./tests/mnt/file0
mkdir ./tests/mnt/dir0/dir1
mkdir ./tests/mnt/dir0/dir1/dir2
touch ./tests/mnt/dir0/dir1/dir2/file4
touch ./tests/mnt/dir0/dir1/dir2/file5
touch ./tests/mnt/dir0/dir1/dir2/file6
touch ./tests/mnt/dir0/dir1/file3
touch ./tests/mnt/dir0/file1

ls ./tests/mnt
ls ./tests/mnt/dir0
ls ./tests/mnt/dir0/dir1
ls ./tests/mnt/dir0/dir1/dir2

fusermount -u ./tests/mnt
ddriver -r

./build/newfs --device=/home/students/200210231/ddriver -f -d -s ./tests/mnt/ &

mkdir ./tests/mnt/hello
ls ./tests/mnt
fusermount -u ./tests/mnt

python3 ./tests/checkbm/checkbm.py -l ./include/fs.layout -r ./tests/checkbm/golden.json