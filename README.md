# Grinder — NINE HANDS | DSP

Compresseur VST3 (Windows et macOS) « classic knee » dans l'esprit de l'Empirical Labs EL8 Distressor tel qu'émulé par UAD : 8 ratios du 1:1 (couleur seule) au NUKE (mur de brique), détecteur et modes audio commutables, distorsion harmonique 2 (lampe) ou 3 (bande), Headroom réglable, MIX parallèle, et les options de la version X (Brit, Image Link).

<p align="center">
  <a href="https://github.com/Nine-Hands-Dsp/Grinder/releases/latest/download/Grinder-VST3-Windows.zip"><img alt="Télécharger pour Windows" src="https://img.shields.io/badge/%E2%AC%87%20T%C3%A9l%C3%A9charger-Windows-0078D6?style=for-the-badge"></a>
  <a href="https://github.com/Nine-Hands-Dsp/Grinder/releases/latest/download/Grinder-VST3-macOS.zip"><img alt="Télécharger pour macOS" src="https://img.shields.io/badge/%E2%AC%87%20T%C3%A9l%C3%A9charger-macOS-111111?style=for-the-badge&logo=apple&logoColor=white"></a>
  <br>
  <a href="https://github.com/Nine-Hands-Dsp/Grinder/releases/latest/download/Grinder-Notice.pdf"><img alt="Télécharger la notice (PDF)" src="https://img.shields.io/badge/%F0%9F%93%96%20Notice-PDF-919496?style=for-the-badge"></a>
  <br>
  <a href="https://github.com/Nine-Hands-Dsp/Grinder/releases/latest"><img alt="Dernière version" src="https://img.shields.io/github/v/release/Nine-Hands-Dsp/Grinder?label=derni%C3%A8re%20version&color=ff4a3d"></a>
</p>

## Fonctions

- **INPUT / ATTACK / RELEASE / OUTPUT** (0 à 10) : INPUT pousse le signal vers le seuil (−30 à +30 dB), attaque de 50 µs à 30 ms, relâchement de 50 ms à 3,5 s, OUTPUT de −25 à +25 dB.
- **RATIO** (8 voyants) : 1:1, 2:1 et 3:1 (genou parabolique très large), 4:1 et 6:1 (genou plus raide), 10:1 Opto (détecteur optique séparé, relâchement jusqu'à 20 s), 20:1 et NUKE (limitation dure, relâchement logarithmique). Clic : ratio suivant ; Maj + clic : précédent ; clic sur un voyant : choix direct.
- **DETECTOR** (8 états) : HP (100 Hz, 6 dB/oct), BAND (accentuation à 6 kHz), LINK (détecteurs stéréo additionnés ; en mono, comportement « Link sans second appareil ») et leurs combinaisons.
- **AUDIO** (6 états) : HP (Bessel 18 dB/oct à 80 Hz), DIST 2 (harmonique 2, lampe), DIST 3 (harmonique 3, bande), HP + DIST 2, HP + DIST 3.
- **Voyants 1 % THD et REDLINE** (environ 3 % ou écrêtage), barre de réduction de gain à 16 voyants (1 à 26 dB).
- **HR (Headroom)** : 4 à 28 dB par pas de 4 (16 dB par défaut) ; clic sur « HR » pour revenir à 16 dB.
- **MIX** Dry / Comp (compression parallèle sans décalage de phase ; clic sur DRY ou COMP pour aller au bout), **BYPASS**, **POWER**.
- **Options X** : BRIT (esprit « tous les boutons enfoncés » d'un 1176) et IMAGE LINK (réduction de gain identique sur les deux canaux).
- Suréchantillonnage 4x (désactivable), presets d'usine, presets utilisateur (`Documents/Nine Hands DSP/Grinder`), MIDI Learn sur tous les réglages (aucun CC par défaut), fenêtre de 60 à 150 %, réglage Glow.

## Installation

Les boutons ci-dessus téléchargent toujours la dernière version.

- **Windows** : décompresser et copier `Grinder.vst3` dans `C:\Program Files\Common Files\VST3`.
- **macOS** : décompresser et copier `Grinder.vst3` dans `~/Library/Audio/Plug-Ins/VST3` (le plugin est signé ad hoc ; au premier lancement, autoriser si besoin dans Réglages Système > Confidentialité et sécurité).

Dans Reaper, le plugin apparaît sous le nom **Grinder (NINE HANDS | DSP)**.

## Compilation

JUCE 8.0.15 (récupéré par CMake) :

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Outil de captures et de mesures sous Linux : `tools/render` (`cmake tools/render -DJUCE_DIR=<JUCE>`). Notice : `python3 tools/manual.py <captures> docs/Grinder_Notice_FR_v<version>.pdf`.

## Historique

- **1.0.3** : logo NINE HANDS | DSP plus petit sur la façade.
- **1.0.2** : façade éclairée en haut à gauche à 45° avec plus de contraste, ombres portées et occlusion renforcées (potards, voyants, boutons, vis) ; bouton BYPASS écrit en un mot (titre en doublon retiré).
- **1.0.1** : moteur audio optimisé : charge CPU divisée par 3 (coefficients calculés par bloc, saturations rationnelles, détecteur sauté sous le genou), détecteur crête qui donne la vraie attaque (30 ms à 10), commutations sans clic (fondus de 10 ms), étage propre plus propre (distorsion en u⁴ au repos), piste mono calculée sur un seul canal.
- **1.0.0** : première version (anti-gel v2 sous Windows).
