#include "PluginEditor.h"
#include "BinaryData.h"
#include <random>

using namespace juce;
using namespace DsUi;
using GrMenu::fnt;

//==============================================================================
//  Couleurs : façade anodisée noire, sérigraphie blanc cassé, accent rouge
//==============================================================================
namespace
{
    const Colour cAccent (0xffff4a3d), cText (0xffe4e7ec), cTextHi (0xfff7f8fa), cDim (0xff9ca3ad),
                 cLine (0xffbfc4cc), cGreen (0xff3fe06a), cYellow (0xffffcc2e), cRed (0xffff3b30), cCyan (0xff38d6ff);

    double nowS() { return Time::getMillisecondCounterHiRes() * 0.001; }

    // anodisation noire brossée (horizontalement), raccordable en x et en y
    Image makeBrushed()
    {
        const int W = 1024, H = 256;
        Image img (Image::RGB, W, H, false);
        std::mt19937 rng (23);
        std::normal_distribution<float> nd (0.0f, 1.0f);
        std::vector<float> row ((size_t) W), tmp ((size_t) W);
        Image::BitmapData bd (img, Image::BitmapData::writeOnly);
        for (int y = 0; y < H; ++y)
        {
            for (auto& v : row) v = nd (rng);
            for (int pass = 0; pass < 3; ++pass)
            {
                const int k = 22;
                float acc = 0.0f;
                for (int i = -k; i <= k; ++i) acc += row[(size_t) ((i + W) % W)];
                for (int x = 0; x < W; ++x)
                {
                    tmp[(size_t) x] = acc / (2 * k + 1);
                    acc += row[(size_t) ((x + k + 1) % W)] - row[(size_t) ((x - k + W) % W)];
                }
                row.swap (tmp);
            }
            const float rowTone = nd (rng) * 0.6f;
            for (int x = 0; x < W; ++x)
            {
                const float v = 21.0f + row[(size_t) x] * 11.0f + rowTone + nd (rng) * 1.2f;
                const uint8 a = (uint8) jlimit (0, 255, (int) v), b = (uint8) jlimit (0, 255, (int) (v + 1.0f)), c = (uint8) jlimit (0, 255, (int) (v + 3.0f));
                bd.setPixelColour (x, y, Colour (a, b, c));
            }
        }
        return img;
    }

    // éclairage d'ambiance : source sur la diagonale haut gauche (45°)
    void paintLight (Graphics& g, Rectangle<float> r)
    {
        const float d = r.getWidth() + r.getHeight();
        // source sur la diagonale exacte du coin haut gauche : même décalage en x et en y
        ColourGradient lit (Colours::white.withAlpha (0.13f), r.getX(), r.getY(),
                            Colours::black.withAlpha (0.42f), r.getX() + d * 0.5f, r.getY() + d * 0.5f, false);
        lit.addColour (0.45, Colours::transparentBlack);
        g.setGradientFill (lit);
        g.fillRect (r);
        const float c = d * 0.38f;
        ColourGradient sat (Colours::transparentWhite, r.getX() + c * 0.5f - d * 0.06f, r.getY() + c * 0.5f - d * 0.06f,
                            Colours::transparentWhite, r.getX() + c * 0.5f + d * 0.06f, r.getY() + c * 0.5f + d * 0.06f, false);
        sat.addColour (0.5, Colours::white.withAlpha (0.06f));
        g.setGradientFill (sat);
        g.fillRect (r);
    }

    ColourGradient diag (Rectangle<float> r, Colour tl, Colour br)
    {
        const float k = 0.5f * jmax (r.getWidth(), r.getHeight());
        const auto c = r.getCentre();
        return ColourGradient (tl, c.x - k, c.y - k, br, c.x + k, c.y + k, false);
    }
    void raisedShadow (Graphics& g, Rectangle<float> r, float rad, float s, float depth)
    {
        for (int i = 6; i >= 0; --i)
        {
            const float e = (0.8f + (float) i * 2.0f) * s, d = depth * s * (0.9f + 0.16f * (float) i);
            g.setColour (Colours::black.withAlpha (0.11f));
            g.fillRoundedRectangle (r.translated (d, d).expanded (e), rad + e);
        }
        for (int i = 3; i >= 1; --i)
        {
            const float e = (float) i * 1.1f * s;
            g.setColour (Colours::black.withAlpha (0.20f));
            g.fillRoundedRectangle (r.expanded (e), rad + e);
        }
    }
    void raisedEdge (Graphics& g, Rectangle<float> r, float rad, float s)
    {
        g.setGradientFill (diag (r, Colours::white.withAlpha (0.30f), Colours::black.withAlpha (0.55f)));
        g.drawRoundedRectangle (r.reduced (0.5f * s), rad, 1.1f * s);
    }
    void recessed (Graphics& g, Rectangle<float> r, float rad, float s, float depth = 1.0f)
    {
        g.setGradientFill (diag (r, Colours::black.withAlpha (0.6f), Colours::white.withAlpha (0.12f)));
        g.drawRoundedRectangle (r.expanded (0.8f * s), rad + 0.8f * s, 1.0f * s);
        g.saveState();
        Path clip; clip.addRoundedRectangle (r, rad);
        g.reduceClipRegion (clip);
        for (int i = 1; i <= 4; ++i)
        {
            const float d = (float) i * 1.3f * s * depth;
            g.setColour (Colours::black.withAlpha (0.22f));
            g.drawRoundedRectangle (r.translated (d, d).expanded (d), rad + d, 2.0f * d);
        }
        g.restoreState();
    }

    const float kGrThr[16] = { 26, 23, 20, 17, 14, 12, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
    Colour grColour (int i) { return kGrThr[i] <= 5 ? cGreen : kGrThr[i] <= 10 ? cYellow : cRed; }
}

//==============================================================================
//  Potard
//==============================================================================
Knob::Knob (GrinderEditor& e, String l, bool b) : label (std::move (l)), big (b), ed (e)
{
    ed.pills.add (this, [this] { return pill(); }, [this] { return ed.font (1, pillFont()); });
}

Knob::~Knob() { ed.pills.remove (this); }

void Knob::attach (RangedAudioParameter* p)
{
    att.reset();
    param = p;
    if (p == nullptr) return;
    att = std::make_unique<ParameterAttachment> (*p, [this] (float v) { norm = param->convertTo0to1 (v); });
    att->sendInitialUpdate();
    shownAngle = prevAngle = angleOf (norm);
    repaint();
}

float Knob::ringRadius() const { return jmin ((float) getWidth(), (float) getHeight()) * 0.5f / 1.34f * 0.98f; }

String Knob::valueText() const
{
    if (param == nullptr) return {};
    return param->getCurrentValueAsText();
}

void Knob::touchPill() { lastActive = nowS(); }

float Knob::pillAlpha() const
{
   #ifdef NHRACK_SHOW_PILLS
    return 1.0f;
   #endif
    const double idle = nowS() - lastActive;
    return (float) jlimit (0.0, 1.0, 1.0 - (idle - 2.5) / 0.35);
}

void Knob::tick()
{
    if (std::abs (norm - lastNorm) > 1.0e-6f) { if (lastNorm >= 0.0f) touchPill(); lastNorm = norm; }
    shownAngle = angleOf (norm);
    const float step = MathConstants<float>::twoPi / 40.0f;
    const float t = std::abs (shownAngle - prevAngle) / step;
    if (std::abs (shownAngle - prevAngle) > 1.0e-5f) blurDir = shownAngle > prevAngle ? -1.0f : 1.0f;
    prevAngle = shownAngle;
    const float lvl = jlimit (0.0f, 1.6f, (t - 0.15f) / 0.6f);
    const float nb = lvl > blur ? lvl : blur * 0.6f;
    if (std::abs (nb - blur) > 0.002f) { blur = nb < 0.01f ? 0.0f : nb; repaint(); }
    const int px = jmax (8, (int) std::ceil (2.0f * nhrack::faceRadius (ringRadius()) * Component::getApproximateScaleFactorForComponent (this)));
    if (px != facePx) { facePx = px; face = nhrack::makeAnodisedFace (px); repaint(); }
}

void Knob::paint (Graphics& g)
{
    nhrack::Look k;
    k.c = centre(); k.ring = ringRadius(); k.angle = shownAngle;
    k.a0 = angleOf (0.0f); k.a1 = angleOf (1.0f); k.from = k.a0;
    k.s = ed.sc(); k.glow = ed.glow(); k.alpha = ed.proc.pPower->load() > 0.5f ? 1.0f : 0.55f;
    k.blur = blur; k.blurDir = blurDir;
    k.colour = colour; k.face = &face;
    nhrack::paintKnob (g, k);
}

float Knob::pillFont() const
{
    const float s = ed.sc();
    return jlimit (12.0f, 16.0f, ringRadius() / s * 0.2f) * s;
}

nhrack::PillOverlay::Pill Knob::pill() const
{
    nhrack::PillOverlay::Pill p;
    if (param == nullptr) return p;
    const float s = ed.sc();
    p.text = valueText();
    const Font f = ed.font (1, pillFont());
    p.r = nhrack::pillRect (centre(), ringRadius(), angleOf (norm), p.text, f, s);
    { const float rr = ringRadius() * 0.92f; p.ringBox = Rectangle<float> (rr * 2, rr * 2).withCentre (centre()); }
    p.c = centre(); p.ring = ringRadius(); p.angle = angleOf (norm); p.gap = 3.5f * s;
    p.colour = colour;
    p.alpha = pillAlpha();
    p.visible = p.alpha > 0.001f;
    return p;
}

void Knob::mouseDown (const MouseEvent& e)
{
    touchPill();
    if (param == nullptr) return;
    if (e.mods.isPopupMenu()) { ed.showParamMidiMenu (param); return; }
    dragging = true; dragStart = norm;
    att->beginGesture();
    e.source.enableUnboundedMouseMovement (true);
}

void Knob::mouseDrag (const MouseEvent& e)
{
    if (! dragging || param == nullptr) return;
    const float k = e.mods.isShiftDown() ? 0.0008f : 0.004f;
    const float v = jlimit (0.0f, 1.0f, dragStart - (float) e.getDistanceFromDragStartY() * k / jmax (0.6f, ed.sc()));
    att->setValueAsPartOfGesture (param->convertFrom0to1 (v));
}

void Knob::mouseUp (const MouseEvent& e)
{
    touchPill();
    if (! dragging) return;
    dragging = false;
    att->endGesture();
    e.source.enableUnboundedMouseMovement (false);
}

void Knob::mouseDoubleClick (const MouseEvent& e)
{
    touchPill();
    if (param == nullptr || e.mods.isPopupMenu()) return;
    att->setValueAsCompleteGesture (param->convertFrom0to1 (param->getDefaultValue()));
}

void Knob::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    touchPill();
    if (param == nullptr) return;
    const float step = (e.mods.isShiftDown() ? 0.005f : 0.02f) * (w.deltaY >= 0 ? 1.0f : -1.0f) * jmin (1.0f, std::abs (w.deltaY) * 4.0f + 0.25f);
    att->setValueAsCompleteGesture (param->convertFrom0to1 (jlimit (0.0f, 1.0f, norm + step)));
}

//==============================================================================
//  Fenêtre
//==============================================================================
GrinderEditor::GrinderEditor (GrinderProcessor& p)
    : AudioProcessorEditor (p), proc (p),
      inputKnob (*this, "INPUT", true), attackKnob (*this, "ATTACK", true), releaseKnob (*this, "RELEASE", true),
      outputKnob (*this, "OUTPUT", true), mixKnob (*this, "MIX", false)
{
    fonts.reg  = Typeface::createSystemTypefaceFor (BinaryData::PoppinsRegular_ttf, BinaryData::PoppinsRegular_ttfSize);
    fonts.med  = Typeface::createSystemTypefaceFor (BinaryData::PoppinsMedium_ttf, BinaryData::PoppinsMedium_ttfSize);
    fonts.bold = Typeface::createSystemTypefaceFor (BinaryData::PoppinsBold_ttf, BinaryData::PoppinsBold_ttfSize);
    menuLnf.f = fonts;
    logo = Drawable::createFromImageData (BinaryData::NineHandsLogoWhite_svg, BinaryData::NineHandsLogoWhite_svgSize);
    brushed = makeBrushed();

    setOpaque (true);
    auto& ap = proc.apvts;
    inputKnob.attach (ap.getParameter ("input"));
    attackKnob.attach (ap.getParameter ("attack"));
    releaseKnob.attach (ap.getParameter ("release"));
    outputKnob.attach (ap.getParameter ("output"));
    mixKnob.attach (ap.getParameter ("mix"));
    for (auto* k : { &inputKnob, &attackKnob, &releaseKnob, &outputKnob, &mixKnob }) addAndMakeVisible (k);
    addAndMakeVisible (pills);
    for (auto* prm : proc.getParameters()) lastParamValues.push_back (prm->getValue());
    powerLevel = proc.pPower->load() > 0.5f ? 1.0f : 0.0f;

    setResizable (true, true);
    const int idx = proc.uiSizeIdx.load();
    static const float scales[3] = { 0.6f, 0.75f, 1.0f };
    applySize (idx < 0 ? kDefaultScale : scales[jlimit (0, 2, idx)]);
    startTimerHz (30);
}

GrinderEditor::~GrinderEditor() { stopTimer(); PopupMenu::dismissAllActiveMenus(); hostLock.detach(); }

Font GrinderEditor::font (int w, float h) const { return fnt (w == 0 ? fonts.reg : w == 1 ? fonts.med : fonts.bold, h); }

void GrinderEditor::applySize (float scale)
{
    const int minW = (int) std::round (kRefW * kDefaultScale), minH = (int) std::round (kRefH * kDefaultScale);
    const int maxW = (int) std::round (kRefW * kMaxScale), maxH = (int) std::round (kRefH * kMaxScale);
    setResizeLimits (minW, minH, maxW, maxH);
    getConstrainer()->setFixedAspectRatio ((double) kRefW / kRefH);
    hostLock.setMinimumSize (minW, minH);
    hostLock.setMaximumSize (maxW, maxH);
    scale = jlimit (kDefaultScale, kMaxScale, scale);
    setSize ((int) std::round (kRefW * scale), (int) std::round (kRefH * scale));
}

void GrinderEditor::setScaleIndex (int idx)
{
    static const float scales[3] = { 0.6f, 0.75f, 1.0f };
    applySize (scales[jlimit (0, 2, idx)]);
}

void GrinderEditor::resized()
{
    auto knobAt = [this] (Component& k, float cx, float cy, float r) { const float e = r * 1.34f; k.setBounds (R (cx - e, cy - e, e * 2, e * 2).toNearestInt()); };
    knobAt (inputKnob,   360, 404, 60);
    knobAt (attackKnob,  600, 404, 60);
    knobAt (releaseKnob, 840, 404, 60);
    knobAt (outputKnob, 1080, 404, 60);
    knobAt (mixKnob,    1256, 404, 34);
    pills.setBounds (getLocalBounds());
    pills.toFront (false);
}

//==============================================================================
//  Dessin de la façade
//==============================================================================
void GrinderEditor::paintPanel (Graphics& g, Rectangle<float> r)
{
    // cadre sérigraphié (trait fin), comme les zones d'une façade de rack
    const float s = sc();
    g.setColour (cLine.withAlpha (0.72f));
    g.drawRoundedRectangle (r, 14 * s, 1.7f * s);
}

void GrinderEditor::paintScale (Graphics& g, Point<float> c, float r)
{
    const float s = sc();
    for (int i = 0; i <= 20; ++i)
    {
        const float a = MathConstants<float>::pi * (-0.75f + 0.075f * (float) i);
        const bool major = i % 2 == 0;
        const float r1 = r + 9.0f, r2 = r + (major ? 17.0f : 13.0f);
        g.setColour (major ? cText : cDim);
        g.drawLine (Line<float> (c + Point<float> (std::sin (a), -std::cos (a)) * r1 * s, c + Point<float> (std::sin (a), -std::cos (a)) * r2 * s),
                    (major ? 1.8f : 1.1f) * s);
        if (major)
        {
            g.setColour (cText);
            g.setFont (font (1, 14 * s));
            const auto t = c + Point<float> (std::sin (a), -std::cos (a)) * (r + 30.0f) * s;
            g.drawText (String (i / 2), Rectangle<float> (30 * s, 16 * s).withCentre (t), Justification::centred, false);
        }
    }
}

void GrinderEditor::paintLed (Graphics& g, Point<float> c, float rad, Colour col, float level)
{
    const float s = sc(), r = rad * s;
    level *= powerLevel;
    if (level > 0.01f)
    {
        ColourGradient halo (col.withAlpha (0.45f * glow() * level), c.x, c.y, col.withAlpha (0.0f), c.x + r * 3.2f, c.y, true);
        g.setGradientFill (halo);
        g.fillEllipse (Rectangle<float> (r * 6.4f, r * 6.4f).withCentre (c));
    }
    // bague en relief : ombre portée à 45° vers le bas à droite
    for (int i = 3; i >= 0; --i)
    {
        const float e = r * 2.0f + (1.8f + 2.0f * (float) i) * s, d = (1.2f + 0.8f * (float) i) * s;
        g.setColour (Colours::black.withAlpha (0.16f));
        g.fillEllipse (Rectangle<float> (e, e).withCentre (c.translated (d, d)));
    }
    ColourGradient ring (Colour (0xff70757e), c.x - r, c.y - r, Colour (0xff121417), c.x + r, c.y + r, false);
    g.setGradientFill (ring);
    g.fillEllipse (Rectangle<float> (r * 2.0f + 3.0f * s, r * 2.0f + 3.0f * s).withCentre (c));
    g.setColour (Colour (0xff070809));
    g.fillEllipse (Rectangle<float> (r * 2.0f + 0.6f * s, r * 2.0f + 0.6f * s).withCentre (c));
    const Colour off = col.withMultipliedSaturation (0.55f).withMultipliedBrightness (0.24f);
    const Colour on = col.interpolatedWith (Colours::white, 0.45f);
    ColourGradient lens (off.interpolatedWith (on, level), c.x - r * 0.3f, c.y - r * 0.3f,
                         off.darker (0.4f).interpolatedWith (col, level), c.x + r, c.y + r, true);
    g.setGradientFill (lens);
    g.fillEllipse (Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c));
    g.setColour (Colours::white.withAlpha (0.35f + 0.2f * level));
    g.fillEllipse (Rectangle<float> (r * 0.7f, r * 0.5f).withCentre (c + Point<float> (-r * 0.35f, -r * 0.4f)));
}

void GrinderEditor::paintButton (Graphics& g, Rectangle<float> r, const String& t, bool down)
{
    // poussoir rectangulaire gris clair, qui s'enfonce au clic
    const float s = sc(), rad = 5 * s;
    if (down) r = r.translated (1.0f * s, 1.5f * s);
    raisedShadow (g, r, rad, s, down ? 1.2f : 3.0f);
    g.setGradientFill (diag (r, Colour (0xffb9bec6), Colour (0xff6d727b)));
    g.fillRoundedRectangle (r, rad);
    const auto top = r.reduced (3 * s);
    g.setGradientFill (ColourGradient (Colour (0xffd4d8de), top.getX(), top.getY(), Colour (0xff9aa0a9), top.getX(), top.getBottom(), false));
    g.fillRoundedRectangle (top, rad * 0.7f);
    raisedEdge (g, r, rad, s);
    g.setColour (Colour (0xff15171b));
    g.setFont (font (2, 13 * s).withExtraKerningFactor (0.06f));
    g.drawText (t, r, Justification::centred, false);
}

void GrinderEditor::paintRocker (Graphics& g)
{
    // interrupteur à bascule (haut = marche), lumière en haut à gauche
    const float s = sc();
    const auto r = powerRect();
    const bool on = proc.pPower->load() > 0.5f;
    raisedShadow (g, r, 6 * s, s, 3.0f);
    g.setColour (Colour (0xff0b0c0e));
    g.fillRoundedRectangle (r, 6 * s);
    recessed (g, r.reduced (5 * s), 4 * s, s, 0.8f);
    const auto inner = r.reduced (7 * s);
    const float half = inner.getHeight() * 0.5f;
    auto upper = inner.withHeight (half), lower = inner.withTrimmedTop (half);
    // la moitié enfoncée est plus sombre et plate ; l'autre ressort, éclairée en haut à gauche
    auto paintHalf = [&] (Rectangle<float> h, bool raised)
    {
        if (raised)
        {
            g.setGradientFill (diag (h, Colour (0xff2e3238), Colour (0xff131518)));
            g.fillRoundedRectangle (h, 3 * s);
            g.setColour (Colours::white.withAlpha (0.18f));
            g.drawLine (h.getX() + 3 * s, h.getY() + 1.2f * s, h.getRight() - 3 * s, h.getY() + 1.2f * s, 1.0f * s);
        }
        else
        {
            g.setColour (Colour (0xff0e1012));
            g.fillRoundedRectangle (h, 3 * s);
        }
    };
    paintHalf (upper, ! on);
    paintHalf (lower, on);
    g.setColour (on ? cText : cDim);
    g.setFont (font (2, 13 * s));
    g.drawText ("I", upper, Justification::centred, false);
    g.drawText ("O", lower, Justification::centred, false);
}

void GrinderEditor::paintScrew (Graphics& g)
{
    // vis de réglage du HR (7 crans), fente orientée selon la valeur
    const float s = sc();
    const auto r = hrRect();
    const auto c = r.getCentre();
    const float rad = 17 * s;
    const int idx = jlimit (0, 6, (int) proc.pHr->load());
    // cuvette dans la façade
    g.setColour (Colour (0xff060708));
    g.fillEllipse (Rectangle<float> (rad * 2.5f, rad * 2.5f).withCentre (c));
    recessed (g, Rectangle<float> (rad * 2.5f, rad * 2.5f).withCentre (c), rad * 1.25f, s, 0.7f);
    // graduations des 7 crans (12 h = 16 dB)
    for (int i = 0; i < 7; ++i)
    {
        const float a = MathConstants<float>::pi * (-0.75f + 0.25f * (float) i);
        g.setColour (i == idx ? cAccent.withAlpha (0.6f + 0.4f * powerLevel) : cDim);
        g.fillEllipse (Rectangle<float> (3.4f * s, 3.4f * s).withCentre (c + Point<float> (std::sin (a), -std::cos (a)) * rad * 1.62f));
    }
    // tête de vis
    for (int i = 3; i >= 0; --i)
    {
        const float d = (1.2f + 1.0f * (float) i) * s;
        g.setColour (Colours::black.withAlpha (0.18f));
        g.fillEllipse (Rectangle<float> (rad * 2 + d, rad * 2 + d).withCentre (c.translated (d, d)));
    }
    ColourGradient head (Colour (0xffc7ccd3), c.x - rad, c.y - rad, Colour (0xff4b5058), c.x + rad, c.y + rad, false);
    g.setGradientFill (head);
    g.fillEllipse (Rectangle<float> (rad * 2, rad * 2).withCentre (c));
    g.setColour (Colours::black.withAlpha (0.45f));
    g.drawEllipse (Rectangle<float> (rad * 2, rad * 2).withCentre (c), 0.8f * s);
    const float a = MathConstants<float>::pi * (-0.75f + 0.25f * (float) idx);
    const auto dir = Point<float> (std::sin (a), -std::cos (a));
    const auto p1 = c + dir * rad * 0.82f, p2 = c - dir * rad * 0.82f;
    g.setColour (Colour (0xff15171a));
    g.drawLine (Line<float> (p1, p2), 4.2f * s);
    g.setColour (Colours::white.withAlpha (0.25f));                       // lèvre éclairée de la fente
    g.drawLine (Line<float> (p1.translated (0.9f * s, 0.9f * s), p2.translated (0.9f * s, 0.9f * s)), 0.9f * s);
    g.setColour (cAccent.withAlpha (0.85f));                                // point repère (comme le point d'une vis de réglage)
    g.fillEllipse (Rectangle<float> (3.6f * s, 3.6f * s).withCentre (c + dir * rad * 0.62f));
}

void GrinderEditor::paintFace (Graphics& g)
{
    const float s = sc();
    const auto face = R (0, 0, kRefW, kRefH);
    g.setTiledImageFill (brushed, 0, 0, 1.0f);
    g.fillRect (face);
    paintLight (g, face);
    // oreilles de rack (sans vis)
    for (float ex : { 0.0f, 1554.0f })
    {
        g.setColour (Colours::black.withAlpha (0.25f));
        g.fillRect (R (ex, 0, 46, kRefH));
        for (float yy : { 76.0f, 464.0f })
        {
            g.setColour (Colour (0xff050506));
            g.fillRoundedRectangle (R (ex + 14, yy - 18, 18, 36), 9 * s);
            recessed (g, R (ex + 14, yy - 18, 18, 36), 9 * s, s, 1.6f);
        }
    }
    g.setColour (Colour (0xff070809)); g.fillRect (R (45, 0, 2, kRefH)); g.fillRect (R (1553, 0, 2, kRefH));
    g.setColour (Colour (0xff3a3e45)); g.fillRect (R (47, 0, 1, kRefH)); g.fillRect (R (1555, 0, 1, kRefH));

    // en-tête : nom, sous-titre, logo
    g.setColour (cTextHi);
    g.setFont (font (2, 36 * s).withExtraKerningFactor (0.01f));
    g.drawText ("Grinder", R (80, 20, 300, 46), Justification::centredLeft, false);
    g.setColour (cDim);
    g.setFont (font (1, 12 * s).withExtraKerningFactor (0.12f));
    g.drawText ("CLASSIC KNEE COMPRESSOR", R (80, 66, 400, 18), Justification::centredLeft, false);
    if (auto* lg = logo.get())
        lg->drawWithin (g, R (1270, 38, 250, 22), RectanglePlacement::xRight | RectanglePlacement::yMid, 0.92f);

    // barre de presets / ligne d'état (même pastille)
    {
        const auto p = pill();
        g.setColour (Colour (0xff060708));
        g.fillRoundedRectangle (p, p.getHeight() * 0.5f);
        recessed (g, p, p.getHeight() * 0.5f, s);
        const float a = 1.0f - statusAlpha;
        g.setFont (font (1, 23 * s));
        if (a > 0.01f)
        {
            g.setColour (cTextHi.withAlpha (a));
            const String name = proc.presetName + (proc.presetDirty.load() ? " *" : "");
            g.drawFittedText (name, p.reduced (34 * s, 0).toNearestInt(), Justification::centred, 1, 0.6f);
            for (int side = 0; side < 2; ++side)
            {
                const auto r = side == 0 ? pillPrev() : pillNext();
                const bool hl = hoverZone == (side == 0 ? 1 : 2);
                Path t;
                const float cx = r.getCentreX() + (side == 0 ? 3.0f : -3.0f) * s, cy = r.getCentreY(), h = 5.5f * s;
                if (side == 0) t.addTriangle (cx + h * 0.6f, cy - h, cx + h * 0.6f, cy + h, cx - h * 0.8f, cy);
                else           t.addTriangle (cx - h * 0.6f, cy - h, cx - h * 0.6f, cy + h, cx + h * 0.8f, cy);
                g.setColour ((hl ? Colours::white : cText.withAlpha (0.6f)).withMultipliedAlpha (a));
                g.fillPath (t);
            }
        }
        if (statusAlpha > 0.01f)
        {
            g.setColour (cAccent.brighter (0.35f).withAlpha (statusAlpha));
            g.drawFittedText (status, p.reduced (p.getHeight() * 0.5f, 0).toNearestInt(), Justification::centred, 1, 0.6f);
        }
    }

    //-------------------------------------------------------------- zone haute : mesure, ratios, commutateurs
    paintPanel (g, R (70, 108, 1460, 178));

    // BYPASS
    paintLed (g, { 144 * s, 160 * s }, 7.0f, cRed, bypassLed);
    paintButton (g, bypassBtn(), "BYPASS", pressed == 1);

    // réduction de gain : 16 voyants
    g.setFont (font (1, 13 * s));
    for (int i = 0; i < 16; ++i)
    {
        g.setColour (cText);
        g.drawText (String ((int) kGrThr[i]), Rectangle<float> (36 * s, 16 * s).withCentre ({ grLedX (i) * s, 136 * s }), Justification::centred, false);
        paintLed (g, { grLedX (i) * s, 162 * s }, 7.5f, grColour (i), grLed[i]);
    }
    g.setColour (cText);
    g.setFont (font (2, 13 * s).withExtraKerningFactor (0.10f));
    g.drawText ("GAIN REDUCTION  dB", R (grLedX (0) - 20, 184, grLedX (15) - grLedX (0) + 40, 18), Justification::centred, false);

    // ratios : 8 voyants (un clic sur un voyant choisit le ratio)
    const int ratio = jlimit (0, 7, (int) proc.pRatio->load());
    g.setColour (cLine.withAlpha (0.45f));
    g.drawLine (R (ratioLedX (0) - 26, 213, 0, 0).getX(), 213 * s, R (ratioLedX (7) + 26, 0, 0, 0).getX(), 213 * s, 1.2f * s);
    for (int i = 0; i < 8; ++i)
    {
        const float x = ratioLedX (i);
        paintLed (g, { x * s, 232 * s }, 7.0f, i == 7 ? cCyan : cRed, i == ratio ? 1.0f : 0.0f);
        g.setColour (i == ratio ? cTextHi : cText);
        g.setFont (font (i == 7 ? 2 : 1, 14 * s));
        g.drawText (GrinderProcessor::ratioNames()[i], Rectangle<float> (70 * s, 18 * s).withCentre ({ x * s, 256 * s }), Justification::centred, false);
        if (i == 5)
        {
            g.setColour (cDim);
            g.setFont (font (1, 10 * s).withExtraKerningFactor (0.08f));
            g.drawText ("OPTO", Rectangle<float> (70 * s, 12 * s).withCentre ({ x * s, 271 * s }), Justification::centred, false);
        }
    }
    paintButton (g, ratioBtn(), "RATIO", pressed == 2);

    // voyants de distorsion
    {
        const float th = proc.thdPct.load();
        ignoreUnused (th);
        paintLed (g, { 920 * s, 140 * s }, 6.5f, cYellow, thd1);
        paintLed (g, { 920 * s, 172 * s }, 6.5f, cRed, thdRed);
        g.setColour (cText);
        g.setFont (font (1, 12.5f * s).withExtraKerningFactor (0.04f));
        g.drawText ("1% THD", R (936, 132, 100, 16), Justification::centredLeft, false);
        g.drawText ("REDLINE", R (936, 164, 100, 16), Justification::centredLeft, false);
    }

    // DETECTOR : HP, BAND, LINK
    {
        const bool v[3] = { proc.pDetHp->load() > 0.5f, proc.pDetBand->load() > 0.5f, proc.pLink->load() > 0.5f };
        const Colour c[3] = { cGreen, cYellow, cRed };
        static const char* t[3] = { "HP", "BAND", "LINK" };
        for (int i = 0; i < 3; ++i)
        {
            const float y = 149.0f + 28.0f * (float) i;
            paintLed (g, { 1084 * s, y * s }, 6.5f, c[i], v[i] ? 1.0f : 0.0f);
            g.setColour (v[i] ? cTextHi : cText);
            g.setFont (font (1, 13 * s));
            g.drawText (t[i], R (1100, y - 9, 80, 18), Justification::centredLeft, false);
        }
        paintButton (g, detBtn(), "DETECTOR", pressed == 3);
    }
    // AUDIO : HP, DIST 2, DIST 3
    {
        const int d = (int) proc.pDist->load();
        const bool v[3] = { proc.pAudioHp->load() > 0.5f, d == 1, d == 2 };
        const Colour c[3] = { cGreen, cYellow, cRed };
        static const char* t[3] = { "HP", "DIST 2", "DIST 3" };
        for (int i = 0; i < 3; ++i)
        {
            const float y = 149.0f + 28.0f * (float) i;
            paintLed (g, { 1236 * s, y * s }, 6.5f, c[i], v[i] ? 1.0f : 0.0f);
            g.setColour (v[i] ? cTextHi : cText);
            g.setFont (font (1, 13 * s));
            g.drawText (t[i], R (1252, y - 9, 80, 18), Justification::centredLeft, false);
        }
        paintButton (g, audioBtn(), "AUDIO", pressed == 4);
    }
    // séparations sérigraphiées entre les groupes
    g.setColour (cLine.withAlpha (0.30f));
    for (float x : { 214.0f, 1040.0f, 1192.0f, 1350.0f })
        g.drawLine (x * s, 124 * s, x * s, 270 * s, 1.2f * s);

    // POWER
    g.setColour (cText);
    g.setFont (font (2, 13 * s).withExtraKerningFactor (0.08f));
    g.drawText ("POWER", R (1372, 126, 120, 18), Justification::centred, false);
    paintRocker (g);

    //-------------------------------------------------------------- zone basse : potards
    paintPanel (g, R (70, 300, 1460, 226));
    auto faceAo = [&] (float cx, float cy, float rr)
    {
        // occlusion autour du pied du potard, puis ombre portée douce vers le bas à droite (45°)
        const Point<float> c (cx * s, cy * s);
        const float R0 = rr * 1.10f * s;
        ColourGradient ao (Colours::black.withAlpha (0.55f), c.x, c.y, Colours::transparentBlack, c.x + R0 * 1.35f, c.y, true);
        ao.addColour (0.70, Colours::black.withAlpha (0.40f));
        g.setGradientFill (ao);
        g.fillEllipse (Rectangle<float> (R0 * 2.7f, R0 * 2.7f).withCentre (c));
        const float off = rr * 0.16f * s;
        const Point<float> sc (c.x + off, c.y + off);
        ColourGradient sh (Colours::black.withAlpha (0.50f), sc.x, sc.y, Colours::transparentBlack, sc.x + R0 * 1.55f, sc.y, true);
        sh.addColour (0.62, Colours::black.withAlpha (0.30f));
        g.setGradientFill (sh);
        g.fillEllipse (Rectangle<float> (R0 * 3.1f, R0 * 3.1f).withCentre (sc));
    };
    for (float x : { 360.0f, 600.0f, 840.0f, 1080.0f }) faceAo (x, 404, 60);
    faceAo (1256, 404, 34);
    for (float x : { 360.0f, 600.0f, 840.0f, 1080.0f }) paintScale (g, { x * s, 404 * s }, 60);
    g.setColour (cTextHi);
    g.setFont (font (2, 17 * s).withExtraKerningFactor (0.10f));
    g.drawText ("INPUT",   R (260, 494, 200, 22), Justification::centred, false);
    g.drawText ("ATTACK",  R (500, 494, 200, 22), Justification::centred, false);
    g.drawText ("RELEASE", R (740, 494, 200, 22), Justification::centred, false);
    g.drawText ("OUTPUT",  R (980, 494, 200, 22), Justification::centred, false);
    // MIX : DRY / COMP cliquables
    g.drawText ("MIX", R (1156, 494, 200, 22), Justification::centred, false);
    g.setFont (font (1, 13 * s));
    g.setColour (hoverZone == 3 ? Colours::white : cText);
    g.drawText ("DRY", dryLabel(), Justification::centred, false);
    g.setColour (hoverZone == 4 ? Colours::white : cText);
    g.drawText ("COMP", compLabel(), Justification::centred, false);

    // HR
    g.setColour (hoverZone == 5 ? Colours::white : cTextHi);
    g.setFont (font (2, 15 * s).withExtraKerningFactor (0.10f));
    g.drawText ("HR", hrLabel(), Justification::centred, false);
    paintScrew (g);
    g.setColour (cText);
    g.setFont (font (1, 13 * s));
    g.drawText (GrinderProcessor::hrNames()[jlimit (0, 6, (int) proc.pHr->load())], R (90, 440, 100, 18), Justification::centred, false);
    g.setColour (cDim);
    g.setFont (font (1, 10.5f * s).withExtraKerningFactor (0.10f));
    g.drawText ("HEADROOM", R (90, 494, 100, 22), Justification::centred, false);
    g.setColour (cLine.withAlpha (0.30f));
    g.drawLine (230 * s, 320 * s, 230 * s, 510 * s, 1.2f * s);
    g.drawLine (1340 * s, 320 * s, 1340 * s, 510 * s, 1.2f * s);

    // options (Brit, Image Link)
    g.setColour (cDim);
    g.setFont (font (2, 11 * s).withExtraKerningFactor (0.14f));
    g.drawText ("X MODS", R (1376, 322, 120, 18), Justification::centred, false);
    paintButton (g, britBtn(), "BRIT", pressed == 5);
    paintButton (g, imageBtn(), "IMAGE LINK", pressed == 6);
    paintLed (g, { 1514 * s, britBtn().getCentreY() }, 5.5f, cAccent, britLed);
    paintLed (g, { 1514 * s, imageBtn().getCentreY() }, 5.5f, cAccent, imageLed);
}

void GrinderEditor::paint (Graphics& g)
{
    lastPaint = nowS(); pendingSince = 0;
    g.fillAll (Colour (0xff0b0c0e));
    paintFace (g);
}

//==============================================================================
//  Commutateurs
//==============================================================================
int GrinderEditor::detectorState (bool hp, bool band, bool link)
{
    // ordre du cycle : Normal, HP, BAND, LINK, HP+BAND, HP+LINK, BAND+LINK, HP+BAND+LINK
    static const int table[8] = { 0, 1, 2, 4, 3, 5, 6, 7 };   // index par masque (hp=1, band=2, link=4)
    return table[(hp ? 1 : 0) | (band ? 2 : 0) | (link ? 4 : 0)];
}

int GrinderEditor::audioState (bool hp, int dist)
{
    // ordre du cycle : Normal, HP, DIST 2, DIST 3, HP+DIST 2, HP+DIST 3
    if (! hp) return dist == 0 ? 0 : dist == 1 ? 2 : 3;
    return dist == 0 ? 1 : dist == 1 ? 4 : 5;
}

void GrinderEditor::setBool (const char* id, bool v)
{
    if (auto* p = proc.apvts.getParameter (id))
    {
        if ((p->getValue() > 0.5f) == v) return;
        p->beginChangeGesture(); p->setValueNotifyingHost (v ? 1.0f : 0.0f); p->endChangeGesture();
        proc.presetDirty = true;
    }
}

void GrinderEditor::toggleParam (const char* id)
{
    if (auto* p = proc.apvts.getParameter (id)) setBool (id, p->getValue() < 0.5f);
}

void GrinderEditor::setChoice (const char* id, int idx)
{
    proc.setParam (id, (float) idx);
    proc.presetDirty = true;
}

void GrinderEditor::cycleRatio (int dir)  { setChoice ("ratio", ((int) proc.pRatio->load() + dir + 8) % 8); }

void GrinderEditor::cycleDetector (int dir)
{
    static const int masks[8] = { 0, 1, 2, 4, 3, 5, 6, 7 };   // masque pour chaque état du cycle
    const int st = detectorState (proc.pDetHp->load() > 0.5f, proc.pDetBand->load() > 0.5f, proc.pLink->load() > 0.5f);
    const int m = masks[(st + dir + 8) % 8];
    setBool ("dethp", (m & 1) != 0); setBool ("detband", (m & 2) != 0); setBool ("link", (m & 4) != 0);
}

void GrinderEditor::cycleAudio (int dir)
{
    static const bool hp[6] = { false, true, false, false, true, true };
    static const int dist[6] = { 0, 0, 1, 2, 1, 2 };
    const int st = (audioState (proc.pAudioHp->load() > 0.5f, (int) proc.pDist->load()) + dir + 6) % 6;
    setBool ("audiohp", hp[st]); setChoice ("dist", dist[st]);
}

void GrinderEditor::stepHr (int dir)
{
    const int v = jlimit (0, 6, (int) proc.pHr->load() + dir);
    if (v != (int) proc.pHr->load()) setChoice ("hr", v);
}

RangedAudioParameter* GrinderEditor::paramAt (Point<float> p) const
{
    auto& ap = proc.apvts;
    if (bypassBtn().contains (p)) return ap.getParameter ("bypass");
    if (powerRect().contains (p)) return ap.getParameter ("power");
    if (ratioBtn().contains (p)) return ap.getParameter ("ratio");
    for (int i = 0; i < 8; ++i) if (ratioHit (i).contains (p)) return ap.getParameter ("ratio");
    if (detHit (0).contains (p)) return ap.getParameter ("dethp");
    if (detHit (1).contains (p)) return ap.getParameter ("detband");
    if (detHit (2).contains (p)) return ap.getParameter ("link");
    if (audioHit (0).contains (p)) return ap.getParameter ("audiohp");
    if (audioHit (1).contains (p) || audioHit (2).contains (p)) return ap.getParameter ("dist");
    if (britBtn().contains (p)) return ap.getParameter ("brit");
    if (imageBtn().contains (p)) return ap.getParameter ("image");
    if (hrRect().contains (p) || hrLabel().contains (p)) return ap.getParameter ("hr");
    return nullptr;
}

//==============================================================================
//  Souris
//==============================================================================
void GrinderEditor::mouseMove (const MouseEvent& e)
{
    const auto p = e.position;
    const int z = pillPrev().contains (p) ? 1 : pillNext().contains (p) ? 2 : dryLabel().contains (p) ? 3 : compLabel().contains (p) ? 4
                : hrLabel().contains (p) ? 5 : 0;
    if (z != hoverZone) { hoverZone = z; repaint(); }
}

void GrinderEditor::mouseDown (const MouseEvent& e)
{
    const auto p = e.position;
    if (e.mods.isPopupMenu())
    {
        if (auto* prm = paramAt (p)) showParamMidiMenu (prm);
        else showMainMenu();
        return;
    }
    const int dir = e.mods.isShiftDown() ? -1 : 1;
    if (pill().contains (p))
    {
        if (statusAlpha > 0.05f) { statusTime = -10.0; return; }
        if (pillPrev().contains (p)) prevNextPreset (-1);
        else if (pillNext().contains (p)) prevNextPreset (1);
        else
        {
            PopupMenu m; m.setLookAndFeel (&menuLnf);
            GrMenu::addHeader (m, fonts, GrMenu::icoPreset, cAccent, "Presets", proc.presetName);
            const auto& fp = GrinderProcessor::factoryPresets();
            for (int i = 0; i < (int) fp.size(); ++i)
                m.addItem (i + 1, fp[(size_t) i].name, true, proc.currentPreset.load() == i && ! proc.presetDirty.load());
            m.addSeparator();
            m.addItem (900, "Save preset...");
            m.addItem (901, "Load preset...");
            m.showMenuAsync (PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (pill().toNearestInt())), [this] (int r)
            {
                if (r >= 1 && r < 900) proc.loadFactoryPreset (r - 1);
                else if (r == 900) savePreset();
                else if (r == 901) loadPreset();
                repaint();
            });
        }
        return;
    }
    if (bypassBtn().contains (p)) { pressed = 1; toggleParam ("bypass"); repaint(); return; }
    if (powerRect().contains (p)) { toggleParam ("power"); repaint(); return; }
    if (ratioBtn().contains (p))  { pressed = 2; cycleRatio (dir); repaint(); return; }
    for (int i = 0; i < 8; ++i)
        if (ratioHit (i).contains (p)) { setChoice ("ratio", i); repaint(); return; }   // choix direct, sans passer par les autres ratios
    if (detBtn().contains (p))    { pressed = 3; cycleDetector (dir); repaint(); return; }
    if (audioBtn().contains (p))  { pressed = 4; cycleAudio (dir); repaint(); return; }
    if (detHit (0).contains (p))  { toggleParam ("dethp"); repaint(); return; }
    if (detHit (1).contains (p))  { toggleParam ("detband"); repaint(); return; }
    if (detHit (2).contains (p))  { toggleParam ("link"); repaint(); return; }
    if (audioHit (0).contains (p)) { toggleParam ("audiohp"); repaint(); return; }
    if (audioHit (1).contains (p)) { setChoice ("dist", (int) proc.pDist->load() == 1 ? 0 : 1); repaint(); return; }
    if (audioHit (2).contains (p)) { setChoice ("dist", (int) proc.pDist->load() == 2 ? 0 : 2); repaint(); return; }
    if (britBtn().contains (p))   { pressed = 5; toggleParam ("brit"); repaint(); return; }
    if (imageBtn().contains (p))  { pressed = 6; toggleParam ("image"); repaint(); return; }
    if (hrLabel().contains (p))   { setChoice ("hr", 3); showStatus ("HEADROOM   16 dB"); repaint(); return; }   // clic sur « HR » : retour à 16 dB
    if (hrRect().contains (p))    { hrDragging = true; hrDragStart = (float) proc.pHr->load(); hrDragAcc = 0; return; }
    if (dryLabel().contains (p))  { proc.setParam ("mix", 0.0f); proc.presetDirty = true; return; }
    if (compLabel().contains (p)) { proc.setParam ("mix", 100.0f); proc.presetDirty = true; return; }
}

void GrinderEditor::mouseDrag (const MouseEvent& e)
{
    if (! hrDragging) return;
    const int steps = (int) std::round (-(float) e.getDistanceFromDragStartY() / (14.0f * jmax (0.6f, sc())));
    const int v = jlimit (0, 6, (int) hrDragStart + steps);
    if (v != (int) proc.pHr->load()) { setChoice ("hr", v); repaint(); }
}

void GrinderEditor::mouseUp (const MouseEvent&)
{
    hrDragging = false;
    if (pressed != 0) { pressed = 0; repaint(); }
}

void GrinderEditor::mouseDoubleClick (const MouseEvent& e)
{
    if (hrRect().contains (e.position) && ! e.mods.isPopupMenu()) { setChoice ("hr", 3); repaint(); }
}

void GrinderEditor::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    if (hrRect().contains (e.position)) { stepHr (w.deltaY >= 0 ? 1 : -1); repaint(); }
}

void GrinderEditor::prevNextPreset (int dir)
{
    const int n = (int) GrinderProcessor::factoryPresets().size();
    const int cur = proc.currentPreset.load();
    proc.loadFactoryPreset (((cur < 0 ? 0 : cur) + dir + n) % n);
    repaint();
}

static File presetFolder()
{
    auto f = File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("Nine Hands DSP").getChildFile ("Grinder");
    f.createDirectory();
    return f;
}

void GrinderEditor::savePreset()
{
    chooser = std::make_unique<FileChooser> ("Save Grinder preset", presetFolder().getChildFile (proc.presetName + ".grinder"), "*.grinder");
    chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
                          [this] (const FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f == File()) return;
        f = f.withFileExtension ("grinder");
        if (auto xml = proc.savePresetTree().createXml()) xml->writeTo (f);
        proc.presetName = f.getFileNameWithoutExtension();
        proc.presetDirty = false; proc.currentPreset = -1;
        repaint();
    });
}

void GrinderEditor::loadPreset()
{
    chooser = std::make_unique<FileChooser> ("Load Grinder preset", presetFolder(), "*.grinder");
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc)
    {
        const auto f = fc.getResult();
        if (! f.existsAsFile()) return;
        if (auto xml = XmlDocument::parse (f))
            proc.loadPresetTree (ValueTree::fromXml (*xml), f.getFileNameWithoutExtension());
        repaint();
    });
}

void GrinderEditor::showStatus (const String& s) { status = s; statusTime = nowS(); }

//------------------------------------------------------------------ menus (toujours ouverts là où on clique)
void GrinderEditor::showParamMidiMenu (RangedAudioParameter* p)
{
    auto* wp = dynamic_cast<AudioProcessorParameterWithID*> (p);
    if (wp == nullptr) return;
    const int pi = proc.paramIndexOf (wp->paramID);
    if (pi < 0) return;
    const int cc = proc.ccForParam (pi);
    const String name = p->getName (40);
    PopupMenu m; m.setLookAndFeel (&menuLnf);
    GrMenu::addHeader (m, fonts, GrMenu::icoMidi, cAccent, name, cc >= 0 ? "MIDI: CC " + String (cc) : String ("MIDI: not assigned"));
    m.addItem (1, "MIDI Learn: " + name, true, proc.learnParam.load() == pi);
    m.addItem (2, "Clear MIDI: " + name, cc >= 0);
    m.addItem (3, "Cancel MIDI learn", proc.learnParam.load() >= 0);
    m.showMenuAsync (PopupMenu::Options().withMousePosition(), [this, pi] (int r)
    {
        if (r == 1) proc.learnParam = pi;
        else if (r == 2) proc.clearMidiFor (pi);
        else if (r == 3) proc.learnParam = -1;
    });
}

static const char* kLearnIds[] = { "input", "attack", "release", "output", "mix", "ratio", "dethp", "detband", "link",
                                   "audiohp", "dist", "hr", "brit", "image", "bypass", "power" };
static constexpr int kNumLearn = (int) (sizeof (kLearnIds) / sizeof (kLearnIds[0]));

PopupMenu GrinderEditor::buildContextMenu()
{
    PopupMenu m; m.setLookAndFeel (&menuLnf);
    GrMenu::addHeader (m, fonts, GrMenu::icoEq, cAccent, "Grinder", "Classic knee compressor");
    PopupMenu fact, settings, ratio, hr, midi, size, gl;
    const auto& fp = GrinderProcessor::factoryPresets();
    for (int i = 0; i < (int) fp.size(); ++i) fact.addItem (1 + i, fp[(size_t) i].name, true, proc.currentPreset.load() == i && ! proc.presetDirty.load());
    m.addSubMenu ("Factory presets", fact);
    m.addItem (90, "Load user preset...");
    m.addItem (91, "Save user preset...");
    m.addSeparator();
    for (int i = 0; i < 8; ++i) ratio.addItem (150 + i, GrinderProcessor::ratioNames()[i], true, (int) proc.pRatio->load() == i);
    settings.addSubMenu ("Ratio", ratio);
    for (int i = 0; i < 7; ++i) hr.addItem (160 + i, GrinderProcessor::hrNames()[i] + (i == 3 ? "  (default)" : ""), true, (int) proc.pHr->load() == i);
    settings.addSubMenu ("Headroom (HR)", hr);
    settings.addItem (100, "Oversampling 4x", true, proc.pOs->load() > 0.5f);
    settings.addItem (101, "Brit mode", true, proc.pBrit->load() > 0.5f);
    settings.addItem (102, "Image link", true, proc.pImage->load() > 0.5f);
    m.addSubMenu ("Settings", settings);
    m.addSeparator();
    midi.addSectionHeader ("MIDI LEARN");
    for (int i = 0; i < kNumLearn; ++i)
    {
        const int pi = proc.paramIndexOf (kLearnIds[i]);
        const int cc = proc.ccForParam (pi);
        midi.addItem (200 + i, "Learn: " + proc.getParameters()[pi]->getName (40) + (cc >= 0 ? "  (CC " + String (cc) + ")" : String ("  (not assigned)")), true, proc.learnParam.load() == pi);
    }
    midi.addSeparator();
    midi.addItem (250, "Cancel MIDI learn", proc.learnParam.load() >= 0);
    midi.addItem (251, "Clear all MIDI assignments");
    m.addSubMenu ("MIDI", midi);
    m.addSeparator();
    static const char* sz[3] = { "60 %", "75 %", "100 %" };
    const int cur = jmax (0, proc.uiSizeIdx.load());
    for (int i = 0; i < 3; ++i) size.addItem (130 + i, sz[i], true, cur == i);
    m.addSubMenu ("Plugin size", size);
    for (int i = 0; i < 4; ++i) gl.addItem (140 + i, "Glow " + String ((i + 1) * 25) + " %" + (i == 3 ? "  (default)" : ""), true, proc.glowIdx.load() == i);
    m.addSubMenu ("Glow intensity", gl);
    return m;
}

void GrinderEditor::showMainMenu()
{
    buildContextMenu().showMenuAsync (PopupMenu::Options().withMousePosition(), [this] (int r) { handleMainMenu (r); });
}

void GrinderEditor::handleMainMenu (int r)
{
    if (r >= 1 && r < 90) proc.loadFactoryPreset (r - 1);
    else if (r == 90) loadPreset();
    else if (r == 91) savePreset();
    else if (r == 100) toggleParam ("os");
    else if (r == 101) toggleParam ("brit");
    else if (r == 102) toggleParam ("image");
    else if (r >= 150 && r < 158) setChoice ("ratio", r - 150);
    else if (r >= 160 && r < 167) setChoice ("hr", r - 160);
    else if (r >= 200 && r < 200 + kNumLearn) proc.learnParam = proc.paramIndexOf (kLearnIds[r - 200]);
    else if (r == 250) proc.learnParam = -1;
    else if (r == 251) { for (auto& x : proc.ccMap) x = -1; proc.learnParam = -1; }
    else if (r >= 130 && r < 133) { proc.uiSizeIdx = r == 130 ? -1 : r - 130; setScaleIndex (r - 130); }
    else if (r >= 140 && r < 144) { proc.glowIdx = r - 140; showStatus ("GLOW  " + String ((r - 139) * 25) + " %"); }
    repaint();
}

//------------------------------------------------------------------ anti-gel v2
void GrinderEditor::useSoftwareRenderer()
{
    auto* peer = getPeer();
    if ((void*) peer != lastPeer) { lastPeer = (void*) peer; repaint(); }
    lastRendererCheck = nowS();
   #if JUCE_WINDOWS
    if (peer != nullptr)
    {
        int sw = peer->getAvailableRenderingEngines().indexOf ("Software Renderer");
        if (sw < 0) sw = 0;
        if (peer->getCurrentRenderingEngine() != sw) peer->setCurrentRenderingEngine (sw);
    }
   #endif
}

void GrinderEditor::trackHostWindow()
{
    if (auto* peer = getPeer())
    {
        void* handle = peer->getNativeHandle();
        if (handle != nullptr && ! hostLock.isAttachedTo (handle))
        {
            Component::SafePointer<Component> top (&peer->getComponent());
            hostLock.attach (handle, [top] { return top != nullptr ? top->getWidth() : 0; });
            hostLock.setMinimumSize ((int) std::round (kRefW * kDefaultScale), (int) std::round (kRefH * kDefaultScale));
            hostLock.setMaximumSize ((int) std::round (kRefW * kMaxScale), (int) std::round (kRefH * kMaxScale));
            hostLock.enforceNow();
        }
        if (getWidth() < (int) (kRefW * kDefaultScale) - 2) hostLock.enforceNow();
    }
    else hostLock.detach();
}

void GrinderEditor::parentHierarchyChanged() { useSoftwareRenderer(); trackHostWindow(); }
void GrinderEditor::visibilityChanged()      { useSoftwareRenderer(); if (isShowing()) repaint(); }

void GrinderEditor::askRepaint (Rectangle<int> r)
{
    if (pendingSince <= 0.0) pendingSince = nowS();
    repaint (r);
}

//------------------------------------------------------------------ 30 Hz
void GrinderEditor::timerCallback()
{
    const double t = nowS();
    if ((void*) getPeer() != lastPeer || t - lastRendererCheck > 1.0) { useSoftwareRenderer(); trackHostWindow(); }
    if (pendingSince > 0.0 && t - pendingSince > 1.5 && isShowing())                 // chien de garde
    {
        lastPeer = nullptr; useSoftwareRenderer(); repaint(); pendingSince = 0.0;
    }

    // réglages qui bougent (souris, automation, MIDI) -> ligne d'état
    auto& params = proc.getParameters();
    bool changed = false;
    for (int i = 0; i < params.size() && i < (int) lastParamValues.size(); ++i)
    {
        const float v = params[i]->getValue();
        if (std::abs (v - lastParamValues[(size_t) i]) > 0.0005f)
        {
            lastParamValues[(size_t) i] = v;
            showStatus (params[i]->getName (40).toUpperCase() + "   " + params[i]->getCurrentValueAsText());
            changed = true;
        }
    }
    if (changed || proc.glowIdx.load() != lastGlow) { lastGlow = proc.glowIdx.load(); repaint(); }

    // MIDI Learn
    const int lp = proc.learnParam.load();
    if (lp >= 0) showStatus ("MIDI LEARN  " + params[lp]->getName (40));
    const int ll = proc.lastLearnedCc.exchange (-1);
    if (ll >= 0)
    {
        const int pi = proc.ccMap[ll].load();
        if (pi >= 0) showStatus ("CC " + String (ll) + "  " + params[pi]->getName (40));
    }

    // voyants de réduction de gain : montée immédiate, retombée douce (barre à LED rapide, plus fine que 1 dB)
    {
        const float gr = proc.grDb.load();
        grShown = gr > grShown ? gr : grShown + (gr - grShown) * 0.35f;
        bool dirty = false;
        for (int i = 0; i < 16; ++i)
        {
            const float thr = kGrThr[i];
            const float lo = i < 15 ? kGrThr[i + 1] : 0.0f;      // voyant suivant (plus faible)
            const float lvl = jlimit (0.0f, 1.0f, (grShown - (thr - 0.5f * (thr - lo))) / (0.5f * (thr - lo) + 0.01f));
            if (std::abs (lvl - grLed[i]) > 0.01f) { grLed[i] = lvl; dirty = true; }
        }
        if (dirty) askRepaint (R (240, 145, 660, 36).toNearestInt());
    }
    // voyants de distorsion (maintenus 150 ms)
    {
        const float thd = proc.thdPct.load();
        if (thd >= 1.0f) thd1Hold = t + 0.15;
        if (thd >= 3.0f || proc.clipped.load()) redHold = t + 0.15;
        const float a1 = t < thd1Hold ? jmin (1.0f, thd1 + 0.5f) : jmax (0.0f, thd1 - 0.25f);
        const float a2 = t < redHold ? jmin (1.0f, thdRed + 0.5f) : jmax (0.0f, thdRed - 0.25f);
        if (std::abs (a1 - thd1) > 0.001f || std::abs (a2 - thdRed) > 0.001f) { thd1 = a1; thdRed = a2; askRepaint (R (900, 120, 40, 70).toNearestInt()); }
    }

    // fondus : ligne d'état, voyants
    const float target = (t - statusTime < 2.5) ? 1.0f : 0.0f;
    const float na = target > statusAlpha ? jmin (1.0f, statusAlpha + 0.28f) : jmax (0.0f, statusAlpha - 0.11f);
    if (std::abs (na - statusAlpha) > 0.001f) { statusAlpha = na; askRepaint (pill().expanded (4).toNearestInt()); }
    auto fade = [] (float& v, bool on) { const float n = on ? jmin (1.0f, v + 0.45f) : jmax (0.0f, v - 0.28f); const bool ch = std::abs (n - v) > 0.001f; v = n; return ch; };
    bool lights = false;
    lights |= fade (powerLevel, proc.pPower->load() > 0.5f);
    lights |= fade (bypassLed, proc.pBypass->load() > 0.5f);
    lights |= fade (britLed, proc.pBrit->load() > 0.5f);
    lights |= fade (imageLed, proc.pImage->load() > 0.5f);
    if (lights) repaint();

    for (auto* k : { &inputKnob, &attackKnob, &releaseKnob, &outputKnob, &mixKnob }) k->tick();
    pills.update();
}
