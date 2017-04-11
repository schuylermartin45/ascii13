# ASCII 13: The Video Processor
#### Author:  Schuyler Martin <sam8050@rit.edu>

![alt tag](/media/hsc.png)


## About
ASCII 13 is the 13th assignment I have written for my Advanced CV course. It is
a "choose your own" assignment and I have decided to use OpenCV 3 to convert
videos into ASCII videos.

## Setup
ASCII 13 is written in OpenCV and originally built on Linux.

### Dependencies
These programs are necessary to build and install 
- g++
- make
- OpenCV 3
- ffmpeg

### Compiling from Source
To compile the project:
```shell
cmake .
make
```
### Running the project
To just render the video file:
```shell
./ascii13 files
```
To just render the video and preserve the audio:
```shell
./ascii13.sh files
```
