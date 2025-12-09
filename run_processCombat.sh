#!/bin/bash
# AUTHOR:       Roberto Castillejo
# VERSION:      1.0

 
# Step 1 - Compilation of father.c and son.c
gcc -o FATHER father.c
gcc -o CHILD child.c
# Step 2 - FIFO creation result
mkfifo result
# Step 3 - Run a cat in the background to read the result
cat result &
# Step 4 - Execution of FATHER passing 10 as argument
./FATHER 10
# step 5 - Unlink the result and delete the created files
unlink result
rm FATHER
rm CHILD
