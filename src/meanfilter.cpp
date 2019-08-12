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
    //int r, c, k;
    int r, c;
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


    for (r = 0; r < frHeight; r++)
    {
        for (c = 0; c < frWidth; c++)
        {
            data_point = (this->*p_getPixel)(static_cast<uint32_t>(r * frWidth + c));
            if (c > int(topLeft.x()) && c <= int(bottomRight.x()))
            {
                frame->spectral_mean[r] += data_point;
            }
            if (r > int(topLeft.y()) && r <= int(bottomRight.y()))
            {
                frame->spatial_mean[c] += data_point;
            }
            frame_mean += data_point;
        }
    }
    frame_mean /= (frWidth * frHeight);

/*
    dft_ready_read = dft.update(frame_mean);
    if (dft_ready_read && cam_running) {
        dft.get(frame->frame_fft);
    } else {
        for (k = 0; k < FFT_INPUT_LENGTH; k++) {
            frame->frame_fft[k] = 0.0f;
        }
    }*/

    for (r = 0; r < frHeight; r++) {
        frame->spectral_mean[r] /= nSamps;
    }

    for (c = 0; c < frWidth; c++) {
        frame->spatial_mean[c] /= nBands;
    }

    getFFTMagnitude(frame, frame_mean, cam_running);
}

void MeanFilter::getFFTMagnitude(LVFrame *frame, float mean, bool cam_running)
{
    if(getFFTType() == FRAME_MEAN)
    {
        dft_ready_read = dft.update(mean);
        dft.get(frame->frame_mean_fft);
        if(dft_ready_read && cam_running)
        {
            for(unsigned i = 0; i < sizeof(dft.dft); i++)
            {
                frame->frame_fft[i] = sqrt(dft.dft[i].real() * dft.dft[i].real() + dft.dft[i].imag() * dft.dft[i].imag());
            }
        }
        else
        {
            for (int k = 0; k <FFT_INPUT_LENGTH; k++)
            {
                frame->frame_fft[k] = 0.0f;
            }
        }
    }
    else if (getFFTType() == COL_PROFILE)
    {
        double* arr[frHeight];
        for(unsigned i = 0; i < sizeof(arr); i++)
        {
            *arr[i] = (double)frame->spectral_mean[i];
        }
        fftw_complex* temp = getFFT(*arr);
        updateFFTMagnitude(frame, temp);

    }
    else if (getFFTType() == TAP_PROFILE)
    {
        double* arr = getTapProfile(getTapNum());
        fftw_complex* temp = getFFT(arr);
        updateFFTMagnitude(frame, temp);
    }
}

fftw_complex* MeanFilter::getFFT(double* arr)
{
    fftw_complex in[sizeof(arr)], *out;
    fftw_plan p;
    int N = sizeof(arr);
    for (int i = 0 ; i < (int) sizeof(arr); i++)
    {
        in[i][0] = arr[i];
        in[i][1] = 0;
    }
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);
    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);
    fftw_cleanup();
    //qDebug() << "out1 = " << out[1];
    return out;
}

//Given the fftw_complex calculate the magnitude of fft and stores it into frame->fftdata so fft_widget can access the
//fftdata and create graph
void MeanFilter::updateFFTMagnitude(LVFrame *frame, fftw_complex* fft)
{
    for (unsigned i = 0; i < sizeof (fft); i++)
    {
        double mag = sqrt(fft[i][0]*fft[i][0] + fft[i][1]*fft[i][1]);
        frame->frame_fft[i] = mag;
    }

}


double* MeanFilter::getTapProfile(int n)
{
    if (n >= getNumTaps())
    {
        throw std::invalid_argument("Tap unavailable");
    }
    int max = std::min(frWidth - TAP_WIDTH * n, TAP_WIDTH);
    double* tap_profile[max * frHeight];
    for (int r = 0; r < max; r++)
    {
        for (int c = 0; c < frHeight; c++)
        {
            *tap_profile[r * max + c] = getRawPixel(static_cast<uint32_t>(frWidth*r + TAP_WIDTH * n + c));
        }
    }
    return *tap_profile;
}

int MeanFilter::getNumTaps()
{
    int w = frWidth;
    return (w / TAP_WIDTH) + (w % TAP_WIDTH > 0);
}

FFT_t MeanFilter::getFFTType()
{
    return fft_type;
}

void MeanFilter::changeFFTType(FFT_t type, int x)
{
    fft_type = type;
    tapNum = x;
}

int MeanFilter::getTapNum()
{
    return tapNum;
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
