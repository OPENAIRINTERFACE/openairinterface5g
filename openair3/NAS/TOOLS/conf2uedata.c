#include <stdio.h>  // perror, printf, fprintf, snprintf
#include <stdlib.h> // exit, free
#include <string.h> // memset, strncpy
#include <getopt.h>

#include "conf2uedata.h"
#include "display.h"
#include "conf_parser.h"

int main(int argc, char**argv) {
	int option;
    const char* conf_file = NULL;
    const char* output_dir = NULL;
    const char options[]="c:o:h";

    while ((option = getopt(argc, argv, options)) != -1) {
		switch (option) {
		case 'c':
			conf_file = optarg;
			break;
		case 'o':
			output_dir = optarg;
			break;
		case 'h':
			_display_usage();
			return true;
			break;
		default:
			break;
		}
	}

	if (output_dir == NULL ) {
		printf("No output option found\n");
		_display_usage();
		exit(1);
	}

    if ( conf_file == NULL ) {
		printf("No Configuration file is given\n");
		_display_usage();
		exit(1);
	}

    if ( parse_config_file(output_dir, conf_file, OUTPUT_ALL) == false ) {
        exit(1);
    }

    display_data_from_directory(output_dir, DISPLAY_ALL);

	exit(0);
}

/*
 * Displays command line usage
 */
void _display_usage(void) {
	fprintf(stderr, "usage: conf2uedata [OPTION]...\n");
	fprintf(stderr, "\t[-c]\tConfig file to use\n");
	fprintf(stderr, "\t[-o]\toutput file directory\n");
	fprintf(stderr, "\t[-h]\tDisplay this usage\n");
}
