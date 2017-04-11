/*
** File:    ascii13.cpp
**
** Author:  Schuyler Martin <sam8050@rit.edu>
**
** Description: Video to ASCII video using OpenCV
**
** Usage:       ./ascii13 files
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
#define USAGE "Usage: ./ascii13 files"

// master debug macro; enables all forms of debugging
//#define DEBUG true

// additional debugging flags that control more specific debug features
#ifdef DEBUG
    // resizes frames to draw on smaller displays
    #define DEBUG_SHRINK_FRAMES     true
    // draws intermediate matrices, for all frames
    //#define DEBUG_ALL_FRAMES        true
    // disables user input keypress wait; runs through debugging automatically
    #define DEBUG_ANIMATE_FRAMES    true
#endif
#ifdef DEBUG_ANIMATE_FRAMES
    #define DEBUG_KEY_WAIT  1
#else
    #define DEBUG_KEY_WAIT  0
#endif

// output file naming
#define OUT_SUFFIX  "_out"

// blur filter size
#define GAUS_SIZE           3
// edge threshold values
#define EDGE_THRESH_RATIO   3
#define EDGE_THRESH_LO      30
#define EDGE_THRESH_HI      (EDGE_THRESH_RATIO * EDGE_THRESH_LO)

// text resolution
#define TEXT_WIDTH  80
#define TEXT_HEIGHT 44

// character encoding macros
// character to use in drawings
#define CHAR_CHAR   "*"
// threshold to use when determining if a character should be drawn
#define CHAR_THRESH 25

// character width of the progress bar
#define PROGRESS_BAR_WIDTH  40

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
    uint64_t sec = (ms / 1000) % 60 ;
    uint64_t min = (ms / (1000*60)) % 60;
    uint64_t hrs   = (ms / (1000*60*60)) % 24;
    buff << setfill('0') << setw(2) << hrs << ":" 
        << setfill('0') << setw(2) << min << ":" 
        << setfill('0') << setw(2) << sec << "." << ms % 1000 << "s";
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
    return fd.substr(0, ext_idx) + OUT_SUFFIX + ".mp4";
}

/*
** Text-based progress bar
**
** @param fr Current frame number
** @param total_fr Total frame count
*/
void draw_progress_bar(uint32_t fr, uint32_t total_fr)
{
    // reduce the amount of time spent on drawing
    if ((fr % 10) != 0)
        return;
    cout << "[";
    // integer progress calculation
    uint32_t pos = fr / (total_fr / PROGRESS_BAR_WIDTH);
    // draw the load bar, depending on the position
    for(uint32_t i=0; i<PROGRESS_BAR_WIDTH; ++i)
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
        int   in_codec   = (int)vid_stream.get(CV_CAP_PROP_FOURCC);
        cout << fd_in[id] << " stats:" << endl;
        cout << "  + Frames: " << frame_n << endl;
        cout << "  + FPS:    " << fps << endl;
        cout << "  + Codec:  " << in_codec << endl;
        cout << "  + Size:   " << frame_w << "x" << frame_h << endl;

        // pre-compute some data about the output video
        string fd_out = output_fd(fd_in[id]);
        int out_codec = CV_FOURCC('M','P','4','V');
        int h_baseline = 0;
        Size char_sz = getTextSize(CHAR_CHAR,
            FONT_HERSHEY_PLAIN, 1, 1, &h_baseline
        );
        // view window for sampling pixels when determing 
        uint32_t window_w = frame_w / TEXT_WIDTH;
        uint32_t window_h = frame_h / TEXT_HEIGHT;
        uint32_t char_fr_w = char_sz.width * TEXT_WIDTH;
        uint32_t char_fr_h = char_sz.height * TEXT_HEIGHT;
        cout << fd_out << " stats:" << endl;
        cout << "  + Codec:  " << out_codec << endl;
        cout << "  + Text Dimensions:   " << TEXT_WIDTH << "x" 
            << TEXT_HEIGHT << endl;
        cout << "  + Pixels per char:   " << char_sz.width << "x" 
            << char_sz.height << endl;
        cout << "  + Pixel Dimensions:  " << char_fr_w << "x" 
            << char_fr_h << endl;

        // calculate the time it takes to process this image
        uint64_t vid_proc_start_time = get_ms_timestamp();

        // buffering all the frames in a vector is impractical so we write
        // to the video stream as soon as we can
        VideoWriter writer;
        // last variable indicates if this is a color or black and white video
        // This makes a huge difference when using 8-bit color
        writer.open(fd_out, out_codec, fps, Size(char_fr_w, char_fr_h), true);
        if (!writer.isOpened())
        {
            cerr << "File " << fd_out << "could not be opened for writing."
                << endl;
            return EXIT_FAILURE;
        }

        cout << "Rendering..." << endl;
        // iterate over the initial video data
        Mat fr_buff, prev_buff;
        uint32_t fr_cntr = 0;
        while(fr_cntr < frame_n)
        {
            vid_stream >> fr_buff;
            // skip the strange empty frames that can occur
            if (fr_buff.empty())
            {
                cerr << "WARNING: Frame " << fr_cntr << " skipped" << endl;
                ++fr_cntr;
                continue;
            }

            // ========== BEGIN Process the Frame ==========
            Mat fr_gry, edge_mask, fr_out;
            // Grayscale
            cvtColor(fr_buff, fr_gry, CV_BGR2GRAY);
            // Gaussian blur
            blur(fr_gry, fr_gry, Size(GAUS_SIZE, GAUS_SIZE));
            // edge detector
            Canny(fr_gry, edge_mask, EDGE_THRESH_LO, EDGE_THRESH_HI, GAUS_SIZE);

            // convert the edge information into characters
            fr_out = Mat(char_fr_h, char_fr_w, CV_8UC3);
            fr_out = Scalar(0);
            // move a sampling window across the frame and determine
            // whether
            for(uint32_t row=0; row<TEXT_HEIGHT; ++row)
            {
                for(uint32_t col=0; col<TEXT_WIDTH; ++col)
                {
                    // pull out the sampling region
                    uint32_t col_px = col * window_w;
                    uint32_t row_px = row * window_h;
                    // view window, region of interest
                    Rect roi = Rect(col_px, row_px, window_w, window_h);
                    Mat sub = edge_mask(roi);
                    // sum the values in the region
                    int32_t sub_sum = mean(sub)[0];
                    if (sub_sum > CHAR_THRESH)
                    {
                        // get the average color of the pixels that form the
                        // edge information
                        Scalar sub_clr = mean(fr_buff(roi), sub);
                        // draw the character on the screen
                        putText(fr_out, CHAR_CHAR,
                            Point2f(col * char_sz.width, 
                                (row * char_sz.height) + char_sz.height),
                            FONT_HERSHEY_PLAIN, 1, sub_clr
                        );
                    }
                }
            }

            #ifdef DEBUG_ALL_FRAMES
                imshow("Test", fr_out);
                waitKey(DEBUG_KEY_WAIT);
            #endif

            // copy the frame data to the file stream
            writer.write(fr_out);
            draw_progress_bar(fr_cntr, frame_n);
            ++fr_cntr;

            // ========== END   Process the Frame ==========

            // limit frames for testing purposes
            #ifdef DEBUG
                if (fr_cntr >= 700)
                    break;
            #endif
        }
        // release the writer
        writer.release();

        // indicate total processing time of the video
        cout << "\nVideo processing time: " << 
            get_time_str(get_ms_timestamp() - vid_proc_start_time) << endl;
    }
    return EXIT_SUCCESS;
}
