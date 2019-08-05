#include "meanfilter.h"
#include <cmath>

MeanFilter::MeanFilter(int frame_width, int frame_height)
    : dft_ready_read(false), frWidth(frame_width), frHeight(frame_height)
{}

MeanFilter::~MeanFilter()
{
}

void MeanFilter::compute_mean(LVFrame *frame, QPointF topLeft, QPointF bottomRight,
                              LV::PlotMode pm, bool cam_running)
{
    int r, c, k;
    double nSamps = bottomRight.x() - topLeft.x();
    double nBands = bottomRight.y() - topLeft.y();
    float frame_mean = 0.0;
    float data_point = 0.0;

    switch (pm) {
    case LV::pmRAW:
        p_getPixel = &MeanFilter::getRawPixel;
        break;
    case LV::pmDSF:
        p_getPixel = &MeanFilter::getDSFPixel;
        break;
    case LV::pmSNR:
        p_getPixel = &MeanFilter::getSNRPixel;
        break;
    }
    curFrame = frame;

    for (r = 0; r < frHeight; r++) {
         frame->spectral_mean[r] = 0;
    }
    for (c = 0; c < frWidth; c++) {
        frame->spatial_mean[c] = 0;
    }
    /*
    for (int i = 0; i < TAP_WIDTH * frHeight; i++)
    {
        frame->tap_profile[i]=0;
    }

    for (r = 0; r < frHeight; r++) {
        for (c = 0; c < frWidth; c++) {
            data_point = (this->*p_getPixel)(static_cast<uint32_t>(r * frWidth + c));
            if (c > int(topLeft.x()) && c <= int(bottomRight.x())) {
                frame->spectral_mean[r] += data_point;
            }
            if (r > int(topLeft.y()) && r <= int(bottomRight.y())) {
                frame->spatial_mean[c] += data_point;
                if( r < TAP_WIDTH)
                {
                    frame->tap_profile1[r * TAP_WIDTH + c % TAP_WIDTH] = data_point;
                }
                else if (r >= TAP_WIDTH && r < 2 * TAP_WIDTH)
                {
                    frame->tap_profile2[r * TAP_WIDTH + c % TAP_WIDTH] = data_point;
                }
                else if (r >= 2 * TAP_WIDTH && r < 3 * TAP_WIDTH)
                {
                    frame->tap_profile3[r * TAP_WIDTH + c % TAP_WIDTH] = data_point;
                }
                else if (r >= 3 * TAP_WIDTH)
                {
                    frame->tap_profile4[r * TAP_WIDTH + c % TAP_WIDTH] = data_point;
                }
            }
            frame_mean += data_point;
        }
    }
    frame_mean /= (frWidth * frHeight);

    dft_ready_read = dft.update(frame_mean);
    if (dft_ready_read && cam_running) {
        dft.get(frame->frame_fft);
    } else {
        for (k = 0; k < FFT_INPUT_LENGTH; k++) {
            frame->frame_fft[k] = 0.0f;
        }
    }
    */

    for (r = 0; r < frHeight; r++) {
        frame->spectral_mean[r] /= nSamps;
    }

    for (c = 0; c < frWidth; c++) {
        frame->spatial_mean[c] /= nBands;
    }
}

float MeanFilter::getRawPixel(uint32_t index)
{
    return curFrame->raw_data[index];
}

float MeanFilter::getDSFPixel(uint32_t index)
{
    return curFrame->dsf_data[index];
}

float MeanFilter::getSNRPixel(uint32_t index)
{
    return curFrame->snr_data[index];
}

bool MeanFilter::dftReady()
{
    return dft_ready_read;
}
