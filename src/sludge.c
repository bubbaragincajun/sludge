// sludge Archiving utility
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include "crc32.h"
// TODO: Add indexing of files
// TODO: removing files from archives
// TODO: if a file is an exact copy, have archive point to the first instance of
//		 the file that is similar
// TODO: ^ use a flag to determine if it is a copy, then point to index of same
//       file.
// TODO: update list, update, extract to reflect new header format
// TODO: update switch case in main to reflect mutually exclusive options for
//       command line invocation

/* New header format: 
 * size_of_file    st.size
 * file_modes      st.mode
 * hash            uint32_t
 * dupe_flag       uint32_t   *This is for future proofing 
   offset                     (big archives with big offsets)
 * name_length     size_t
 * file_name       dependent upon value of name_length
*/

/***********************************************
 * function: update                            *
 *                                             *
 * update functions as both an appending       *
 * utility and also the default archive        *
 * creator.                                    *
 *                                             *
 * Note: file contents are only added if there *
 * is not another copy of the exact same data  *
 * already in the archive                      *
 **********************************************/


int update(int argc, char **argv, const char *mode) {
	FILE *archive = fopen(argv[1], mode);
	struct stat s;
	uint8_t buf[1024];
	for (int i = 2; i < argc; i++) {
		if (stat(argv[i], &s) == -1) {
			fprintf(stderr, "Failed to stat file %s\n", argv[i]);
			return 1;
		}
		fwrite(&s.st_size, sizeof(s.st_size), 1, archive);
		fwrite(&s.st_mode, sizeof(s.st_mode), 1, archive);
		size_t l = strlen(argv[i]);
		fwrite(&l, sizeof(size_t), 1, archive);
		fwrite(argv[i], l, 1, archive);

		FILE *in = fopen(argv[i], "r");
		while (!feof(in)) {
			size_t n = fread(buf, 1, sizeof(buf), in);
			fwrite(buf, 1, n, archive);
		}
		fclose(in);
	}
	fclose(archive);
	return 0;
}


/***********************************************
 * function: list                              *
 *                                             *
 * list gives a listing of all unique file     *
 * file descriptors in the archive             *
 *                                             *
 *                                             *
 *                                             *
 *                                             *
 **********************************************/
int list(int argc, char **argv) {
	FILE *archive = fopen(argv[1], "r");
	struct stat s;
	uint8_t buf[1024];
	while (!feof(archive)) {
		if (!fread(&s.st_size, sizeof(s.st_size), 1, archive)) {
			break;
		}
		fread(&s.st_mode, sizeof(s.st_mode), 1, archive);
		size_t l;
		fread(&l, sizeof(size_t), 1, archive);
		char name[l + 1];
		fread(name, l, 1, archive);
		name[l] = 0;
		printf("%s\n", name);
		
		fseek(archive,s.st_size,SEEK_CUR);		
	}
	fclose(archive);
	return 0;
}

/***********************************************
 * function: extract                           *
 *                                             *
 * extract creates a new file with the same    *
 * descriptor, permissions, and file contents  *
 * as the original contained in the archive    *
 *                                             *
 *                                             *
 *                                             *
 **********************************************/

int extract(int argc, char **argv){
	FILE *archive = fopen(argv[1], "r");
	struct stat s;
	uint8_t buf[1024];
	while (!feof(archive)) {
		if (!fread(&s.st_size, sizeof(s.st_size), 1, archive)) {
			break;
		}
		fread(&s.st_mode, sizeof(s.st_mode), 1, archive);
		size_t l;
		fread(&l, sizeof(s.st_size), 1, archive);
		char name[l + 1];
		fread(name, l, 1, archive);
		name[l] = 0;
		bool extract = true;
		for (size_t i = 2; i < argc; ++i) {
			extract = false;
			if (strcmp(argv[i], name) == 0) {
				extract = true;
				break;
			}
		}
		if (extract) {
			FILE *out = fopen(name, "w");
			while (s.st_size) {
				size_t n = fread(buf, 1, s.st_size, archive);
				fwrite(buf, n, 1, out);
				s.st_size -= n;
			}
			fclose(out);
			chmod(name, s.st_mode);
		}
	}
	fclose(archive);
	return 0;

}

/***********************************************
 * function: remove                            *
 *                                             *
 * Removes specified files from the archive    *
 * and also extracts data into new files       *
 *                                             *
 * Note: file data is only removed IF there    *
 * are no other descriptors that reference     *
 * the same data block                         *
 **********************************************/

int remove(int argc, char* argv[]) {

  return 0;
}

/***********************************************
 * function: main                              *
 *                                             *
 * Handles user input and directs program      *
 * flow towards the repsective functions       *
 *                                             *
 *                                             *
 *                                             *
 *                                             *
 **********************************************/

int main(int argc, char **argv) {
	const char *usage =
		"	sludge is an archiving utility\n"
		"Default: ./sludge <archiveName> <file> ...\n"
		"	Archives all files passed to it.\n"
		"List: 	  ./sludge -l <acrhiveName>\n"
		"	Lists files from the archive.\n"
		"Append:  ./sludge -a <archiveName> <file1> ...\n"
		"	Appends the file to the archive.\n"
		"Extract: ./sludge -e <archiveName> <file1> ...\n"
		"	Extracts all files unless giving a filename.\n"
	        "Remove:  ./sludge -r <archiveName> <file1> ...\n"
	        "       Removes all files unless given a filename\n";
	switch (getopt(argc, argv, "l:a::e::")) {
	case 'l':
		return list(argc - 1, argv + 1);
	case 'a':
		return update(argc - 1, argv + 1, "a+");
	case 'e':
		return extract(argc - 1, argv + 1);
	default:
		if (argc < 2) {
			fprintf(stderr, "%s", usage);
			return 1;
		}
		return update(argc, argv, "w+");
	}
	return 0;
}
