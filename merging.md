# TiledC64 Merging

All data for each map is exported separately so in order to share character data between levels it needs to be merged.

As each map gets exported there is also a .log file saved with the settings for each. As long as the settings match (bits per screen/color/meta map, etc byte) maps can be merged.

To merge maps first create a text file with source and destination maps (one per line separated by a color). The text file would look something like:

    ..\bin\Castle4x4:..\bin\merged\CastleMerged
    ..\bin\Inside4x4:..\bin\merged\InsideMerged

Save the file with a name such as merge.txt

Use the **-merge** command line switch to enable merging. Example:

    TiledC64 -merge merge.txt ..\bin\merged\MapMerge

This will read the merge.txt file for a list of maps to merge, then export a combined character set, meta tiles, etc. and for each map a meta tilemap, or screen + color if not using meta maps.

