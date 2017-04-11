/*
** File:    ascii13.cpp
**
** Author:  Schuyler Martin <sam8050@rit.edu>
**
** Description: Video to ASCII video in OpenCV
*/

#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/time.h>

// Namespaces
using namespace cv;
using namespace std;

// Macros
// Usage message
#define USAGE "Usage: ./ascii13 [files]"

// master debug macro; enables all forms of debugging
//#define DEBUG true

// additional debugging flags that control more specific debug features
#ifdef DEBUG
    // resizes frames to draw on smaller displays
    #define DEBUG_SHRINK_FRAMES     true
    // draws intermediate matrices, for all frames
    #define DEBUG_ALL_FRAMES        true
    // disables user input keypress wait; runs through debugging automatically
    #define DEBUG_ANIMATE_FRAMES    true
#endif
#ifdef DEBUG_ANIMATE_FRAMES
    #define DEBUG_KEY_WAIT  40
#else
    #define DEBUG_KEY_WAIT  0
#endif

// output file naming
#define OUT_SUFFIX  "_out"
#define OUT_EXT     ".mp4"

// blur filter size
#define GAUS_SIZE           7

/*
** Fetches the current millisecond timestamp
**
** @return Current timestamp in milliseconds
*/
uint64_t get_ms_timestamp()
{
    struct timeval time;
    uint64_t cycle_cntr = 0;
    // "Fritter and waste the hours in an off-hand way"
    gettimeofday(&time, NULL);
    uint64_t cur_time = 
        (uint64_t)(time.tv_sec) * 1000 +
        (uint64_t)(time.tv_usec) / 1000;
    return cur_time;
}

/*
** Fetches the current millisecond timestamp
**
** @param ms Milliseconds
** @return Human-friendly time stamp string
*/
string get_time_str(uint64_t ms)
{
    stringstream buff;
    buff << ms / 1000 << "." << ms % 1000 << "s";
    return buff.str();
}

/*
** Create the final output file name
**
** @param fd Original filename (Output file will have an output suffix)
** @return Output file name
*/
string output_fd(string fd)
{
    int ext_idx = fd.find_last_of(".");
    return fd.substr(0, ext_idx) + OUT_SUFFIX + fd.substr(ext_idx);
}

/*
** Text-based progress bar
**
** @param fr Current frame number
** @param total_fr Total frame count
*/
void draw_progress_bar(uint32_t fr, uint32_t total_fr)
{
    uint32_t bar_width = 50;
    cout << "[";
    // integer progress calculation
    uint32_t pos = fr / (total_fr / bar_width);
    // draw the load bar, depending on the position
    for(uint32_t i=0; i<bar_width; ++i)
    {
        if (i < pos)
            cout << "=";
        else if (i == pos)
            cout << ">";
        else
            cout << " ";
    }
    cout << "] " << fr << "/" << total_fr << " \r";
    cout.flush();
}

/*
** Main execution point of the program
**
** @param argc Argument count
** @param argv Argument list
*/
int main(int argc, char** argv)
{
    int fd_len = 1;
    // error
    if (argc == 1)
    {
        cerr << "No file(s) specified." << endl;
        return EXIT_FAILURE;
    }
    else
        fd_len = argc - 1;

    // load the images into the fd in
    string fd_in[fd_len];
    // load the file(s) specified from the command line
    for(int i=1; i<argc; ++i)
        fd_in[i-1] = argv[i];

    // process all the images in a batch process
    for(int id=0; id<fd_len; ++id)
    {
        cout << "Reading in " << fd_in[id] << "..." << endl;

        // video stream controller
        VideoCapture vid_stream;

        // read in the file stream
        vid_stream.open(fd_in[id]);
        if (!vid_stream.isOpened())
        {
            cout << "Faild to read " << fd_in[id] << ". Exiting." << endl;
            return EXIT_FAILURE;
        }

        // stat the file
        uint32_t frame_n = (uint32_t)vid_stream.get(CAP_PROP_FRAME_COUNT);
        uint32_t frame_w = (uint32_t)vid_stream.get(CAP_PROP_FRAME_WIDTH);
        uint32_t frame_h = (uint32_t)vid_stream.get(CAP_PROP_FRAME_HEIGHT);
        double   fps     = (double)vid_stream.get(CAP_PROP_FPS);
        int      codec   = (int)vid_stream.get(CV_CAP_PROP_FOURCC);
        cout << fd_in[id] << " stats:" << endl;
        cout << "  + Frames: " << frame_n << endl;
        cout << "  + Size:   " << frame_w << "x" << frame_h << endl;

        // calculate the time it takes to process this image
        uint64_t vid_proc_start_time = get_ms_timestamp();

        // buffering all the frames in a vector is impractical so we write
        // to the video stream as soon as we can
        VideoWriter writer;
        string fd_out = output_fd(fd_in[id]);
        writer.open(fd_out, codec, fps, Size(frame_w, frame_h), true);
        if (!writer.isOpened())
        {
            cerr << "File " << fd_out << "could not be opened for writing."
                << endl;
            return EXIT_FAILURE;
        }

        // iterate over the initial video data
        Mat fr_buff;
        uint32_t fr_cntr = 0;
        while(fr_cntr < frame_n)
        {
            vid_stream >> fr_buff;
            // skip the strange empty frames that can occur
            if (fr_buff.empty())
                continue;

            /** Process the Frame **/
            //Mat fr_gry;
            //cvtColor(fr_buff, fr_gry, COLOR_BGR2GRAY);
            // Gaussian blur
            blur(fr_buff, fr_buff, Size(GAUS_SIZE, GAUS_SIZE));
            //Canny(fr_gry, fr_buff, 30, 300, GAUS_SIZE);

            // copy the frame data to the file stream
            writer.write(fr_buff);
            draw_progress_bar(fr_cntr, frame_n);
            ++fr_cntr;
        }

        // indicate total processing time of the video
        cout << "\n" << "  + Video processing time: " << 
            get_time_str(get_ms_timestamp() - vid_proc_start_time) << endl;
    }
    return EXIT_SUCCESS;
}
