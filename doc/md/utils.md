Command line tools
==================

telemac-info
------------
`telemac-info [-v] [-f] filename`

Prints information about a TELEMAC result file, including the stored variables
and other simulation parameters.

| Option | Description                                     |
|--------|-------------------------------------------------|
| -v     | Verbose output. Specify twice for more details. |
| -f     | Force mode. Attempt to continue on errors       |

@see telemac-info.c

telemac-parse
-------------
`telemac-parse [-v] [-b] [-o path] filename`

Exports TELEMAC results into a number of flat text files for examination or use
in other tools. This includes the mesh data as well as the values of each
variable at each timestep.

See [File Formats](doc/md/formats.md) for file format details

| Option  | Description                                          |
|---------|------------------------------------------------------|
| -v      | Verbose mode. Specify twice for more details.        |
| -b      | Write variable data in binary format (default: text) |
| -o path | Output files to specified path.                      |

@see telemac-parse.c

telemac-vtu
-----------
`telemac-vtu [-c] [-F] [-f n] [-z n] [-u n] [-v n] [-w n] [-o path] filename`

Export TELEMAC results in a form suitable for use with Paraview, an open source
piece of visualisation software.

| Option  | Description                                          |
|---------|------------------------------------------------------|
| -c      | Verbose output.                                      |
| -F      | Force mode. Attempt to continue on errors            |
| -f n    | Output every n^th timestep                           |
| -z n    | Specify variable number for node Z values (height)   |
| -u n    | Specify variable number for X velocity component (u) |
| -v n    | Specify variable number for Y velocity component (v) |
| -w n    | Specify variable number for Z velocity component (w) |
| -o path | Output files to specified path.                      |

@see telemac-vtu.c


