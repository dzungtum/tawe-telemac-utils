/******************************************************************************
telemac-loader - part of tawe-telemac-utils
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
#include <sys/stat.h>
#include <math.h>

#include "telemac-loader.h"

/*!
 * @file
 * @brief TELEMAC SELAFIN file reader
 */

uint32_t int_swap(const uint32_t input) {
/*!
 * @brief Swap byte order of an integer
 *
 * Used to convert data read from results files
 * @param input	Integer to be converted
 * @returns		Converted value
 */
	const char *ip;
	char op[4];
	ip = (const char *)&input;
	op[3] = ip[0];
	op[2] = ip[1];
	op[1] = ip[2];
	op[0] = ip[3];
	uint32_t out = 0;
	memcpy(&out, &op, 4);
	return out;
}

float float_swap(const float value){
/*!
 * @brief Swap byte order of an float
 *
 * Used to convert data read from results files
 * @param value		Float to be converted
 * @returns		Converted value
 */
	const char *ip;
	char op[4];
	ip = (const char *)&value;
	op[3] = ip[0];
	op[2] = ip[1];
	op[1] = ip[2];
	op[0] = ip[3];
	float out = 0;
	memcpy(&out, &op, 4);
	return out;
}

int fortran_read(void* cstruct, size_t csize, size_t num, FILE* fromfile) {
/*!
 * @brief Read data from a FORTRAN formatted file
 *
 * Reads data from a binary file created using FORTRAN.
 * Data is read directly into the provided structures, without conversion.
 * @param cstruct	Pointer to start of output structure or array of structures
 * @param csize		Size of cstruct
 * @param num		Number of records to read
 * @param fromfile	FILE pointer to results file
 * @returns		Number of records read
 */
	uint32_t start_rec = 0;
	uint32_t end_rec = 0;
	int counter = 0;

	fread(&start_rec, sizeof(start_rec), 1, fromfile); // Read start of record marker

	counter = fread(cstruct, csize, num, fromfile); // Read contents

	fread(&end_rec, sizeof(end_rec), 1, fromfile);

	if (start_rec != end_rec) {
		fprintf(stderr, "Error: Reading requested record\n");
		fprintf(stderr, "\t Start and end of record yield different lengths. Variable length wrong?\n");
		fprintf(stderr, "\t start_rec: %d\t\tend_rec: %d\n", int_swap(start_rec), int_swap(end_rec));
		return -1;
	}
	return counter;
}

int open_telemac(resfile_t *rfile, int verbose) {
/*!
 * @brief Open a TELEMAC results file and store results for later use
 *
 * Reads single precision SERAFIN files and converts results from big-endian
 * format. Will not open run on double precision files.
 *
 * File structure taken from the TELEMAC user guide.
 *
 * @param rfile	Pointer to results structure
 * @param verbose	Non-zero for verbose output
 * @returns		EXIT_SUCCESS/EXIT_FAILURE
 */

	if (get_telemac_header(rfile, verbose) < 0) {
		perror("get_telemac_header");
		return -1;
	}

	if (get_telemac_mesh(rfile, verbose) < 0) {
		perror("get_telemac_mesh");
		return -2;
	}

	telemac_data_t *results = &rfile->tmdat;
	int fsr = fseeko(rfile->file, rfile->datastart + rfile->datasize * (results->nt), SEEK_SET);
	if (fsr != 0) {
		fprintf(stderr, "Failed to seek to end of file\n");
		perror("open_telemac");
		return EXIT_FAILURE|fsr;
	}
	char tmp = NULL;
	int fr = fread(&tmp, sizeof(char), 1, rfile->file);
	if (fr && !feof(rfile->file)) {
		off_t floc = ftello(rfile->file);
		fprintf(stderr, "Extra results->data at end of file:\n");
		fprintf(stderr, "EoF expected at 0x%jx\n", (intmax_t)floc);
		return EXIT_FAILURE|0x3;
	}

	return EXIT_SUCCESS;
}

int get_telemac_header(resfile_t * rfile, int verbose) {
//! Process file header and set up results structures

/*!
 * Take a resfile_t structure containing an open results file handle and
 * populates the size fields and telemac_data_t structure.
 *
 * @param rfile	A resfile_t structure containing an already opened file handle
 * @param verbose	Set non-zero to enable verbose output
 * @retval -1		Failure to reset file position or read R1
 * @retval -2		Failure to read R2
 * @retval -3		Failure to allocate memory for variable names
 * @retval -4		Failure to read IPARAM block
 * @retval -5		Failure to read simulation date
 * @retval -6		Failure to read R6
 */

	// Set file loc to 0
	// Read title, format, nbv{1,2}, date, iparam, nelem, npoin, ndp
	telemac_data_t *results = &rfile->tmdat;
	if (fseeko(rfile->file, 0, SEEK_SET) != 0) {
		fprintf(stderr, "Failed to seek to start of file\n");
		perror("get_telemac_header");
		return -1;
	}

	R1 resfile_r1;
	if (fortran_read(&resfile_r1, sizeof(R1), 1, rfile->file) == 1) {
		snprintf(results->title, 72, "%s", resfile_r1.title);
		snprintf(results->format, 8, "%s", resfile_r1.format);
		if (verbose) {
			fprintf(stdout, "Record 1:\n\tTitle:\t%s\n\tFormat:\t%s\n", results->title, results->format);
		}
	} else {
		perror("Error reading Record 1");
		return -1;
	}

	if (strcmp(results->format, "SERAFIN") != 0) {
		fprintf(stderr, "Only SERAFIN format currently supported\nFile identifies as:\t%s\n", results->format);
		return -1;
	}

	R2 resfile_r2;
	if (fortran_read(&resfile_r2, sizeof(R2), 1, rfile->file) == 1) {
		results->nbv_1 = int_swap(resfile_r2.nbv_1);
		results->nbv_2 = int_swap(resfile_r2.nbv_2);

		if (verbose) {
			fprintf(stdout, "Record 2:\n\tNBV(1):\t%d\n\tNBV(2):\t%d\n", results->nbv_1, results->nbv_2);
		}
	} else {
		perror("Error reading Record 2");
		return -2;
	}


	results->var_names = calloc(sizeof(char *), results->nbv_1);

	if (results->var_names == NULL) {
		perror("Unable to allocate results->var_names array\n");
		return -3;
	}

	if (verbose) {
		fprintf(stdout, "Simulation Variables:\n");
	}
	for (int i=0; i<results->nbv_1; i++) {
		results->var_names[i] = calloc(sizeof(char), 32);
		if (results->var_names == NULL) {
			perror("Unable to allocate results->var_names element\n");
			return -3;
		}
		fortran_read(results->var_names[i], 32, 1, rfile->file);
		results->var_names[i][31] = '\0';
		if (verbose) {
			fprintf(stdout, "\tVariable %d:\t%s\n", i, results->var_names[i]);
		}
	}

	if (verbose) {
		fprintf(stdout, "IPARAMS (R4:)\n");
	}
	if (fortran_read(results->iparam, 10*sizeof(uint32_t), 1, rfile->file)==1) {
		for (int i=0; i < 10; i++) {
			results->iparam[i] = int_swap(results->iparam[i]);
			if (verbose) {
				fprintf(stdout, "\tIPARAM(%d):\t%d\n", i, results->iparam[i]);
			}
		}
	} else  {
		perror("Unable to read IPARAMs");
		return -4;
	}

	if (results->iparam[9] == 1) {
		R5 resfile_r5;
		if (fortran_read(&resfile_r5, sizeof(R5), 1, rfile->file) == 1) {
			results->date.year = int_swap(resfile_r5.year);
			results->date.month = int_swap(resfile_r5.month);
			results->date.day = int_swap(resfile_r5.day);
			results->date.hour = int_swap(resfile_r5.hour);
			results->date.minute = int_swap(resfile_r5.minute);
			results->date.second = int_swap(resfile_r5.second);
			if (verbose) {
				fprintf(stdout, "Simulation Date: %d-%d-%d %d:%d:%d\n", results->date.year, results->date.month, results->date.day, results->date.hour, results->date.minute, results->date.second);
			}
		} else {
			perror("Unable to read date");
			return -5;
		}
	}

	R6 resfile_r6;
	if (fortran_read(&resfile_r6, sizeof(R6), 1, rfile->file) == 1) {
		results->nelem = int_swap(resfile_r6.nelem);
		results->npoin = int_swap(resfile_r6.npoin);
		results->ndp = int_swap(resfile_r6.ndp);
		resfile_r6.one = int_swap(resfile_r6.one);

		if (verbose) {
			fprintf(stdout, "Record 6:\n\tNumber of elements: \t%d\n\tNumber of points: \t%d\n\tPoints per element: \t%d\n", results->nelem, results->npoin, results->ndp);
		}

		if (resfile_r6.one != 1) {
			fprintf(stderr, "R6.one isn't equal to one! (R6.one = %d)\n", resfile_r6.one);
		}
	} else {
		perror("Error reading record containing NELEM, NPOIN, NDP");
		return -6;
	}

	results->state = 1;
	rfile->meshstart = ftello(rfile->file);
	return 0;
}

int get_telemac_mesh(resfile_t *rfile, int verbose) {
//! Load mesh data from file

/*!
 * Populate mesh fields of telemac_data_t structure
 * @param rfile	resfile_t structure populated by open_telemac and get_telemac_header
 * @param verbose Set non-zero for verbose output
 * @retval -1	Invalid telemac_data_t state or failed to read IKLE
 * @retval -2	Failed to read or allocate memory for IPOBO
 * @retval -3	Failed to read or allocate memory for X coordinates
 * @retval -4	Failed to read or allocate memory for Y coordinates
 * @retval 0	Success
 */
	telemac_data_t *results = &rfile->tmdat;

	if (results->state != 1) {
		fprintf(stderr, "get_telemac_data called with bad results state (Want state = 1, got %d)\n", results->state);
		return -1;
	}

	if (fseeko(rfile->file, rfile->meshstart, SEEK_SET) != 0) {
		fprintf(stderr, "Failed to seek to start of mesh (0x%jx)\n", (intmax_t)rfile->meshstart);
		perror("get_telemac_mesh");
		return -1;
	}

	results->ikle = calloc(sizeof(uint32_t), results->nelem*results->ndp);
	if (results->ikle == NULL) {
		perror("Unable to allocate first dimension of IKLE");
		return -1;
	}
	int t =0;
	if ((t=fortran_read(results->ikle, sizeof(uint32_t), results->nelem * results->ndp, rfile->file)) == results->nelem * results->ndp) {
		for (int i = 0; i < results->nelem*results->ndp; i++) {
			results->ikle[i] = int_swap(results->ikle[i]);
		}

		if (verbose) {
			fprintf(stdout, "Succesfully read %d entries into IKLE\n", t);
		}

	} else {
		perror("Failed to read entries into IKLE");
		fprintf(stderr, "Read %d of %d entries\n", t, results->nelem * results->ndp);
		return -1;
	}

	results->ipobo = calloc(sizeof(uint32_t), results->npoin);
	if (results->ipobo == NULL) {
		perror("Unable to allocate results->ipobo");
		return -2;
	}

	if ((t=fortran_read(results->ipobo, sizeof(uint32_t), results->npoin, rfile->file)) == results->npoin) {
		for (int i = 0; i < results->npoin; i++) {
			results->ipobo[i] = int_swap(results->ipobo[i]);
		}
		if (verbose) {
			fprintf(stdout, "Successfully read %d entries into IPOBO\n", t);
		}
	} else {
		perror("Failed to read entries into IPOBO");
		fprintf(stderr, "Read %d of %d entries\n", t, results->npoin);
		return -2;
	}

	results->X = calloc(sizeof(float), results->npoin);

	if (results->X == NULL) {
		perror("Failed to allocate memory for X coordinates");
		return -3;
	}

	results->XYrange[0] = INFINITY;
	results->XYrange[1] = -INFINITY;
	results->XYrange[2] = INFINITY;
	results->XYrange[3] = -INFINITY;

	if ((t = fortran_read(results->X, sizeof(float), results->npoin, rfile->file)) == results->npoin) {
		for (int i = 0; i < results->npoin; i++) {
			results->X[i] = float_swap(results->X[i]);
			if (results->X[i] < results->XYrange[0]) {
				results->XYrange[0] = results->X[i];
			}

			if (results->X[i] > results->XYrange[1]) {
				results->XYrange[1] = results->X[i];
			}
		}
		if (verbose) {
			fprintf(stdout, "Successfully read %d entries into X\n", t);
		}
	} else {
		perror("Failed to read entries into X");
		if (verbose) {
			fprintf(stderr, "Read %d of %d entries\n", t, results->npoin);
		}
		return -3;
	}

	results->Y = calloc(sizeof(float), results->npoin);

	if (results->Y == NULL) {
		perror("Failed to allocate memory for Y coordinates");
		return -4;
	}
	if ((t = fortran_read(results->Y, sizeof(float), results->npoin, rfile->file)) == results->npoin) {
		for (int i = 0; i < results->npoin; i++) {
			results->Y[i] = float_swap(results->Y[i]);
			if (results->Y[i] < results->XYrange[2]) {
				results->XYrange[2] = results->Y[i];
			}

			if (results->Y[i] > results->XYrange[3]) {
				results->XYrange[3] = results->Y[i];
			}
		}
		if (verbose) {
			fprintf(stdout, "Successfully read %d entries into Y\n", t);
		}
	} else {
		perror("Failed to read entries into Y");
		fprintf(stderr, "Read %d of %d entries\n", t, results->npoin);
		return -4;
	}


	if (verbose) {
		fprintf(stdout, "\nHeader data complete.\n\n");
	}

	int fd = fileno(rfile->file);
	struct stat buf;
	int fsret = fstat(fd, &buf);
	if (fsret != 0) {
		fprintf(stderr, "Error calling fstat on rfile->file\n");
		perror("fsret");
		return fsret;
	}
	off_t size = buf.st_size;

	if (verbose) {
		fprintf(stdout, "Mesh data ends at position %jd. File size is %jd\n", (intmax_t)ftello(rfile->file), (intmax_t)size);
	}

	rfile->datasize = (8 + sizeof(uint32_t) + (results->nbv_1 + results->nbv_2) * (sizeof(float) * results->npoin + 8));
	results->nt = ((long long)(size - ftello(rfile->file)) / rfile->datasize);
	if (verbose) {
		fprintf(stdout, "Number of timesteps: \t%d\n", results->nt);
	}

	results->timestamp = calloc(sizeof(float), results->nt);
	if (results->timestamp == NULL) {
		perror("Unable to allocate timestamp array");
		return -5;
	}

	results->state = 2;
	rfile->datastart = ftello(rfile->file);
	return 0;
}

float** get_telemac_data(resfile_t *rfile, int timestep, int verbose) {
//! Return simulation results for a given timestep

/*!
 * Read variable information for timestep, using information from resfile_t
 * to seek to the correct file location. Returns an a
 * populates the size fields and telemac_data_t structure.
 *
 * @param rfile	A resfile_t structure containing an already opened file handle
 * @param timestep	Timestep to load
 * @param verbose	Set non-zero for verbose output
 * @retval float**	Pointer to requested data
 * @retval NULL 	Returned on error
 */
	// Using info in rfile, set file position to start of timestep
	// Read timestamp
	// Read data

	telemac_data_t *results = &rfile->tmdat;

	if (results->state != 2) {
		fprintf(stderr, "get_telemac_data called with bad results state (Want state >= 2, got %d)\n", results->state);
		return NULL;
	}

	if (fseeko(rfile->file, rfile->datastart + timestep * rfile->datasize, SEEK_SET) != 0) {
		fprintf(stderr, "Unable to seek to start of timestep\n");
		perror("get_telemac_data");
		return NULL;
	}

	fortran_read(&results->timestamp[timestep], sizeof(float), 1, rfile->file);
	results->timestamp[timestep] = float_swap(results->timestamp[timestep]);
	if (verbose) {
		fprintf(stdout, "Step: \t%d\t\tTime: \t%f\n", timestep, results->timestamp[timestep]);
	}
	float **data = NULL;
	data = calloc(sizeof(float *), (results->nbv_1 + results->nbv_2));

	if (data == NULL) {
		perror("get_telemac_data: Allocating data[]");
		return NULL;
	}

	for (int j = 0; j < (results->nbv_1 + results->nbv_2); j++) {
		data[j] = NULL;
		data[j] = calloc(sizeof(float), results->npoin);
		if (data[j] == NULL) {
			free(data);
			perror("get_telemac_data: Allocating data[][]");
			return NULL;
		}
		fortran_read(data[j], sizeof(float), results->npoin, rfile->file);
		for (int i = 0; i < results->npoin; i++) {
			data[j][i] = float_swap(data[j][i]);
		}
	}
	return data;
}

void free_telemac_data(resfile_t *rfile, float **data) {
//! Free simulation results array

/*! Iterate over an array returned by get_telemac_data and free memory
 * @param rfile	A resfile_t structure containing an already opened file handle
 * @param data	Array to be freed
 * @returns void
 */
	// Free data allocated in get_telemac_data

	telemac_data_t results = rfile->tmdat;
	for (int j = 0; j < (results.nbv_1 + results.nbv_2); j++) {
		free(data[j]);
		data[j] = NULL;
	}
	free(data);
	data = NULL;
}
