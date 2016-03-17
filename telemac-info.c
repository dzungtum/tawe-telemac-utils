/******************************************************************************
telemac-info - part of tawe-telemac-utils
Copyright (C) 2016 Thomas Lake

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, US
*******************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include "telemac-loader.h"

/*!
 * @file
 * @brief Display summary information about a results file
 *
 *
 * Reads a SELAFIN file and outputs summary information.
 * Returns zero on success and non-zero if an error occurs
 *
 */
int main (int argc, char** argv) {
	FILE *resfile =NULL;
	char *filename = NULL;
	char *basefilename = NULL;
	int verbose = 0;
	int force = 0;

	const char *usage = "Usage: %s [-v] [-f] filename\n\t-v\tVerbose output\n\t-f\tForce mode\n";

	telemac_data_t results;

	int go = 0;
	while ((go = getopt(argc, argv, "vf")) != -1) {
		switch (go) {
			case 'v':
				verbose++;
				break;
			case 'f':
				force = 1;
				break;
			case '?':
				fprintf(stderr, "Unrecognised option '-%c'\n", optopt);
			default:
				fprintf(stderr, usage, argv[0]);
				return EXIT_FAILURE;
		}
	}

	if (argc - optind != 1) {
		fprintf(stderr, "Must specify a single input file\n");
		fprintf(stderr, usage, argv[0]);
		return EXIT_FAILURE;
	}
	filename = argv[optind];

	resfile = fopen(filename, "rb");
	if (resfile == NULL) {
		perror("Unable to open file");
		return EXIT_FAILURE;
	}

	basefilename=basename(filename);

	printf("\nOpening results file %s:\n", basefilename);
	resfile_t rfs = {resfile, 0, 0, 0};

	// Call open_telemac, but only pass on verbose option if verbose set to 2 or more
	int rval = open_telemac(&rfs, (verbose > 1 ? 1 : 0));
	results = rfs.tmdat;
	if (verbose) {
		printf("open_telemac returned %d\n", rval);
	}
	if (rval < 0) {
		if (force) {
			printf("** Errors found - will attempt to continue\n");
		} else {
			printf("** Errors found - aborting\nRun in force mode (-f) to attempt to continue\n");
			return rval;
		}
	}

	printf("\nTitle: \t\t%s\nFormat: \t%s\n", results.title, results.format);
	if (results.iparam[9]==1) {
		printf("Date: \t\t%04d-%02d-%02d %02d:%02d:%02d\n", results.date.year, results.date.month, results.date.day, results.date.hour, results.date.minute, results.date.second);
	}

	if (verbose) {
		printf("\nIPARAM table:\n");
		printf("\t1: %d\n\t2: %d\n\t3: %d\n\t4: %d\n\t5: %d\n\t6: %d\n\t7: %d\n\t8: %d\n\t9: %d\n\t10: %d\n",
				results.iparam[0], results.iparam[1], results.iparam[2], results.iparam[3], results.iparam[4],
				results.iparam[5], results.iparam[6], results.iparam[7], results.iparam[8], results.iparam[9]);
	}
	printf("\nRecorded variables:\n");
	for (int n = 0; n < results.nbv_1; n++) {
		printf("\t%d: %s\n", n, results.var_names[n]);
	}
	printf("\nCoordinate Range:\n\tX: %+f, %+f\n\tY: %+f, %+f\n", results.XYrange[0], results.XYrange[1], results.XYrange[2], results.XYrange[3]);

	printf("\n%d Nodes\n%d Elements\n%d nodes per element\n", results.npoin, results.nelem, results.ndp);

	printf("\nSimulation Times:\n");
	if (verbose == 1) {
		for (int t = 0; t < results.nt; t++) {
			float **data = NULL;
			data = get_telemac_data(&rfs, t, verbose);
			if (data == NULL) {
				perror("NULL returned from get_telemac_data");
				return EXIT_FAILURE;
			}
			printf("\t%d: %+f\n", t, results.timestamp[t]);
			free_telemac_data(&rfs, data);
		}
	} else {
		float **data = NULL;
		data = get_telemac_data(&rfs, 0, verbose);
		if (data == NULL) {
			fprintf(stderr, "Error reading initial timestep data - NULL returned by get_telemac_data\n");
			return EXIT_FAILURE;
		}
		free_telemac_data(&rfs, data);
		data = get_telemac_data(&rfs, results.nt-1, verbose);
		if (data == NULL) {
			fprintf(stderr, "Error reading initial timestep data - NULL returned by get_telemac_data\n");
			return EXIT_FAILURE;
		}
		free_telemac_data(&rfs, data);
		printf("\t%d timesteps\n", results.nt);
		printf("\tSimulation start: t = %+f\n", results.timestamp[0]);
		printf("\tSimulation end:   t = %+f\n", results.timestamp[results.nt - 1]);
		printf("\tRun again with verbose flag to list individual timestamps\n");
	}

	fclose(resfile);
	printf("\nEnd.\n");

	return EXIT_SUCCESS;
}

