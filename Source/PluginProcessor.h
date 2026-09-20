/*
  ==============================================================================

    PluginProcessor.h
    Yamaha CP-30 Electronic Piano Replication
    Hosts IK Multimedia SampleTank 4 (VST3) with 16ch -> 2ch summing,
    independent Sound I / Sound II channels, Tremolo, Tone, and Balance.
    Compatible with JUCE 9.0.2 / C++17 / MSVC x64.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <algorithm>

#if ! JUCE_64BIT
 #error "SampleTank 4 is a 64-bit VST3 plugin! Please configure Visual Studio to build for x64 architecture."
#endif

//==============================================================================
class CP30AudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    CP30AudioProcessor();
    ~CP30AudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Accessor for APVTS to connect Editor UI
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Parameter ID Constants
    static constexpr const char* ID_POWER            = "power";
    static constexpr const char* ID_TONE_I_PIANO1    = "tone_i_piano1";
    static constexpr const char* ID_TONE_I_PIANO2    = "tone_i_piano2";
    static constexpr const char* ID_TONE_I_PIANO3    = "tone_i_piano3";
    static constexpr const char* ID_TONE_I_HARPSI    = "tone_i_harpsi";

    static constexpr const char* ID_TONE_II_PIANO1   = "tone_ii_piano1";
    static constexpr const char* ID_TONE_II_PIANO2   = "tone_ii_piano2";
    static constexpr const char* ID_TONE_II_PIANO3   = "tone_ii_piano3";
    static constexpr const char* ID_TONE_II_HARPSI   = "tone_ii_harpsi";

    static constexpr const char* ID_PITCH_I          = "pitch_i";
    static constexpr const char* ID_PITCH_II         = "pitch_ii";
    static constexpr const char* ID_DECAY_I          = "decay_i";
    static constexpr const char* ID_DECAY_II         = "decay_ii";

    static constexpr const char* ID_TREMOLO_I        = "tremolo_i";
    static constexpr const char* ID_TREMOLO_II       = "tremolo_ii";
    static constexpr const char* ID_TREMOLO_SPEED    = "tremolo_speed";
    static constexpr const char* ID_TREMOLO_INTENSITY= "tremolo_intensity";

    static constexpr const char* ID_BASS             = "bass";
    static constexpr const char* ID_TREBLE           = "treble";
    static constexpr const char* ID_BALANCE          = "balance";
    static constexpr const char* ID_VOLUME           = "volume";

private:
    //==============================================================================
    void loadSampleTank4Async();
    void restoreST4Preset (juce::AudioPluginInstance& instance);
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // APVTS parameter manager
    juce::AudioProcessorValueTreeState apvts;

    // Cached atomic float pointers for lock-free audio thread reads
    std::atomic<float>* powerParam           = nullptr;
    std::atomic<float>* toneIPiano1Param     = nullptr;
    std::atomic<float>* toneIPiano2Param     = nullptr;
    std::atomic<float>* toneIPiano3Param     = nullptr;
    std::atomic<float>* toneIHarpsiParam     = nullptr;
    std::atomic<float>* toneIIPiano1Param    = nullptr;
    std::atomic<float>* toneIIPiano2Param    = nullptr;
    std::atomic<float>* toneIIPiano3Param    = nullptr;
    std::atomic<float>* toneIIHarpsiParam    = nullptr;
    std::atomic<float>* pitchIParam          = nullptr;
    std::atomic<float>* pitchIIParam         = nullptr;
    std::atomic<float>* decayIParam          = nullptr;
    std::atomic<float>* decayIIParam         = nullptr;
    std::atomic<float>* tremoloIParam        = nullptr;
    std::atomic<float>* tremoloIIParam       = nullptr;
    std::atomic<float>* tremoloSpeedParam    = nullptr;
    std::atomic<float>* tremoloIntensityParam= nullptr;
    std::atomic<float>* bassParam            = nullptr;
    std::atomic<float>* trebleParam          = nullptr;
    std::atomic<float>* balanceParam         = nullptr;
    std::atomic<float>* volumeParam          = nullptr;

    // VST3 plugin format manager (JUCE 9.0.2)
    juce::AudioPluginFormatManager formatManager;

    // Hosted ST4 plugin instance
    std::unique_ptr<juce::AudioPluginInstance> st4Instance;
    juce::CriticalSection pluginLock;
    std::shared_ptr<bool> aliveToken;

    // Internal multi-channel audio buffers
    juce::AudioBuffer<float> st4Buffer;
    juce::AudioBuffer<float> soundIBuffer;
    juce::AudioBuffer<float> soundIIBuffer;

    // 8 stereo pairs = 16 channels from ST4
    static constexpr int numMixChannels = 16;

    // Tremolo LFO state
    double tremoloPhase = 0.0;
    double currentSampleRate = 44100.0;

    // Simple 1-pole shelving filters for Bass (100Hz) and Treble (10kHz)
    struct ShelfFilter
    {
        float s1 = 0.0f;
        void reset() { s1 = 0.0f; }
        inline float processLowShelf (float x, float gainLinear, float alpha)
        {
            s1 += alpha * (x - s1);
            return x + (gainLinear - 1.0f) * s1;
        }
        inline float processHighShelf (float x, float gainLinear, float alpha)
        {
            s1 += alpha * (x - s1);
            const float high = x - s1;
            return x + (gainLinear - 1.0f) * high;
        }
    };

    ShelfFilter bassFilterL, bassFilterR;
    ShelfFilter trebleFilterL, trebleFilterR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CP30AudioProcessor)
};
