#pragma once
#include <cmath>
#include <algorithm>

//==============================================================================
//  Grinder (NINE HANDS | DSP) : moteur du compresseur « classic knee »
//
//  Chaîne par canal (tout tourne dans le suréchantillonnage 4x) :
//    entrée (gain INPUT, étage non linéaire dont la marge dépend du HR)
//    -> détecteur (passe-haut 100 Hz 6 dB/oct, accentuation 6 kHz, liaison stéréo)
//    -> calcul de gain (8 courbes : 1:1 à NUKE, genoux paraboliques, détecteur opto séparé en 10:1)
//    -> attaque 50 µs à 30 ms, relâchement 50 ms à 3,5 s (jusqu'à 20 s en opto), forme qui dépend du ratio
//    -> VCA -> modes audio (passe-haut de Bessel 18 dB/oct, Dist 2 lampe, Dist 3 bande)
//    -> sortie (gain OUTPUT, étage non linéaire)
//  Le mode Brit (EL8-X) reprend le principe du « tous les boutons enfoncés » d'un 1176 :
//  surcompression au-delà du seuil, attaque plus vive, un peu plus d'harmonique 2.
//==============================================================================
namespace ds
{
    constexpr double kPi = 3.14159265358979323846;

    inline double dbToGain (double db) { return std::pow (10.0, db / 20.0); }
    inline double gainToDb (double g)  { return 20.0 * std::log10 (std::max (g, 1.0e-12)); }

    //------------------------------------------------------------------ réglages façade (0 à 10) -> grandeurs physiques
    inline double attackSeconds (double k, int ratio)
    {
        if (k <= 0.02 && ratio >= 1 && ratio <= 3) return 20.0e-6;              // à 0, en 2:1, 3:1 et 4:1 : encore plus rapide
        return 50.0e-6 * std::pow (600.0, std::clamp (k, 0.0, 10.0) / 10.0);    // 50 µs à 30 ms
    }
    inline double releaseSeconds (double k) { return 0.05 * std::pow (70.0, std::clamp (k, 0.0, 10.0) / 10.0); }   // 50 ms à 3,5 s
    inline double inputDb (double k)  { return 6.0 * (k - 5.0); }      // -30 à +30 dB, 5 = 0 dB
    inline double outputDb (double k) { return 5.0 * (k - 5.0) + 0.0; } // -25 à +25 dB, 5 = 0 dB

    //------------------------------------------------------------------ biquad (forme directe transposée)
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        void reset() { z1 = z2 = 0; }
        void identity() { b0 = 1; b1 = b2 = a1 = a2 = 0; }
        inline double process (double x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
        void set (double B0, double B1, double B2, double A0, double A1, double A2)
        { b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0; }
        void highPass (double f, double q, double fs)
        {
            const double w = 2 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
            set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
        }
        void highPass1 (double f, double fs)          // passe-haut du 1er ordre (6 dB/oct)
        {
            const double k = std::tan (kPi * f / fs);
            set (1, -1, 0, 1 + k, k - 1, 0);
        }
        void peak (double f, double q, double db, double fs)
        {
            const double A = std::pow (10.0, db / 40.0), w = 2 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
            set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
        }
    };

    //------------------------------------------------------------------ saturation douce (étages d'entrée et de sortie)
    // u / (1 + |u|^p)^(1/p) : linéaire au repos, plafond doux en ±1 ; asym > 0 ajoute un peu d'harmonique 2
    inline double softClip (double x, double ceil, double asym = 0.0)
    {
        const double u = x / ceil;
        const double v = u + asym * u * u;
        const double p = 2.6;
        return ceil * v / std::pow (1.0 + std::pow (std::abs (v), p), 1.0 / p);
    }

    //------------------------------------------------------------------ un canal du compresseur
    struct Params
    {
        int ratio = 4;             // 0 1:1, 1 2:1, 2 3:1, 3 4:1, 4 6:1, 5 10:1 opto, 6 20:1, 7 NUKE
        bool detHp = false, detBand = false, link = false, deadPatch = false;
        bool audioHp = false; int dist = 0;   // 0 off, 1 Dist 2, 2 Dist 3
        bool brit = false;
        double attack = 5, release = 5;
        double hr = 16;            // marge interne (dB) : 4 à 28
    };

    // seuil, largeur du genou (dB) et pente pour chaque ratio
    struct Curve { double ratio, knee, thresh; };
    inline Curve curveFor (int r)
    {
        switch (r)
        {
            case 0: return { 1.0,    0.0,  0.0 };
            case 1: return { 2.0,   30.0,  0.0 };     // genou parabolique très large (jusqu'à 30 dB)
            case 2: return { 3.0,   22.0,  0.0 };
            case 3: return { 4.0,   14.0,  0.0 };     // genou plus raide
            case 4: return { 6.0,   10.0,  0.0 };
            case 5: return { 10.0,   5.0, -1.0 };     // opto : genou court
            case 6: return { 20.0,   2.0,  0.0 };     // limitation dure
            default: return { 1000.0, 1.0, 2.0 };     // NUKE : mur de brique, seuil moyen
        }
    }

    // réduction de gain statique (dB, >= 0) pour un niveau détecté d (dB internes) au-dessus du seuil t
    inline double staticGr (double over, const Curve& c)
    {
        if (c.ratio <= 1.0) return 0.0;
        const double s = 1.0 - 1.0 / c.ratio, W = c.knee;
        if (W <= 0.0) return over > 0 ? s * over : 0.0;
        if (over <= -W * 0.5) return 0.0;
        if (over >= W * 0.5)  return s * over;
        const double z = over + W * 0.5;
        return s * z * z / (2.0 * W);                // genou parabolique
    }

    struct Channel
    {
        double fs = 192000.0;
        Biquad scHp, scBand, aHp1, aHp2, dcBlock;
        bool designedDetHp = false, designedBand = false, designedAudioHp = false;
        double gr = 0.0;                // réduction de gain lissée (dB)
        double optoMem = 0.0;           // mémoire de la cellule opto (0..1)
        double optoLevel = 0.0;         // niveau détecté par la cellule opto (lent)
        double peakEnv = 0.0;

        void prepare (double sampleRate)
        {
            fs = sampleRate;
            scHp.highPass1 (100.0, fs);
            scBand.peak (6000.0, 1.1, 11.0, fs);
            // passe-haut de Bessel du 3e ordre (-3 dB vers 65 Hz, -12 dB vers 30 Hz, 18 dB/oct)
            const double fc = 65.0;
            aHp1.highPass1 (fc / 0.9416, fs);
            aHp2.highPass (fc / 1.0305, 0.6911, fs);
            dcBlock.highPass1 (4.0, fs);
            reset();
        }
        void reset()
        {
            for (auto* b : { &scHp, &scBand, &aHp1, &aHp2, &dcBlock }) b->reset();
            gr = 0.0; optoMem = 0.0; optoLevel = 0.0; peakEnv = 0.0;
        }

        //-- étage d'entrée : renvoie le signal après l'ampli d'entrée ; err reçoit l'écart non linéaire
        static inline double inputStage (double x, double ceil, double& err)
        {
            const double y = softClip (x, ceil, 0.035);
            err += (y - x) * (y - x);
            return y;
        }

        //-- niveau du sidechain (signal redressé, avant liaison)
        inline double sidechain (double x, const Params& p)
        {
            double s = x;
            if (p.detHp) s = scHp.process (s);
            if (p.detBand) s = scBand.process (s);
            return std::abs (s);
        }

        //-- détecteur + calcul de gain : met à jour gr à partir du niveau redressé lvl ; renvoie le gain linéaire
        inline double computeGain (double lvl, const Params& p, double threshDb)
        {
            const Curve c = curveFor (p.ratio);
            const double att = attackSeconds (p.attack, p.ratio) * (p.brit ? 0.6 : 1.0);
            const double rel = releaseSeconds (p.release);
            double target = 0.0;

            if (p.ratio == 5)
            {
                // cellule opto : le détecteur suit le niveau avec l'inertie de la lumière (attaque ~ 1 à 30 ms)
                const double aO = 1.0 - std::exp (-1.0 / (std::max (att, 0.001) * fs));
                const double rO = 1.0 - std::exp (-1.0 / (0.06 * fs));
                optoLevel += (lvl - optoLevel) * (lvl > optoLevel ? aO : rO);
                const double d = gainToDb (optoLevel);
                target = staticGr (d - (threshDb + c.thresh), c);
            }
            else
            {
                const double d = gainToDb (lvl);
                target = staticGr (d - (threshDb + c.thresh), c);
            }
            if (p.brit)
            {
                // tous les boutons enfoncés : au-delà du seuil la réduction dépasse ce que demande le ratio
                const double over = gainToDb (lvl) - (threshDb - 4.0);
                if (over > 0) target = std::max (target, 0.0) + 0.32 * over;
            }

            // relâchement : forme qui dépend du ratio (et de la durée de compression en opto)
            double relT = rel;
            if (p.ratio == 5)
            {
                const double aM = 1.0 - std::exp (-1.0 / (1.5 * fs)), rM = 1.0 - std::exp (-1.0 / (6.0 * fs));
                const double busy = gr > 1.0 ? 1.0 : 0.0;
                optoMem += (busy - optoMem) * (busy > optoMem ? aM : rM);
                relT = std::min (20.0, rel * (1.0 + 2.5 * optoMem) + 0.08 * optoMem * gr);   // jusqu'à 20 s
            }
            else if (p.ratio >= 6)
                relT = rel * (0.22 + 0.78 * (1.0 - std::exp (-1.5 / std::max (0.25, gr))));  // NUKE / 20:1 : logarithmique, vif puis lent
            else
                relT = rel * (0.55 + 0.45 * std::exp (-gr / 12.0));                            // un peu plus vif sous forte compression

            const double aC = 1.0 - std::exp (-1.0 / (att * fs));
            const double rC = 1.0 - std::exp (-1.0 / (relT * fs));
            gr += (target - gr) * (target > gr ? aC : rC);
            if (gr < 1.0e-6) gr = 0.0;
            return dbToGain (-gr);
        }

        //-- après le VCA : modes audio, puis renvoie le signal ; err reçoit l'écart non linéaire des VCA de distorsion
        inline double audioModes (double y, const Params& p, double ceil, double& err)
        {
            if (p.audioHp) y = aHp2.process (aHp1.process (y));
            const double u = softClip (y / ceil, 1.0);   // les VCA plafonnent doucement : les polynômes restent monotones
            double v = u;
            if (p.dist == 1)          v = u + 0.62 * u * u - 0.08 * u * u * u;                    // Dist 2 : harmonique 2 (lampe)
            else if (p.dist == 2)     { const double g = 3.2; v = std::tanh (g * u) / g; }         // Dist 3 : harmonique 3 (bande)
            else                      v = u + 0.012 * u * u - 0.006 * u * u * u;                  // propre : résidu de la carte
            if (p.brit)               v += 0.08 * u * u;
            if (p.link && p.deadPatch) v += 0.05 * u * u + 0.03 * u * u * u;                       // Link sans second appareil (mono)
            const double out = dcBlock.process (v * ceil);
            const double e = (v - u) * ceil;
            err += e * e;
            return out;
        }
    };
}
