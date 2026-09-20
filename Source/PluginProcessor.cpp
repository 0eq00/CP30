/*
  ==============================================================================

    PluginProcessor.cpp
    Yamaha CP-30 Electronic Piano Replication
    Hosts IK Multimedia SampleTank 4 (VST3) with 16ch -> 2ch summing,
    independent Sound I / Sound II channels, Tremolo, Tone, and Balance.
    Compatible with JUCE 9.0.2 / C++17 / MSVC x64.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout CP30AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Power switch
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID (ID_POWER, 1), "Power", true));

    // Sound I Tone Tablets
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID (ID_TONE_I_PIANO1, 1), "Sound I: Piano 1", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID (ID_TONE_I_PIANO2, 1), "Sound I: Piano 2", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID (ID_TONE_I_PIANO3, 1), "Sound I: Piano 3", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID (ID_TONE_I_HARPSI, 1), "Sound I: Harpsichord", false));

    // Sound II Tone Tablets
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID (ID_TONE_II_PIANO1, 1), "Sound II: Piano 1", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID (ID_TONE_II_PIANO2, 1), "Sound II: Piano 2", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID (ID_TONE_II_PIANO3, 1), "Sound II: Piano 3", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID (ID_TONE_II_HARPSI, 1), "Sound II: Harpsichord", false));

    // Pitch & Decay (Fine tuning for detune / honky-tonk effect)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ID_PITCH_I, 1), "Pitch I", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ID_PITCH_II, 1), "Pitch II", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.05f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ID_DECAY_I, 1), "Decay I", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ID_DECAY_II, 1), "Decay II", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

    // Tremolo Section (Manual p.11: 0.5Hz to 14Hz, Intensity max 70%)
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID (ID_TREMOLO_I, 1), "Tremolo I", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID (ID_TREMOLO_II, 1), "Tremolo II", true));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ID_TREMOLO_SPEED, 1), "Tremolo Speed",
        juce::NormalisableRange<float> (0.5f, 14.0f, 0.1f, 0.6f), 4.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ID_TREMOLO_INTENSITY, 1), "Tremolo Intensity",
        juce::NormalisableRange<float> (0.0f, 0.70f, 0.01f), 0.45f));

    // Tone Control (Manual p.12: Bass 100Hz +-10dB, Treble 10kHz +-10dB)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ID_BASS, 1), "Bass",
        juce::NormalisableRange<float> (-10.0f, 10.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ID_TREBLE, 1), "Treble",
        juce::NormalisableRange<float> (-10.0f, 10.0f, 0.1f), 0.0f));

    // Balance (Sound I vs Sound II mix) & Master Volume
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ID_BALANCE, 1), "Balance",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ID_VOLUME, 1), "Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.85f));

    return { params.begin(), params.end() };
}

//==============================================================================
CP30AudioProcessor::CP30AudioProcessor()
     : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts (*this, nullptr, "Parameters", createParameterLayout()),
       aliveToken (std::make_shared<bool> (true))
{
    // Cache atomic parameter pointers
    powerParam            = apvts.getRawParameterValue (ID_POWER);
    toneIPiano1Param      = apvts.getRawParameterValue (ID_TONE_I_PIANO1);
    toneIPiano2Param      = apvts.getRawParameterValue (ID_TONE_I_PIANO2);
    toneIPiano3Param      = apvts.getRawParameterValue (ID_TONE_I_PIANO3);
    toneIHarpsiParam      = apvts.getRawParameterValue (ID_TONE_I_HARPSI);
    toneIIPiano1Param     = apvts.getRawParameterValue (ID_TONE_II_PIANO1);
    toneIIPiano2Param     = apvts.getRawParameterValue (ID_TONE_II_PIANO2);
    toneIIPiano3Param     = apvts.getRawParameterValue (ID_TONE_II_PIANO3);
    toneIIHarpsiParam     = apvts.getRawParameterValue (ID_TONE_II_HARPSI);
    pitchIParam           = apvts.getRawParameterValue (ID_PITCH_I);
    pitchIIParam          = apvts.getRawParameterValue (ID_PITCH_II);
    decayIParam           = apvts.getRawParameterValue (ID_DECAY_I);
    decayIIParam          = apvts.getRawParameterValue (ID_DECAY_II);
    tremoloIParam         = apvts.getRawParameterValue (ID_TREMOLO_I);
    tremoloIIParam        = apvts.getRawParameterValue (ID_TREMOLO_II);
    tremoloSpeedParam     = apvts.getRawParameterValue (ID_TREMOLO_SPEED);
    tremoloIntensityParam = apvts.getRawParameterValue (ID_TREMOLO_INTENSITY);
    bassParam             = apvts.getRawParameterValue (ID_BASS);
    trebleParam           = apvts.getRawParameterValue (ID_TREBLE);
    balanceParam          = apvts.getRawParameterValue (ID_BALANCE);
    volumeParam           = apvts.getRawParameterValue (ID_VOLUME);

    // Register VST3 format into manager
    formatManager.addFormat (std::make_unique<juce::VST3PluginFormat>());

    // Trigger asynchronous loading of SampleTank 4
    loadSampleTank4Async();
}

CP30AudioProcessor::~CP30AudioProcessor()
{
    if (aliveToken != nullptr)
        *aliveToken = false;

    const juce::ScopedLock sl (pluginLock);

    if (st4Instance != nullptr)
        st4Instance->releaseResources();

    st4Instance.reset();
}

//==============================================================================
void CP30AudioProcessor::loadSampleTank4Async()
{
    const juce::StringArray candidatePaths = {
        "C:\\Program Files\\Common Files\\VST3\\SampleTank 4.vst3",
        "C:\\Program Files\\Common Files\\VST3\\IK Multimedia\\SampleTank 4.vst3",
        "C:\\Program Files\\Common Files\\VST3\\SampleTank 4\\SampleTank 4.vst3"
    };

    juce::File vst3File;
    for (const auto& path : candidatePaths)
    {
        const juce::File f (path);
        if (f.exists())
        {
            vst3File = f;
            break;
        }
    }

    if (! vst3File.exists())
    {
        DBG ("[CP30AudioProcessor] Error: SampleTank 4.vst3 not found.");
        return;
    }

    juce::File binaryFileToLoad = vst3File;
    if (vst3File.isDirectory())
    {
        const juce::File bundleBinary = vst3File.getChildFile ("Contents")
                                                .getChildFile ("x86_64-win")
                                                .getChildFile (vst3File.getFileName());
        if (bundleBinary.existsAsFile())
            binaryFileToLoad = bundleBinary;
    }

    auto* format = formatManager.getFormat (0);
    if (format == nullptr)
        return;

    juce::OwnedArray<juce::PluginDescription> descriptions;
    format->findAllTypesForFile (descriptions, vst3File.getFullPathName());

    if (descriptions.isEmpty() && binaryFileToLoad != vst3File)
        format->findAllTypesForFile (descriptions, binaryFileToLoad.getFullPathName());

    if (descriptions.isEmpty())
        return;

    const juce::PluginDescription& desc = *descriptions[0];
    const double initialSampleRate = (getSampleRate() > 0.0) ? getSampleRate() : 44100.0;
    const int initialBlockSize     = (getBlockSize() > 0)    ? getBlockSize()    : 512;

    std::weak_ptr<bool> weakToken = aliveToken;

    formatManager.createPluginInstanceAsync (
        desc,
        initialSampleRate,
        initialBlockSize,
        [this, weakToken] (std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& errorMsg)
        {
            auto token = weakToken.lock();
            if (! token || ! *token)
                return;

            if (errorMsg.isNotEmpty() || instance == nullptr)
            {
                DBG ("[CP30AudioProcessor] Failed to instantiate ST4: " + errorMsg);
                return;
            }

            DBG ("[CP30AudioProcessor] SampleTank 4 instantiated successfully!");

            instance->enableAllBuses();

            const int st4InChannels  = instance->getTotalNumInputChannels();
            const int st4OutChannels = instance->getTotalNumOutputChannels();

            const double currentRate = getSampleRate();
            const int currentBlock   = getBlockSize();
            if (currentRate > 0.0 && currentBlock > 0)
                instance->prepareToPlay (currentRate, currentBlock);

            restoreST4Preset (*instance);

            {
                const juce::ScopedLock sl (pluginLock);
                const int requiredChannels = juce::jmax (numMixChannels, juce::jmax (st4InChannels, st4OutChannels));
                const int samplesToAlloc = (currentBlock > 0) ? currentBlock : 512;
                st4Buffer.setSize (requiredChannels, samplesToAlloc, false, false, true);
                st4Buffer.clear();

                soundIBuffer.setSize (2, samplesToAlloc, false, false, true);
                soundIIBuffer.setSize (2, samplesToAlloc, false, false, true);

                st4Instance = std::move (instance);
            }

            DBG ("[CP30AudioProcessor] CP-30 Engine ready!");
        });
}

//==============================================================================
void CP30AudioProcessor::restoreST4Preset (juce::AudioPluginInstance& instance)
{
    juce::File vendorFolder = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory().getParentDirectory().getParentDirectory().getParentDirectory();
    const juce::File presetFile = vendorFolder.getChildFile("CP30.vstpreset");
//  const juce::File presetFile ("C:\\CP30.vstpreset");
    if (!presetFile.existsAsFile())
    {
        DBG("[CP30AudioProcessor] Error preset[" + presetFile.getFullPathName() + "]");
        return;
    }

    juce::MemoryBlock presetData;
    if (! presetFile.loadFileAsData (presetData) || presetData.getSize() == 0)
        return;

    if (auto* vst3Client = instance.getVST3Client())
    {
        vst3Client->setPreset (presetData);
        DBG ("[CP30AudioProcessor] Restored preset via getVST3Client()->setPreset()");
    }
    else
    {
        instance.setStateInformation (presetData.getData(), static_cast<int> (presetData.getSize()));
    }
}

//==============================================================================
const juce::String CP30AudioProcessor::getName() const      { return JucePlugin_Name; }
bool CP30AudioProcessor::acceptsMidi() const               { return true; }
bool CP30AudioProcessor::producesMidi() const              { return false; }
bool CP30AudioProcessor::isMidiEffect() const              { return false; }
double CP30AudioProcessor::getTailLengthSeconds() const    { return 0.0; }
int CP30AudioProcessor::getNumPrograms()                   { return 1; }
int CP30AudioProcessor::getCurrentProgram()                { return 0; }
void CP30AudioProcessor::setCurrentProgram (int)           {}
const juce::String CP30AudioProcessor::getProgramName (int){ return {}; }
void CP30AudioProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void CP30AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const juce::ScopedLock sl (pluginLock);

    currentSampleRate = sampleRate;
    tremoloPhase = 0.0;

    bassFilterL.reset();
    bassFilterR.reset();
    trebleFilterL.reset();
    trebleFilterR.reset();

    int requiredChannels = numMixChannels;

    if (st4Instance != nullptr)
    {
        st4Instance->enableAllBuses();
        st4Instance->prepareToPlay (sampleRate, samplesPerBlock);

        requiredChannels = juce::jmax (requiredChannels,
                                       juce::jmax (st4Instance->getTotalNumInputChannels(),
                                                   st4Instance->getTotalNumOutputChannels()));
    }

    st4Buffer.setSize (requiredChannels, samplesPerBlock, false, false, true);
    st4Buffer.clear();

    soundIBuffer.setSize (2, samplesPerBlock, false, false, true);
    soundIIBuffer.setSize (2, samplesPerBlock, false, false, true);
    soundIBuffer.clear();
    soundIIBuffer.clear();
}

void CP30AudioProcessor::releaseResources()
{
    const juce::ScopedLock sl (pluginLock);

    if (st4Instance != nullptr)
        st4Instance->releaseResources();

    st4Buffer.setSize (0, 0);
    soundIBuffer.setSize (0, 0);
    soundIIBuffer.setSize (0, 0);
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CP30AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}
#endif

//==============================================================================
void CP30AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples   = buffer.getNumSamples();
    const int hostChannels = buffer.getNumChannels();

    if (numSamples <= 0 || hostChannels <= 0)
        return;

    buffer.clear();

    // Check power switch
    if (powerParam != nullptr && powerParam->load() < 0.5f)
        return; // Output complete silence when Power is OFF

    const juce::ScopedLock sl (pluginLock);

    if (st4Instance == nullptr)
        return;

    // Ensure buffers have sufficient size
    const int st4InChannels  = st4Instance->getTotalNumInputChannels();
    const int st4OutChannels = st4Instance->getTotalNumOutputChannels();
    const int requiredChannels = juce::jmax (numMixChannels, juce::jmax (st4InChannels, st4OutChannels));

    if (st4Buffer.getNumChannels() < requiredChannels || st4Buffer.getNumSamples() < numSamples)
        st4Buffer.setSize (requiredChannels, numSamples, false, false, true);

    if (soundIBuffer.getNumSamples() < numSamples)
    {
        soundIBuffer.setSize (2, numSamples, false, false, true);
        soundIIBuffer.setSize (2, numSamples, false, false, true);
    }

    st4Buffer.clear();
    soundIBuffer.clear();
    soundIIBuffer.clear();

    // Render ST4 into 16-channel buffer
    st4Instance->processBlock (st4Buffer, midiMessages);

    // Read Tone Tablet Switches
    const bool tI_p1 = (toneIPiano1Param != nullptr) ? (toneIPiano1Param->load() > 0.5f) : true;
    const bool tI_p2 = (toneIPiano2Param != nullptr) ? (toneIPiano2Param->load() > 0.5f) : false;
    const bool tI_p3 = (toneIPiano3Param != nullptr) ? (toneIPiano3Param->load() > 0.5f) : false;
    const bool tI_hp = (toneIHarpsiParam != nullptr) ? (toneIHarpsiParam->load() > 0.5f) : false;

    const bool tII_p1 = (toneIIPiano1Param != nullptr) ? (toneIIPiano1Param->load() > 0.5f) : false;
    const bool tII_p2 = (toneIIPiano2Param != nullptr) ? (toneIIPiano2Param->load() > 0.5f) : true;
    const bool tII_p3 = (toneIIPiano3Param != nullptr) ? (toneIIPiano3Param->load() > 0.5f) : false;
    const bool tII_hp = (toneIIHarpsiParam != nullptr) ? (toneIIHarpsiParam->load() > 0.5f) : false;

    // Sum active Sound I channels into soundIBuffer (stereo)
    // Sound I:
    // Ch 0, 1: Piano 1
    // Ch 2, 3: Piano 2
    // Ch 4, 5: Piano 3
    // Ch 6, 7: Harpsichord
    if (tI_p1) { soundIBuffer.addFrom (0, 0, st4Buffer, 0, 0, numSamples, 1.0f);
                 soundIBuffer.addFrom (1, 0, st4Buffer, 1, 0, numSamples, 1.0f); }
    if (tI_p2) { soundIBuffer.addFrom (0, 0, st4Buffer, 2, 0, numSamples, 1.0f);
                 soundIBuffer.addFrom (1, 0, st4Buffer, 3, 0, numSamples, 1.0f); }
    if (tI_p3) { soundIBuffer.addFrom (0, 0, st4Buffer, 4, 0, numSamples, 1.0f);
                 soundIBuffer.addFrom (1, 0, st4Buffer, 5, 0, numSamples, 1.0f); }
    if (tI_hp) { soundIBuffer.addFrom (0, 0, st4Buffer, 6, 0, numSamples, 1.0f);
                 soundIBuffer.addFrom (1, 0, st4Buffer, 7, 0, numSamples, 1.0f); }

    // Sum active Sound II channels into soundIIBuffer (stereo)
    // Sound II:
    // Ch 8, 9  : Piano 1
    // Ch 10, 11: Piano 2
    // Ch 12, 13: Piano 3
    // Ch 14, 15: Harpsichord
    if (tII_p1) { soundIIBuffer.addFrom (0, 0, st4Buffer, 8,  0, numSamples, 1.0f);
                  soundIIBuffer.addFrom (1, 0, st4Buffer, 9,  0, numSamples, 1.0f); }
    if (tII_p2) { soundIIBuffer.addFrom (0, 0, st4Buffer, 10, 0, numSamples, 1.0f);
                  soundIIBuffer.addFrom (1, 0, st4Buffer, 11, 0, numSamples, 1.0f); }
    if (tII_p3) { soundIIBuffer.addFrom (0, 0, st4Buffer, 12, 0, numSamples, 1.0f);
                  soundIIBuffer.addFrom (1, 0, st4Buffer, 13, 0, numSamples, 1.0f); }
    if (tII_hp) { soundIIBuffer.addFrom (0, 0, st4Buffer, 14, 0, numSamples, 1.0f);
                  soundIIBuffer.addFrom (1, 0, st4Buffer, 15, 0, numSamples, 1.0f); }

    // Tremolo parameters
    const bool tremI  = (tremoloIParam != nullptr) ? (tremoloIParam->load() > 0.5f) : false;
    const bool tremII = (tremoloIIParam != nullptr) ? (tremoloIIParam->load() > 0.5f) : false;
    const float tremSpeed = (tremoloSpeedParam != nullptr) ? tremoloSpeedParam->load() : 4.0f;
    const float tremIntensity = (tremoloIntensityParam != nullptr) ? tremoloIntensityParam->load() : 0.5f;

    // Balance parameter (0.0 = Sound I only, 0.5 = Equal, 1.0 = Sound II only)
    const float bal = (balanceParam != nullptr) ? balanceParam->load() : 0.5f;
    const float balGainI  = juce::jmin (1.0f, 2.0f * (1.0f - bal));
    const float balGainII = juce::jmin (1.0f, 2.0f * bal);

    // Master volume parameter
    const float masterVol = (volumeParam != nullptr) ? volumeParam->load() : 0.8f;

    // Tone parameters (Bass @ 100Hz, Treble @ 10kHz)
    const float bassDb = (bassParam != nullptr) ? bassParam->load() : 0.0f;
    const float trebleDb = (trebleParam != nullptr) ? trebleParam->load() : 0.0f;
    const float bassGain = juce::Decibels::decibelsToGain (bassDb);
    const float trebleGain = juce::Decibels::decibelsToGain (trebleDb);

    const float sampleRateFloat = static_cast<float> (currentSampleRate > 0.0 ? currentSampleRate : 44100.0);
    const float alphaBass = 2.0f * juce::MathConstants<float>::pi * 100.0f / sampleRateFloat;
    const float alphaTreble = 2.0f * juce::MathConstants<float>::pi * 10000.0f / sampleRateFloat;

    // Pointers for sample processing
    float* outL = buffer.getWritePointer (0);
    float* outR = (hostChannels >= 2) ? buffer.getWritePointer (1) : nullptr;

    const float* inI_L = soundIBuffer.getReadPointer (0);
    const float* inI_R = soundIBuffer.getReadPointer (1);
    const float* inII_L = soundIIBuffer.getReadPointer (0);
    const float* inII_R = soundIIBuffer.getReadPointer (1);

    const double phaseIncrement = (2.0 * juce::MathConstants<double>::pi * tremSpeed) / currentSampleRate;

    for (int i = 0; i < numSamples; ++i)
    {
        // Stereo Tremolo Calculation (CP-30 Manual p.11: 180 degrees out-of-phase LFO)
        const float lfoI  = 0.5f + 0.5f * static_cast<float> (std::sin (tremoloPhase));
        const float lfoII = 0.5f + 0.5f * static_cast<float> (std::sin (tremoloPhase + juce::MathConstants<double>::pi));

        const float tremGainI  = tremI  ? (1.0f - tremIntensity * lfoI)  : 1.0f;
        const float tremGainII = tremII ? (1.0f - tremIntensity * lfoII) : 1.0f;

        tremoloPhase += phaseIncrement;
        if (tremoloPhase >= 2.0 * juce::MathConstants<double>::pi)
            tremoloPhase -= 2.0 * juce::MathConstants<double>::pi;

        // Apply Balance, Tremolo, and mix Sound I & Sound II
        float mixedL = (inI_L[i] * balGainI * tremGainI) + (inII_L[i] * balGainII * tremGainII);
        float mixedR = (inI_R[i] * balGainI * tremGainI) + (inII_R[i] * balGainII * tremGainII);

        // Apply Bass & Treble Shelving Tone Controls
        mixedL = bassFilterL.processLowShelf (mixedL, bassGain, alphaBass);
        mixedL = trebleFilterL.processHighShelf (mixedL, trebleGain, alphaTreble);

        mixedR = bassFilterR.processLowShelf (mixedR, bassGain, alphaBass);
        mixedR = trebleFilterR.processHighShelf (mixedR, trebleGain, alphaTreble);

        // Master Volume
        mixedL *= masterVol;
        mixedR *= masterVol;

        // Write to DAW output
        outL[i] = mixedL;
        if (outR != nullptr)
            outR[i] = mixedR;
    }
}

//==============================================================================
bool CP30AudioProcessor::hasEditor() const
{
    return true; // GUI Editor is now fully implemented!
}

juce::AudioProcessorEditor* CP30AudioProcessor::createEditor()
{
    return new CP30AudioProcessorEditor (*this);
}

//==============================================================================
void CP30AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void CP30AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CP30AudioProcessor();
}
