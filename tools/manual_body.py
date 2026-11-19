# Corps de la notice de Grinder (exécuté par tools/manual.py : f, P, bullets, table, img, W, ann1, bars, top sont déjà définis)

# ---------- 1. Présentation ----------
f.append(P("1. Présentation", "h1"))
f += [P("<b>Grinder</b> est un compresseur dans l'esprit de l'<b>Empirical Labs EL8 Distressor</b>, tel qu'il est émulé par Universal Audio : un compresseur moderne de studio qui reprend le comportement des grands compresseurs « à genou » d'autrefois, avec un bruit très bas et une couleur réglable. Il est autant un outil de nivellement discret qu'un effet radical sur les batteries, les guitares et les voix."),
      P("Son caractère tient à trois choses : des <b>courbes de ratio</b> très différentes les unes des autres (du genou parabolique très doux au mur de brique), une <b>attaque très rapide</b> (jusqu'à 50 µs) dont la forme du relâchement dépend du ratio, et un <b>générateur de distorsion</b> qui ajoute l'harmonique 2 (côté lampe) ou 3 (côté bande). Même au ratio 1:1, sans compression, Grinder colore le son."),
      P("Grinder reprend aussi les réglages propres au plugin (HR et MIX) et les deux options de la version X du matériel : <b>BRIT</b> et <b>IMAGE LINK</b>.")]

# ---------- 2. Installation ----------
f.append(P("2. Installation", "h1"))
f.append(P("Windows", "h2"))
f.append(P("Téléchargez <b>Grinder-VST3-Windows.zip</b>, décompressez-le et copiez le dossier <b>Grinder.vst3</b> dans <b>C:\\Program Files\\Common Files\\VST3</b>."))
f.append(P("macOS", "h2"))
f.append(P("Téléchargez <b>Grinder-VST3-macOS.zip</b>, décompressez-le et copiez <b>Grinder.vst3</b> dans <b>~/Library/Audio/Plug-Ins/VST3</b> (binaire universel Apple Silicon et Intel). Le plugin est signé ad hoc : au premier lancement, autorisez-le si besoin dans Réglages Système &gt; Confidentialité et sécurité."))
f.append(P("Dans Reaper", "h2"))
f.append(P("Options &gt; Preferences &gt; Plug-ins &gt; VST : <b>Re-scan</b>. Grinder apparaît sous le nom <b>Grinder (NINE HANDS | DSP)</b>. Il accepte une piste mono ou stéréo : en stéréo, chaque canal a son propre compresseur, comme deux appareils côte à côte, sauf si LINK ou IMAGE LINK est allumé."))
f.append(PageBreak())

# ---------- 3. Vue d'ensemble ----------
f.append(P("3. La façade", "h1"))
f.append(img(ann1, W)); f.append(Spacer(1, 3 * mm))
f.append(table([["N°", "Élément", "Rôle"],
      ["1", "Barre de presets", "Flèches : preset précédent / suivant ; clic sur le nom : liste des presets. Devient la ligne d'état pendant un réglage."],
      ["2", "BYPASS", "Contourne le circuit (le son direct passe, aligné sur la latence) ; la barre de réduction de gain continue de bouger. Le voyant rouge s'allume."],
      ["3", "Réduction de gain", "16 voyants, de 1 dB (à droite) à 26 dB (à gauche). Plus la barre s'allume vers la gauche, plus la compression est forte."],
      ["4", "Voyants de ratio", "Ratio en cours. Un clic sur un voyant ou son nom choisit ce ratio directement, sans passer par les autres."],
      ["5", "RATIO", "Clic : ratio suivant ; Maj + clic : ratio précédent (1:1, 2:1, 3:1, 4:1, 6:1, 10:1, 20:1, NUKE)."],
      ["6", "1% THD / REDLINE", "Distorsion mesurée en direct : jaune vers 1 %, rouge vers 3 % ou dès que la sortie écrête."],
      ["7", "DETECTOR", "HP, BAND, LINK : réglages du détecteur (voir section 5). Le bouton fait défiler les 8 combinaisons ; un clic sur un voyant allume ou éteint cette fonction."],
      ["8", "AUDIO", "HP, DIST 2, DIST 3 : traitement du son lui-même (section 6). Le bouton fait défiler les 6 combinaisons ; un clic sur un voyant l'allume ou l'éteint."],
      ["9", "POWER", "Interrupteur : éteint, Grinder laisse passer le son direct et ses lumières s'éteignent."],
      ["10", "HR", "Headroom : marge interne avant saturation, de 4 à 28 dB par pas de 4 (16 dB par défaut). Glisser ou molette sur la vis ; clic sur « HR » ou double-clic : retour à 16 dB."],
      ["11", "INPUT", "Gain d'entrée, de −30 dB (0) à +30 dB (10), 5 = 0 dB. Le seuil est fixe : monter INPUT augmente la compression et pousse les étages de distorsion."],
      ["12", "ATTACK", "De 50 µs (0) à 30 ms (10), en progression logarithmique. À 0 en 2:1, 3:1 et 4:1, l'attaque est encore plus rapide."],
      ["13", "RELEASE", "De 50 ms (0) à 3,5 s (10). La forme du relâchement dépend du ratio ; en 10:1 Opto il peut s'allonger jusqu'à 20 s."],
      ["14", "OUTPUT", "Gain de sortie, de −25 dB (0) à +25 dB (10), 5 = 0 dB, avant l'étage de sortie (qui sature doucement s'il est poussé)."],
      ["15", "MIX", "Compression parallèle : DRY = son direct seul, COMP = son traité seul (par défaut). Mélange sans décalage de phase. Clic sur DRY ou COMP : va au bout."],
      ["16", "BRIT / IMAGE LINK", "Options de la version X (section 7). Leur voyant s'allume quand elles sont actives."]],
      [9 * mm, 32 * mm, W - 41 * mm]))

# ---------- 4. Ratios ----------
f.append(P("4. Les ratios", "h1"))
f += bullets(["<b>1:1</b> : aucune compression. Le son traverse seulement les étages d'entrée, de distorsion et de sortie : avec DIST 2 ou DIST 3 et un INPUT poussé, c'est un saturateur.",
              "<b>2:1 et 3:1</b> : compression douce, genou parabolique très large (jusqu'à 30 dB en 2:1) : la compression commence très tôt et s'installe progressivement. Ne limite jamais complètement.",
              "<b>4:1 et 6:1</b> : genou plus raide, qui se rapproche peu à peu de la limitation. 6:1 est un bon point de départ sur une voix, une basse ou un instrument acoustique.",
              "<b>10:1 (OPTO)</b> : détecteur séparé qui imite une cellule optique (lumière + photorésistance) : réponse plus lente et arrondie, et relâchement qui s'allonge quand la compression dure (jusqu'à 20 s). Gardez une attaque au-dessus de 4 pour ce caractère ; plus rapide, il devient agressif.",
              "<b>20:1 et NUKE</b> : limitation dure. NUKE est un mur de brique au seuil un peu plus haut, avec un relâchement logarithmique (rapide au début, puis de plus en plus lent)."])
f.append(Spacer(1, 2 * mm))
f.append(KeepTogether([table([["Entrée (1 kHz)", "−20 dBFS", "−15 dBFS", "−10 dBFS", "−5 dBFS", "0 dBFS"],
                ["2:1", "0,7 dB", "1,6 dB", "3,0 dB", "4,8 dB", "7,0 dB"],
                ["4:1", "0 dB", "1,0 dB", "3,2 dB", "6,7 dB", "10,5 dB"],
                ["6:1", "0 dB", "0,7 dB", "3,4 dB", "7,5 dB", "11,7 dB"],
                ["10:1 Opto", "0 dB", "0,5 dB", "4,4 dB", "8,9 dB", "13,4 dB"],
                ["NUKE", "0 dB", "0 dB", "2,0 dB", "6,9 dB", "12,0 dB"]],
               [30 * mm] + [(W - 30 * mm) / 5] * 5),
    P("Réduction de gain mesurée en régime établi (INPUT 5, ATTACK 5, RELEASE 5, détecteur neutre).", "small")]))

# ---------- 5. Détecteur ----------
f.append(P("5. Le détecteur (DETECTOR)", "h1"))
f.append(img(top, W)); f.append(Spacer(1, 2 * mm))
f += bullets(["<b>HP</b> (voyant vert) : passe-haut de 100 Hz, 6 dB/oct, dans le détecteur seulement. Le grave déclenche moins la compression : moins de pompage sur un mix ou une batterie, et un grave qui garde son poids.",
              "<b>BAND</b> (voyant jaune) : accentuation du détecteur autour de 6 kHz. Grinder réagit davantage aux médiums durs et aux sifflantes : utile pour adoucir une voix ou des cymbales.",
              "<b>LINK</b> (voyant rouge) : en stéréo, les deux détecteurs sont additionnés et les canaux compressent ensemble. Sur une piste mono, LINK reproduit le comportement du matériel lié à rien : seuil plus bas et un peu plus de distorsion.",
              "Toutes les combinaisons sont possibles : HP + BAND, HP + LINK, BAND + LINK, HP + BAND + LINK."])

# ---------- 6. Audio ----------
f.append(P("6. Les modes audio (AUDIO) et la distorsion", "h1"))
f += bullets(["<b>HP</b> (vert) : filtre de Bessel du 3e ordre (18 dB/oct) sur le son : environ −3 dB à 65 Hz et −12 dB à 30 Hz. Nettoie le sub-grave sans déphaser le reste.",
              "<b>DIST 2</b> (jaune) : ajoute surtout l'harmonique 2, chaude, façon lampe. Mesuré à 1 kHz au ratio 1:1 : 0,4 % à −20 dBFS, 2,2 % à −6 dBFS, 4,4 % à 0 dBFS.",
              "<b>DIST 3</b> (rouge) : ajoute surtout l'harmonique 3, façon bande saturée ; très discrète à bas niveau, elle monte vite quand on pousse (0,5 % à −6 dBFS, 1,9 % à 0 dBFS, bien plus avec INPUT et HR hauts).",
              "Combinaisons : HP + DIST 2 et HP + DIST 3. Sans mode de distorsion, Grinder reste propre : 0,02 % à −30 dBFS, 0,27 % à −6 dBFS, presque uniquement de l'harmonique 2.",
              "Un relâchement très court ajoute aussi de la distorsion, surtout sur la basse : le détecteur suit la crête du signal (retombée de 5 ms), la compression module alors chaque période des notes graves, comme sur le matériel.",
              "<b>HR</b> déplace la limite des étages non linéaires : 4 dB = peu de marge, saturation rapide et beaucoup de couleur ; 28 dB = grande marge, son plus propre. Les voyants 1% THD et REDLINE aident à doser."])

# ---------- 7. Options X ----------
f.append(P("7. Options BRIT et IMAGE LINK", "h1"))
f += bullets(["<b>BRIT</b> : inspiré du réglage « tous les boutons enfoncés » d'un compresseur britannique des années 1960 : au-delà du seuil, la compression dépasse ce que demande le ratio (le son « recule » sur les attaques), l'attaque est plus vive et un peu d'harmonique 2 s'ajoute. Fonctionne avec tous les ratios ; gardez l'attaque sous 3 à 4 pour ce caractère. Sur la basse : BRIT + NUKE + attaque 1 à 2 donne une saturation très « lampe ».",
              "<b>IMAGE LINK</b> : en stéréo, la réduction de gain est exactement la même sur les deux canaux (la plus forte des deux) : l'image stéréo ne bouge plus. À utiliser sur un bus ou une prise stéréo ; avec LINK éteint, chaque canal décide encore de son niveau détecté mais la réduction appliquée est commune."])

# ---------- 8. MIDI ----------
f.append(P("8. MIDI et automation", "h1"))
f += bullets(["Tous les réglages sont automatisables dans le DAW (Input, Attack, Release, Output, Mix, Headroom, Ratio, Detector HP, Detector Band Emphasis, Stereo Link, Audio HP, Distortion, Brit Mode, Image Link, Bypass, Power, Oversampling 4x).",
              "<b>MIDI Learn</b> : clic droit sur un potard, un bouton, un voyant ou la vis HR &gt; <b>MIDI Learn</b>, puis bougez la pédale d'expression ou le bouton MIDI. Le menu principal &gt; <b>MIDI</b> liste aussi tous les réglages. La ligne d'état affiche MIDI LEARN puis le CC appris.",
              "Aucun CC n'est assigné par défaut. <b>Clear MIDI</b> retire une assignation, <b>Clear all MIDI assignments</b> les retire toutes. Les assignations sont sauvegardées avec le projet.",
              "La course réelle de la pédale d'expression est apprise : une pédale qui n'envoie que 2 à 116 atteint quand même 0 et 100 %. Les réglages sont lissés (pas de saccades)."])

# ---------- 9. Fenêtre ----------
f.append(P("9. Fenêtre et ligne d'état", "h1"))
wt = Table([[[P("La fenêtre s'ouvre à la taille par défaut (<b>60 %</b>) et ne peut pas être réduite en dessous. Clic droit &gt; <b>Plugin size</b> : 60 %, 75 % ou 100 % (taille enregistrée avec le projet) ; le coin en bas à droite agrandit jusqu'à 150 % en gardant les proportions. Clic droit &gt; <b>Glow intensity</b> : 25, 50, 75 ou 100 % pour l'intensité des halos (voyants, anneaux des potards)."),
              P("La <b>barre de presets</b> et la <b>ligne d'état</b> partagent la même pastille. Dès qu'un réglage bouge (souris, molette, automation, MIDI), la ligne d'état prend sa place avec la valeur en direct ; elle s'efface 2,5 s après le dernier mouvement. Un clic sur la pastille fait revenir la barre de presets tout de suite. Chaque potard affiche aussi sa valeur dans une pastille au bout de son anneau lumineux.")],
             [img(bars, 66 * mm), P("Au repos (barre de presets) et pendant un réglage (ligne d'état).", "small")]]], colWidths=[W - 70 * mm, 70 * mm])
wt.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"), ("LEFTPADDING", (0, 0), (0, 0), 0)]))
f.append(wt)
f.append(P("Souris : glisser verticalement sur un potard (Maj : réglage fin), molette, double-clic pour la valeur par défaut, clic droit pour le MIDI. Clic droit ailleurs : menu principal (presets ; Settings : Ratio, Headroom, Oversampling 4x, Brit mode, Image link ; MIDI ; Plugin size ; Glow intensity)."))

# ---------- 10. Presets ----------
f.append(P("10. Presets", "h1"))
f.append(P("Flèches de la barre de presets, clic sur son nom, clic droit &gt; Factory presets, ou menu de presets du DAW. Un astérisque après le nom signale un preset modifié. <b>Save user preset...</b> et <b>Load user preset...</b> enregistrent et rechargent vos réglages (fichiers .grinder, dossier Documents/Nine Hands DSP/Grinder par défaut)."))
f.append(table([["Preset", "Réglages"],
                ["Default", "6:1, INPUT 5, ATTACK 5, RELEASE 5, OUTPUT 5, détecteur et audio neutres, HR 16 dB, MIX COMP."],
                ["Vocal 6:1", "6:1, INPUT 5,5, ATTACK 5, RELEASE 4, détecteur HP : voix posée et régulière."],
                ["Vocal Opto", "10:1 Opto, ATTACK 10, RELEASE 0, détecteur HP : nivellement lent et doux, à la manière d'un compresseur optique."],
                ["Vocal Warm Dist 2", "4:1, INPUT 6, DIST 2, détecteur HP : voix plus dense et chaude."],
                ["De-Harsh Band", "3:1, attaque et relâchement rapides, détecteur HP + BAND : adoucit les médiums durs et les sifflantes."],
                ["Kick Punch", "6:1, ATTACK 6,5, RELEASE 2,5 : laisse passer l'attaque de la grosse caisse et resserre la queue."],
                ["Snare Crack", "4:1, INPUT 6,5, RELEASE 1,5, DIST 3 : caisse claire plus claquante."],
                ["Drum Room Nuke", "NUKE, INPUT 7,5, ATTACK 2, DIST 3, MIX 55 % : pièce écrasée, mélangée au son direct."],
                ["Parallel Drums", "20:1, INPUT 7, ATTACK 1, DIST 2, détecteur HP, MIX 40 % : densité sans perdre les attaques."],
                ["Bass Even", "6:1, ATTACK 4, RELEASE 5 : basse régulière, note à note."],
                ["Bass Brit Tube", "NUKE + BRIT, ATTACK 1,5, DIST 2 : saturation « lampe » très tenue."],
                ["Acoustic Gentle 2:1", "2:1, détecteur HP : guitare acoustique ou piano, compression presque invisible."],
                ["Guitar Bite", "4:1, INPUT 6,2, DIST 3, HP audio : guitare plus mordante, sub-grave nettoyé."],
                ["Mix Bus Glue 2:1", "2:1, ATTACK 7, détecteur HP + LINK, IMAGE LINK : cohésion douce d'un bus stéréo."],
                ["Tape Color 1:1", "1:1, INPUT 6,5, DIST 3, HR 24 dB : couleur bande sans compression."],
                ["Tube Color 1:1", "1:1, INPUT 6,5, DIST 2, HR 24 dB : couleur lampe sans compression."]],
               [36 * mm, W - 36 * mm]))

# ---------- 11. Conseils ----------
f.append(P("11. Conseils d'utilisation", "h1"))
f += bullets(["Commencez en 6:1, ATTACK et RELEASE à 5 : montez INPUT jusqu'à voir 3 à 6 voyants de réduction de gain, puis rattrapez le niveau avec OUTPUT.",
              "Sur une basse ou une guitare 7 cordes en Sol ou plus bas, évitez un RELEASE à 0 ou 1 (craquements sur le grave) ; allumez HP dans DETECTOR si le grave fait trop pomper.",
              "Dans Reaper, sur une guitare, Grinder se place avant Ampire et Impulse pour niveler le jeu et attaquer l'ampli plus fort, ou après pour tenir le son d'ampli.",
              "Pour la compression parallèle, inutile de dupliquer la piste : poussez la compression (NUKE ou 20:1, DIST 2 ou 3) et réglez MIX entre 30 et 60 %.",
              "Pour la couleur seule, prenez 1:1 avec DIST 2 ou DIST 3, réglez la quantité avec INPUT et HR, et surveillez les voyants 1% THD et REDLINE."])

# ---------- 12. Caractéristiques ----------
f.append(KeepTogether([P("12. Caractéristiques techniques", "h1"), table([
    ["Format", "VST3 (Windows 64 bits, macOS universel), mono ou stéréo"],
    ["Ratios", "1:1, 2:1, 3:1, 4:1, 6:1, 10:1 Opto, 20:1, NUKE ; genoux paraboliques (jusqu'à 30 dB) à durs"],
    ["Temps", "attaque 50 µs à 30 ms (plus rapide à 0 en 2:1 à 4:1) ; relâchement 50 ms à 3,5 s, jusqu'à 20 s en Opto"],
    ["Détecteur", "HP 100 Hz 6 dB/oct, accentuation 6 kHz, liaison stéréo"],
    ["Audio", "HP Bessel 18 dB/oct (−3 dB à 65 Hz), DIST 2 (harmonique 2), DIST 3 (harmonique 3)"],
    ["Headroom", "4 à 28 dB par pas de 4, 16 dB par défaut"],
    ["Suréchantillonnage", "4x (demi-bande RIF), désactivable ; latence de quelques échantillons, compensée par le DAW"],
    ["Commutations", "fondu de 10 ms sur les modes, les filtres, BRIT, LINK et HR : aucun clic"],
    ["Charge CPU", "environ 3 % d'un cœur en 4x, moins de 1 % sans suréchantillonnage (stéréo, 48 kHz)"],
    ["MIDI", "MIDI Learn sur tous les réglages, aucun CC par défaut"],
    ["Fenêtre", "60 / 75 / 100 %, jusqu'à 150 %, jamais sous 60 % ; anti-gel v2 sous Windows"]],
    [36 * mm, W - 36 * mm], header=False)]))
