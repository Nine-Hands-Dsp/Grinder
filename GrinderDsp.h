#pragma once
#include <cmath>
#include <algorithm>

//==============================================================================
//  Grinder (NINE HANDS | DSP) : moteur du compresseur « classic knee »
//
//  Chaîne par canal (tout tourne dans le suréchantillonnage 4x) :
//    entrée (gain INPUT, étage non linéaire dont la marge dépend du HR)
//    -> détecteur (passe-haut 100 Hz 6 dB/oct, accentuation 6 kHz, liaison stéréo)
//       -> redresseur crête (retombée 5 ms : l'ondulation du grave reste, comme sur le matériel)
//    -> calcul de gain (8 courbes : 1:1 à NUKE, genoux paraboliques, détecteur opto séparé en 10:1)
//    -> attaque 50 µs à 30 ms, relâchement 50 ms à 3,5 s (jusqu'à 20 s en opto), forme qui dépend du ratio
//    -> VCA -> modes audio (passe-haut de Bessel 18 dB/oct, Dist 2 lampe, Dist 3 bande)
//    -> sortie (gain OUTPUT, étage non linéaire)
//
//  Optimisation : aucun pow / exp / tanh par échantillon. Les coefficients sont calculés une fois par bloc
//  (Block), les conversions dB passent par log / exp naturels, les saturations sont des fonctions rationnelles,
//  et chaque commutation (modes audio, filtres du détecteur, Brit, HR) passe par un fondu de 10 ms : aucun clic.
//==============================================================================
namespace ds
{
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kDbPerNeper = 8.6858896380650365;     // 20 / ln 10
    constexpr double kNeperPerDb = 0.11512925464970229;    // ln 10 / 20

    inline double dbToGain (double db) { return std::exp (db * kNeperPerDb); }
    inline double gainToDb (double g)  { return kDbPerNeper * std::log (std::max (g, 1.0e-9)); }

    //------------------------------------------------------------------ réglages façade (0 à 10) -> grandeurs physiques
    inline double attackSeconds (double k, int ratio)
    {
        if (k <= 0.02 && ratio >= 1 && ratio <= 3) return 20.0e-6;              // à 0, en 2:1, 3:1 et 4:1 : encore plus rapide
        return 50.0e-6 * std::pow (600.0, std::clamp (k, 0.0, 10.0) / 10.0);    // 50 µs à 30 ms
    }
    inline double releaseSeconds (double k) { return 0.05 * std::pow (70.0, std::clamp (k, 0.0, 10.0) / 10.0); }   // 50 ms à 3,5 s
    inline double inputDb (double k)  { return 6.0 * (k - 5.0); }      // -30 à +30 dB, 5 = 0 dB
    inline double outputDb (double k) { return 5.0 * (k - 5.0); }      // -25 à +25 dB, 5 = 0 dB

    // coefficient d'un lissage du 1er ordre de constante de temps t (s)
    inline double coef (double t, double fs) { return 1.0 - std::exp (-1.0 / std::max (1.0e-9, t * fs)); }

    //------------------------------------------------------------------ biquad (forme directe transposée)
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        void reset() { z1 = z2 = 0; }
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

    //------------------------------------------------------------------ saturations (rationnelles, sans fonction transcendante)
    // plafond doux en ±1 : u / (1 + u^4)^(1/4) ; quasi linéaire au repos (distorsion en u^4, pas en u^2),
    // genou doux près du plafond
    inline double sat1 (double u) { u = std::clamp (u, -12.0, 12.0); const double u2 = u * u; return u / std::sqrt (std::sqrt (1.0 + u2 * u2)); }
    // étage d'entrée / de sortie : plafond 'ceil', asym > 0 ajoute un peu d'harmonique 2
    inline double softClip (double x, double invCeil, double ceil, double asym)
    {
        const double u = std::clamp (x * invCeil, -12.0, 12.0);
        const double v = u + asym * u * u;                  // monotone sur toute la plage (asym petit)
        const double v2 = v * v;
        return ceil * v / std::sqrt (std::sqrt (1.0 + v2 * v2));
    }
    // tanh (x) rationnelle (Padé), exacte aux extrémités, erreur < 0,2 %
    inline double fastTanh (double x)
    {
        x = std::clamp (x, -3.0, 3.0);
        const double x2 = x * x;
        return x * (27.0 + x2) / (27.0 + 9.0 * x2);
    }

    //------------------------------------------------------------------ réglages
    struct Params
    {
        int ratio = 4;             // 0 1:1, 1 2:1, 2 3:1, 3 4:1, 4 6:1, 5 10:1 opto, 6 20:1, 7 NUKE
        bool detHp = false, detBand = false, link = false, deadPatch = false;
        bool audioHp = false; int dist = 0;   // 0 off, 1 Dist 2, 2 Dist 3
        bool brit = false;
        double attack = 5, release = 5;
        double hr = 16;            // marge interne (dB) : 4 à 28
    };

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

    // réduction de gain statique (dB, >= 0) pour un dépassement 'over' (dB) du seuil
    inline double staticGr (double over, double slope, double W)
    {
        if (slope <= 0.0) return 0.0;
        if (W <= 0.0) return over > 0 ? slope * over : 0.0;
        if (over <= -W * 0.5) return 0.0;
        if (over >= W * 0.5)  return slope * over;
        const double z = over + W * 0.5;
        return slope * z * z / (2.0 * W);            // genou parabolique
    }

    //------------------------------------------------------------------ coefficients d'un bloc (calculés une fois par bloc)
    struct Block
    {
        int ratio = 4;
        double slope = 0, knee = 0, thresh = -14;   // seuil effectif (dB internes)
        double aAtt = 1, relSamples = 1;            // attaque (coef) et relâchement (en échantillons)
        double aOptoAtt = 1, aOptoRel = 1, aMemUp = 0, aMemDn = 0, envDecay = 0;
        double ceilT = 1;                           // plafond visé (lissé par échantillon)
        double quietLin = 0;                        // niveau linéaire sous lequel la réduction visée est nulle
        double aFade = 0;                           // fondu des commutations (10 ms)
        // cibles des poids de mélange
        double wDetHp = 0, wDetBand = 0, wAudioHp = 0, wD2 = 0, wD3 = 0, wBrit = 0, wDead = 0;

        void make (const Params& p, double fs, double baseThresh)
        {
            ratio = p.ratio;
            const Curve c = curveFor (p.ratio);
            slope = c.ratio <= 1.0 ? 0.0 : 1.0 - 1.0 / c.ratio;
            knee = c.knee;
            thresh = baseThresh + c.thresh;
            // sous le début du genou (et sous le seuil Brit, 4 dB plus bas) : aucune réduction, inutile de passer en dB
            quietLin = slope <= 0.0 ? 1.0e30 : dbToGain (thresh - std::max (knee * 0.5, 0.0) - (p.brit ? 4.5 : 0.0) - 0.1);
            const double att = attackSeconds (p.attack, p.ratio) * (p.brit ? 0.6 : 1.0);
            aAtt = coef (att, fs);
            relSamples = releaseSeconds (p.release) * fs;
            aOptoAtt = coef (std::max (att, 0.001), fs);
            aOptoRel = coef (0.06, fs);
            aMemUp = coef (1.5, fs); aMemDn = coef (6.0, fs);
            envDecay = std::exp (-1.0 / (0.005 * fs));        // redresseur crête : retombée 5 ms
            ceilT = dbToGain (32.0 - p.hr);
            aFade = coef (0.010, fs);
            wDetHp = p.detHp ? 1 : 0; wDetBand = p.detBand ? 1 : 0; wAudioHp = p.audioHp ? 1 : 0;
            wD2 = p.dist == 1 ? 1 : 0; wD3 = p.dist == 2 ? 1 : 0;
            wBrit = p.brit ? 1 : 0; wDead = (p.link && p.deadPatch) ? 1 : 0;
        }
    };

    //------------------------------------------------------------------ un canal du compresseur
    struct Channel
    {
        double fs = 192000.0;
        Biquad scHp, scBand, aHp1, aHp2, dcBlock;
        double gr = 0.0;                // réduction de gain lissée (dB)
        double env = 0.0;               // redresseur crête du détecteur
        double optoMem = 0.0;           // mémoire de la cellule opto (0..1)
        double optoLevel = 0.0;         // niveau détecté par la cellule opto (lent)
        // poids lissés (fondus des commutations) et plafond lissé
        double wDetHp = 0, wDetBand = 0, wAudioHp = 0, wD2 = 0, wD3 = 0, wBrit = 0, wDead = 0, ceil = 0;

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
            gr = 0.0; env = 0.0; optoMem = 0.0; optoLevel = 0.0;
            ceil = 0.0;                        // repris au premier bloc (pas de fondu au démarrage)
        }
        // première fois : les poids partent directement de leur cible
        void snapTo (const Block& b)
        {
            wDetHp = b.wDetHp; wDetBand = b.wDetBand; wAudioHp = b.wAudioHp; wD2 = b.wD2; wD3 = b.wD3;
            wBrit = b.wBrit; wDead = b.wDead; ceil = b.ceilT;
        }

        //-- avance les fondus d'un échantillon
        inline void fades (const Block& b)
        {
            const double a = b.aFade;
            wDetHp += (b.wDetHp - wDetHp) * a; wDetBand += (b.wDetBand - wDetBand) * a; wAudioHp += (b.wAudioHp - wAudioHp) * a;
            wD2 += (b.wD2 - wD2) * a; wD3 += (b.wD3 - wD3) * a; wBrit += (b.wBrit - wBrit) * a; wDead += (b.wDead - wDead) * a;
            ceil += (b.ceilT - ceil) * a;
        }

        //-- étage d'entrée ; err reçoit l'écart non linéaire
        inline double inputStage (double x, double& err) const
        {
            const double y = softClip (x, 1.0 / ceil, ceil, 0.035);
            err += (y - x) * (y - x);
            return y;
        }

        //-- niveau du sidechain : filtres (toujours en marche, fondus), redresseur crête
        inline double sidechain (double x, const Block& b)
        {
            const double h = scHp.process (x);
            const double s1 = x + (h - x) * wDetHp;
            const double e = scBand.process (s1);
            const double s = s1 + (e - s1) * wDetBand;
            const double r = std::abs (s);
            env = r > env ? r : env * b.envDecay;
            return env;
        }

        //-- calcul de gain : met à jour gr à partir du niveau détecté lvl ; renvoie le gain linéaire
        inline double computeGain (double lvl, const Block& b)
        {
            double level = lvl;
            if (b.ratio == 5)
            {
                // cellule opto : le détecteur suit le niveau avec l'inertie de la lumière
                optoLevel += (lvl - optoLevel) * (lvl > optoLevel ? b.aOptoAtt : b.aOptoRel);
                level = optoLevel;
            }
            double target = 0.0, d = -200.0;
            if (level > b.quietLin || wBrit > 0.001)
            {
                d = gainToDb (level);
                target = staticGr (d - b.thresh, b.slope, b.knee);
            }
            if (wBrit > 0.001)
            {
                // tous les boutons enfoncés : au-delà du seuil la réduction dépasse ce que demande le ratio
                const double over = d - (b.thresh - 4.0);
                if (over > 0) target += 0.32 * over * wBrit;
            }

            // relâchement : forme qui dépend du ratio (et de la durée de compression en opto)
            double rel = b.relSamples;
            if (b.ratio == 5)
            {
                const double busy = gr > 1.0 ? 1.0 : 0.0;
                optoMem += (busy - optoMem) * (busy > optoMem ? b.aMemUp : b.aMemDn);
                rel = std::min (20.0 * fs, rel * (1.0 + 2.5 * optoMem) + 0.08 * fs * optoMem * gr);   // jusqu'à 20 s
            }
            else if (b.ratio >= 6)
                rel *= 0.22 + 0.78 * 1.5 / (1.5 + gr);            // NUKE / 20:1 : logarithmique, vif puis de plus en plus lent
            else
                rel *= 0.55 + 0.45 / (1.0 + gr / 12.0);           // un peu plus vif sous forte compression

            if (target > gr) gr += (target - gr) * b.aAtt;
            else             gr += (target - gr) / (rel + 0.5);   // = 1 - exp(-1/rel) à mieux que 0,01 % (rel >= 2 400 éch.)
            if (gr < 1.0e-7) { gr = 0.0; return 1.0; }
            return std::exp (-gr * kNeperPerDb);
        }

        //-- après le VCA : modes audio ; err reçoit l'écart non linéaire des VCA de distorsion
        inline double audioModes (double y, double& err)
        {
            const double h = aHp2.process (aHp1.process (y));      // le filtre tourne toujours : commutation sans clic
            y += (h - y) * wAudioHp;
            const double inv = 1.0 / ceil;
            const double u = sat1 (y * inv);                       // les VCA plafonnent doucement
            const double u2 = u * u, u3 = u2 * u;
            const double vClean = u + 0.012 * u2 - 0.006 * u3;     // résidu de la carte
            const double v2 = u + 0.5 * u2;                        // Dist 2 : harmonique 2 (lampe), monotone sur ]-1, 1[
            const double v3 = fastTanh (3.2 * u) * (1.0 / 3.2);    // Dist 3 : harmonique 3 (bande)
            const double wc = std::max (0.0, 1.0 - wD2 - wD3);
            double v = wc * vClean + wD2 * v2 + wD3 * v3;
            v += 0.08 * u2 * wBrit;
            v += (0.05 * u2 + 0.03 * u3) * wDead;                  // Link sans second appareil (mono)
            const double out = dcBlock.process (v * ceil);
            const double e = (v - u) * ceil;
            err += e * e;
            return out;
        }
    };
}
