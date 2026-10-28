#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>

ArmNongMuangLA2AAudioProcessorEditor::ArmNongMuangLA2AAudioProcessorEditor(ArmNongMuangLA2AAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p) {
    background = juce::ImageCache::getFromMemory(BinaryData::LA2A_Interface_png, BinaryData::LA2A_Interface_pngSize);
    setSize(1180, 608);
    setResizable(true, true);
    setResizeLimits(886, 456, 1476, 760);
    getConstrainer()->setFixedAspectRatio(1476.0 / 760.0);

    setupRotary(gain, "GAIN"); setupRotary(peak, "PEAK REDUCTION");
    setupRotary(attack, "ATTACK"); setupRotary(release, "RELEASE");
    addAndMakeVisible(gain); addAndMakeVisible(peak);
    // Attack/release remain automatable parameters but are not drawn on this LA-2A faceplate.
    attack.setVisible(false); release.setVisible(false);
    gain.setRange(0, 100, 0.1); peak.setRange(0, 100, 0.1); attack.setRange(0.1, 10, 0.01); release.setRange(50, 5000, 1);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, "gain", gain);
    peakAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, "peak", peak);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, "attack", attack);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, "release", release);

    addHitButton(topPower, "top power"); topPower.setTooltip("Power: ON / OFF"); topPower.onClick = [this] { setPower(!powerOn); };
    addHitButton(preset, "preset menu"); preset.onClick = [this] {
        juce::PopupMenu menu; menu.addItem(1, "Default"); menu.addItem(2, "Save current settings to A"); menu.addItem(3, "Save current settings to B");
        menu.showMenuAsync(juce::PopupMenu::Options(), [this](int id) { if (id == 1) { processor.parameters.getParameter("gain")->setValueNotifyingHost(0.5f); processor.parameters.getParameter("peak")->setValueNotifyingHost(0.35f); } else if (id == 2) { saveSnapshot(0); status.setText("Current settings saved to A", juce::dontSendNotification); } else if (id == 3) { saveSnapshot(1); status.setText("Current settings saved to B", juce::dontSendNotification); } });
    };
    addHitButton(modelPicker, "load NAM model"); modelPicker.onClick = [this] { loadModelDialog(); };
    addHitButton(buttonA, "snapshot A"); buttonA.onClick = [this] { restoreSnapshot(0); };
    addHitButton(buttonB, "snapshot B"); buttonB.onClick = [this] { restoreSnapshot(1); };
    addHitButton(copyAB, "copy A to B"); copyAB.onClick = [this] { saveSnapshot(0); saveSnapshot(1); status.setText("Snapshot A copied to B", juce::dontSendNotification); };
    addHitButton(settings, "settings"); settings.onClick = [this] {
        juce::PopupMenu menu; menu.addItem(1, "Reset controls"); menu.addItem(2, "Unload NAM model");
        menu.showMenuAsync(juce::PopupMenu::Options(), [this](int id) {
            if (id == 1) { processor.parameters.getParameter("gain")->setValueNotifyingHost(0.5f); processor.parameters.getParameter("peak")->setValueNotifyingHost(0.35f); }
            if (id == 2) { processor.unloadNamFile(); status.setText(processor.getNamStatus(), juce::dontSendNotification); repaint(); }
        });
    };
    addHitButton(meterSwitch, "meter mode");
    meterSwitch.setLookAndFeel(&switchLookAndFeel);
    meterSwitch.setClickingTogglesState(true);
    meterSwitch.setTooltip("Pull UP: OUTPUT | Pull DOWN: GAIN REDUCTION");
    meterSwitch.onClick = [this] { cycleMeter(); };
    addHitButton(bottomPower, "bottom power switch");
    bottomPower.setLookAndFeel(&switchLookAndFeel);
    bottomPower.setClickingTogglesState(true);
    bottomPower.setTooltip("Pull UP: ON | Pull DOWN: OFF");
    bottomPower.onClick = [this] { setPower(!powerOn); bottomPower.setToggleState(powerOn, juce::dontSendNotification); };
    addHitButton(limitSwitch, "limit compress switch");
    limitSwitch.setLookAndFeel(&switchLookAndFeel);
    limitSwitch.setClickingTogglesState(true);
    limitSwitch.setTooltip("Pull UP: LIMIT | Pull DOWN: COMPRESS");
    limitSwitch.onClick = [this] { setLimit(!limitOn); limitSwitch.setToggleState(limitOn, juce::dontSendNotification); };

    addAndMakeVisible(status); status.setColour(juce::Label::textColourId, juce::Colours::black);
    status.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    status.setJustificationType(juce::Justification::centred);
    status.setText(processor.getNamStatus(), juce::dontSendNotification);
    powerOn = processor.parameters.getRawParameterValue("power")->load() >= 0.5f;
    limitOn = processor.parameters.getRawParameterValue("limit")->load() >= 0.5f;
    bottomPower.setToggleState(powerOn, juce::dontSendNotification);
    limitSwitch.setToggleState(limitOn, juce::dontSendNotification);
    meterSwitch.setToggleState(processor.getMeterMode() == 1, juce::dontSendNotification);
    saveSnapshot(0); saveSnapshot(1);
    startTimerHz(30);
}

ArmNongMuangLA2AAudioProcessorEditor::~ArmNongMuangLA2AAudioProcessorEditor() {
    for (auto* b : { &topPower, &preset, &modelPicker, &buttonA, &buttonB, &copyAB, &settings, &meterSwitch, &bottomPower, &limitSwitch }) b->setLookAndFeel(nullptr);
    for (auto* s : { &gain, &peak, &attack, &release }) s->setLookAndFeel(nullptr);
}

void ArmNongMuangLA2AAudioProcessorEditor::setupRotary(juce::Slider& slider, const juce::String& name) {
    slider.setName(name); slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setLookAndFeel(&pointerLookAndFeel);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
}
void ArmNongMuangLA2AAudioProcessorEditor::addHitButton(juce::TextButton& b, const juce::String& name) {
    b.setName(name); b.setButtonText(juce::String{}); b.setLookAndFeel(&invisibleLookAndFeel);
    b.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    b.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    b.setColour(juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
    b.setColour(juce::TextButton::textColourOnId, juce::Colours::transparentBlack);
    addAndMakeVisible(b);
}
void ArmNongMuangLA2AAudioProcessorEditor::SwitchLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hovered, bool down) {
    // Lever artwork follows the chrome/black-socket style of the three original
    // lever switches in Assets/LA2A_Interface.png. The PNG remains the faceplate;
    // this overlay replaces only the switch region so its lever can move.
    const auto r = b.getLocalBounds().toFloat();
    const bool up = b.getToggleState(); // true = UP, false = DOWN
    const bool isMeter = b.getName() == "meter mode";
    const bool isPower = b.getName() == "bottom power switch";

    // Match the faceplate colour and preserve a compact switch footprint.
    g.setColour(juce::Colour(0xff171717));
    g.fillRect(r);

    const float cx = r.getCentreX();
    const float cy = r.getCentreY();
    const float bezelW = juce::jmin(r.getWidth() * 0.72f, r.getHeight() * 0.82f);
    const float bezelH = juce::jmin(r.getHeight() * 0.67f, r.getWidth() * 1.08f);
    auto bezel = juce::Rectangle<float>(cx - bezelW * 0.5f, cy - bezelH * 0.5f, bezelW, bezelH);

    // Chrome rim and dark recessed socket, like the supplied PNG switch hardware.
    g.setColour(juce::Colour(0xff777a7b));
    g.fillEllipse(bezel.expanded(1.0f));
    g.setColour(juce::Colour(0xffe9e9e5));
    g.fillEllipse(bezel.reduced(1.0f));
    g.setColour(juce::Colour(0xff777a7b));
    g.drawEllipse(bezel.reduced(1.0f), 0.8f);
    auto socket = bezel.reduced(bezelW * 0.17f, bezelH * 0.12f);
    g.setColour(juce::Colour(0xff161819));
    g.fillEllipse(socket);
    g.setColour(juce::Colour(0xff55595a));
    g.drawEllipse(socket, 1.0f);

    const float leverLen = socket.getHeight() * 0.39f;
    const float tilt = up ? -0.22f : 0.22f;
    const juce::Point<float> pivot(cx, cy + socket.getHeight() * 0.10f);
    const juce::Point<float> tip(pivot.x + std::sin(tilt) * leverLen,
        pivot.y + (up ? -1.0f : 1.0f) * std::cos(tilt) * leverLen);

    // Draw a metallic shaft with dark edge and bright chrome highlight.
    const float shaft = juce::jmax(2.2f, bezelW * 0.095f);
    g.setColour(juce::Colour(0xff080909));
    g.drawLine(pivot.x + 1.0f, pivot.y + 1.0f, tip.x + 1.0f, tip.y + 1.0f, shaft + 1.6f);
    g.setColour(juce::Colour(0xff686d6f));
    g.drawLine(pivot.x, pivot.y, tip.x, tip.y, shaft + 1.0f);
    g.setColour(juce::Colour(0xffdfe2e2));
    g.drawLine(pivot.x - shaft * 0.12f, pivot.y, tip.x - shaft * 0.12f, tip.y, shaft * 0.42f);

    const float ballD = juce::jmax(5.0f, bezelW * 0.30f);
    auto ball = juce::Rectangle<float>(tip.x - ballD * 0.5f, tip.y - ballD * 0.5f, ballD, ballD);
    g.setColour(juce::Colour(0xff373b3c));
    g.fillEllipse(ball.expanded(0.7f));
    g.setColour(juce::Colour(0xffc9cdce));
    g.fillEllipse(ball.reduced(0.7f));
    g.setColour(juce::Colour(0xfff8f8f6).withAlpha(0.95f));
    g.fillEllipse(ball.withSizeKeepingCentre(ballD * 0.27f, ballD * 0.27f)
        .translated(-ballD * 0.12f, -ballD * 0.14f));
    g.setColour(juce::Colour(0xff252829));
    g.fillEllipse(pivot.x - shaft * 0.55f, pivot.y - shaft * 0.55f, shaft * 1.1f, shaft * 1.1f);

    juce::String upper, lower;
    if (isMeter) { upper = "OUTPUT"; lower = "GAIN REDUCTION"; }
    else if (isPower) { upper = "ON"; lower = "OFF"; }
    else { upper = "LIMIT"; lower = "COMPRESS"; }
    const float labelSize = juce::jlimit(6.5f, 9.0f, r.getWidth() * 0.115f);
    g.setFont(juce::Font(labelSize, juce::Font::bold));
    g.setColour(juce::Colour(0xff202020));
    g.drawFittedText(upper, juce::Rectangle<int>(juce::roundToInt(r.getX()), juce::roundToInt(r.getY()),
        juce::roundToInt(r.getWidth()), juce::jmax(1, juce::roundToInt(bezel.getY() - r.getY()))), juce::Justification::centred, 1);
    g.drawFittedText(lower, juce::Rectangle<int>(juce::roundToInt(r.getX()), juce::roundToInt(bezel.getBottom()),
        juce::roundToInt(r.getWidth()), juce::jmax(1, juce::roundToInt(r.getBottom() - bezel.getBottom()))), juce::Justification::centred, 1);

    if (hovered || down) {
        g.setColour(juce::Colours::white.withAlpha(hovered ? 0.08f : 0.14f));
        g.fillEllipse(bezel.reduced(2.0f));
    }
}

void ArmNongMuangLA2AAudioProcessorEditor::PointerLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) {
    juce::ignoreUnused(start, end);
    const float cx = x + w * 0.5f, cy = y + h * 0.5f;
    const float angle = juce::MathConstants<float>::pi * (2.0f / 3.0f + (5.0f / 3.0f) * pos);
    const float radius = juce::jmin(w, h) * 0.31f;
    const float ex = cx + std::cos(angle) * radius, ey = cy + std::sin(angle) * radius;
    g.setColour(juce::Colours::black.withAlpha(0.85f)); g.drawLine(cx + 1.2f, cy + 1.2f, ex + 1.2f, ey + 1.2f, 5.0f);
    g.setColour(juce::Colours::white); g.drawLine(cx, cy, ex, ey, 2.4f);
    g.setColour(juce::Colours::black); g.fillEllipse(cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
}

void ArmNongMuangLA2AAudioProcessorEditor::setPower(bool on) {
    powerOn = on; processor.parameters.getParameter("power")->setValueNotifyingHost(on ? 1.0f : 0.0f); repaint();
}
void ArmNongMuangLA2AAudioProcessorEditor::setLimit(bool on) {
    limitOn = on; processor.parameters.getParameter("limit")->setValueNotifyingHost(on ? 1.0f : 0.0f); repaint();
}
void ArmNongMuangLA2AAudioProcessorEditor::cycleMeter() {
    // Two-position LA-2A meter switch: DOWN = Gain Reduction, UP = Output.
    // AudioParameterChoice stores choice indices as normalized values: index / (numChoices - 1).
    const float normalized = processor.parameters.getParameter("meter")->getValue();
    const int currentIndex = juce::jlimit(0, 2, juce::roundToInt(normalized * 2.0f));
    const int nextIndex = (currentIndex == 1) ? 2 : 1;
    processor.parameters.getParameter("meter")->setValueNotifyingHost(static_cast<float>(nextIndex) / 2.0f);
    meterSwitch.setToggleState(nextIndex == 1, juce::dontSendNotification);
    repaint();
}
void ArmNongMuangLA2AAudioProcessorEditor::loadModelDialog() {
    chooser = std::make_unique<juce::FileChooser>("Choose NAM model", juce::File{}, "*.nam");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) {
            const auto file = fc.getResult(); if (!file.existsAsFile()) return;
            juce::String message;
            processor.loadNamFile(file, message);
            status.setText(message, juce::dontSendNotification); repaint();
        });
}
void ArmNongMuangLA2AAudioProcessorEditor::saveSnapshot(int slot) {
    auto state = processor.parameters.copyState();
    if (slot == 0) snapshotA = state.createCopy(); else snapshotB = state.createCopy();
}
void ArmNongMuangLA2AAudioProcessorEditor::restoreSnapshot(int slot) {
    const auto& state = (slot == 0 ? snapshotA : snapshotB);
    if (state.isValid()) processor.parameters.replaceState(state.createCopy());
    powerOn = processor.parameters.getRawParameterValue("power")->load() >= 0.5f;
    limitOn = processor.parameters.getRawParameterValue("limit")->load() >= 0.5f;
    bottomPower.setToggleState(powerOn, juce::dontSendNotification);
    limitSwitch.setToggleState(limitOn, juce::dontSendNotification);
    meterSwitch.setToggleState(processor.getMeterMode() == 1, juce::dontSendNotification);
}
void ArmNongMuangLA2AAudioProcessorEditor::timerCallback() {
    powerOn = processor.parameters.getRawParameterValue("power")->load() >= 0.5f;
    limitOn = processor.parameters.getRawParameterValue("limit")->load() >= 0.5f;
    bottomPower.setToggleState(powerOn, juce::dontSendNotification);
    limitSwitch.setToggleState(limitOn, juce::dontSendNotification);
    meterSwitch.setToggleState(processor.getMeterMode() == 1, juce::dontSendNotification);
    repaint();
}

void ArmNongMuangLA2AAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::white);
    if (background.isValid()) g.drawImage(background, getLocalBounds().toFloat());
    const float sx = getWidth() / 1476.0f, sy = getHeight() / 760.0f;
    // Actual moving VU needle: scale is calibrated to the -20 ... +3 markings in the supplied PNG.
    const float db = juce::jlimit(-20.0f, 3.0f, processor.getVuValueDb());
    const float norm = (db + 20.0f) / 23.0f;
    const float angle = -2.74f + norm * 2.21f;
    const float cx = (770.0f - 30.0f) * sx, cy = (430.0f - 154.0f) * sy, radius = 142.0f * sx;
    const float ex = cx + std::cos(angle) * radius, ey = cy + std::sin(angle) * radius;
    g.setColour(juce::Colours::black.withAlpha(0.65f)); g.drawLine(cx + 1.2f*sx, cy + 1.2f*sy, ex + 1.2f*sx, ey + 1.2f*sy, 5.0f*sx);
    g.setColour(juce::Colour(0xff17120b)); g.drawLine(cx, cy, ex, ey, 3.0f*sx);
    g.setColour(juce::Colour(0xffb8a16f)); g.fillEllipse(cx - 4.5f*sx, cy - 4.5f*sy, 9.0f*sx, 9.0f*sy);
    // A live status line sits in the white margin below the faceplate and does not alter the PNG artwork.
    g.setColour(juce::Colours::black.withAlpha(0.85f));
    juce::String meterLabel = processor.getMeterMode() == 0 ? "INPUT" : (processor.getMeterMode() == 1 ? "OUTPUT" : "GAIN REDUCTION");
    const juce::String live = processor.isNamLoaded() ? ("NAM: " + processor.getLoadedNamName()) : "BUILT-IN COMPRESSOR";
    g.setFont(juce::Font(12.0f * sx, juce::Font::bold));
    g.drawFittedText(live + "  |  METER: " + meterLabel + "  |  GR: " + juce::String(processor.getGainReductionDb(), 1) + " dB",
        juce::Rectangle<int>(static_cast<int>(575*sx), static_cast<int>(430*sy), static_cast<int>(330*sx), static_cast<int>(18*sy)), juce::Justification::centred, 1);
    // Power indicator over the lamp printed in the reference PNG.
    g.setColour(powerOn ? juce::Colour(0xffffb52e) : juce::Colour(0xff777777));
    g.fillEllipse((183-30)*sx, (219-154)*sy, 18*sx, 18*sy);
    g.setColour(juce::Colours::white.withAlpha(powerOn ? 0.75f : 0.25f));
    g.fillEllipse((187-30)*sx, (222-154)*sy, 6*sx, 6*sy);
}

void ArmNongMuangLA2AAudioProcessorEditor::resized() {
    const float sx = getWidth() / 1476.0f, sy = getHeight() / 760.0f;
    auto place = [&](juce::Component& c, float x, float y, float w, float h) {
        // Layout coordinates refer to the original 1536x1024 generated artwork.
        // Offset by the tight crop so the outer checkerboard/white margin is removed.
        c.setBounds(juce::roundToInt((x - 30.0f)*sx), juce::roundToInt((y - 154.0f)*sy), juce::roundToInt(w*sx), juce::roundToInt(h*sy));
    };
    // All hit zones use the new 1536x1024 PNG as the single coordinate reference.
    // Main rotary controls are centered on the printed GAIN and PEAK REDUCTION knobs.
    place(gain, 228, 392, 164, 164);       // center approx (310,474)
    place(peak, 1140, 392, 164, 164);      // center approx (1222,474)
    // These parameters are available to automation/state but are not visible on this artwork.
    place(attack, 0, 0, 1, 1);
    place(release, 0, 0, 1, 1);
    // Top bar controls aligned to the new PNG artwork.
    place(topPower, 83, 205, 118, 70);
    place(preset, 253, 207, 302, 51);
    place(modelPicker, 568, 207, 332, 52);
    place(buttonA, 964, 207, 55, 60);
    place(buttonB, 1032, 207, 55, 60);
    place(copyAB, 1107, 207, 76, 53);
    place(settings, 1228, 207, 63, 53);
    // Bottom row lever switches: down=Compress/Off/GR, up=Limit/On/Output.
    place(limitSwitch, 278, 777, 80, 100);
    place(bottomPower, 729, 777, 80, 100);
    place(meterSwitch, 1180, 777, 80, 100);
    place(status, 570, 690, 330, 18);
}
