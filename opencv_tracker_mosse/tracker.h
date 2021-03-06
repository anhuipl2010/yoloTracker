#ifndef TRACKER_H
#define TRACKER_H

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <string>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/core/ocl.hpp>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <vector>
#include <algorithm>
#include "opencv2/xfeatures2d.hpp"
#include "opencv2/features2d.hpp"

using namespace std;
using namespace cv;
using namespace cv::xfeatures2d;

typedef struct
{
    float x, y, w, h;
} box;

typedef struct thing_info
{
    Mat im;
    box bbox;
    int tag;
    string name;
    int hit;
} thing_info;

class tracking_dot
{
private:
public:
    Ptr<Tracker> tracker;
    vector<Point> stack_point;
    Mat im;
    box bbox;
    Point p;
    string name;
    Point velocity;
    int tag;
    int stack_num = 60;
    int miss_stack = 0;
    bool is_missed = false;
    bool is_mosse_updated = false;
    float score_limit = 5;
    float distance_limit = 100;
    int miss_limit = 30;

    tracking_dot();
    tracking_dot(Point pp);
    tracking_dot(Point pp, int tagg);
    Point get_point();
    int get_tag();
    Point get_v();
    void put_point_to_stack(Point pp);
    Point cal_v();
    bool is_empty();
    void goto_next_point();
    Point predict_next_point();
    int which_thing_is_my_thing(vector<thing_info> current_thing, int distance_limit, int score_limit);
    int which_thing_is_my_thing2(vector<thing_info> *current_thing);
    int update_dot(Mat frame, vector<thing_info> *current_thing);
    bool mosse_update(Mat frame, int frame_index, vector<thing_info> *current_thing);
    void update_current_things_hit(vector<thing_info> *current_thing);
    bool is_tracker_get_out_screen();
};

float doublecalcAngleFromPoints(Point2f _ptFirstPos, Point2f _ptSecondPos);
void alignImages(Mat &im1, Mat &im2, Mat &im1Reg, Mat &h);
float align_Images(Mat &im1, Mat &im2);

Point cal_center_point(box bbox);
void init_dot(int size);
void put_init_value_to_dot(vector<thing_info> tmp_thing);
void init_msg(char *msg);
string make_msg();
void make_txt(vector<vector<string>> file);
int image_read_enable(vector<vector<string>> file);
void get_frame_size(vector<vector<string>> file);
void sendMessage(int s, const char *buf);
int connect_to_server(char *ip, char *port);
vector<vector<string>> read_txt(string file_path);
Rect2d box_to_Rect2d(box bbox);
box Rect2d_to_box(Rect2d rect2d);
vector<thing_info> file_to_box(Mat im, vector<vector<string>> file);
int get_empty_tag();
float cal_distance(Point input1, thing_info input2);
vector<int> get_least_dis_index_list(vector<float> dis_list, float limit);
//vector<Rect2d> watchdog(Mat frame, vector<thing_info> current_thing);

#endif
