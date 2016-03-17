/******************************************************************************
telemac-parse - part of tawe-telemac-utils
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
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdbool.h>
#include <unistd.h>

#include "telemac-loader.h"

/*!
 * @file
 * @brief Parse TELEMAC results data into a series of flat files
 *
 * Reads a SELAFIN file and writes data to a series of files in ASCII or binary format.
 *
 * Returns zero on success and non-zero if an error occurs
 */

int main (int argc, char** argv) {
	FILE *resfile =NULL;
	char *filename = NULL;
	char *basefilename = NULL;
	char *outputdir = ".";

	bool verbose = false;
	bool binaryout = false;

	char *usage = "%s [-v] [-b] [-o dir] <filename>\n\t-v\tVerbose output\n\t-b\tEnable binary output of variable data\n\t-o\tOutput directory\n";

	opterr = 0;
	int go = 0;
	while ((go = getopt(argc, argv, "vbo:")) != -1) {
		switch (go) {
			case 'v':
				verbose = true;
				break;
			case 'b':
				binaryout = true;
				break;
			case 'o':
				outputdir = strdup(optarg);
				break;
			case '?':
				fprintf(stderr, "Unrecognised option '-%c'\n", optopt);
				fprintf(stderr, usage, argv[0]);
				return EXIT_FAILURE;
		}
	}

	if (argc - optind != 1) {
		fprintf(stderr, "Must provide a file to convert\n");
		fprintf(stderr, usage, argv[0]);
		return EXIT_FAILURE;
	}

	filename = argv[optind];
	resfile = fopen(filename, "rb");
	if (resfile == NULL) {
		perror("Unable to open file");
		return EXIT_FAILURE;
	}

	resfile_t rfs = {resfile, 0, 0, 0};

	asprintf(&basefilename, "%s/%s", outputdir, basename(filename));

	fprintf(stdout, "\nOpening OpenTelemac RES file %s:\n", basefilename);
	int rval = open_telemac(&rfs, verbose);

	telemac_data_t results = rfs.tmdat;

	fprintf(stdout, "open_telemac call returned %d\n", rval);

	FILE *xfile, *yfile, *connfile, *varfile, *datafile, *tsfile;
	char *xfilename, *yfilename, *connfilename, *varfilename, *datafilename, *tsfilename;

	if (verbose) {
		fprintf(stdout, "Writing out coordinates...\n");
	}

	asprintf(&xfilename, "%s.x.txt", basefilename);
	xfile = fopen(xfilename, "w+");
	if (xfile == NULL) {
		perror("Unable to open X output file");
		return EXIT_FAILURE;
	}

	asprintf(&yfilename, "%s.y.txt", basefilename);
	yfile = fopen(yfilename, "w+");
	if (yfile == NULL) {
		perror("Unable to open Y output file");
		return EXIT_FAILURE;
	}

	fprintf(xfile, "%d\n", results.npoin);
	fprintf(yfile, "%d\n", results.npoin);

	for (int i=0; i<results.npoin; i++) {
		fprintf(xfile,"%d\t%.10f\n", i, results.X[i]);
		fprintf(yfile,"%d\t%.10f\n", i, results.Y[i]);

	}

	free(xfilename);
	free(yfilename);
	fclose(xfile);
	fclose(yfile);

	if (verbose) {
		fprintf(stdout, "Writing out connectivity...\n");
	}

	asprintf(&connfilename, "%s.conn.txt", basefilename);
	connfile = fopen(connfilename, "w+");
	if (connfile == NULL) {
		perror("Unable to open connectivity output file");
		return EXIT_FAILURE;
	}

	fprintf(connfile, "%d\t%d\n", results.nelem, results.ndp);
	for (int e=0; e<results.nelem; e++) {
		for (int j=0; j<results.ndp; j++) {
			/* Seems to work fine for 2D and 3D despite the line in the manual RE: array shape*/
			fprintf(connfile, "%d\t%d\n", e, results.ikle[(e*results.ndp)+j]-1);
		}
	}


	free(connfilename);
	fclose(connfile);

	if (verbose) {
		fprintf(stdout, "Writing out variable names...\n");
	}

	asprintf(&varfilename, "%s.vars.txt", basefilename);
	varfile = fopen(varfilename, "w+");
	if (varfile == NULL) {
		perror("Unable to open variable names output file");
		return EXIT_FAILURE;
	}

	fprintf(varfile, "%d\n", results.nbv_1);
	for (int i = 0; i < results.nbv_1; i++) {
		fprintf(varfile,"%d\t%s\n", i, results.var_names[i]);
	}

	free(varfilename);
	fclose(varfile);

	if (verbose) {
		if (binaryout) {
			fprintf(stdout, "Writing out data (binary mode)...\n");
		} else {
			fprintf(stdout, "Writing out data (text mode)...\n");
		}
	}

	for (int t = 0; t < results.nt; t++) {
		float **data = NULL;
		data = get_telemac_data(&rfs, t, verbose);
		if (data == NULL) {
			perror("NULL returned from get_telemac_data");
			return EXIT_FAILURE;
		}

		for (int i = 0; i<results.nbv_1; i++) {
			if (binaryout) {
				asprintf(&datafilename, "%s.var%d.t%d.dat", basefilename, i, t);
				datafile = fopen(datafilename, "wb+");
			} else {
				asprintf(&datafilename, "%s.var%d.t%d.txt", basefilename, i, t);
				datafile = fopen(datafilename, "w+");
			}

			if (datafile == NULL) {
				fprintf(stderr, "Unable to open output file for variable %d (%s) at timestep number %d\n", i, results.var_names[i], t);
				perror(NULL);
				return EXIT_FAILURE;
			}

			if (binaryout) {
				double *dd = NULL;
				dd = calloc(sizeof(double), results.npoin);
				if (dd == NULL) {
					fprintf(stderr, "Unable to allocate memory for double conversion\n");
					perror("calloc");
					return EXIT_FAILURE;
				}

				for (int k = 0; k < results.npoin; k++) {
					dd[k] = data[i][k];
				}

				int fwcount = fwrite(dd, sizeof(double), results.npoin, datafile);
				if (fwcount != results.npoin) {
					free(dd);
					fprintf(stderr, "Error writing results for variable %d (%s) at timestep number %d\n", i, results.var_names[i], t);
					perror("fwrite");
					return EXIT_FAILURE;
				}
				free(dd);
			} else {
				for (int k = 0; k < results.npoin; k++) {
					fprintf(datafile, "%d\t%+.10f\n", k, data[i][k]);
				}
			}
			fclose(datafile);
			free(datafilename);
		}
		free_telemac_data(&rfs, data);
	}

	if (verbose) {
		fprintf(stdout, "Writing out timestamps...\n");
	}

	asprintf(&tsfilename, "%s.times.txt", basefilename);
	tsfile = fopen(tsfilename, "w+");
	if (connfile == NULL) {
		perror("Unable to open timestep output file");
		return EXIT_FAILURE;
	}

	fprintf(tsfile, "%d\n", results.nt);
	for (int i = 0; i < results.nt; i++) {
		fprintf(tsfile,"%d\t%+.10f\n", i, results.timestamp[i]);
	}

	free(tsfilename);
	fclose(tsfile);

	return EXIT_SUCCESS;
}

