#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#define STRLEN 512

typedef struct pid_struct {
	int pid;
	int num_fd;
	struct pid_struct *next;
} pid_s;

int openDirectory(int pid, struct dirent **pDirent, DIR **pDir) {
	char path[30];
	snprintf(path, 30, "/proc/%d/fd", pid);

    *pDir = opendir(path);
    if (*pDir == NULL) { // No permissions (not a process for the current user) so don't open
    	return -1;
    }

    // Bypass . and .. directories
    *pDirent = readdir(*pDir);
    *pDirent = readdir(*pDir);
    *pDirent = readdir(*pDir);
    return 0;
}

int saveToFile(int pid, int is_all, int file_line, int output_txt, int output_bin) {
	struct dirent *pDirent;
    DIR *pDir;
    ssize_t r;
    struct stat file_stat;
    FILE *file_txt, *file_bin;
    char spaces[] = "       "; // Spaces for entries in tables in binary file (since there isn't string padding)
    int ret;

    if (file_line == 0) { // Delete file if it exists already (on first iteration)
    	if (output_txt)
    		remove("compositeTable.txt");
    	if (output_bin)
    		remove("compositeTable.bin");
    }

    // Open respective files for appending (since this function is called repeatedly for each PID)
    if (output_txt)
    	file_txt = fopen("compositeTable.txt", "a");
     
    if (output_bin) {
    	file_bin = fopen("compositeTable.bin", "ab");
    }
    

    if (file_line == 0) { // Write header on first iteration
    	char header[] = "         PID     FD       Filename       Inode\n        ========================================\n";
    	if (output_txt)
    		fputs(header, file_txt);
    	if (output_bin)
    		// Need to use strlen(str) + 1 to include null terminator
    		fwrite(header, sizeof(char), strlen(header) + 1, file_bin);
    }

    int status = openDirectory(pid, &pDirent, &pDir);
    if (status == -1) return file_line; // No permissions for this process so don't write anything

    char path[30];
    snprintf(path, 30, "/proc/%d/fd", pid);
    char linkname[STRLEN];

    while (pDirent != NULL) {
    	ret = snprintf(path, 30, "/proc/%d/fd/%s", pid, pDirent->d_name);
    	if (ret < 0) { // Error checking for path name too long
    		pDirent = readdir(pDir);
    		continue;
    	}
    	
    	if (is_all == -1 && output_txt) { // Write line numbers if all PIDs are being printed
    		fprintf(file_txt, " %-7d %-7d %-7s ", file_line, pid, pDirent->d_name);
    	} else if (output_txt) {
    		fprintf(file_txt, "         %-7d %-7s ", pid, pDirent->d_name);
    	}

    	if (is_all == -1 && output_bin) { // Write line numbers if all PIDs are being printed
    		// Write spaces between entries in the table as an alternative to string padding
    		fwrite(&file_line, sizeof(int), 1, file_bin);
    		fwrite(spaces, sizeof(char), strlen(spaces) + 1, file_bin);
    		fwrite(&pid, sizeof(int), 1, file_bin);
    		fwrite(spaces, sizeof(char), strlen(spaces) + 1, file_bin);
    		fwrite(pDirent->d_name, sizeof(char), strlen(pDirent->d_name) + 1, file_bin);
    		fwrite(spaces, sizeof(char), strlen(spaces) + 1, file_bin);
    	} else if (output_bin) {
    		fwrite(spaces, sizeof(char), strlen(spaces) + 1, file_bin);
    		fwrite(&pid, sizeof(int), 1, file_bin);
    		fwrite(spaces, sizeof(char), strlen(spaces) + 1, file_bin);
    		fwrite(pDirent->d_name, sizeof(char), strlen(pDirent->d_name) + 1, file_bin);
    		fwrite(spaces, sizeof(char), strlen(spaces) + 1, file_bin);
    	}

    	r = readlink(path, linkname, STRLEN); // Get filename info.
    	status = stat(path, &file_stat); // Get inode info.

    	// Write filename if possible
    	if (!(r == -1 || r >= STRLEN)) { // Check if unable to open link or path too long
    		linkname[r] = '\0'; // Add '\0' since linkname isn't null-terminated
    		if (output_txt)
    			fprintf(file_txt, "%-14s   ", linkname);
    		if (output_bin) {
    			fwrite(linkname, sizeof(char), strlen(linkname) + 1, file_bin);
    			fwrite(spaces, sizeof(char), strlen(spaces) + 1, file_bin);
    		}
    	} else if (output_txt || output_bin) { // Write an empty space if unable to print filename
    		if (output_txt) fprintf(file_txt, "        ");
    		if (output_bin) fwrite(spaces, sizeof(char), strlen(spaces) + 1, file_bin);
    	}

    	// Write inode # if possible
    	if (status >= 0) {
    		if (output_txt)
    			fprintf(file_txt, "%-7lu", file_stat.st_ino);
    		if (output_bin)
    			fwrite(&file_stat.st_ino, sizeof(unsigned long), 1, file_bin);
    	}

    	// Write newline character
    	char newline[] = "\n";
    	if (output_txt)
    		fputs(newline, file_txt);
    	if (output_bin)
    		fwrite(newline, sizeof(char), strlen(newline) + 1, file_bin);

    	pDirent = readdir(pDir);
    	file_line++;
    }

    if (output_txt) fclose(file_txt);
    if (output_bin) fclose(file_bin);

    closedir(pDir);
    return file_line;
}

pid_s *getOffender(int pid, int threshold) {
	struct dirent *pDirent;
    DIR *pDir;
    int count = 0;

    int status = openDirectory(pid, &pDirent, &pDir);
    if (status == -1) return NULL; // No permissions for this process so skip this PID

    while (pDirent != NULL) { // Get # of currently open FDs for this PID
    	count++;
    	pDirent = readdir(pDir);
    }

    if (count > threshold) { // Create a new node for this PID if it is an offender
    	pid_s *new = (pid_s *) malloc(sizeof(pid_s));
    	new->pid = pid;
    	new->num_fd = count;
    	new->next = NULL;
    	closedir(pDir);
    	return new;
    }

    closedir(pDir);
    return NULL; // Not an offender, so return NULL

}

int printComposite(int pid, int is_all, int i) {
	struct dirent *pDirent;
    DIR *pDir;
    ssize_t r;
    struct stat file_stat;
    int ret;

    int status = openDirectory(pid, &pDirent, &pDir);
    if (status == -1) return i; // No permissions for this process so don't print anything

    char path[30];
    snprintf(path, 30, "/proc/%d/fd", pid);
    char linkname[STRLEN];

    while (pDirent != NULL) {
    	ret = snprintf(path, 30, "/proc/%d/fd/%s", pid, pDirent->d_name);
    	if (ret < 0) { // Error checking for path name too long
    		pDirent = readdir(pDir);
    		continue;
    	}
    	
    	if (is_all == -1) { // Print line numbers if all PIDs are being printed
    		printf(" %-7d %-7d %-7s ", i, pid, pDirent->d_name);
    	} else {
    		printf("         %-7d %-7s ", pid, pDirent->d_name);
    	}

    	r = readlink(path, linkname, STRLEN); // Get filename info.
    	status = stat(path, &file_stat); // Get inode info.

    	// Print filename if possible
    	if (!(r == -1 || r >= STRLEN)) { // Check if unable to open link or path too long
    		linkname[r] = '\0';
    		printf("%-14s   ", linkname);
    	} else {
    		printf("        ");
    	}

    	// Print inode # if possible
    	if (status >= 0) {
    		printf("%-7lu", file_stat.st_ino);
    	}
    	printf("\n");
    	pDirent = readdir(pDir);
    	i++;
    }

    closedir(pDir);
    return i;
}

int printVNodes(int pid, int is_all, int i) {
	struct dirent *pDirent;
    DIR *pDir;
    struct stat file_stat;
    int ret;

    int status = openDirectory(pid, &pDirent, &pDir);
    if (status == -1) return i; // No permissions for this process so don't print anything

    char path[30];
    snprintf(path, 30, "/proc/%d/fd", pid);

    while (pDirent != NULL) {
    	ret = snprintf(path, 30, "/proc/%d/fd/%s", pid, pDirent->d_name);
    	if (ret < 0) { // Error checking for path name too long
    		pDirent = readdir(pDir);
    		continue;
    	}
    	status = stat(path, &file_stat); // Get inode info.
    	if (status < 0) { // Error getting file stats (no inode), so skip this iteration
    		pDirent = readdir(pDir);
    		continue;
    	}
    	if (is_all == -1) { // Print line numbers if all PIDs are being printed
    		printf(" %-7d   %-14s %-7lu\n", i, pDirent->d_name, file_stat.st_ino);
    	} else {
    		printf("           %-14s %-7lu\n", pDirent->d_name, file_stat.st_ino);
    	}
    	pDirent = readdir(pDir);
    	i++;
    }

    closedir(pDir);
    return i;
}

int printSystem(int pid, int is_all, int i) {
	struct dirent *pDirent;
    DIR *pDir;
    ssize_t r;
    int ret;

    int status = openDirectory(pid, &pDirent, &pDir);
    if (status == -1) return i; // No permissions for this process so don't print anything

    char path[30];
    snprintf(path, 30, "/proc/%d/fd", pid);
    char linkname[STRLEN];

    while (pDirent != NULL) {
    	ret = snprintf(path, 30, "/proc/%d/fd/%s", pid, pDirent->d_name);
    	if (ret < 0) { // Error checking for path name too long
    		pDirent = readdir(pDir);
    		continue;
    	}
    	r = readlink(path, linkname, STRLEN); // Get filename info.
    	if (r == -1 || r >= STRLEN) { // Check if unable to open link or path too long
    		pDirent = readdir(pDir);
    		continue;
    	}
    	linkname[r] = '\0';
    	
    	if (is_all == -1) { // Print line numbers if all PIDs are being printed
    		printf(" %-7d %-7d %-7s %-7s\n", i, pid, pDirent->d_name, linkname);
    	} else {
    		printf("         %-7d %-7s %-7s\n", pid, pDirent->d_name, linkname);
    	}
    	pDirent = readdir(pDir);
    	i++;
    }

    closedir(pDir);
    return i;

}

int printProc(int pid, int is_all, int i) {
	struct dirent *pDirent;
    DIR *pDir;

    int status = openDirectory(pid, &pDirent, &pDir);
    if (status == -1) return i; // No permissions for this process so don't print anything

    while (pDirent != NULL) {
    	if (is_all == -1) { // Print line numbers if all PIDs are being printed
    		printf(" %-7d %-7d %-7s\n", i, pid, pDirent->d_name);
    	} else {
    		printf("         %-7d %-7s\n", pid, pDirent->d_name);
    	}
    	pDirent = readdir(pDir);
    	i++;
    }

    closedir(pDir);
    return i;

}

int main(int argc, char **argv) {
	int per_proc = 0;
	int system = 0;
	int vnodes = 0;
	int composite = 0;
	int output_txt = 0;
	int output_bin = 0;
	int threshold = -1;
	int i = 0; // Variable to keep track of the current line number for each table
	int file_line = 0;

	int pid = -1;

	struct dirent *pDirent;
    DIR *pDir;

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '-') { // If the argument is not a flag, then it is the pid
			pid = strtol(argv[i], NULL, 10);
			break;
		}
	}

	// Get flags using getopt
	struct option opts[] = 
	{
		{"per-process", 0, 0, 'p'},
		{"systemWide", 0, 0, 's'},
		{"Vnodes", 0, 0, 'v'},
		{"composite", 0, 0, 'c'},
		{"output_TXT", 0, 0, 'x'},
		{"output_binary", 0, 0, 'b'},
		{"threshold", required_argument, 0, 't'},
		{0,0,0,0}
	};

	int opt_val = 0;
	int opt_index = 0;

	while ((opt_val = getopt_long(argc, argv, "psvcxbt::", opts, &opt_index)) != -1) {
		switch (opt_val)
		{
		case 'p':
			per_proc = 1;
			break;
		case 's':
			system = 1;
			break;
		case 'v':
			vnodes = 1;
			break;
		case 'c':
			composite = 1;
			break;
		case 'x':
			output_txt = 1;
			break;
		case 'b':
			output_bin = 1;
			break;
		case 't':
			threshold = atoi(optarg);
			break;
		case '?':
			break;
		}
	}

	if (!(per_proc || system || vnodes || composite)) composite = 1; // Default behavior if no flag (table) specified

	pDir = opendir("/proc"); // Open /proc directory
	if (pDir == NULL) {
		perror("Error opening /proc");
		return 1;
	}

	int curr_pid; // Variable corresponding to current PID for each iteration of the while loop
	if (per_proc) { // Block to print per-process FD table
		printf("         PID     FD \n        ============\n");
		while ((pDirent = readdir(pDir)) != NULL) {
			curr_pid = atoi(pDirent->d_name);
			if (curr_pid != 0 && (pid == -1 || curr_pid == pid)) {
				i = printProc(curr_pid, pid, i);
			}
		}
		printf("        ===========\n\n");
		rewinddir(pDir);
		i = 0;
	}

	if (system) { // Block to print systemWide FD table
		printf("         PID     FD      Filename \n        ========================================\n");
		while ((pDirent = readdir(pDir)) != NULL) {
			curr_pid = atoi(pDirent->d_name);
			if (curr_pid != 0 && (pid == -1 || curr_pid == pid)) {
				i = printSystem(curr_pid, pid, i);
			}
		}
		printf("       ========================================\n\n");
		rewinddir(pDir);
		i = 0;
	}

	if (vnodes) { // Block to print Vnodes FD table
		printf("           FD            Inode\n        ========================================\n");
		while ((pDirent = readdir(pDir)) != NULL) {
			curr_pid = atoi(pDirent->d_name);
			if (curr_pid != 0 && (pid == -1 || curr_pid == pid)) {
				i = printVNodes(curr_pid, pid, i);
			}
		}
		printf("        ========================================\n\n");
		rewinddir(pDir);
		i = 0;
	}

	if (composite || output_txt || output_bin) { // Block to print composite FD table & output to a txt or binary file if applicable
		if (composite) printf("         PID     FD       Filename       Inode\n        ========================================\n");
		while ((pDirent = readdir(pDir)) != NULL) {
			curr_pid = atoi(pDirent->d_name);
			if (curr_pid != 0 && (pid == -1 || curr_pid == pid)) {
				if (composite) i = printComposite(curr_pid, pid, i);
				if (output_txt || output_bin) file_line = saveToFile(curr_pid, pid, file_line, output_txt, output_bin);
			}
		}
		
		char footer[] = "        =========================================\n\n";
		FILE *file = NULL;
		if (output_txt) {
			file = fopen("compositeTable.txt", "a");
			fputs(footer, file);
			fclose(file);
		}
		if (output_bin) {
			file = fopen("compositeTable.bin", "ab");
			fwrite(footer, sizeof(char), strlen(footer), file);
			fclose(file);
		}
		if (composite) printf("%s", footer);
		rewinddir(pDir);
	}


	if (threshold >= 0) { // Block to handle threshold flag if applicable
		pid_s *head = NULL;
		pid_s *curr = NULL;
		while ((pDirent = readdir(pDir)) != NULL) {
			curr_pid = atoi(pDirent->d_name);
			if (curr_pid != 0) { // Create a linked list of offenders
				curr = getOffender(curr_pid, threshold);
				if (curr != NULL) {
					curr->next = head; // Insert at head
					head = curr;
				}
			}
		}

		curr = head;
		pid_s *prev = NULL;
		printf("## Offending processes: -- #FD threshold=%d\n", threshold);
		if (curr == NULL) printf("None\n");
		while (curr != NULL) { // Print linked list of offenders
			if (curr->next == NULL) { // Print last PID in list without a comma
				printf("%d (%d)\n", curr->pid, curr->num_fd);
				prev = curr;
				curr = curr->next;
				free(prev); // Free memory after printing
				break;
			}
			printf("%d (%d), ", curr->pid, curr->num_fd);
			prev = curr;
			curr = curr->next;
			free(prev); // Free memory after printing
		}
	}

	closedir(pDir);
	
	return 0;
}