
#pragma once


class SpectrogramComponent   : public AudioAppComponent,
                               private Timer
{
public:
    SpectrogramComponent()
        : forwardFFT(fftOrder),
          spectrogramImage(Image::RGB, 512, 512, true)
    {
        setOpaque(true);
        setAudioChannels(2, 0);  // want a couple of input channels but no outputs
        startTimerHz(60);
        setSize(700, 500);
    }

    ~SpectrogramComponent()
    {
        shutdownAudio();
    }

    enum
    {
        fftOrder = 10,
        fftSize  = 1 << fftOrder
    };

    
    void prepareToPlay(int, double) override {}
    void releaseResources() override          {}

    void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) override 
    {
        if(bufferToFill.buffer->getNumChannels() > 0)
        {
            auto* channelData = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);

            for(auto i = 0; i < bufferToFill.numSamples;  i++)
                pushNextSampleIntoFifo(channelData[i]);
        }   
    }

    
    void paint(Graphics& g) override
    {
        g.fillAll(Colours::black);
        g.setOpacity(1.0f);
        g.drawImage(spectrogramImage, getLocalBounds().toFloat());
    }

    void timerCallback() override 
    {
        if(nextFFTBlockReady)
        {
            drawNextLineOfSpectrogram();
            nextFFTBlockReady = false;
            repaint();
        }     
    }

    void pushNextSampleIntoFifo(float sample) noexcept
    {   // if the fifo contains enough data, set a flag to say
        // that the next line should now be rendered..

        if(fifoIndex == fftSize)
        {
            if(! nextFFTBlockReady)
            {
                zeromem(fftData, sizeof(fftData));
                memcpy(fftData, fifo, sizeof(fifo));
                nextFFTBlockReady = true;
            }
            fifoIndex = 0;
        }
        fifo[fifoIndex++] = sample;
    }

    void drawNextLineOfSpectrogram() 
    {
        auto rightHandEdge = spectrogramImage.getWidth() - 1;
        auto imageHeight   = spectrogramImage.getHeight();

        spectrogramImage.moveImageSection(0, 0, 1, 0, rightHandEdge, imageHeight);

        forwardFFT.performFrequencyOnlyForwardTransform(fftData);

        auto maxLevel = FloatVectorOperations::findMinAndMax(fftData, fftSize / 2);

        for(auto y = 1; y < imageHeight; ++y)
        {
            auto skewedProportionY = 1.0f - std::exp(std::log(y /(float) imageHeight) * 0.2f);
            auto fftDataIndex      = jlimit(0, fftSize / 2,(int)(skewedProportionY * fftSize / 2));
            auto level             = jmap(fftData[fftDataIndex], 0.0f, jmax(maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);
            spectrogramImage.setPixelAt(rightHandEdge, y, Colour::fromHSV(level, 1.0f, level, 1.0f));
        }      
    }

private:
    dsp::FFT forwardFFT;
    Image spectrogramImage;

    float fifo[fftSize];
    float fftData[fftSize * 2];
    int   fifoIndex         = 0;
    bool  nextFFTBlockReady = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrogramComponent)
};
