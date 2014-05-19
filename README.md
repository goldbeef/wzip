wzip
====

wzip is a block-sorting compressor,based on BWT tranformation, wavelte tree, run-length code.Its compression ratio is very good      and close to bzip2 for most files except for radio and video file.      Its compression speed is much faster than bzip2 even in single-thread workstate ,and while in multi-thread workstate it will be more faster.

AUTHOR
     goldbeef (goldbeef@163.com) ,xidian university ,CHINA.


NOTE:

you should first get SAu.tgz form http://pizzachili.dcc.uchile.cl/indexes/Suffix_Array/  ,then decompress it and you will get directory  
SAu_FILES, then do the following:
1.change directory to: SAu_FILES/ds/
2.make 
3.rename mv ds_ssort.a to libds_ssort.a
4.up to now we get the lib libds_ssort.a ,and then copy out this lib to wzip project directroy, and change directory to wzip project               directory, and then make to get wzip.
     
see the manul.txt for more about wzip.
     
     
