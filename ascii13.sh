#!/bin/bash
#
# Wrapper script for the ASCII 13 project. This will copy the original audio
# track back into the final video and attempt to compress the video further.
#
# Author: Schuyler Martin <sam8050@rit.edu>
#


./ascii13 $@
if [ "$?" -ne 0 ]; then
    >&2 echo "ERROR: ascii13 failed"
    exit 2
fi

# iterate over all the videos provided on the command line
for fd in "$@"; do
    # splice in the audio from the original file into the second video
    ffmpeg -i "${fd}" -i "${fd%.*}""_out.mp4" -y -c \
        copy -c:a mp3 -map 0:a:0 -map 1:v:0 -shortest "tmp_out.mp4"
    if [ "$?" -ne 0 ]; then
        >&2 echo "ERROR: ffmpeg failed"
        exit 2
    fi
    # TODO
    # and attempt to compress the video further

    # remove and rename final file, if something didn't fail
    if [ -f "tmp_out.mp4" ]; then
        rm "${fd%.*}""_out.mp4"
        mv "tmp_out.mp4" "${fd%.*}""_out.mp4"
    fi
done

