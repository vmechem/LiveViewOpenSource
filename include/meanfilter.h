#ifndef MEANFILTER_H
#define MEANFILTER_H

#include <stdint.h>

#include <QPointF>
#include <QDebug>
#include <vector>

#include "lvframe.h"
#include "sliding_dft.h"

enum FFT_t {FRAME_MEAN, COL_PROFILE, TAP_PROFILE};

class MeanFilter
{
public:
    MeanFilter(int frame_width, int frame_height);
    ~MeanFilter();

    void compute_mean(LVFrame *frame, QPointF topLeft, QPointF bottomRight,
                      LV::PlotMode pm, bool cam_running);
    void getFFTMagnitude(/*LVFrame *frame,*/ FFT_t profileType, int numTap = -1);
    fftw_complex* getFFT(double* arr);
    void updateFFTMagnitude(fftw_complex* fft);
    double* getTapProfile(int n);
    int getNumTaps();
    bool dftReady();

private:
    float (MeanFilter::*p_getPixel)(uint32_t);
    float getRawPixel(uint32_t index);
    float getDSFPixel(uint32_t index);
    float getSNRPixel(uint32_t index);

    LVFrame *curFrame;

    SlidingDFT<float, FFT_INPUT_LENGTH> dft;
    bool dft_ready_read;

    int frWidth;
    int frHeight;
};


#endif // MEANFILTER_H
