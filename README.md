#Wzip
##What is it?
Wzip is a block-sorting compressor, based on Burrows-Wheeler transform (BWT), wavelet tree, run-length code . Its compression ratio is very good and close to bzip2 for most files except for audio and video files. Its compression speed is much faster than bzip2 even in single-thread workstate , and while in multi-thread workstate it will be much faster.
## How to use it?
###install  program
step 1: first get SAu.tgz form http://pizzachili.dcc.uchile.cl/indexes/Suffix_Array/,then decompress it and you will get directory  
SAu_FILES
step 2: change directory to: SAu_FILES/ds/ , then make
step 3: rename ds_ssort.a to libds_ssort.a
step 4: up to now we get the lib libds_ssort.a ,and then copy out this lib to wzip project directroy
step 5: change directory to wzip project directory, and then make to get wzip.
###example for use
while compressing a file "book",
  wzip -c book 
  or
  wzip -c -b 9 -e 3 -g 1 -p 2 -v 3  book

while decompressing a file "book.wz"
  wzip -d book.wz
  
while getting help
  wzip -L
###about
AUTHOR
goldbeef (goldbeef@163.com) ,xidian university , CHINA.
NOTE
these source codes were written on 32 bit  computer with linux OS.
