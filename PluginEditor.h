#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "GrMenu.h"
#include "WindowMinSize.h"
#include "NhRackKnob.h"

class GrinderEditor;

namespace DsUi
{
    using namespace juce;
    // repère de dessin : 1600 x 540 unités
    constexpr float kRefW = 1600.0f, kRefH = 540.0f;
    constexpr float kDefaultScale = 0.6f, kMaxScale = 1.5f;   // fenêtre par défaut annoncée « 60 % », jusqu'à 150 %

    //------------------------------------------------------------------ potard de la série rack (lumière en haut à gauche, 45°)
    class Knob : public Component
    {
    public:
        Knob (GrinderEditor& e, String l, bool bigKnob);
        ~Knob() override;
        void attach (RangedAudioParameter* p);
        void paint (Graphics&) override;
        void mouseDown (const MouseEvent&) override;
        void mouseDrag (const MouseEvent&) override;
        void mouseUp (const MouseEvent&) override;
        void mouseDoubleClick (const MouseEvent&) override;
        void mouseWheelMove (const MouseEvent&, const MouseWheelDetails&) override;
        void tick();
        Point<float> centre() const { return getLocalBounds().toFloat().getCentre(); }
        float ringRadius() const;
        String valueText() const;
        float pillFont() const;
        float pillAlpha() const;
        void touchPill();
        nhrack::PillOverlay::Pill pill() const;
        RangedAudioParameter* param = nullptr;
        String label;
        bool big = false;
        Colour colour { 0xffff4a3d };
        float value() const { return norm; }
    private:
        GrinderEditor& ed;
        std::unique_ptr<ParameterAttachment> att;
        float norm = 0.0f, dragStart = 0.0f, shownAngle = 0.0f, prevAngle = 0.0f, blur = 0.0f, blurDir = 1.0f;
        Image face;
        int facePx = 0;
        double lastActive = -100.0;
        float lastNorm = -1.0f;
        bool dragging = false;
        static float angleOf (float v) { return MathConstants<float>::pi * (-0.75f + 1.5f * v); }
    };
}

//==============================================================================
class GrinderEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit GrinderEditor (GrinderProcessor&);
    ~GrinderEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void parentHierarchyChanged() override;
    void visibilityChanged() override;

    GrinderProcessor& proc;
    GrMenu::Fonts fonts;
    GrMenu::MenuLnf menuLnf;
    juce::Font font (int weight, float h) const;   // 0 regular, 1 medium, 2 bold
    float sc() const { return (float) getWidth() / DsUi::kRefW; }
    float glow() const { return 0.25f * (float) (juce::jlimit (0, 3, proc.glowIdx.load()) + 1); }

    void showStatus (const juce::String& s);
    void showMainMenu();
    void showParamMidiMenu (juce::RangedAudioParameter* p);
    juce::PopupMenu buildContextMenu();
    void handleMainMenu (int r);
    void askRepaint (juce::Rectangle<int> r);
    juce::Rectangle<float> R (float x, float y, float w, float h) const { const float s = sc(); return { x * s, y * s, w * s, h * s }; }

    nhrack::PillOverlay pills;
    DsUi::Knob inputKnob, attackKnob, releaseKnob, outputKnob, mixKnob;

    // états des commutateurs (détecteur 8 états, audio 6 états)
    static int detectorState (bool hp, bool band, bool link);
    static int audioState (bool hp, int dist);

private:
    friend int main();                         // outil de captures (tools/render)
    void timerCallback() override;
    void applySize (float scale);
    void setScaleIndex (int idx);
    void useSoftwareRenderer();
    void trackHostWindow();
    void prevNextPreset (int dir);
    void savePreset();
    void loadPreset();
    void setBool (const char* id, bool v);
    void toggleParam (const char* id);
    void setChoice (const char* id, int idx);
    void cycleRatio (int dir);
    void cycleDetector (int dir);
    void cycleAudio (int dir);
    void stepHr (int dir);
    juce::RangedAudioParameter* paramAt (juce::Point<float> p) const;

    // façade
    void paintFace (juce::Graphics&);
    void paintScale (juce::Graphics&, juce::Point<float> c, float r);
    void paintLed (juce::Graphics&, juce::Point<float> c, float rad, juce::Colour col, float level);
    void paintButton (juce::Graphics&, juce::Rectangle<float> r, const juce::String& t, bool pressed);
    void paintRocker (juce::Graphics&);
    void paintScrew (juce::Graphics&);
    void paintPanel (juce::Graphics&, juce::Rectangle<float> r);
    void drawStatusText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Font);

    // zones (repère 1600 x 540)
    juce::Rectangle<float> pill() const       { return R (600, 31, 400, 38); }
    juce::Rectangle<float> pillPrev() const   { return R (600, 31, 34, 38); }
    juce::Rectangle<float> pillNext() const   { return R (966, 31, 34, 38); }
    juce::Rectangle<float> bypassBtn() const  { return R (98, 196, 92, 38); }
    juce::Rectangle<float> ratioBtn() const   { return R (904, 214, 100, 40); }
    juce::Rectangle<float> detBtn() const     { return R (1060, 230, 112, 38); }
    juce::Rectangle<float> audioBtn() const   { return R (1212, 230, 112, 38); }
    juce::Rectangle<float> powerRect() const  { return R (1404, 152, 56, 92); }
    juce::Rectangle<float> britBtn() const    { return R (1376, 360, 120, 38); }
    juce::Rectangle<float> imageBtn() const   { return R (1376, 424, 120, 38); }
    juce::Rectangle<float> hrRect() const     { return R (104, 360, 72, 72); }
    juce::Rectangle<float> hrLabel() const    { return R (110, 324, 60, 24); }
    juce::Rectangle<float> dryLabel() const   { return R (1186, 456, 56, 22); }
    juce::Rectangle<float> compLabel() const  { return R (1262, 456, 64, 22); }
    static float grLedX (int i)    { return 266.0f + 40.0f * (float) i; }    // 16 voyants, de 26 dB (gauche) à 1 dB (droite)
    static float ratioLedX (int i) { return 290.0f + 80.0f * (float) i; }    // 8 ratios
    juce::Rectangle<float> ratioHit (int i) const { return R (ratioLedX (i) - 36, 222, 72, 50); }
    juce::Rectangle<float> detHit (int i) const   { return R (1060, 136 + 28.0f * (float) i, 112, 26); }
    juce::Rectangle<float> audioHit (int i) const { return R (1212, 136 + 28.0f * (float) i, 112, 26); }

    std::unique_ptr<juce::Drawable> logo;
    juce::Image brushed;
    wiga::WindowMinSize hostLock;
    void* lastPeer = nullptr;
    double lastRendererCheck = 0, lastPaint = 0, pendingSince = 0;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::String status;
    double statusTime = -10.0;
    float statusAlpha = 0.0f;
    int hoverZone = 0;
    int pressed = 0;                     // bouton enfoncé (1 bypass, 2 ratio, 3 détecteur, 4 audio, 5 brit, 6 image)
    float hrDragStart = 0, hrDragAcc = 0;
    bool hrDragging = false;

    // lumières
    float grLed[16] {}, thd1 = 0, thdRed = 0, powerLevel = 1, bypassLed = 0, britLed = 0, imageLed = 0;
    float grShown = 0.0f;
    double thd1Hold = 0, redHold = 0;
    std::vector<float> lastParamValues;
    int lastGlow = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrinderEditor)
};
