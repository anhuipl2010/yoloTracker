#include "tracker.h"

using namespace std;
using namespace cv;

extern int width, height;

tracking_dot::tracking_dot()
{
    p = Point(0, 0);
    tag = -1;
    velocity = Point(-1, -1);
    stack_point = vector<Point>(stack_num);
}

tracking_dot::tracking_dot(Point pp)
{
    p.x = pp.x;
    p.y = pp.y;
    tag = -1;
    velocity = Point(-1, -1);
    stack_point = vector<Point>(stack_num);
}

tracking_dot::tracking_dot(Point pp, int tagg)
{
    p.x = pp.x;
    p.y = pp.y;
    tag = tagg;
    velocity = Point(-1, -1);
    stack_point = vector<Point>(stack_num);
}

Point tracking_dot::get_point()
{
    return p;
}

int tracking_dot::get_tag()
{
    return tag;
}

Point tracking_dot::get_v()
{
    velocity = cal_v();
    return velocity;
}

void tracking_dot::put_point_to_stack(Point pp)
{
    for (int i = stack_point.size() - 1; i > 0; i--)
        stack_point[i] = stack_point[i - 1];

    stack_point[0] = pp;
}

Point tracking_dot::cal_v()
{
    float sum_x = 0, sum_y = 0;
    int stack_n = 0;

    for (int i = 0; i < stack_point.size() - 1; i++)
        if (stack_point[i].x != -1)
            stack_n++;

    sum_x = (float)(stack_point[stack_n - 1].x - stack_point[0].x) / (float)stack_n;
    sum_y = (float)(stack_point[stack_n - 1].y - stack_point[0].y) / (float)stack_n;

    if (stack_n < 4)
        return Point(-sum_x / 2, -sum_y / 2);
    else
        return Point(-sum_x, -sum_y);
}

bool tracking_dot::is_empty()
{
    if (tag == -1)
        return true;
    else
        return false;
}

void tracking_dot::goto_next_point()
{
    Point v = cal_v();
    put_point_to_stack(p + v);
    p += v;
}

Point tracking_dot::predict_next_point()
{
    Point v = cal_v();
    put_point_to_stack(p + v);
    p += v;
    return p;
}

int tracking_dot::which_thing_is_my_thing(vector<thing_info> current_thing, int distance_limit, int score_limit)
{
    vector<float> dis_list;
    vector<int> index_list;
    float score = -1;

    for (int i = 0; i < current_thing.size(); i++)
        dis_list.push_back(cal_distance(p, current_thing[i])); //current thing의 위치랑 똑같이 저장 됨

    index_list = get_least_dis_index_list(dis_list, distance_limit);
    sort(index_list.begin(), index_list.end());

    if (index_list.size() == 1)
        return index_list[0];
    else if (index_list.size() > 1)
    {
        for (int i = 0; i < index_list.size(); i++)
        {
            cout << "align_Images start" << endl;
            //cout << current_thing[index_list[i]].im.cols << " " << input.im.cols << endl;
            score = align_Images(current_thing[index_list[i]].im, im);

            if (score < score_limit && (score >= 0))
                return index_list[i];
        }
        return index_list[0];
    }
    else
        return -1;
}

int tracking_dot::which_thing_is_my_thing2(vector<thing_info> *current_thing)
{
    float dis_min = 100000;
    int min_index = 0;

    for (int i = 0; i < current_thing->size(); i++)
    {
        if (current_thing->at(i).hit == 0)
        {
            Point tmp = cal_center_point(bbox);
            float tmp_f = cal_distance(tmp, current_thing->at(i));
            if (tmp_f < dis_min)
            {
                dis_min = tmp_f;
                min_index = i;
            }
        }
    }

    return min_index;
}

//this func update tracking_dot's state
int tracking_dot::update_dot(Mat frame, vector<thing_info> *current_thing)
{
    int flag = which_thing_is_my_thing2(current_thing);
    //일치하는 물체의 current_thing 인덱스를 반환

    if (flag != -1 && current_thing->at(flag).hit != 1) //flag 가 -1이 아닌경우는 which_thing_is_my_thing가 잘못되어서 나왔던것!!
    {
        im = current_thing->at(flag).im;
        bbox = current_thing->at(flag).bbox;
        p = cal_center_point(current_thing->at(flag).bbox);
        name = current_thing->at(flag).name;
        is_missed = false;
        miss_stack = 0;
        velocity = cal_v();
    }
    else //miss
    {
        predict_next_point();
        bbox.x = (float)p.x / (float)width - bbox.w / 2;
        bbox.y = (float)p.y / (float)height - bbox.h / 2;
        is_missed = true;
    }
    put_point_to_stack(p);

    return flag;
}

bool tracking_dot::mosse_update(Mat frame, int frame_index, vector<thing_info> *current_thing)
{
    Rect2d tmp = box_to_Rect2d(bbox);
    if (frame_index % 5 == 0)
    {
        tracker = TrackerMOSSE::create();

        float dis_min = 100000;
        int min_index = 0;

        for (int i = 0; i < current_thing->size(); i++)
        {
            if (current_thing->at(i).hit == 0)
            {
                Point tmp = cal_center_point(bbox);
                float tmp_f = cal_distance(tmp, current_thing->at(i));
                if (tmp_f < dis_min)
                {
                    dis_min = tmp_f;
                    min_index = i;
                }
            }
        }

        is_mosse_updated = tracker->init(frame, box_to_Rect2d(current_thing->at(min_index).bbox));
        bbox = current_thing->at(min_index).bbox;
        current_thing->at(min_index).hit = 1;
    }
    else
    {
        is_mosse_updated = tracker->update(frame, tmp);
    }

    if (is_mosse_updated)
    {
        p = cal_center_point(bbox);
        put_point_to_stack(p);
        bbox = Rect2d_to_box(tmp);
    }

    return is_mosse_updated;
}

void tracking_dot::update_current_things_hit(vector<thing_info> *current_thing)
{
    float dis_min = 100000;
    int min_index = 0;

    for (int i = 0; i < current_thing->size(); i++)
    {
        if (current_thing->at(i).hit == 0)
        {
            Point tmp = cal_center_point(bbox);
            float tmp_f = cal_distance(tmp, current_thing->at(i));
            if (tmp_f < dis_min)
            {
                dis_min = tmp_f;
                min_index = i;
            }
        }
    }

    if (dis_min < distance_limit)
        current_thing->at(min_index).hit = 1;
}

bool tracking_dot::is_tracker_get_out_screen()
{
    int padding = 20;
    Point p1 = stack_point[0] - Point(width / 2, height / 2);
    Point p2 = stack_point[0] - Point(0, 0);
    Point p3 = stack_point[0] - Point(width, height);

    if ((p1.x < padding && p1.x > -padding) || (p1.y < padding && p1.y > -padding) || (p2.x < padding && p2.x > -padding) || (p2.y < padding && p2.y > -padding) || (p3.x < padding && p3.x > -padding) || (p3.y < padding && p3.y > -padding))
        return true;
    else
        return false;
}
