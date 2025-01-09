# openbw-desync-finder
Tool to compare replay playback in BW and OpenBW to find out where OpenBW desyncs

## Usage - compare mode

The intended way to use this tool is to do the following:

1. Build the DLL module to dump the replay data
2. Use BWAPI to run the replay in BW 1.16 with the module from step 1 attached
3. Build and run the executable that runs the replay in OpenBW, comparing the unit data with the data produced in step 2

### Building the module to dump the replay data

A MSVC solution is provided in the `vs` directory. Building it should produce `vs/src/Release/DumpData.dll`.

### Building the executable to verify in OpenBW

For the OpenBW side of the tool, the project uses CMake, so any IDE that understands CMake should be able to build it.

In order to run OpenBW, the BW MPQ files must be placed in the same directory as the built executable.

## Usage - standalone mode

Also included is a tool that only analyzes a replay with OpenBW and tries to determine if a desync occurred based on unit behaviour. If it sees a large number of units that do not get any orders after creation, it treats this as a probable desync.

The intended use case is to run against a large set of replays to either look for unknown desyncs or regression test a new build of OpenBW.

Building is identical to the section on OpenBW above. Modify the path to wherever you have the replay files you want to analyze, or toggle the commented lines if you want to analyze just one replay.

In `StandaloneDesyncFinderModule.h`, you can also toggle the `LOGGING` define to `true` to get some additional logs on all decisions made by the analyzer. This is useful when a replay is found that gives an unexpected result.