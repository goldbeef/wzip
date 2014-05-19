wzip
====

wzip is a block-sorting compressor,based on BWT tranformation, wavelte tree, run-length code.Its compression ratio is very good      and close to bzip2 for most files except for radio and video file.      Its compression speed is much faster than bzip2 even in single-thread workstate ,and while in multi-thread workstate it will be more faster.

NAME
    wzip is a block-sorting file compressor based on BWT,wavelet tree,run-legnth code. version 1.0 .

SYNOPSIS
    for compression:
        wzip -c [-b blksize] [-e treetype] [-g node] [-k] [-f] [-p #thread] [-v verbose] filename

    for decompress:
        wzip -d  [-k ] [ -f ] [-v verbose ] filename

    for help information:
        wzip -h
    
    for license:
        wzip -L

OPTIONS
    -b blockSiz100k
        while compressing file, we need set the block size of BWT with blockSiz100k. you can set blockSiz100k with 1,2..9,
        and the real BWT block size will be 100000,...900000 bytes. By default, we set the blockSiz100k 9.
    -c  
        let wzip works wtih compression state for compressing file

    -d  
        let wzip works with decompression state for decompressing file

    -e  treeType 
        select the kind of wavelet tree , like huffman tree ，balanced tree, hu-tacker tree with treeType  1 ，2，3，separately.
        By default,we set the treeType 3.

    -g  nodeCode
	    To select the code format of the wavelet tree node, like run-length gamma,run-length delta with nodeCode 1,2, separately.
	    By default, we set the nodeCode 1.

    -f  
        force overwrite of output files. Normally, wzip will not overwrite existing output files. 

    -k 
        do not delete the input file.

    -p #thread
        set the number of threads while compressing file,you can set the value 1,2 4.Noraml the larger #thread is ,the faster wzip will be.
        By default,we set the #thead 2.
    -h
	    print the use manual of the wzip.

    -v verbose 
        verbose mode will show the compression ratio for each file processed and real time.You can set vorvose 0,1,2.
        By default,wo set the verbose 0.

    -L 
        Display the software version, license terms and conditions.

DESTRIPTON
    wzip is a block-sorting compressor,based on BWT tranformation, wavelte tree, run-length code.Its compression ratio is very good 
    and close to bzip2 for most files except for radio and video file. 
    Its compression speed is much faster than bzip2 even in single-thread workstate ,and while in multi-thread workstate it will be more faster.
    
AUTHOR
     goldbeef (goldbeef@163.com) ,xidian university ,CHINA.
      
