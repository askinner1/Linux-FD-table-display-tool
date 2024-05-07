Linux System-Wide File Descriptor Tables Assignment 2  
By: Adam Skinner  
Date: March 10th, 2023  

This code has been tested only on Linux operating systems.  

## How to compile
#### Using provided makefile (ensure `makefile`, `B09A2.c` are in current working directory):
`make a2`

## Positional argument
`[pid]`: Indicates a particular positive process ID number (PID), if not specified the program will attempt to process all the currently running processes for the user executing the program. Also, line numbers for each table will be printed in the leftmost position on each line for better readability.

## Flags (Optional)
`--per-process`: Display the process File Descriptor table (PID & FD).  
`--systemWide`: Display the system-wide File Descriptor table (PID, FD, & Filename).  
`--Vnodes`: Display the Vnodes File Descriptor table (FD & Inode).  
`--composite`: Display the composed table (PID, FD, Filename, & Inode). This is the default table that will be displayed if no other tables (per-process, systemWide, or Vnodes) are specified.  
`--threshold=X`: where `X` denotes an integer, indicating that processes which have a number of FD assigned larger than X should be flagged in the output. Note that behaviour is not affected by presence of `[pid]` positional argument.  
`--output_TXT`: Saves the composite table in text (ASCII) format. This saves the composite table in text regardless of whether it is printed to the screen or not. Saves to `compositeTable.txt`.  
`--output_binary`: Saves the composite table in binary format. This saves the composite table in binary regardless of whether it is printed to the screen or not. Saves to `compositeTable.bin`.  

Note: if no flag (for a given table) is selected, then the composite table is printed by default.
  
## Sample usage 
	./a2 [pid] [--per-process] [--systemWide] [--Vnodes] [--composite] 
		 [--threshold=X] [--output_TXT] [--output_binary]
		 
## Documentation
### Important Constants
`STRLEN` defines the maximum length of a line, which is used throughout the program. Note that under normal conditions, any line length should not exceed `STRLEN`.  

### Functions
`int main(int argc, char **argv)`  
Reads command line arguments (using for loops & getopt), initializes & declares variables, opens `/proc` and iterates through each directory entry using `dirent` (that are numeric so correspond to PIDs), calls functions to print each table as necessary (also prints headers & footers), prints PIDs having a FD count greater than `threshold` if applicable.  
  
`int openDirectory(int pid, struct dirent **pDirent, DIR **pDir)`  
Opens `/fd` directory for a given PID pid and sets pDir to point to that directory and pDirent to point to the third directory entry (skipping the `.` and `..` directories), i.e., the start of the FDs for that PID. Returns -1 if no permission or 0 if the directory was opened successfully.  

`int printProc(int pid, int is_all, int i)`  
Prints the data for the process FD table for a given PID `pid`. If `is_all` is -1, then the line numbers are printed as all running processes will be processed to enhance readability. `i` is used to keep track of the current line number for the table.  

`int printSystem(int pid, int is_all, int i)`  
Prints the data for the system-wide FD table for a given PID `pid`. If `is_all` is -1, then the line numbers are printed as all running processes will be processed to enhance readability. `i` is used to keep track of the current line number for the table.  

`int printVNodes(int pid, int is_all, int i)`  
Prints the data for the Vnodes FD table for a given PID `pid`. If `is_all` is -1, then the line numbers are printed as all running processes will be processed to enhance readability. `i` is used to keep track of the current line number for the table.  

`int printComposite(int pid, int is_all, int i)`  
Prints the data for the composed FD table for a given PID `pid`. If `is_all` is -1, then the line numbers are printed as all running processes will be processed to enhance readability. `i` is used to keep track of the current line number for the table.  

Note: all the table printing functions return an `int` corresponding to the last line number that was printed for better readability when printing all PIDs.

`pid_s *getOffender(int pid, int threshold`  
Checks if a given PID `pid` has a number of open File Descriptors greater than threshold. If true, then return a pointer to a `pid_s` struct that is a node to a linked list of PIDs that have an open FD count greater than threshold. If false, return `NULL`. This linked list implementation enables enhanced readability when printing offending processes, as we know how many offending FDs there are so we can avoid printing an extra comma after the last one.  

`int saveToFile(int pid, int is_all, int file_line, int output_txt, int output_bin)`  
Handles printing the composite table to text (ASCII) & binary files. Will print to a text and/or binary file depending on whether `output_txt == 1` and/or `output_bin == 1` respectfully. `file_line` is used to keep track of the current line number for the table when saving to a text file.  

## Challenges & how they were solved
- One challenge was constructing each table to ensure the columns were aligned, so to accomplish this I used special padding when printing the numbers, such as `%-7d` in the `printf()` statement so that there would be "space" for at least 7 characters, as well as `%-14s` when printing filenames, since those entries are generally longer than the others
- To handle directories in `/proc` that the user did not have permission to open (i.e., processes that didn't belong to the user), I would simply check if `opendir()` returned `NULL` in my `openDirectory` function, in which case `openDirectory` returned -1 and that entry would be skipped in the table printing functions. Thus, no text indicating "Permission denied" would be printed in the table as if the user didn't own the process, it would be skipped.
- For the formatting of the binary table, I included a fixed amount of spaces in between each entry as an alternative to padding to ensure better consistency between the text (ASCII) table's & binary table's formats  
- For timing text file & binary file output, I used the `time` command and added the values from `user` & `sys` (`user` + `sys`) (as a lot of time was spent sifting through directories that were not permitted to be open & skipping them as well as printing the composite table) so that I could focus on the exact amount of time to write to the text vs. binary files

## Time comparison between text & binary output
###Case 1: With PID `785822`  

Text file output:  

Trial | Time  | Filesize
-----------|------------- | -------------
1|0.002 s  | 6.9 kB
2|0.002 s  | 6.9 kB
3|0.002 s  | 6.9 kB
4|0.002 s  | 9.9 kB
5|0.002 s  | 6.9 kB
6|0.002 s  | 6.9 kB
7|0.002 s  | 6.9 kB
Avg. time: 0.002 s (Standard deviation: 0)  
Avg. filesize: 7.33 kB (Standard deviation: 1.05)

Binary file output:  

Trial | Time  | Filesize
-----------|------------- | -------------
1|0.002 s  | 7.5 kB
2|0.002 s  | 11 kB
3|0.002 s  | 11 kB
4|0.002 s  | 7.5 kB
5|0.002 s  | 7.5 kB
6|0.002 s  | 7.5 kB
7|0.002 s  | 7.5 kB  
Avg. time: 0.002 s (Standard deviation: 0)  
Avg. filesize: 8.5 kB (Standard deviation: 1.58)  

####Observations & Conclusion
For a relatively small amount of entries in the table, the difference in writing time is practically non-existent, and the filesizes vary but in general the binary filesizes are larger than the text ones. This could mean that there are more numbers in the table that are *stored* more efficiently in text, i.e., smaller numbers, compared to their binary representations. Also, there could have been FDs being opened and closed in between trials, causing a discrepancy between different filesizes.

###Case 2: All PIDs for current user  

Text file output:  

Trial | Time  | Filesize
-----------|------------- | -------------
1|0.112 s  | 90 kB
2|0.102 s  | 90 kB
3|0.078 s  | 90 kB
4|0.121 s  | 90 kB
5|0.115 s  | 90 kB
6|0.106 s  | 90 kB
7|0.106 s  | 90 kB
Avg. time: 0.106 s (Standard deviation: 0.013)  
Avg. filesize: 90 kB (Standard deviation: 0)

Binary file output:  

Trial | Time  | Filesize
-----------|------------- | -------------
1|0.114 s  | 110 kB
2|0.089 s  | 110 kB
3|0.099 s  | 110 kB
4|0.107 s  | 110 kB
5|0.080 s  | 109 kB
6|0.102 s  | 110 kB
7|0.103 s  | 110 kB  
Avg. time: 0.099 s (Standard deviation: 0.011)  
Avg. filesize: 109.9 kB (Standard deviation: 0.35)  

####Observations & Conclusion
For a large amount of PIDs, binary file output is generally faster and more consistently faster (given smaller standard deviation) compared to text file output. Though the binary filesize is consistently larger, these files are still able to be written to quicker, indicating an increased efficiency for writing data compared to text files in this case (approx. 849 kB/s for text vs. 1,110 kB/s for binary). This could mean that the binary representations of most of the numbers in the table are able to be written more efficiently in binary format compared to their text format, and in this case it would suggest that there was a much larger amount of these "more efficiently written in binary" numbers compared to case 1 due to the smaller average time. Also, with the binary filesizes being consistently larger, it would suggest that some numbers are more efficiently *stored* in text form; however, there could equally be a discrepancy between text & binary filesizes due to minor variations in formatting that accumulate with a large number of PIDs.
