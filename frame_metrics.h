#pragma once

#include <time.h>
#include "ctk3/ctk3.h"

using namespace CTK;

/// Data
////////////////////////////////////////////////////////////
struct FrameMetrics
{
    clock_t start;
    float64 fps_update_freq;
    uint32  frame_count;
    float64 fps;
};

/// Interface
////////////////////////////////////////////////////////////
static void InitFrameMetrics(FrameMetrics* frame_metrics, float64 fps_update_freq)
{
    frame_metrics->start = clock();
    frame_metrics->fps_update_freq = fps_update_freq;
}

static bool Tick(FrameMetrics* frame_metrics)
{
    clock_t end = clock();
    float64 elapsed_time = (float64)(end - frame_metrics->start) / CLOCKS_PER_SEC;
    ++frame_metrics->frame_count;
    if (elapsed_time > frame_metrics->fps_update_freq)
    {
        frame_metrics->fps = frame_metrics->frame_count / elapsed_time;
        frame_metrics->start = end;
        frame_metrics->frame_count = 0;
        return true;
    }
    else
    {
        return false;
    }
}
