# openbw-desync-finder
Tool to compare replay playback in BW and OpenBW to find out where OpenBW desyncs

## Usage

The intended way to use this tool is to do the following:

1. Build the DLL module to dump the replay data
2. Use BWAPI to run the replay in BW 1.16 with the module from step 1 attached
3. Build and run the executable that runs the replay in OpenBW, comparing the unit data with the data produced in step 2

## Building the module to dump the replay data

A MSVC solution is provided in the `vs` directory. Building it should produce `vs/src/Release/DumpData.dll`.

## Building the executable to verify in OpenBW

For the OpenBW side of the tool, the project uses CMake, so any IDE that understands CMake should be able to build it.

In order to run OpenBW, the BW MPQ files must be placed in the same directory as the built executable.
