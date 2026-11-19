#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

const StringArray& GrinderProcessor::ratioNames()
{
    static const StringArray r { "1:1", "2:1", "3:1", "4:1", "6:1", "10:1", "20:1", "NUKE" };
    return r;
}

const StringArray& GrinderProcessor::hrNames()
{
    static const StringArray r { "4 dB", "8 dB", "12 dB", "16 dB", "20 dB", "24 dB", "28 dB" };
    return r;
}

AudioProcessorValueTreeState::ParameterLayout GrinderProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    auto onOff = [] (bool v, int) { return v ? String ("On") : String ("Off"); };
    auto knob  = [] (float v, int) { return String (v, 1); };
    auto add = [&] (std::unique_ptr<RangedAudioParameter> p) { l.add (std::move (p)); };
    using F = AudioParameterFloat; using B = AudioParameterBool; using C = AudioParameterChoice;
    using A = AudioParameterFloatAttributes; using BA = AudioParameterBoolAttributes;

    add (std::make_unique<B> (ParameterID { "power", 1 }, "Power", true, BA().withStringFromValueFunction (onOff)));
    add (std::make_unique<B> (ParameterID { "bypass", 1 }, "Bypass", false, BA().withStringFromValueFunction (onOff)));
    add (std::make_unique<F> (ParameterID { "input", 1 }, "Input", NormalisableRange<float> (0.0f, 10.0f, 0.01f), 5.0f,
                              A().withStringFromValueFunction ([] (float v, int) { const double d = ds::inputDb (v); return String (v, 1) + "  (" + (d > 0.04 ? "+" : "") + String (d, 1) + " dB)"; })));
    add (std::make_unique<F> (ParameterID { "attack", 1 }, "Attack", NormalisableRange<float> (0.0f, 10.0f, 0.01f), 5.0f,
                              A().withStringFromValueFunction ([] (float v, int)
                              {
                                  const double t = ds::attackSeconds (v, 4);
                                  return String (v, 1) + "  (" + (t < 0.001 ? String ((int) std::round (t * 1.0e6)) + " us" : String (t * 1000.0, t < 0.01 ? 1 : 0) + " ms") + ")";
                              })));
    add (std::make_unique<F> (ParameterID { "release", 1 }, "Release", NormalisableRange<float> (0.0f, 10.0f, 0.01f), 5.0f,
                              A().withStringFromValueFunction ([] (float v, int)
                              {
                                  const double t = ds::releaseSeconds (v);
                                  return String (v, 1) + "  (" + (t < 1.0 ? String ((int) std::round (t * 1000.0)) + " ms" : String (t, 2) + " s") + ")";
                              })));
    add (std::make_unique<F> (ParameterID { "output", 1 }, "Output", NormalisableRange<float> (0.0f, 10.0f, 0.01f), 5.0f,
                              A().withStringFromValueFunction ([] (float v, int) { const double d = ds::outputDb (v); return String (v, 1) + "  (" + (d > 0.04 ? "+" : "") + String (d, 1) + " dB)"; })));
    add (std::make_unique<F> (ParameterID { "mix", 1 }, "Mix", NormalisableRange<float> (0.0f, 100.0f, 0.01f), 100.0f,
                              A().withStringFromValueFunction ([] (float v, int) { return v <= 0.05f ? String ("Dry") : v >= 99.95f ? String ("Comp") : String ((int) std::round (v)) + " %"; })));
    add (std::make_unique<C> (ParameterID { "hr", 1 }, "Headroom", hrNames(), 3));
    add (std::make_unique<C> (ParameterID { "ratio", 1 }, "Ratio", ratioNames(), 4));
    add (std::make_unique<B> (ParameterID { "dethp", 1 }, "Detector HP", false, BA().withStringFromValueFunction (onOff)));
    add (std::make_unique<B> (ParameterID { "detband", 1 }, "Detector Band Emphasis", false, BA().withStringFromValueFunction (onOff)));
    add (std::make_unique<B> (ParameterID { "link", 1 }, "Stereo Link", false, BA().withStringFromValueFunction (onOff)));
    add (std::make_unique<B> (ParameterID { "audiohp", 1 }, "Audio HP", false, BA().withStringFromValueFunction (onOff)));
    add (std::make_unique<C> (ParameterID { "dist", 1 }, "Distortion", StringArray { "Off", "Dist 2", "Dist 3" }, 0));
    add (std::make_unique<B> (ParameterID { "brit", 1 }, "Brit Mode", false, BA().withStringFromValueFunction (onOff)));
    add (std::make_unique<B> (ParameterID { "image", 1 }, "Image Link", false, BA().withStringFromValueFunction (onOff)));
    add (std::make_unique<B> (ParameterID { "os", 1 }, "Oversampling 4x", true, BA().withStringFromValueFunction (onOff)));
    return l;
}

//==============================================================================
GrinderProcessor::GrinderProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "GRINDER", createLayout())
{
    pPower   = apvts.getRawParameterValue ("power");
    pBypass  = apvts.getRawParameterValue ("bypass");
    pInput   = apvts.getRawParameterValue ("input");
    pAttack  = apvts.getRawParameterValue ("attack");
    pRelease = apvts.getRawParameterValue ("release");
    pOutput  = apvts.getRawParameterValue ("output");
    pMix     = apvts.getRawParameterValue ("mix");
    pHr      = apvts.getRawParameterValue ("hr");
    pRatio   = apvts.getRawParameterValue ("ratio");
    pDetHp   = apvts.getRawParameterValue ("dethp");
    pDetBand = apvts.getRawParameterValue ("detband");
    pLink    = apvts.getRawParameterValue ("link");
    pAudioHp = apvts.getRawParameterValue ("audiohp");
    pDist    = apvts.getRawParameterValue ("dist");
    pBrit    = apvts.getRawParameterValue ("brit");
    pImage   = apvts.getRawParameterValue ("image");
    pOs      = apvts.getRawParameterValue ("os");
    for (int i = 0; i < 128; ++i) { ccMap[i] = -1; ccMin[i] = 127; ccMax[i] = 0; }
    for (int i = 0; i < kMaxParams; ++i) { pendingValue[i] = 0.0f; pendingSet[i] = false; }
    jassert (getParameters().size() <= kMaxParams);
    startTimerHz (60);
}

GrinderProcessor::~GrinderProcessor() { stopTimer(); }

bool GrinderProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    if (in != out) return false;
    return out == AudioChannelSet::mono() || out == AudioChannelSet::stereo();
}

void GrinderProcessor::prepareChannels (double rate)
{
    osRate = rate;
    for (auto& c : chan) c.prepare (rate);
}

void GrinderProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    maxBlock = jmax (64, samplesPerBlock);
    os = std::make_unique<dsp::Oversampling<float>> (2, 2, dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    os->initProcessing ((size_t) maxBlock);
    osLatency = (int) std::round (os->getLatencyInSamples());
    lastOs = pOs->load() > 0.5f ? 1 : 0;
    prepareChannels (lastOs ? sr * 4.0 : sr);
    needSnap = true;
    activeLatency = lastOs ? osLatency : 0;
    setLatencySamples (activeLatency);

    dryDelay.setSize (2, osLatency + maxBlock + 16); dryDelay.clear(); dryPos = 0;
    dryBuf.setSize (2, maxBlock);
    wetBuf.setSize (2, maxBlock);
    sIn = ds::dbToGain (ds::inputDb (pInput->load()));
    sOut = ds::dbToGain (ds::outputDb (pOutput->load()));
    sMix = pMix->load() * 0.01;
    sWet = (pPower->load() > 0.5f && pBypass->load() < 0.5f) ? 1.0 : 0.0;
    fadeIn = 0;
    monoOut = getTotalNumOutputChannels() < 2;
}

//==============================================================================
void GrinderProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int numCh = jmin (2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    for (int c = numCh; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);
    if (n == 0 || numCh == 0) return;
    if (n > maxBlock)                                         // bloc plus grand qu'annoncé : découpé, jamais d'allocation
    {
        for (int s0 = 0; s0 < n; s0 += maxBlock)
        {
            AudioBuffer<float> part (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), s0, jmin (maxBlock, n - s0));
            processBlock (part, midi);
        }
        return;
    }

    //-------------------------------------------------------------- MIDI : Learn + CC appris (file vers le thread de messages)
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (! m.isController()) continue;
        const int cc = m.getControllerNumber(), v = m.getControllerValue();
        const int lp = learnParam.load();
        if (lp >= 0)
        {
            for (auto& x : ccMap) if (x.load() == lp) x = -1;
            ccMap[cc] = lp; learnParam = -1; lastLearnedCc = cc;
            ccMin[cc] = v; ccMax[cc] = v;
        }
        const int pi = ccMap[cc].load();
        if (pi < 0 || pi >= kMaxParams) continue;
        if (v < ccMin[cc].load()) ccMin[cc] = v;
        if (v > ccMax[cc].load()) ccMax[cc] = v;
        const int lo = ccMin[cc].load(), hi = ccMax[cc].load();
        float x = (float) v / 127.0f;
        if (hi - lo >= 64) x = jlimit (0.0f, 1.0f, ((float) (v - lo) - 1.0f) / jmax (1.0f, (float) (hi - lo) - 2.0f));
        pendingValue[pi] = x;
        pendingSet[pi] = true;
    }

    //-------------------------------------------------------------- suréchantillonnage on/off
    const bool osOn = pOs->load() > 0.5f;
    if ((osOn ? 1 : 0) != lastOs)
    {
        lastOs = osOn ? 1 : 0;
        prepareChannels (osOn ? sr * 4.0 : sr);
        needSnap = true;
        if (os != nullptr) os->reset();
        activeLatency = osOn ? osLatency : 0;
        dryDelay.clear();
        fadeIn = (int) (0.03 * sr);
    }

    //-------------------------------------------------------------- réglages du bloc
    ds::Params p;
    p.ratio   = jlimit (0, 7, (int) pRatio->load());
    p.detHp   = pDetHp->load() > 0.5f;
    p.detBand = pDetBand->load() > 0.5f;
    p.link    = pLink->load() > 0.5f;
    p.deadPatch = p.link && numCh < 2;               // Link sans second appareil : seuil et distorsion changent (mono)
    p.audioHp = pAudioHp->load() > 0.5f;
    p.dist    = jlimit (0, 2, (int) pDist->load());
    p.brit    = pBrit->load() > 0.5f;
    p.attack  = pAttack->load();
    p.release = pRelease->load();
    p.hr      = 4.0 + 4.0 * jlimit (0, 6, (int) pHr->load());
    const bool image = pImage->load() > 0.5f && numCh > 1;
    const bool power = pPower->load() > 0.5f, bypass = pBypass->load() > 0.5f;

    // seuil du détecteur (dB, niveau interne après INPUT) ; Link sans second appareil le fait descendre
    const double thresh = -14.0 - (p.deadPatch ? 6.0 : 0.0);
    const double coreRate = osOn ? sr * 4.0 : sr;
    ds::Block blkP;
    blkP.make (p, coreRate, thresh);           // marge interne : plafond des étages non linéaires à (32 - HR) dB au-dessus de 0 dBFS
    if (needSnap) { for (auto& c : chan) c.snapTo (blkP); sLink = p.link ? 1.0 : 0.0; needSnap = false; }
    const double linkT = (p.link && numCh > 1) ? 1.0 : 0.0;
    const int nc = numCh > 1 ? 2 : 1;          // piste mono : un seul canal calculé

    float* out[2] = { buffer.getWritePointer (0), buffer.getWritePointer (numCh > 1 ? 1 : 0) };
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < n; ++i) dryBuf.setSample (ch, i, out[ch < numCh ? ch : 0][i]);

    //-------------------------------------------------------------- gain d'entrée (avant suréchantillonnage, lissé)
    const double aS = ds::coef (0.02, sr);
    const double inT = ds::dbToGain (ds::inputDb (pInput->load()));
    for (int i = 0; i < n; ++i)
    {
        sIn += (inT - sIn) * aS;
        for (int ch = 0; ch < nc; ++ch) wetBuf.setSample (ch, i, (float) (dryBuf.getSample (ch, i) * sIn));
    }
    if (nc == 1) wetBuf.clear (1, 0, n);

    //-------------------------------------------------------------- compresseur + étage de sortie (dans le suréchantillonnage)
    double sigE = 1.0e-12, errE = 0.0, grMax = 0.0;
    const double outT = ds::dbToGain (ds::outputDb (pOutput->load()));
    const double aO = ds::coef (0.02, coreRate);
    auto runCore = [&] (float* const* d, int len)
    {
        for (int i = 0; i < len; ++i)
        {
            double x[2] { 0, 0 }, lvl[2] { 0, 0 }, g[2] { 1, 1 };
            double e = 0.0;
            for (int ch = 0; ch < nc; ++ch)
            {
                chan[ch].fades (blkP);
                x[ch] = chan[ch].inputStage (d[ch][i], e);
                lvl[ch] = chan[ch].sidechain (x[ch], blkP);
            }
            if (nc == 2)
            {
                sLink += (linkT - sLink) * blkP.aFade;                      // détecteurs additionnés (fondu)
                const double m = 0.5 * (lvl[0] + lvl[1]);
                lvl[0] += (m - lvl[0]) * sLink; lvl[1] += (m - lvl[1]) * sLink;
            }
            for (int ch = 0; ch < nc; ++ch) g[ch] = chan[ch].computeGain (lvl[ch], blkP);
            if (image && nc == 2) { const double gm = jmin (g[0], g[1]); g[0] = g[1] = gm; }   // réduction identique
            sOut += (outT - sOut) * aO;
            for (int ch = 0; ch < nc; ++ch)
            {
                const double y = chan[ch].audioModes (x[ch] * g[ch], e) * sOut;
                const double c = chan[ch].ceil;
                const double z = ds::softClip (y, 1.0 / c, c, 0.02);       // étage de sortie
                e += (z - y) * (z - y);
                d[ch][i] = (float) z;
                sigE += z * z;
            }
            errE += e;
            grMax = jmax (grMax, chan[0].gr, nc > 1 ? chan[1].gr : 0.0);
        }
    };
    if (osOn && os != nullptr)
    {
        dsp::AudioBlock<float> blk (wetBuf.getArrayOfWritePointers(), 2, (size_t) n);
        auto up = os->processSamplesUp (blk);
        float* ptr[2] = { up.getChannelPointer (0), up.getChannelPointer (1) };
        runCore (ptr, (int) up.getNumSamples());
        os->processSamplesDown (blk);
    }
    else
    {
        float* ptr[2] = { wetBuf.getWritePointer (0), wetBuf.getWritePointer (1) };
        runCore (ptr, n);
    }
    if (nc == 1) wetBuf.copyFrom (1, 0, wetBuf, 0, 0, n);
    grDb = power ? (float) grMax : 0.0f;
    // distorsion estimée (écart non linéaire / signal), lissée sur ~150 ms
    const double thd = std::sqrt (errE / sigE) * 100.0;
    thdAcc += (thd - thdAcc) * ds::coef (0.15, sr / n);
    thdPct = power && ! bypass ? (float) thdAcc : 0.0f;

    //-------------------------------------------------------------- MIX (parallèle, sans décalage de phase), BYPASS, POWER
    const double mixT = pMix->load() * 0.01, wetT = (power && ! bypass) ? 1.0 : 0.0;
    const int D = dryDelay.getNumSamples();
    bool clip = false;
    for (int i = 0; i < n; ++i)
    {
        sMix += (mixT - sMix) * aS; sWet += (wetT - sWet) * aS;
        double fade = 1.0;
        if (fadeIn > 0) { fade = 1.0 - (double) fadeIn / (0.03 * sr); --fadeIn; }
        for (int ch = 0; ch < 2; ++ch)
        {
            dryDelay.setSample (ch, dryPos, dryBuf.getSample (ch, i));
            const double dry = dryDelay.getSample (ch, (dryPos - activeLatency + D) % D);
            const double wet = wetBuf.getSample (ch, i) * fade + dry * (1.0 - fade);
            const double m = dry * (1.0 - sMix) + wet * sMix;
            const double o = dry * (1.0 - sWet) + m * sWet;
            if (std::abs (o) > 0.999) clip = true;
            if (ch < numCh) out[ch][i] = (float) o;
        }
        dryPos = (dryPos + 1) % D;
    }
    clipped = clip && power;
}

//==============================================================================
void GrinderProcessor::timerCallback()
{
    auto& params = getParameters();
    for (int i = 0; i < params.size() && i < kMaxParams; ++i)
        if (pendingSet[i].exchange (false))
        {
            auto* p = params[i];
            p->beginChangeGesture();
            p->setValueNotifyingHost (pendingValue[i].load());
            p->endChangeGesture();
        }
    const int lat = pOs->load() > 0.5f ? osLatency : 0;
    if (lat != getLatencySamples()) setLatencySamples (lat);
    monoOut = getTotalNumOutputChannels() < 2;
}

int GrinderProcessor::paramIndexOf (const String& id) const
{
    auto& params = getParameters();
    for (int i = 0; i < params.size(); ++i)
        if (auto* wp = dynamic_cast<AudioProcessorParameterWithID*> (params[i]))
            if (wp->paramID == id) return i;
    return -1;
}

int GrinderProcessor::ccForParam (int pi) const
{
    for (int c = 0; c < 128; ++c) if (ccMap[c].load() == pi) return c;
    return -1;
}

void GrinderProcessor::clearMidiFor (int pi)
{
    for (auto& x : ccMap) if (x.load() == pi) x = -1;
    if (learnParam.load() == pi) learnParam = -1;
}

void GrinderProcessor::setParam (const String& id, float realValue)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (realValue));
        p->endChangeGesture();
    }
}

//==============================================================================
//  Presets d'usine (ratio : 0 1:1, 1 2:1, 2 3:1, 3 4:1, 4 6:1, 5 10:1, 6 20:1, 7 NUKE ; dist : 1 Dist 2, 2 Dist 3)
const std::vector<GrinderProcessor::FactoryPreset>& GrinderProcessor::factoryPresets()
{
    static const std::vector<FactoryPreset> p = {
        { "Default",              {} },
        { "Vocal 6:1",            { { "ratio", 4 }, { "input", 5.5f }, { "attack", 5 }, { "release", 4 }, { "dethp", 1 }, { "output", 5.6f } } },
        { "Vocal Opto",           { { "ratio", 5 }, { "input", 5.8f }, { "attack", 10 }, { "release", 0 }, { "dethp", 1 }, { "output", 5.8f } } },
        { "Vocal Warm Dist 2",    { { "ratio", 3 }, { "input", 6 }, { "attack", 6 }, { "release", 3 }, { "dist", 1 }, { "dethp", 1 }, { "output", 5.4f } } },
        { "De-Harsh Band",        { { "ratio", 2 }, { "input", 5.2f }, { "attack", 1.5f }, { "release", 2 }, { "detband", 1 }, { "dethp", 1 }, { "output", 5.2f } } },
        { "Kick Punch",           { { "ratio", 4 }, { "input", 6 }, { "attack", 6.5f }, { "release", 2.5f }, { "dethp", 0 }, { "output", 5.6f } } },
        { "Snare Crack",          { { "ratio", 3 }, { "input", 6.5f }, { "attack", 5.5f }, { "release", 1.5f }, { "dist", 2 }, { "output", 5.3f } } },
        { "Drum Room Nuke",       { { "ratio", 7 }, { "input", 7.5f }, { "attack", 2 }, { "release", 3 }, { "dist", 2 }, { "mix", 55 }, { "output", 5.6f } } },
        { "Parallel Drums",       { { "ratio", 6 }, { "input", 7 }, { "attack", 1 }, { "release", 2 }, { "dist", 1 }, { "dethp", 1 }, { "mix", 40 }, { "output", 6 } } },
        { "Bass Even",            { { "ratio", 4 }, { "input", 5.6f }, { "attack", 4 }, { "release", 5 }, { "audiohp", 0 }, { "output", 5.5f } } },
        { "Bass Brit Tube",       { { "input", 6 }, { "attack", 1.5f }, { "release", 4.5f }, { "brit", 1 }, { "ratio", 7 }, { "dist", 1 }, { "output", 5.4f } } },
        { "Acoustic Gentle 2:1",  { { "ratio", 1 }, { "input", 5.5f }, { "attack", 5 }, { "release", 5 }, { "dethp", 1 }, { "output", 5.4f } } },
        { "Guitar Bite",          { { "ratio", 3 }, { "input", 6.2f }, { "attack", 3 }, { "release", 3 }, { "dist", 2 }, { "audiohp", 1 }, { "output", 5.3f } } },
        { "Mix Bus Glue 2:1",     { { "ratio", 1 }, { "input", 5.2f }, { "attack", 7 }, { "release", 4 }, { "dethp", 1 }, { "link", 1 }, { "image", 1 }, { "output", 5.2f } } },
        { "Tape Color 1:1",       { { "ratio", 0 }, { "input", 6.5f }, { "dist", 2 }, { "hr", 5 }, { "output", 4.3f } } },
        { "Tube Color 1:1",       { { "ratio", 0 }, { "input", 6.5f }, { "dist", 1 }, { "hr", 5 }, { "output", 4.4f } } },
    };
    return p;
}

int GrinderProcessor::getNumPrograms() { return (int) factoryPresets().size(); }
const String GrinderProcessor::getProgramName (int i) { return isPositiveAndBelow (i, (int) factoryPresets().size()) ? String (factoryPresets()[(size_t) i].name) : String(); }
void GrinderProcessor::setCurrentProgram (int i) { if (isPositiveAndBelow (i, getNumPrograms())) loadFactoryPreset (i); }

void GrinderProcessor::loadFactoryPreset (int index)
{
    const auto& fp = factoryPresets()[(size_t) jlimit (0, (int) factoryPresets().size() - 1, index)];
    for (auto* p : getParameters())
        if (auto* wp = dynamic_cast<AudioProcessorParameterWithID*> (p))
            if (wp->paramID != "power" && wp->paramID != "os" && wp->paramID != "bypass")
            {
                p->beginChangeGesture(); p->setValueNotifyingHost (p->getDefaultValue()); p->endChangeGesture();
            }
    for (const auto& v : fp.values) setParam (v.first, v.second);
    currentPreset = index;
    presetName = fp.name;
    presetDirty = false;
}

ValueTree GrinderProcessor::savePresetTree() const
{
    ValueTree t ("GRINDERPRESET");
    for (auto* p : getParameters())
        if (auto* wp = dynamic_cast<AudioProcessorParameterWithID*> (p))
            if (wp->paramID != "power" && wp->paramID != "bypass")
                t.setProperty (wp->paramID, p->getValue(), nullptr);
    return t;
}

void GrinderProcessor::loadPresetTree (const ValueTree& t, const String& name)
{
    for (auto* p : getParameters())
        if (auto* wp = dynamic_cast<AudioProcessorParameterWithID*> (p))
            if (t.hasProperty (wp->paramID))
                p->setValueNotifyingHost ((float) t[wp->paramID]);
    presetName = name;
    currentPreset = -1;
    presetDirty = false;
}

//==============================================================================
void GrinderProcessor::getStateInformation (MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("uiSize", uiSizeIdx.load(), nullptr);
    state.setProperty ("glow", glowIdx.load(), nullptr);
    state.setProperty ("preset", currentPreset.load(), nullptr);
    state.setProperty ("presetName", presetName, nullptr);
    state.setProperty ("presetDirty", presetDirty.load(), nullptr);
    String map;
    for (int c = 0; c < 128; ++c)
        if (ccMap[c].load() >= 0)
            if (auto* wp = dynamic_cast<AudioProcessorParameterWithID*> (getParameters()[ccMap[c].load()]))
                map << c << "=" << wp->paramID << ";";
    state.setProperty ("midiMap", map, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void GrinderProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto t = ValueTree::fromXml (*xml);
        if (! t.isValid()) return;
        apvts.replaceState (t);
        uiSizeIdx     = (int) t.getProperty ("uiSize", -1);
        glowIdx       = jlimit (0, 3, (int) t.getProperty ("glow", 3));
        currentPreset = (int) t.getProperty ("preset", 0);
        presetName    = t.getProperty ("presetName", "Default").toString();
        presetDirty   = (bool) t.getProperty ("presetDirty", false);
        for (auto& x : ccMap) x = -1;
        for (auto& e : StringArray::fromTokens (t.getProperty ("midiMap").toString(), ";", ""))
        {
            const int c = e.upToFirstOccurrenceOf ("=", false, false).getIntValue();
            const int pi = paramIndexOf (e.fromFirstOccurrenceOf ("=", false, false));
            if (isPositiveAndBelow (c, 128) && pi >= 0) ccMap[c] = pi;
        }
    }
}

AudioProcessorEditor* GrinderProcessor::createEditor() { return new GrinderEditor (*this); }
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GrinderProcessor(); }
