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

/*!
 * @file
 * @brief TELEMAC SELAFIN file reader
 */

#ifndef TELEMAC_PARSE_H
#define TELEMAC_PARSE_H

#include <stdint.h>
/*!
 * @defgroup records SELAFIN record structures
 * @brief Data structures corresponding to records in the saved results files
 *
 * These structs are matched to the fortran format records stored in SELAFIN
 * results files, such that each can be retrieved with a call to fortran_read()
 *
 * More information about each of these fields is given in the TELEMAC
 * documentation, specifically Appendix 2 of the "Guide to programming in the TELEMAC system"
 * and Appendix 3 of the TELEMAC 2D User Manual.
 * @see telemac-loader.h
 * @{
 */

//! Title and format information
typedef struct R1 {
	char title[72]; //!< Simulation title (user defined)
	char format[8]; //!< Format marker. Should always be SERAFIN
} R1;

//! Number of variables (linear and quadratic?)
typedef struct R2 {
	uint32_t nbv_1; //!< Number of linear variables
	uint32_t nbv_2; //!< Number of quadratic(?) variables
} R2;

//! Date/time corresponding to the start of the simulation
typedef struct R5 {
	uint32_t year; //!< Simulation start year
	uint32_t month; //!< Simulation start month
	uint32_t day; //!< Simulation start day
	uint32_t hour; //!< Simulation start hour
	uint32_t minute; //!< Simulation start minute
	uint32_t second; //!< Simulation start second
} R5;

//! Alias for R5. Stores a date/time as a set of integer values
typedef R5 datetime_t; //!< @ingroup api_struct

//! Mesh structure information
typedef struct R6 {
	uint32_t nelem; //!< Number of elements (2D faces or 3D elements)
	uint32_t npoin; //!< Number of nodes/points
	uint32_t ndp; //!< Number of nodes per element. Values of 3 (2D), 4 (2D) and 6 (3D) supported.
	uint32_t one; //!< This should always end up set to 1. Purpose unknown.
} R6;
//! @}

/*!
 * @defgroup api_struct Exposed data structures
 * @brief Data structures exposed via library functions
 * @{
 */

//! Structure holding all data read from a given results file
typedef struct telemac_data {
	char title[73]; //!< Simulation title. NULL Terminated. @sa R1
	char format[9]; //!< Returned file format. NULL terminated @sa R1

	uint32_t nbv_1; //!< Number of linear variables @sa R2
	uint32_t nbv_2; //!< Number of quadratic variables @sa R2

	datetime_t date; //!< Date/time of simulation start @sa R5

	char **var_names; //!< Names of simulation variables, in order

	uint32_t iparam[10]; //!< Contents of IPARAM array from results file
	uint32_t nelem; //!< Number of elements in mesh. Can be 2D or 3D elements
	uint32_t npoin; //!< Number of points/nodes in mesh
	uint32_t ndp; //!< Number of nodes forming each element. 3 or 4 for 2D, 6 for 3D

	uint32_t *ikle; //!< IKLE value from results file
	uint32_t *ipobo; //!< IPOBO value from results file

	float *X; //!< Array of X coordinates, indexed by node number
	float *Y; //!< Array of Y coordinates, indexed by node number
	float XYrange[4]; //!< Maximum and minimum X and Y coordinates

	uint32_t nt; //!< Number of timesteps
	float *timestamp; //!< Array of realtime values for each timestep

	int state; //!< Structure state: 0 = uninitialised, 1 = headers set, 2 = mesh set

} telemac_data_t;

//! Results file information

/*! 
 * The offsets of different sections within the file represented in *file
 * are stored for re-use in various functions, along with the associated
 * telemac_data_t structure being populated or read from.
 */
typedef struct {
	FILE *file; //!< Open file handle
	off_t meshstart; //!< Offset to start of mesh
	off_t datastart; //!< Offset to start of simulation results
	off_t datasize; //!< Size of simulation data for each timestep
	telemac_data_t tmdat; //!< telemac_data_t corresponding to this file
} resfile_t;

/*! @} */

uint32_t int_swap(const uint32_t input);
float float_swap(float value);
int fortran_read(void* cstruct, size_t csize, size_t num, FILE* fromfile);
int open_telemac(resfile_t *rfile, int verbose);
int get_telemac_header(resfile_t *rfile, int verbose);
int get_telemac_mesh(resfile_t *rfile, int verbose);
float **get_telemac_data(resfile_t *rfile, int timestep, int verbose);
void free_telemac_data(resfile_t *rfile, float **data);
#endif // TELEMAC_PARSE_H
