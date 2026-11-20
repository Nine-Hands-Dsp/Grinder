// Outil hors plugin (tools/render) : mesures du moteur de Grinder et captures de l'interface
#include "PluginEditor.h"
#include <random>
using namespace juce;

static void snap (Component& c, const String& name, float scale)
{
    auto img = c.createComponentSnapshot (c.getLocalBounds(), true, scale);
    File out = File::getCurrentWorkingDirectory().getChildFile (name);
    out.deleteFile();
    FileOutputStream os (out); PNGImageFormat png; png.writeImageToStream (img, os);
    std::cout << "ecrit " << out.getFullPathName() << " (" << img.getWidth() << "x" << img.getHeight() << ")\n";
}

static void set (GrinderProcessor& p, const char* id, float v) { p.setParam (id, v); }

static void run (GrinderProcessor& p, AudioBuffer<float>& y)
{
    MidiBuffer m;
    for (int s0 = 0; s0 < y.getNumSamples(); s0 += 256)
    {
        AudioBuffer<float> blk (y.getArrayOfWritePointers(), 2, s0, jmin (256, y.getNumSamples() - s0));
        p.processBlock (blk, m);
    }
}

static AudioBuffer<float> tone (double f, std::vector<std::pair<double, double>> segs, double fs)
{
    int N = 0; for (auto& s : segs) N += (int) (s.first * fs);
    AudioBuffer<float> y (2, N);
    int n = 0;
    for (auto& s : segs)
    {
        const double a = Decibels::decibelsToGain (s.second, -200.0);
        for (int k = 0; k < (int) (s.first * fs); ++k, ++n)
        {
            const float v = (float) (a * std::sin (2 * MathConstants<double>::pi * f * n / fs));
            y.setSample (0, n, v); y.setSample (1, n, v);
        }
    }
    return y;
}

static double rmsDb (const AudioBuffer<float>& y, int a, int len)
{
    double s = 0; for (int i = a; i < a + len; ++i) s += (double) y.getSample (0, i) * y.getSample (0, i);
    return 10 * std::log10 (s / len + 1e-30) + 3.0103;
}

// THD d'un sinus de 1 kHz (DFT des harmoniques 2 à 9)
static double thd (const AudioBuffer<float>& y, int a, int len, double fs, double& h2, double& h3)
{
    auto mag = [&] (double f)
    {
        double re = 0, im = 0;
        for (int i = 0; i < len; ++i)
        {
            const double w = 0.5 - 0.5 * std::cos (2 * MathConstants<double>::pi * i / (len - 1));
            const double v = y.getSample (0, a + i) * w;
            re += v * std::cos (2 * MathConstants<double>::pi * f * i / fs);
            im -= v * std::sin (2 * MathConstants<double>::pi * f * i / fs);
        }
        return std::sqrt (re * re + im * im);
    };
    const double f1 = mag (1000);
    double hs = 0;
    for (int k = 2; k <= 9; ++k) { const double m = mag (1000.0 * k); hs += m * m; if (k == 2) h2 = m / f1 * 100; if (k == 3) h3 = m / f1 * 100; }
    return std::sqrt (hs) / f1 * 100.0;
}

int main()
{
    ScopedJuceInitialiser_GUI init;
    const double fs = 48000.0;
    GrinderProcessor p;
    p.prepareToPlay (fs, 256);
    MessageManager::getInstance()->runDispatchLoopUntil (50);
    if (std::getenv ("FLICKER") != nullptr)

    //------------------------------------------------------------------ scintillement de la ligne d'état : image par image
    {
        p.loadFactoryPreset (1);
        p.prepareToPlay (fs, 1600);
        std::unique_ptr<GrinderEditor> ed (dynamic_cast<GrinderEditor*> (p.createEditor()));
        ed->setSize (1600, 540); ed->setVisible (true);
        Image strip (Image::RGB, 420, 60 * 120, true);
        Graphics gs (strip);
        File log = File::getCurrentWorkingDirectory().getChildFile ("flicker.txt"); log.deleteFile();
        String txt;
        for (int f = 0; f < 120; ++f)
        {
            AudioBuffer<float> buf (2, 1600); MidiBuffer m;
            for (int i = 0; i < 1600; ++i) { const float x = 0.3f * std::sin (0.05f * (float) (f * 1600 + i)); buf.setSample (0, i, x); buf.setSample (1, i, x); }
            p.processBlock (buf, m);
            if (f >= 10 && f < 40) set (p, "attack", 3.0f + 0.07f * (float) (f - 10));   // potard qui tourne pendant 1 s
            ed->timerCallback();
            auto img = ed->createComponentSnapshot (Rectangle<int> (590, 26, 420, 48), true, 1.0f);
            gs.drawImageAt (img, 0, f * 60);
            // luminance moyenne et étendue horizontale du texte (pixels clairs)
            double sum = 0; int x0 = 999, x1 = -1;
            for (int y = 8; y < 40; ++y) for (int x = 40; x < 380; ++x) { auto c = img.getPixelAt (x, y); const float l = c.getBrightness(); sum += l; if (l > 0.55f) { x0 = jmin (x0, x); x1 = jmax (x1, x); } }
            txt << f << " alpha=" << String (ed->statusAlpha, 2) << " lum=" << String (sum / (32 * 340), 4) << " text=" << x0 << ".." << x1 << " status='" << ed->status << "'\n";
        }
        log.replaceWithText (txt);
        File out = File::getCurrentWorkingDirectory().getChildFile ("flicker_strip.png"); out.deleteFile();
        FileOutputStream os (out); PNGImageFormat png; png.writeImageToStream (strip, os);
        std::cout << "flicker.txt ecrit\n";
        return 0;
    }

    std::cout << "== courbes statiques (INPUT 5, attaque 5, release 5) : entree dBFS -> GR dB ==\n";
    for (int r = 0; r < 8; ++r)
    {
        set (p, "ratio", (float) r); set (p, "input", 5); set (p, "attack", 5); set (p, "release", 5);
        std::cout << String (GrinderProcessor::ratioNames()[r]).paddedRight (' ', 5);
        for (double lev = -40; lev <= 0.1; lev += 5)
        {
            p.prepareToPlay (fs, 256);
            auto y = tone (1000, { { 1.5, lev } }, fs);
            run (p, y);
            std::cout << "  " << (int) lev << ":" << String (p.grDb.load(), 1);
        }
        std::cout << "\n";
    }

    std::cout << "== temps d'attaque (6:1, saut de -40 a -6 dBFS) : temps pour atteindre 63 % de la GR finale ==\n";
    for (float a : { 0.0f, 2.0f, 5.0f, 8.0f, 10.0f })
    {
        set (p, "ratio", 4); set (p, "attack", a); set (p, "release", 5);
        p.prepareToPlay (fs, 64);
        // GR mesurée bloc par bloc de 16 échantillons
        AudioBuffer<float> y = tone (1000, { { 0.5, -40 }, { 0.3, -6 } }, fs);
        MidiBuffer m; std::vector<float> gr;
        for (int s0 = 0; s0 < y.getNumSamples(); s0 += 16)
        {
            AudioBuffer<float> blk (y.getArrayOfWritePointers(), 2, s0, jmin (16, y.getNumSamples() - s0));
            p.processBlock (blk, m); gr.push_back (p.grDb.load());
        }
        const int st = (int) (0.5 * fs / 16);
        const float fin = gr.back();
        int k = st; while (k < (int) gr.size() && gr[(size_t) k] < 0.632f * fin) ++k;
        std::cout << "  ATTACK " << a << " : " << String ((k - st) * 16 / fs * 1000.0, 2) << " ms (GR finale " << String (fin, 1) << " dB)\n";
    }

    std::cout << "== temps de relachement (saut de -6 a -40 dBFS) : temps pour retomber a 37 % ==\n";
    for (int r : { 4, 5, 7 })
        for (float rl : { 0.0f, 5.0f, 10.0f })
        {
            set (p, "ratio", (float) r); set (p, "attack", 3); set (p, "release", rl);
            p.prepareToPlay (fs, 64);
            AudioBuffer<float> y = tone (1000, { { 2.0, -6 }, { 8.0, -60 } }, fs);
            MidiBuffer m; std::vector<float> gr;
            for (int s0 = 0; s0 < y.getNumSamples(); s0 += 64)
            {
                AudioBuffer<float> blk (y.getArrayOfWritePointers(), 2, s0, jmin (64, y.getNumSamples() - s0));
                p.processBlock (blk, m); gr.push_back (p.grDb.load());
            }
            const int st = (int) (2.0 * fs / 64);
            const float g0 = gr[(size_t) st - 1];
            int k = st; while (k < (int) gr.size() && gr[(size_t) k] > 0.368f * g0) ++k;
            std::cout << "  " << GrinderProcessor::ratioNames()[r] << "  RELEASE " << rl << " : " << String ((k - st) * 64 / fs, 3) << " s (GR " << String (g0, 1) << " dB)\n";
        }

    std::cout << "== THD a 1 kHz (ratio 1:1, INPUT 5, HR 16) selon le niveau ==\n";
    for (int d = 0; d < 3; ++d)
    {
        set (p, "ratio", 0); set (p, "dist", (float) d); set (p, "input", 5); set (p, "output", 5); set (p, "hr", 3);
        std::cout << (d == 0 ? "CLEAN " : d == 1 ? "DIST 2" : "DIST 3");
        for (double lev : { -30.0, -20.0, -12.0, -6.0, 0.0 })
        {
            p.prepareToPlay (fs, 256);
            auto y = tone (1000, { { 0.6, lev } }, fs);
            run (p, y);
            double h2 = 0, h3 = 0;
            const double t = thd (y, (int) (0.3 * fs), 8192, fs, h2, h3);
            std::cout << "  " << (int) lev << ": " << String (t, 2) << "% (h2 " << String (h2, 2) << " h3 " << String (h3, 2) << ")";
        }
        std::cout << "\n";
    }
    set (p, "dist", 0);

    // gain unitaire à 1:1 et niveau de sortie
    {
        set (p, "ratio", 0); p.prepareToPlay (fs, 256);
        auto y = tone (1000, { { 0.5, -18 } }, fs); run (p, y);
        std::cout << "== 1:1, -18 dBFS -> " << String (rmsDb (y, (int) (0.3 * fs), 4800), 2) << " dBFS ==\n";
    }



    std::cout << "== commutations sans clic : saut maximal de la derivee seconde (sinus 200 Hz, -12 dBFS) ==\n";
    for (const char* id : { "dist", "audiohp", "dethp", "hr", "brit" })
    {
        set (p, "ratio", 4); set (p, "dist", 0); set (p, "audiohp", 0); set (p, "dethp", 0); set (p, "hr", 3); set (p, "brit", 0);
        p.prepareToPlay (fs, 256);
        auto y = tone (200, { { 1.0, -12 } }, fs);
        MidiBuffer m; int half = (int) (0.5 * fs);
        for (int s0 = 0; s0 < y.getNumSamples(); s0 += 256)
        {
            if (s0 == (half / 256) * 256) set (p, id, String (id) == "hr" ? 0.0f : String (id) == "dist" ? 2.0f : 1.0f);
            AudioBuffer<float> blk (y.getArrayOfWritePointers(), 2, s0, jmin (256, y.getNumSamples() - s0));
            p.processBlock (blk, m);
        }
        auto d2 = [&] (int a, int b) { double mx = 0; for (int i = a; i < b; ++i) mx = jmax (mx, (double) std::abs (y.getSample (0, i + 1) - 2 * y.getSample (0, i) + y.getSample (0, i - 1))); return mx; };
        std::cout << "  " << id << " : avant " << String (d2 (half - 4800, half - 512), 5) << "  pendant " << String (d2 (half - 256, half + 2400), 5)
                  << "  apres " << String (d2 (half + 9600, half + 14400), 5) << "\n";
    }
    set (p, "dist", 0); set (p, "audiohp", 0); set (p, "dethp", 0); set (p, "hr", 3); set (p, "brit", 0);

    std::cout << "== charge CPU : 20 s de bruit stereo a 48 kHz, blocs de 256 ==\n";
    for (int osOn : { 1, 0 })
    {
        set (p, "os", (float) osOn); set (p, "ratio", 4); set (p, "dist", 2); set (p, "dethp", 1); set (p, "detband", 1);
        p.prepareToPlay (fs, 256);
        AudioBuffer<float> y (2, (int) (20 * fs));
        std::mt19937 rng (5); std::normal_distribution<float> nd (0.0f, 0.2f);
        for (int c = 0; c < 2; ++c) for (int i = 0; i < y.getNumSamples(); ++i) y.setSample (c, i, nd (rng));
        const double t0 = Time::getMillisecondCounterHiRes();
        run (p, y);
        const double ms = Time::getMillisecondCounterHiRes() - t0;
        std::cout << "  OS " << (osOn ? "4x" : "off") << " : " << String (ms, 0) << " ms pour 20 s (" << String (ms / 200.0, 2) << " % d'un coeur)\n";
    }
    set (p, "os", 1); set (p, "dist", 0); set (p, "dethp", 0); set (p, "detband", 0);

    //------------------------------------------------------------------ captures
    {
        p.loadFactoryPreset (0);
        p.prepareToPlay (fs, 1600);
        std::unique_ptr<GrinderEditor> ed (dynamic_cast<GrinderEditor*> (p.createEditor()));
        ed->setSize (1600, 540);
        ed->setVisible (true);
        auto feed = [&] (int frames)
        {
            std::mt19937 rng (3); std::normal_distribution<float> nd (0.0f, 1.0f);
            float b0 = 0, b1 = 0, b2 = 0, ph = 0;
            for (int k = 0; k < frames; ++k)
            {
                AudioBuffer<float> buf (2, 1600); MidiBuffer m;
                for (int i = 0; i < 1600; ++i)
                {
                    const float w = nd (rng) * 0.08f;
                    b0 = 0.99765f * b0 + w * 0.0990460f; b1 = 0.96300f * b1 + w * 0.2965164f; b2 = 0.57000f * b2 + w * 1.0526913f;
                    ph += 110.0f / 48000.0f;
                    const float x = (b0 + b1 + b2 + w * 0.1848f) * 1.4f + 0.35f * std::sin (MathConstants<float>::twoPi * ph);
                    buf.setSample (0, i, x); buf.setSample (1, i, x);
                }
                p.processBlock (buf, m);
                ed->timerCallback();
            }
        };
        auto quiet = [&] { ed->statusTime = -10; ed->statusAlpha = 0; };
        set (p, "input", 6.5f); set (p, "dethp", 1); set (p, "dist", 1);
        feed (40); quiet(); feed (2); quiet();
        snap (*ed, "grinder_apercu.png", 1.0f);
        snap (*ed, "grinder_apercu_2x.png", 2.0f);
        ed->showStatus ("ATTACK   5.0  (1.2 ms)"); ed->statusAlpha = 1.0f; snap (*ed, "grinder_status.png", 1.0f);
    }
    return 0;
}
