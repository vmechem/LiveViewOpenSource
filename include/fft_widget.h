#ifndef FFT_WIDGET_H
#define FFT_WIDGET_H

#include <QVector>
#include <QSpinBox>
#include <fftw3.h>

#include "qcustomplot.h"
#include "lvtabapplication.h"

class fft_widget : public LVTabApplication
{
    Q_OBJECT
public:
    explicit fft_widget(FrameWorker *fw, QWidget *parent = nullptr);
    ~fft_widget() = default;

    QRadioButton *plMeanButton;
    QRadioButton *vCrossButton;
    QRadioButton *tapPrfButton;
    QSpinBox tapToProfile;

public slots:
    void handleNewFrame();
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
