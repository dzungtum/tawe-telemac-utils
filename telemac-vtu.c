/******************************************************************************
telemac-vtu - part of tawe-telemac-utils
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
#include <unistd.h>
#include <libxml/xmlwriter.h>
#include <libxml/tree.h>

#include "telemac-loader.h"

/*!
 * @file
 * @brief Export mesh to VTU/PVD files for paraview
 *
 * Reads a SELAFIN file and exports details to VTU and PVD files suitable
 * for use with Paraview - an open-source visualisation tool.
 *
 * Returns zero on success and non-zero if an error occurs
 */

int writeTimestep(void *wtsargs);

//! Arguments for writeTimestep
typedef struct {
	int t; //!< Timestep
	resfile_t *rfs; //!< Results file structure
	char *file; //!< Filename
	int z; //!< Variable to use for Z coordinates
	int u; //!< Variable to use for U velocity component
	int v; //!< Variable to use for V velocity component
	int w; //!< Variable to use for W velocity component
	int verbose; //!< Non-zero for verbose output
} wTSargs;


int main(int argc, char **argv) {

	char *filename = NULL; // Assigned below getopt() switch
	char *outputpath = "./";
	int verbose = 0;
	int force = 0;
	int z = 0;
	int u = 1;
	int v = 2;
	int w = 3;
	int printfreq = 1;
	int ts = -1;

	char *usage =  "Usage: %s [-z Z] [-u U] [-v V] [-w W] [-t T|-f n] [-c] [-o output_path] <results file>\n"
		"\t-c\tVerbose output\n"
		"\t-F\tForce continuation on certain errors\n"
		"\t-f\tExport every n^th timestep\n"
		"\t-t\tExport single timestep T\n"
		"\t-z\t|\n"
		"\t-u\t|\n"
		"\t-v\t} Specify index for Z (height) and velocity components (u,v,w)\n"
		"\t-w\t|\n"
		"\t-o\tSpecify output folder for result files\n";
	opterr = 0;

	int go = 0;
	int oplength = -1;
	while ((go = getopt (argc, argv, "u:v:w:z:f:o:t:cF")) != -1) {
		switch(go) {
			case 'z':
				z = atoi(optarg);
				break;
			case 'u':
				u = atoi(optarg);
				break;
			case 'v':
				v = atoi(optarg);
				break;
			case 'w':
				w = atoi(optarg);
				break;
			case 'c':
				verbose = 1;
				break;
			case 'F':
				force = 1;
				break;
			case 't':
				ts = atoi(optarg);
				break;
			case 'f':
				printfreq = atoi(optarg);
				if (printfreq <= 0) {
					fprintf(stderr, "Print frequency must be greater than 0\n");
					return EXIT_FAILURE;
				}
				break;
			case 'o':
				oplength = strlen(optarg);
				if (optarg[oplength - 1] != '/' || optarg[oplength -1] != '\\') {
					asprintf(&outputpath, "%s/", optarg);
				} else {
					asprintf(&outputpath, "%s", optarg);
				}
				break;
			case '?':
				if (optopt == 'f' || optopt == 't') {
					fprintf(stderr, "The -%c option requires a (positive, integer) value\n", optopt);
					return EXIT_FAILURE;
				} else {
					fprintf(stderr, "Unknown option `-%c'.\n", optopt);
					fprintf(stderr, usage, argv[0]);
					return EXIT_FAILURE;
				}
				break;
		}
	}

	if (argc - optind != 1) {
		fprintf(stderr, "%s: A single SLF file must be provided\n", argv[0]);
		fprintf(stderr, usage, argv[0]);
		return EXIT_FAILURE;
	} else {
		filename = argv[optind];
	}

	if (printfreq != 1 && ts >= 0) {
		fprintf(stderr, "Single timestep and output frequency options are mutually exclusive.\n");
		fprintf(stderr, usage, argv[0]);
		return EXIT_FAILURE;
	}

	FILE *resfile = NULL;
	resfile = fopen(filename, "rb");
	if (resfile == NULL) {
		perror("Unable to open input file");
		return EXIT_FAILURE;
	}

	resfile_t rfs = {resfile, 0, 0, 0};

	int otres;
	otres = open_telemac(&rfs, verbose);

	if (otres != 0) {
		fprintf(stderr, "Error: open_telemac call returned %d\n", otres);
		if (force) {
			fprintf(stderr, "Force mode specified - will attempt to continue\n");
		} else {
			return EXIT_FAILURE;
		}
	}
	telemac_data_t *mesh = &rfs.tmdat;

	int tStart = ts >= 0 ? ts : 0;
	int tLimit = ts >= 0 ? ts + 1 : mesh->nt;

	for (int t = tStart; t < tLimit; t+=printfreq) {
		char *vtuFileName = NULL;
		asprintf(&vtuFileName, "%s%s.t%d.vtu", outputpath, basename(filename), t);
		wTSargs pt;
		pt.file = strdup(vtuFileName);
		pt.t = t;
		pt.rfs = &rfs;
		pt.z = z;
		pt.u = u;
		pt.v = v;
		pt.w = w;
		pt.verbose = verbose;
		if (writeTimestep((void *) &pt)) {
			fprintf(stderr, "Unable to write results to %s\n", vtuFileName);
			return EXIT_FAILURE;
		}
	}
	//Done writing individual files, now write PVD file

	if (ts >= 0) {
		fprintf(stdout, "VTU file successfully written in %s\n", outputpath);
		return EXIT_SUCCESS;
	}

	xmlTextWriterPtr pvdFile = NULL;
	char *pvdFileName = NULL;
	asprintf(&pvdFileName, "%s%s.pvd", outputpath, basename(filename));
	pvdFile = xmlNewTextWriterFilename(pvdFileName, 0);
	xmlTextWriterSetIndent(pvdFile, 1);

	xmlTextWriterStartDocument(pvdFile, NULL, "UTF-8", NULL);
	xmlTextWriterStartElement(pvdFile, BAD_CAST "VTKFile");
	xmlTextWriterWriteAttribute(pvdFile, BAD_CAST "type", BAD_CAST "Collection");
	xmlTextWriterStartElement(pvdFile, BAD_CAST "Collection");
	for (int t = 0; t < mesh->nt; t+=printfreq) {
		xmlTextWriterStartElement(pvdFile, BAD_CAST "DataSet");
		xmlTextWriterWriteFormatAttribute(pvdFile, BAD_CAST "timestep", "%.10f", mesh->timestamp[t]);
		xmlTextWriterWriteAttribute(pvdFile, BAD_CAST "part", BAD_CAST "0");
		xmlTextWriterWriteFormatAttribute(pvdFile, BAD_CAST "file", "%s.t%d.vtu", basename(filename), t);
		xmlTextWriterEndElement(pvdFile); //DataSet
	}
	xmlTextWriterEndElement(pvdFile); //Collection
	xmlTextWriterEndElement(pvdFile); //VTKFile
	if (xmlTextWriterEndDocument(pvdFile) < 0) {
		fprintf(stderr, "Failed to save PVD file\n");
		return EXIT_FAILURE;
	}
	xmlFreeTextWriter(pvdFile);

	fprintf(stdout, "VTU files successfully written in %s\n", outputpath);
	return EXIT_SUCCESS;
}

int writeTimestep(void *wtsargs) {
/*!
 * @brief Write a single VTU file, based on information provided in \c wtsargs
 *
 * @param wtsargs Mesh information and parameters - see @ref wTSargs for details
 * @returns 0 on success
 */
	wTSargs args = *(wTSargs *) wtsargs;
	telemac_data_t mesh = args.rfs->tmdat;
	int t = args.t;
	int z = args.z;
	int u = args.u;
	int v = args.v;
	int w = args.w;

	if (args.verbose) {
		fprintf(stdout, "Writing VTU file for timestep %d of %d...\n", t, mesh.nt);
	}
	xmlTextWriterPtr vtuFile = NULL;
	vtuFile = xmlNewTextWriterFilename(args.file, 0);
	xmlTextWriterSetIndent(vtuFile, 1);

	xmlTextWriterStartDocument(vtuFile, NULL, "UTF-8", NULL);
	xmlTextWriterStartElement(vtuFile, BAD_CAST "VTKFile");
	xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "type", BAD_CAST "UnstructuredGrid");

	xmlTextWriterStartElement(vtuFile, BAD_CAST "UnstructuredGrid");

	xmlTextWriterStartElement(vtuFile, BAD_CAST "Piece");

	xmlTextWriterWriteFormatAttribute(vtuFile, BAD_CAST "NumberOfPoints", "%d", mesh.npoin);
	xmlTextWriterWriteFormatAttribute(vtuFile, BAD_CAST "NumberOfCells", "%d", mesh.nelem);

	xmlTextWriterStartElement(vtuFile, BAD_CAST "Points");

	xmlTextWriterStartElement(vtuFile, BAD_CAST "DataArray");
	xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "Name", BAD_CAST "Coordinates");
	xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "type", BAD_CAST "Float32");
	xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "NumberOfComponents", BAD_CAST "3");
	xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "format", BAD_CAST "ascii");

	float **data = NULL;
	data = get_telemac_data(args.rfs, t, 0);
	if (data == NULL) {
		perror("NULL returned from get_telemac_data");
		return -1;
	}

	for (int p = 0; p < mesh.npoin; p++) {
		xmlTextWriterWriteFormatString(vtuFile, "%+.10f %+.10f %+.10f\n", mesh.X[p], mesh.Y[p], data[z][p]);
	}

	xmlTextWriterEndElement(vtuFile); //DataArray
	xmlTextWriterEndElement(vtuFile); //Points

	xmlTextWriterStartElement(vtuFile, BAD_CAST "Cells");
	xmlTextWriterStartElement(vtuFile, BAD_CAST "DataArray");
	xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "Name", BAD_CAST "connectivity");
	xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "type", BAD_CAST "Int32");
	xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "format", BAD_CAST "ascii");

	for (int p = 0; p < mesh.nelem; p++) {
		for (int j = 0; j < mesh.ndp; j++) {
			xmlTextWriterWriteFormatString(vtuFile, "%d ", mesh.ikle[(p*mesh.ndp)+j]-1);
		}
		xmlTextWriterWriteFormatString(vtuFile, "\n");
	}
	xmlTextWriterEndElement(vtuFile); //DataArray

	xmlTextWriterStartElement(vtuFile, BAD_CAST "DataArray");
	xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "Name", BAD_CAST "types");
	xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "type", BAD_CAST "Int32");
	xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "format", BAD_CAST "ascii");
	for (int p = 0; p < mesh.nelem; p++) {
		if (mesh.ndp == 6) {
			xmlTextWriterWriteString(vtuFile, BAD_CAST "13 ");
		} else if (mesh.ndp == 3) {
			xmlTextWriterWriteString(vtuFile, BAD_CAST "5 ");
		} else if (mesh.ndp == 4) {
			xmlTextWriterWriteString(vtuFile, BAD_CAST "9 ");
		}
	}
	xmlTextWriterEndElement(vtuFile); //DataArray

	xmlTextWriterStartElement(vtuFile, BAD_CAST "DataArray");
	xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "Name", BAD_CAST "offsets");
	xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "type", BAD_CAST "Int32");
	xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "format", BAD_CAST "ascii");
	for (int p = 0; p < mesh.nelem; p++) {
		xmlTextWriterWriteFormatString(vtuFile, "%d ", (p+1)*mesh.ndp);
	}
	xmlTextWriterEndElement(vtuFile); //DataArray
	xmlTextWriterEndElement(vtuFile); //Cells

	xmlTextWriterStartElement(vtuFile, BAD_CAST "PointData");

	for (int d = 0; d < mesh.nbv_1; d++) {
		xmlTextWriterStartElement(vtuFile, BAD_CAST "DataArray");
		xmlTextWriterWriteFormatAttribute(vtuFile, BAD_CAST "Name", "%s", mesh.var_names[d]);
		xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "type", BAD_CAST "Float32");
		xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "format", BAD_CAST "ascii");
		for (int p = 0; p < mesh.npoin; p++) {
			xmlTextWriterWriteFormatString(vtuFile, "%+.10f ", data[d][p]);
		}
		xmlTextWriterEndElement(vtuFile);
	}

	xmlTextWriterStartElement(vtuFile, BAD_CAST "DataArray");
	xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "Name", BAD_CAST "Vector Velocity");
	xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "type", BAD_CAST "Float32");
	xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "format", BAD_CAST "ascii");
	xmlTextWriterWriteAttribute(vtuFile, BAD_CAST "NumberOfComponents", BAD_CAST "3");
	for (int p = 0; p < mesh.npoin; p++) {
		float wval = (mesh.ndp == 6 ? data[w][p] : 0); // Assume 2D if ndp != 6
		xmlTextWriterWriteFormatString(vtuFile, "%+.10f %.10f %.10f", data[u][p], data[v][p], wval);
	}
	xmlTextWriterEndElement(vtuFile); //DataArray (Vector)
	xmlTextWriterEndElement(vtuFile); //PointData
	xmlTextWriterEndElement(vtuFile); //Piece
	xmlTextWriterEndElement(vtuFile); //UnstructuredGrid
	xmlTextWriterEndElement(vtuFile); //VTKFile

	if (xmlTextWriterEndDocument(vtuFile) < 0) {
		fprintf(stderr, "Failed to save VTU file for timestep %d", t);
		return EXIT_FAILURE;
	}

	for (int d = 0; d < mesh.nbv_1; d++) {
		free(data[d]);
	}
	free(data);
	xmlFreeTextWriter(vtuFile);
	return 0;
}
