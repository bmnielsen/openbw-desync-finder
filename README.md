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

# Investigation into McRave-Iron-2

These are the findings for the replay McRave-Iron-2, which can be found in the `files` directory:

- The first observed desync can be fixed completely by adding an extra RNG usage at the start of frame 15128. This indicates that BW is making an additional RNG call that OpenBW is not, causing their random number sequences to get out of sync. Since adding an extra dummy call fixes the problem, BW's extra usage of RNG must be something that doesn't have any other measurable effect on the game.
- The second observed desync is when two SCVs get different velocities in OpenBW on frame 23456. On the next frame, the RNG sequence is also out of sync, as a lot of RNG-dependent values start deviating. However, in this case neither adding an additional dummy RNG call or removing one can fix the SCV deviation, so this appears to have a different root cause.

The `find_missing_rng.cpp` file contains my work-in-progress attempt to fix the issues by adjusting the RNG sequence.
