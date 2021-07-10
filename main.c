#include "stdio.h"
#include "depend.h"
#include "stdlib.h"

int main(int argc, char** argv) {
	if (argc != 2) {
		perror("You did not provide any file name!");
		exit(1);
	}

	if (aiger_filename_check(argv[1])) {
		printf("Success loading the AIG file\n");
	} else {
		perror("Error, file name does not exist or malformatted string input!");
		exit(1);
	}

	printf("Hey there, welcome to ");
	greetings();

	read_aig_wrapper(argv[1]);

	
	return 0;	
}
