#ifndef FFT_WIDGET_H
#define FFT_WIDGET_H

#include <QVector>
#include <fftw3.h>

#include "lvtabapplication.h"

class fft_widget : public LVTabApplication
{
    Q_OBJECT
public:
    explicit fft_widget(FrameWorker *fw, QWidget *parent = nullptr);
    ~fft_widget() = default;
    FrameWorker *fw;

    QRadioButton *plMeanButton;
    QRadioButton *vCrossButton;
    QRadioButton *tapPrfButton;
    QSpinBox tapToProfile;

public slots:
    void handleNewFrame(FFT_t FFTtype);
    void barsScrolledY(const QCPRange &newRange);
    void rescaleRange();
    fftw_complex* getFFT(double* arr);
    void updateFFT();


private:
    QCPBars *fft_bars;
    QCheckBox *DCMaskBox;
    QVector<double> freq_bins;
    QVector<double> rfft_data_vec;

};

#endif // FFT_WIDGET_H
