#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "GrinderDsp.h"

//==============================================================================
//  Grinder (NINE HANDS | DSP) : compresseur « classic knee » à 8 ratios,
//  détecteur et modes audio commutables, distorsion harmonique 2 / 3, options Brit et Image Link
//==============================================================================
class GrinderProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    GrinderProcessor();
    ~GrinderProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return juce::jmax (0, currentPreset.load()); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //------------------------------------------------------------------ paramètres
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static const juce::StringArray& ratioNames();
    static const juce::StringArray& hrNames();

    std::atomic<float> *pPower = nullptr, *pBypass = nullptr, *pInput = nullptr, *pAttack = nullptr, *pRelease = nullptr,
                       *pOutput = nullptr, *pMix = nullptr, *pHr = nullptr, *pRatio = nullptr, *pDetHp = nullptr, *pDetBand = nullptr,
                       *pLink = nullptr, *pAudioHp = nullptr, *pDist = nullptr, *pBrit = nullptr, *pImage = nullptr, *pOs = nullptr;

    //------------------------------------------------------------------ mesures (thread audio -> interface)
    std::atomic<float> grDb { 0.0f };          // réduction de gain (dB, la plus forte des deux canaux)
    std::atomic<float> thdPct { 0.0f };        // distorsion estimée (%)
    std::atomic<bool>  clipped { false };      // écrêtage de la sortie (bloc courant)
    std::atomic<bool>  monoOut { false };

    //------------------------------------------------------------------ réglages d'interface (enregistrés avec le projet)
    std::atomic<int> uiSizeIdx { -1 };    // -1 = taille par défaut (60 %)
    std::atomic<int> glowIdx { 3 };       // 25 / 50 / 75 / 100 %
    std::atomic<int> currentPreset { 0 };
    std::atomic<bool> presetDirty { false };
    juce::String presetName { "Default" };

    //------------------------------------------------------------------ presets
    struct FactoryPreset { const char* name; std::vector<std::pair<const char*, float>> values; };
    static const std::vector<FactoryPreset>& factoryPresets();
    void loadFactoryPreset (int index);
    juce::ValueTree savePresetTree() const;
    void loadPresetTree (const juce::ValueTree& t, const juce::String& name);
    void setParam (const juce::String& id, float realValue);   // thread de messages uniquement

    //------------------------------------------------------------------ MIDI Learn (aucun CC par défaut)
    static constexpr int kMaxParams = 32;
    std::atomic<int> learnParam { -1 };
    std::atomic<int> ccMap[128];
    std::atomic<int> lastLearnedCc { -1 };
    int  paramIndexOf (const juce::String& id) const;
    int  ccForParam (int paramIndex) const;
    void clearMidiFor (int paramIndex);

private:
    void timerCallback() override;
    std::atomic<float> pendingValue[kMaxParams];
    std::atomic<bool>  pendingSet[kMaxParams];
    std::atomic<int>   ccMin[128], ccMax[128];

    double sr = 48000.0;
    int maxBlock = 512;

    ds::Channel chan[2];
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    int osLatency = 0, activeLatency = 0, lastOs = -1;
    double osRate = 192000.0;
    void prepareChannels (double rate);

    juce::AudioBuffer<float> dryDelay, dryBuf, wetBuf;
    int dryPos = 0;
    int fadeIn = 0;

    // valeurs lissées
    double sIn = 1, sOut = 1, sMix = 1, sWet = 1;
    double thdAcc = 0.0, sLink = 0.0;
    bool needSnap = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrinderProcessor)
};
