#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ArmNongMuangLA2AAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit ArmNongMuangLA2AAudioProcessorEditor(ArmNongMuangLA2AAudioProcessor&);
    ~ArmNongMuangLA2AAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    class InvisibleLookAndFeel final : public juce::LookAndFeel_V4 {
    public:
        void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override {}
        void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override {}
    };
    class SwitchLookAndFeel final : public juce::LookAndFeel_V4 {
    public:
        void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
        void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override {}
    };
    class PointerLookAndFeel final : public juce::LookAndFeel_V4 {
    public:
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    };
    void timerCallback() override;
    void setupRotary(juce::Slider&, const juce::String&);
    void addHitButton(juce::TextButton&, const juce::String&);
    void loadModelDialog();
    void setPower(bool);
    void setLimit(bool);
    void cycleMeter();
    void saveSnapshot(int slot);
    void restoreSnapshot(int slot);
    ArmNongMuangLA2AAudioProcessor& processor;
    juce::Image background;
    InvisibleLookAndFeel invisibleLookAndFeel;
    SwitchLookAndFeel switchLookAndFeel;
    PointerLookAndFeel pointerLookAndFeel;
    juce::Slider gain, peak, attack, release;
    juce::ToggleButton powerState, limitState;
    juce::TextButton topPower, preset, modelPicker, buttonA, buttonB, copyAB, settings, meterSwitch, bottomPower, limitSwitch;
    juce::Label status;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment, peakAttachment, attackAttachment, releaseAttachment;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::ValueTree snapshotA, snapshotB;
    bool powerOn { true }, limitOn { false };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArmNongMuangLA2AAudioProcessorEditor)
};
