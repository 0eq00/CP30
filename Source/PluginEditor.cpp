/*
  ==============================================================================

    PluginEditor.cpp
    Yamaha CP-30 Electronic Piano Graphical User Interface (GUI)
    Faithfully reproduces the front panel controls and vintage aesthetics.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CP30LookAndFeel::CP30LookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffe6a15c));
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff2b2b2b));
}

void CP30LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPosProportional, float rotaryStartAngle,
                                       float rotaryEndAngle, juce::Slider& slider)
{
    auto radius = (float) juce::jmin (width / 2, height / 2) - 4.0f;
    auto centreX = (float) x + (float) width  * 0.5f;
    auto centreY = (float) y + (float) height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Knob outer shadow
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillEllipse (rx + 1.0f, ry + 2.0f, rw, rw);

    // Knob metal bezel (Vintage fluted aluminum / silver skirt)
    juce::ColourGradient skirtGrad (juce::Colour (0xff686b6e), rx, ry,
                                    juce::Colour (0xff1e2022), rx + rw, ry + rw, false);
    g.setGradientFill (skirtGrad);
    g.fillEllipse (rx, ry, rw, rw);

    // Inner knob body (matte black)
    auto innerRadius = radius * 0.76f;
    auto inRx = centreX - innerRadius;
    auto inRy = centreY - innerRadius;
    auto inRw = innerRadius * 2.0f;

    juce::ColourGradient bodyGrad (juce::Colour (0xff323538), inRx, inRy,
                                   juce::Colour (0xff121314), inRx, inRy + inRw, false);
    g.setGradientFill (bodyGrad);
    g.fillEllipse (inRx, inRy, inRw, inRw);

    // Brushed metal top cap
    auto capRadius = innerRadius * 0.60f;
    g.setColour (juce::Colour (0xffc2c7cc));
    g.fillEllipse (centreX - capRadius, centreY - capRadius, capRadius * 2.0f, capRadius * 2.0f);

    // Pointer line (clean vintage silver needle)
    juce::Path p;
    auto pointerLength = innerRadius * 0.95f;
    auto pointerThickness = 2.4f;
    p.addRoundedRectangle (-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength, 1.0f);
    p.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
    g.setColour (juce::Colour (0xffffffff));
    g.fillPath (p);
}

void CP30LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                       bool isHighlighted, bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (2.0f);
    const bool isOn = button.getToggleState();

    // Yamaha CP-30 Rocker Tablet Switch (Seesaw design)
    // Dark housing bezel
    g.setColour (juce::Colour (0xff161718));
    g.fillRoundedRectangle (bounds, 3.0f);

    auto rockerBounds = bounds.reduced (3.0f);

    // When ON: Tablet is pressed downwards at the bottom, top lifts with white indicator
    if (isOn)
    {
        // Engaged (Pressed down): Subtle glowing vintage cream-white body
        juce::ColourGradient tabGrad (juce::Colour (0xffe8ebee), rockerBounds.getX(), rockerBounds.getY(),
                                      juce::Colour (0xffbcc2c8), rockerBounds.getX(), rockerBounds.getBottom(), false);
        g.setGradientFill (tabGrad);
        g.fillRoundedRectangle (rockerBounds, 2.0f);

        // Accent indicator stripe (Vintage orange-red / active line)
        g.setColour (juce::Colour (0xffe65100));
        g.fillRect (rockerBounds.getX(), rockerBounds.getBottom() - 4.0f, rockerBounds.getWidth(), 3.0f);

        // Label text (Dark contrast on cream-white pressed rocker)
        g.setColour (juce::Colour (0xff1a1c1e));
    }
    else
    {
        // Disengaged (OFF): Dark satin rocker switch
        juce::ColourGradient tabGrad (juce::Colour (0xff3a3d42), rockerBounds.getX(), rockerBounds.getY(),
                                      juce::Colour (0xff222427), rockerBounds.getX(), rockerBounds.getBottom(), false);
        g.setGradientFill (tabGrad);
        g.fillRoundedRectangle (rockerBounds, 2.0f);

        // Subtle top bevel reflection
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.fillRect (rockerBounds.getX(), rockerBounds.getY(), rockerBounds.getWidth(), 2.0f);

        // Label text (Crisp silver-white on dark rocker)
        g.setColour (juce::Colour (0xffadb5bd));
    }

    g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
    g.drawFittedText (button.getButtonText(), rockerBounds.toNearestInt(), juce::Justification::centred, 2);
}

//==============================================================================
CP30AudioProcessorEditor::CP30AudioProcessorEditor (CP30AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&cp30LookAndFeel);

    // Helper lambda to setup rocker buttons
    auto setupTablet = [this] (juce::ToggleButton& b, const juce::String& text)
    {
        b.setButtonText (text);
        b.setClickingTogglesState (true);
        addAndMakeVisible (b);
    };

    // Power switch
    powerButton.setButtonText ("POWER");
    powerButton.setClickingTogglesState (true);
    addAndMakeVisible (powerButton);
    powerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_POWER, powerButton);

    // Sound I Tablets (Piano 1, 2, 3, Harpsichord)
    setupTablet (toneIPiano1, "PIANO 1");
    setupTablet (toneIPiano2, "PIANO 2");
    setupTablet (toneIPiano3, "PIANO 3");
    setupTablet (toneIHarpsi, "HARPSI-CHORD");

    toneIPiano1Att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_TONE_I_PIANO1, toneIPiano1);
    toneIPiano2Att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_TONE_I_PIANO2, toneIPiano2);
    toneIPiano3Att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_TONE_I_PIANO3, toneIPiano3);
    toneIHarpsiAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_TONE_I_HARPSI, toneIHarpsi);

    // Sound II Tablets (Piano 1, 2, 3, Harpsichord)
    setupTablet (toneIIPiano1, "PIANO 1");
    setupTablet (toneIIPiano2, "PIANO 2");
    setupTablet (toneIIPiano3, "PIANO 3");
    setupTablet (toneIIHarpsi, "HARPSI-CHORD");

    toneIIPiano1Att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_TONE_II_PIANO1, toneIIPiano1);
    toneIIPiano2Att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_TONE_II_PIANO2, toneIIPiano2);
    toneIIPiano3Att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_TONE_II_PIANO3, toneIIPiano3);
    toneIIHarpsiAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_TONE_II_HARPSI, toneIIHarpsi);

    // Tremolo Section Tablets
    setupTablet (tremoloI, "TREMOLO I");
    setupTablet (tremoloII, "TREMOLO II");
    tremoloIAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_TREMOLO_I, tremoloI);
    tremoloIIAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_TREMOLO_II, tremoloII);

    // Rotary Sliders
    setupSlider (pitchI, "PITCH I");
    setupSlider (pitchII, "PITCH II");
    setupSlider (decayI, "DECAY I");
    setupSlider (decayII, "DECAY II");
    setupSlider (tremoloSpeed, "SPEED", " Hz");
    setupSlider (tremoloIntensity, "INTENSITY");
    setupSlider (bassSlider, "BASS", " dB");
    setupSlider (trebleSlider, "TREBLE", " dB");
    setupSlider (balanceSlider, "BALANCE");
    setupSlider (volumeSlider, "VOLUME");

    pitchIAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_PITCH_I, pitchI);
    pitchIIAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_PITCH_II, pitchII);
    decayIAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_DECAY_I, decayI);
    decayIIAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_DECAY_II, decayII);

    tremoloSpeedAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_TREMOLO_SPEED, tremoloSpeed);
    tremoloIntensityAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_TREMOLO_INTENSITY, tremoloIntensity);

    bassAtt    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_BASS, bassSlider);
    trebleAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_TREBLE, trebleSlider);
    balanceAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_BALANCE, balanceSlider);
    volumeAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), CP30AudioProcessor::ID_VOLUME, volumeSlider);

    // Panel Size (Yamaha CP-30 proportions)
    setSize (980, 360);
}

CP30AudioProcessorEditor::~CP30AudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void CP30AudioProcessorEditor::setupSlider (juce::Slider& s, const juce::String& label, const juce::String& suffix)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setTextValueSuffix (suffix);
    addAndMakeVisible (s);
}

//==============================================================================
void CP30AudioProcessorEditor::paint (juce::Graphics& g)
{
    // Outer Vintage Rosewood Cabinet (Manual p.14: ピックボトルローズダップ合板)
    juce::ColourGradient woodGrad (juce::Colour (0xff3d2314), 0, 0,
                                   juce::Colour (0xff1f1008), 0, (float) getHeight(), false);
    g.setGradientFill (woodGrad);
    g.fillAll();

    // Wood Grain lines
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    for (int y = 0; y < getHeight(); y += 8)
        g.drawHorizontalLine (y, 0.0f, (float) getWidth());

    // Inner Control Panel (Brushed Anodized Black Aluminum)
    auto panelArea = getLocalBounds().reduced (16, 14);
    juce::ColourGradient panelGrad (juce::Colour (0xff242629), (float) panelArea.getX(), (float) panelArea.getY(),
                                    juce::Colour (0xff141517), (float) panelArea.getX(), (float) panelArea.getBottom(), false);
    g.setGradientFill (panelGrad);
    g.fillRoundedRectangle (panelArea.toFloat(), 6.0f);

    // Bezel border
    g.setColour (juce::Colour (0xff495057));
    g.drawRoundedRectangle (panelArea.toFloat(), 6.0f, 1.5f);

    // Yamaha CP-30 Brand Badge (Top Center-Left)
    g.setColour (juce::Colour (0xffffffff));
    g.setFont (juce::FontOptions (18.0f).withStyle ("Bold"));
    g.drawText ("CP-30", 38, 22, 120, 22, juce::Justification::left);

    g.setFont (juce::FontOptions (12.0f).withStyle ("Bold"));
    g.setColour (juce::Colour (0xffadb5bd));
    g.drawText ("ELECTRONIC PIANO", 38, 44, 240, 18, juce::Justification::left);

    // Section Dividers and Sub-labels
    g.setColour (juce::Colour (0xff343a40));
    g.drawVerticalLine (300, 75.0f, 310.0f);
    g.drawVerticalLine (660, 75.0f, 310.0f);

    // Section Header Titles (White silkscreen style)
    g.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    g.setColour (juce::Colour (0xffe9ecef));

    // Section 1 Header: PITCH / DECAY / TREMOLO
    g.drawText ("TUNING & TREMOLO", 40, 78, 240, 16, juce::Justification::centred);

    // Section 2 Header: TONE SELECTORS (SOUND I & SOUND II)
    g.drawText ("TONE SELECTOR (SOUND I)", 320, 78, 150, 16, juce::Justification::centred);
    g.drawText ("TONE SELECTOR (SOUND II)", 490, 78, 150, 16, juce::Justification::centred);

    // Section 3 Header: TONE & MASTER
    g.drawText ("TONE CONTROL & OUTPUT", 680, 78, 260, 16, juce::Justification::centred);

    // Draw Knob Labels
    auto drawLabel = [&g] (const juce::String& text, int x, int y, int w)
    {
        g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
        g.setColour (juce::Colour (0xffced4da));
        g.drawText (text, x, y, w, 14, juce::Justification::centred);
    };

    // Tuning & Decay Labels
    drawLabel ("PITCH I",   35,  172, 55);
    drawLabel ("PITCH II",  95,  172, 55);
    drawLabel ("DECAY I",   155, 172, 55);
    drawLabel ("DECAY II",  215, 172, 55);

    // Tremolo Labels
    drawLabel ("SPEED",     65,  278, 55);
    drawLabel ("INTENSITY", 125, 278, 55);

    // Tone & Master Labels
    drawLabel ("BASS",    680, 172, 55);
    drawLabel ("TREBLE",  740, 172, 55);
    drawLabel ("BALANCE", 800, 172, 55);
    drawLabel ("VOLUME",  860, 172, 55);

    // Power Pilot Lamp (Vintage Red Jewel LED)
    const bool isPowerOn = powerButton.getToggleState();
    const float lampX = 920.0f;
    const float lampY = 32.0f;
    const float lampR = 7.0f;

    // Outer metal bezel ring
    g.setColour (juce::Colour (0xff868e96));
    g.drawEllipse (lampX - lampR - 2.0f, lampY - lampR - 2.0f, (lampR + 2.0f) * 2.0f, (lampR + 2.0f) * 2.0f, 2.0f);

    if (isPowerOn)
    {
        // Glowing red pilot lamp
        g.setColour (juce::Colour (0x66ff0000));
        g.fillEllipse (lampX - lampR - 6.0f, lampY - lampR - 6.0f, (lampR + 6.0f) * 2.0f, (lampR + 6.0f) * 2.0f);

        juce::ColourGradient ledGrad (juce::Colour (0xffff6b6b), lampX - 2.0f, lampY - 2.0f,
                                      juce::Colour (0xffc92a2a), lampX + lampR, lampY + lampR, false);
        g.setGradientFill (ledGrad);
        g.fillEllipse (lampX - lampR, lampY - lampR, lampR * 2.0f, lampR * 2.0f);
    }
    else
    {
        // Off (Dark ruby glass)
        g.setColour (juce::Colour (0xff491212));
        g.fillEllipse (lampX - lampR, lampY - lampR, lampR * 2.0f, lampR * 2.0f);
    }
}

void CP30AudioProcessorEditor::resized()
{
    // Tuning & Decay row
    pitchI.setBounds  (35,  110, 55, 55);
    pitchII.setBounds (95,  110, 55, 55);
    decayI.setBounds  (155, 110, 55, 55);
    decayII.setBounds (215, 110, 55, 55);

    // Tremolo row
    tremoloSpeed.setBounds     (65,  215, 55, 55);
    tremoloIntensity.setBounds (125, 215, 55, 55);
    tremoloI.setBounds         (190, 215, 42, 60);
    tremoloII.setBounds        (236, 215, 42, 60);

    // Sound I Tablet switches (Piano 1, 2, 3, Harpsichord)
    const int tabWidth  = 38;
    const int tabHeight = 110;
    const int tabY      = 120;

    toneIPiano1.setBounds (320, tabY, tabWidth, tabHeight);
    toneIPiano2.setBounds (360, tabY, tabWidth, tabHeight);
    toneIPiano3.setBounds (400, tabY, tabWidth, tabHeight);
    toneIHarpsi.setBounds (440, tabY, tabWidth, tabHeight);

    // Sound II Tablet switches (Piano 1, 2, 3, Harpsichord)
    toneIIPiano1.setBounds (490, tabY, tabWidth, tabHeight);
    toneIIPiano2.setBounds (530, tabY, tabWidth, tabHeight);
    toneIIPiano3.setBounds (570, tabY, tabWidth, tabHeight);
    toneIIHarpsi.setBounds (610, tabY, tabWidth, tabHeight);

    // Tone & Master controls
    bassSlider.setBounds    (680, 110, 55, 55);
    trebleSlider.setBounds  (740, 110, 55, 55);
    balanceSlider.setBounds (800, 110, 55, 55);
    volumeSlider.setBounds  (860, 110, 55, 55);

    // Power Toggle Switch (Top right)
    powerButton.setBounds (810, 22, 90, 28);
}
