#include "depend.h"
#include "stdio.h"
#include "unistd.h"
#include "string.h"

void greetings() {
	printf("Ian's World!\n");
}

bool aiger_filename_check(const char* filename) {
	// Check that filename length is at least 4
	if (strlen(filename) < 4) {
		return false;
	}

	char* last_four_filename = filename + strlen(filename) - 4;
	
	// Check that file ends in .aig and that the file exists
	if (strcmp(last_four_filename, ".aig") || access(filename, F_OK) != 0) {
		return false;
	}

	return true;
}

unsigned aiger_read_unsigned( unsigned char ** ppPos )
{
    unsigned x = 0, i = 0;
    unsigned char ch;
    while ((ch = *(*ppPos)++) & 0x80)
        x |= (ch & 0x7f) << (7 * i++);
    return x | (ch << (7 * i));
}

void read_aig(const char* filename) {
	FILE *fp;
	int c;

	fp = fopen(filename, "r");

	if (!fp) {
		printf("Error opening the file %s", filename);
		exit(1);
	}

	char* line = NULL;
	char* header;
    size_t len = 0;
    ssize_t line_len;

	size_t line_cnt = 0;
	
	int MILOA[5];


	// Read in the AIG file line-by-line
	while ((line_len = getline(&line, &len, fp)) != -1) {
		// Read the AIG header in MILOA format
		if (line_cnt == 0) {
			char* token = strtok(line, " ");
			if (strcmp("aig", token)) {
				perror("Malformed AIG header!\n");
			}

			for (int i = 0; i < 5; i++) {
				token = strtok(NULL, " ");
				if (token == NULL) { 
					perror("Malformed AIG header!\n");
				}

				MILOA[i] = atoi(token);
				printf("%d\n", MILOA[i]);
			}
		}

		if (line_cnt > 3) {
			printf("%s\n", line);
			// Read the AND gates?
			for (int i = 0; i < MILOA[4]; i++) {
				unsigned uLit = ((i + 1 + MILOA[1] + MILOA[2]) << 1);
				unsigned uLit1 = uLit  - aiger_read_unsigned(&line);
        		unsigned uLit0 = uLit1 - aiger_read_unsigned(&line);
				printf("%u %u %u\n", uLit, uLit1, uLit0);
			}
			
		}
		
		line_cnt += 1;
    }





	fclose(fp);
}