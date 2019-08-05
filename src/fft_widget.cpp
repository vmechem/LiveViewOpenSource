#include "fft_widget.h"
#include <fftw3.h>
//#include "meanfilter.h"

fft_widget::fft_widget(FrameWorker *fw, QWidget *parent) :
    LVTabApplication(fw, parent)
{
    this->fw = fw;

    DCMaskBox = new QCheckBox(QString("Mask DC component"), this);
    DCMaskBox->setChecked(true);
    plMeanButton = new QRadioButton("Plane Mean", this);
    plMeanButton->setChecked(true);
    vCrossButton = new QRadioButton("Vertical Crosshair", this);
    vCrossButton->setChecked(false);
    tapPrfButton = new QRadioButton("Tap Profile", this);
    tapPrfButton->setChecked(false);

    qcp->xAxis->setLabel("Frequency [Hz]");
    qcp->yAxis->setLabel("Magnitude");

    tapToProfile.setMinimum(0);
    tapToProfile.setMaximum(fw->getNumTaps());
    tapToProfile.setSingleStep(1);
    tapToProfile.setEnabled(false);
    connect(tapPrfButton, SIGNAL(toggled(bool)), &tapToProfile, SLOT(setEnabled(bool)));
    //connect(&tapToProfile, SIGNAL(valueChanged(int)), fw, SLOT(tapPrfChanged(int)));
    //connect(tapToProfile, SIGNAL(valueChanged(int), fw, SLOT(tapPrfChanged(int))));
    connect(plMeanButton, SIGNAL(clicked()), this, SLOT(updateFFT()));
    connect(vCrossButton, SIGNAL(clicked()), this, SLOT(updateFFT()));
    connect(tapPrfButton, SIGNAL(clicked()), this, SLOT(updateFFT()));
    fft_bars = new QCPBars(qcp->xAxis, qcp->yAxis);
    fft_bars->setName("Magnitude of FFT for Mean Frame Pixel Value");

    freq_bins = QVector<double>(FFT_INPUT_LENGTH / 2);
    rfft_data_vec = QVector<double>(FFT_INPUT_LENGTH / 2);

    auto qgl = new QGridLayout(this);
    qgl->addWidget(qcp, 0, 0, 8, 8);
    qgl->addWidget(DCMaskBox, 8, 0, 1, 2);
    qgl->addWidget(plMeanButton, 8, 2, 1, 1);
    qgl->addWidget(vCrossButton, 8, 3, 1, 1);
    qgl->addWidget(tapPrfButton, 8, 4, 1, 1);
    this->setLayout(qgl);
    setCeiling(100.0);
    setPrecision(true);

    if (fw->settings->value(QString("dark"), USE_DARK_STYLE).toBool()) {
        fft_bars->setPen(QPen(Qt::lightGray));
        fft_bars->setBrush(QBrush(QColor(0x31363B)));

        qcp->setBackground(QBrush(QColor(0x31363B)));
        qcp->xAxis->setTickLabelColor(Qt::white);
        qcp->xAxis->setBasePen(QPen(Qt::white));
        qcp->xAxis->setLabelColor(Qt::white);
        qcp->xAxis->setTickPen(QPen(Qt::white));
        qcp->xAxis->setSubTickPen(QPen(Qt::white));
        qcp->yAxis->setTickLabelColor(Qt::white);
        qcp->yAxis->setBasePen(QPen(Qt::white));
        qcp->yAxis->setLabelColor(Qt::white);
        qcp->yAxis->setTickPen(QPen(Qt::white));
        qcp->yAxis->setSubTickPen(QPen(Qt::white));
        qcp->xAxis2->setTickLabelColor(Qt::white);
        qcp->xAxis2->setBasePen(QPen(Qt::white));
        qcp->xAxis2->setTickPen(QPen(Qt::white));
        qcp->xAxis2->setSubTickPen(QPen(Qt::white));
        qcp->yAxis2->setTickLabelColor(Qt::white);
        qcp->yAxis2->setBasePen(QPen(Qt::white));
        qcp->yAxis2->setTickPen(QPen(Qt::white));
        qcp->yAxis2->setSubTickPen(QPen(Qt::white));
    }

    connect(qcp->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(graphScrolledX(QCPRange)));
    connect(qcp->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(barsScrolledY(QCPRange)));

    connect(&renderTimer, &QTimer::timeout, this, &fft_widget::handleNewFrame);
    renderTimer.start(FRAME_DISPLAY_PERIOD_MSECS);
}

void fft_widget::handleNewFrame(FFT_t FFTtype)
{
    if (!this->isHidden()) {
        double framerate = frame_handler->fps > 0 ? frame_handler->fps : 1;

        //Why not framerate/2?
        double nyquist_freq =  500.0 / framerate;

        //What do all the cases mean? -> need to redefine nyquist_freq for all?
        switch(FFTtype){
        case FRAME_MEAN:
            //What is delta and why does nyquist frequency depend on it
            //nyquist_freq = fw->delta / 2.0;
            break;
        case COL_PROFILE:
            //nyquist_freq = fw->getFrameHeight() * fw->delta / 2.0;
            break;
        case TAP_PROFILE:
        {
            nyquist_freq = TAP_WIDTH * frame_handler->getFrameHeight() / 2.0;
            //Need to replace argument with actual tap number
            double* temp = fw->getTapProfile(1);
            fftw_complex in[sizeof(temp)], *out;
            fftw_plan p;
            int N = sizeof(temp);
            for (int i = 0 ; i < (int) sizeof(temp); i++)
            {
                in[i][0] = temp[i];
                in[i][1] = 0;
            }
            out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
            p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
            fftw_execute(p);
            fftw_destroy_plan(p);
            fftw_free(in);
            fftw_free(out);
            fftw_cleanup();
            break;
        }
            /*
        case TAP_PROFILE_2:
            nyquist_freq = TAP_WIDTH * frame_handler->getFrameHeight() / 2.0;
            break;
        case TAP_PROFILE_3:
            nyquist_freq = TAP_WIDTH * frame_handler->getFrameHeight() / 2.0;
            break;
        case TAP_PROFILE_4:
            nyquist_freq = TAP_WIDTH * frame_handler->getFrameHeight() / 2.0;
            break;
            */
        }


        double increment = nyquist_freq / (FFT_INPUT_LENGTH / 2);
        fft_bars->setWidth(increment);

        for (int i = 0; i < FFT_INPUT_LENGTH / 2; i++) {
            freq_bins[i] = increment * i;
        }

        float *fft_data_ptr = frame_handler->getFrameFFT();
        for (int b = 0; b < FFT_INPUT_LENGTH / 2; b++) {
            rfft_data_vec[b] = static_cast<double>(fft_data_ptr[b]);
        }
        if (DCMaskBox->isChecked()) {
            rfft_data_vec[0] = 0;
        }
        fft_bars->setData(freq_bins, rfft_data_vec);
        qcp->xAxis->setRange(QCPRange(0, nyquist_freq));
        qcp->replot();
    }
}

void fft_widget::barsScrolledY(const QCPRange &newRange)
{    Q_UNUSED(newRange);
    rescaleRange();
}

void fft_widget::rescaleRange()
{
    qcp->yAxis->setRange(QCPRange(getFloor(), getCeiling()));
}

fftw_complex* getFFT(double* arr)
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
    qDebug() << "out1 = " << out[1];
    return out;
}

void fft_widget::updateFFT()
{
    if(plMeanButton->isChecked())
        fw->update_FFT_range(FRAME_MEAN);
    else if(vCrossButton->isChecked())
        fw->update_FFT_range(COL_PROFILE);
    else if(tapPrfButton->isChecked())
        fw->update_FFT_range(TAP_PROFILE, tapToProfile.value());

}
