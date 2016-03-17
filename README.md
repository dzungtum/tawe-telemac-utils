**tawe-telemac-utils** is a small collection of programs that allow SELAFIN files
(TELEMAC results files) to be read, parsed and exported for further use and
analysis.

The functions required to read data from TELEMAC files can also be included in
other programs and projects.

The programs included allow results to be exported to a set of flat files or in
a format suitable for use with [Paraview](www.paraview.org), "an open-source,
multi-platform data analysis and visualization application".

Each timestep is written to a VTU file which may be viewed independently, with
a PVD file for viewing the full simulation.

For further details, see the following:
 * [Command line tools](doc/md/utils.md)
 * [TELEMAC loader library](doc/md/loader.md)
 * [File formats](doc/md/formats.md)

This project is licensed under the GNU General Public License, version 2.
For further details, see the accompanying LICENSE file.
