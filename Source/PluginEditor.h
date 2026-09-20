/*
  ==============================================================================

    PluginEditor.h
    Yamaha CP-30 Electronic Piano Graphical User Interface (GUI)
    Faithfully reproduces the front panel controls and vintage aesthetics:
    - Sound I Tablets (Piano 1, 2, 3, Harpsichord)
    - Sound II Tablets (Piano 1, 2, 3, Harpsichord)
    - Tremolo Section (I/II switches, Speed, Intensity)
    - Pitch I/II & Decay I/II Knobs
    - Tone (Bass, Treble), Balance, Master Volume
    - Power Switch & Red Illuminated Pilot Lamp

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Custom LookAndFeel for authentic Yamaha CP-30 vintage rocker switches & knobs
class CP30LookAndFeel : public juce::LookAndFeel_V4
{
public:
    CP30LookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;
};

//==============================================================================
class CP30AudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    CP30AudioProcessorEditor (CP30AudioProcessor&);
    ~CP30AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    CP30AudioProcessor& audioProcessor;
    CP30LookAndFeel cp30LookAndFeel;

    // Power Section
    juce::ToggleButton powerButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttachment;

    // Sound I Tablets
    juce::ToggleButton toneIPiano1;
    juce::ToggleButton toneIPiano2;
    juce::ToggleButton toneIPiano3;
    juce::ToggleButton toneIHarpsi;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toneIPiano1Att;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toneIPiano2Att;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toneIPiano3Att;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toneIHarpsiAtt;

    // Sound II Tablets
    juce::ToggleButton toneIIPiano1;
    juce::ToggleButton toneIIPiano2;
    juce::ToggleButton toneIIPiano3;
    juce::ToggleButton toneIIHarpsi;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toneIIPiano1Att;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toneIIPiano2Att;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toneIIPiano3Att;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toneIIHarpsiAtt;

    // Tremolo Section
    juce::ToggleButton tremoloI;
    juce::ToggleButton tremoloII;
    juce::Slider tremoloSpeed;
    juce::Slider tremoloIntensity;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tremoloIAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tremoloIIAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tremoloSpeedAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tremoloIntensityAtt;

    // Tuning & Decay
    juce::Slider pitchI, pitchII;
    juce::Slider decayI, decayII;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchIAtt, pitchIIAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayIAtt, decayIIAtt;

    // Tone & Master
    juce::Slider bassSlider, trebleSlider;
    juce::Slider balanceSlider, volumeSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bassAtt, trebleAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> balanceAtt, volumeAtt;

    // Helper for configuring sliders
    void setupSlider (juce::Slider& slider, const juce::String& labelText, const juce::String& suffix = "");

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CP30AudioProcessorEditor)
};
