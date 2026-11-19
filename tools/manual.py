# GRINDER (NINE HANDS | DSP) - notice PDF en français, illustrée avec le rendu réel du plugin (tools/render).
# Usage : python3 tools/manual.py <dossier des captures (sortie de tools/render)> <fichier PDF de sortie>
import os, sys, re, subprocess
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.lib import colors
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import (BaseDocTemplate, PageTemplate, NextPageTemplate, Frame, Paragraph, Spacer, Image as RLImage,
                                Table, TableStyle, PageBreak, KeepTogether)
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHOTS = sys.argv[1]
OUT = sys.argv[2]
VERSION = re.search(r"project\(Grinder VERSION ([0-9.]+)\)", open(os.path.join(ROOT, "CMakeLists.txt")).read()).group(1)
TMP = os.path.join(os.path.dirname(os.path.abspath(OUT)), "notice_tmp")
os.makedirs(TMP, exist_ok=True)

FONTS = os.path.join(ROOT, "fonts"); FONTS2 = os.path.join(ROOT, "tools", "fonts_notice")
for n, f, d in (("Pop", "Poppins-Regular", FONTS), ("PopL", "Poppins-Light", FONTS2), ("PopM", "Poppins-Medium", FONTS), ("PopB", "Poppins-Bold", FONTS),
                ("Title", "BarlowCondensed-ExtraBold", FONTS2)):
    pdfmetrics.registerFont(TTFont(n, os.path.join(d, f + ".ttf")))

ACC = colors.HexColor("#FF4A3D"); ACC_L = colors.HexColor("#FF9A90")        # rouge d'accent de Grinder
DARK = colors.HexColor("#1A1C22"); GREY = colors.HexColor("#646B76"); LIGHT = colors.HexColor("#EAF2FB")
LOGO_PNG = os.path.join(TMP, "nine_hands_logo_blanc.png")
subprocess.run(["rsvg-convert", "-w", "1200", os.path.join(ROOT, "logo", "NineHandsLogoWhite.svg"), "-o", LOGO_PNG], check=True)

def shot(name): return Image.open(os.path.join(SHOTS, name)).convert("RGB")
src = shot("grinder_apercu_2x.png")
k = src.width / 1600.0                                  # repère de l'interface : 1600 unités de large

def annotate(im, marks, name):
    a = im.copy(); dr = ImageDraw.Draw(a)
    fnt = ImageFont.truetype(os.path.join(FONTS, "Poppins-Bold.ttf"), int(15 * k))
    for n, x, y in marks:
        cx, cy, r = x * k, y * k, 12 * k
        dr.ellipse((cx - r, cy - r, cx + r, cy + r), fill=(255, 255, 255), outline=(255, 74, 61), width=int(2.5 * k))
        dr.text((cx, cy + 1), str(n), font=fnt, fill=(140, 20, 12), anchor="mm")
    p = os.path.join(TMP, name); a.save(p); return p

ann1 = annotate(src, [(1, 585, 50), (2, 98, 160), (3, 238, 162), (4, 258, 232), (5, 958, 196), (6, 896, 156), (7, 1050, 148),
                      (8, 1202, 148), (9, 1384, 172), (10, 96, 392), (11, 290, 330), (12, 530, 330), (13, 770, 330), (14, 1010, 330),
                      (15, 1206, 362), (16, 1362, 342)], "annotated.png")

def crop_u(im, box): return im.crop(tuple(int(v * k) for v in box))
bars = os.path.join(TMP, "bars.png")
a = crop_u(src, (590, 24, 1010, 76)); b = crop_u(shot("grinder_status.png").resize(src.size), (590, 24, 1010, 76))
st_img = Image.new("RGB", (a.width, a.height * 2 + 12), (255, 255, 255)); st_img.paste(a, (0, 0)); st_img.paste(b, (0, a.height + 12)); st_img.save(bars)
top = os.path.join(TMP, "top.png"); crop_u(src, (70, 106, 1532, 288)).save(top)

# ---------- couverture : fenêtre détourée (coins arrondis) sur fond noir, reflet au sol, halo bleu très doux ----------
cutw, cuth = src.size
rmask = Image.new("L", src.size, 0); ImageDraw.Draw(rmask).rounded_rectangle((0, 0, cutw - 1, cuth - 1), radius=int(10 * k), fill=255)
cut = Image.new("RGBA", src.size, (0, 0, 0, 0)); cut.paste(src, (0, 0), rmask)
cw, ch = cut.size
pad = int(cw * 0.10)
canvas = Image.new("RGBA", (cw + 2 * pad, int(ch * 1.30) + pad), (0, 0, 0, 255))
yy, xx = np.mgrid[0:canvas.height, 0:canvas.width].astype(np.float64)
gcx, gcy = canvas.width / 2, pad + ch * 0.45
glow = np.exp(-(((xx - gcx) / (cw * 0.52)) ** 2 + ((yy - gcy) / (ch * 0.7)) ** 2)) * 34
edge = np.clip(np.minimum.reduce([xx, canvas.width - 1 - xx, yy, canvas.height - 1 - yy]) / (pad * 0.9), 0, 1)
glow *= edge ** 2
halo = np.dstack([glow * 0.18, glow * 0.56, glow * 1.0, np.full_like(glow, 255)]).astype(np.uint8)
canvas = Image.alpha_composite(canvas, Image.fromarray(halo, "RGBA"))
refl = cut.transpose(Image.FLIP_TOP_BOTTOM)
ra = np.array(refl).astype(np.float64)
fade = np.clip(1 - np.arange(refl.height) / (refl.height * 0.30), 0, 1) ** 1.6 * 0.22
ra[..., 3] *= fade[:, None]
canvas.alpha_composite(Image.fromarray(ra.astype(np.uint8), "RGBA"), (pad, pad + ch + 6))
canvas.alpha_composite(cut, (pad, pad))
canvas = canvas.crop((0, 0, canvas.width, pad + int(ch * 1.28)))
cover = os.path.join(TMP, "cover_cutout.png"); canvas.convert("RGB").save(cover)
COVER_PAD, COVER_CW, COVER_CH = pad, cw, ch

def img(path, width):
    im = Image.open(path); h = width * im.height / im.width
    return RLImage(path, width=width, height=h)

# ---------- styles ----------
st = {
    "h1": ParagraphStyle("h1", fontName="PopB", fontSize=20, leading=24, textColor=ACC, spaceAfter=8, spaceBefore=4, keepWithNext=1),
    "h2": ParagraphStyle("h2", fontName="PopM", fontSize=13, leading=17, textColor=DARK, spaceAfter=4, spaceBefore=10, keepWithNext=1),
    "p": ParagraphStyle("p", fontName="Pop", fontSize=9.6, leading=14, textColor=DARK, spaceAfter=5),
    "small": ParagraphStyle("small", fontName="Pop", fontSize=8.2, leading=11.5, textColor=GREY, spaceAfter=6),
    "cell": ParagraphStyle("cell", fontName="Pop", fontSize=8.4, leading=11, textColor=DARK),
    "cellb": ParagraphStyle("cellb", fontName="PopM", fontSize=8.4, leading=11, textColor=DARK),
}
def P(t, s="p"): return Paragraph(t, st[s])
def bullets(items): return [P("&bull;&nbsp;&nbsp;" + i) for i in items]
def table(rows, widths, header=True):
    data = [[P(c, "cellb" if (header and r == 0) else "cell") for c in row] for r, row in enumerate(rows)]
    t = Table(data, colWidths=widths, repeatRows=1 if header else 0)
    style = [("VALIGN", (0, 0), (-1, -1), "TOP"), ("LINEBELOW", (0, 0), (-1, -1), 0.3, colors.HexColor("#D5DDE5")),
             ("TOPPADDING", (0, 0), (-1, -1), 3), ("BOTTOMPADDING", (0, 0), (-1, -1), 3)]
    if header: style += [("BACKGROUND", (0, 0), (-1, 0), LIGHT), ("LINEBELOW", (0, 0), (-1, 0), 0.8, ACC)]
    t.setStyle(TableStyle(style)); return t

W = A4[0] - 36 * mm
f = [NextPageTemplate("p"), Spacer(1, 1), PageBreak()]
exec(open(os.path.join(ROOT, "tools", "manual_body.py"), encoding="utf-8").read())

def deco(c, doc):
    c.saveState()
    c.setFillColor(DARK); c.rect(0, A4[1] - 6 * mm, A4[0], 6 * mm, stroke=0, fill=1)
    c.setFillColor(ACC); c.rect(0, A4[1] - 6.8 * mm, A4[0], 0.8 * mm, stroke=0, fill=1)
    c.setFont("Pop", 7.5); c.setFillColor(GREY)
    c.drawString(18 * mm, 10 * mm, "Grinder (NINE HANDS | DSP) · v" + VERSION)
    c.drawRightString(A4[0] - 18 * mm, 10 * mm, str(doc.page))
    c.restoreState()

# ---- Couverture « minimale » (modèle 6) : gabarit fixe commun à toutes les notices ----
COVER_NAME = "GRINDER"; COVER_SUB = "Compresseur classic knee"
PEDAL_H, PEDAL_MAXW, PEDAL_TOP = 82 * mm, 115 * mm, 84 * mm
LANG_TOP = 18 * mm
NAME_TOP, NAME_PT = 194 * mm, 30
SUB_TOP, SUB_PT = 208 * mm, 10
LOGO_W, LOGO_BOTTOM = 38 * mm, 22 * mm
BAND_H = 3 * mm
def cover_bg(c, doc):
    c.saveState(); c.setFillColor(colors.black); c.rect(0, 0, A4[0], A4[1], stroke=0, fill=1)
    c.setFillColor(ACC); c.rect(0, 0, A4[0], BAND_H, stroke=0, fill=1)
    def line(text, top, font, size, cs, col):
        w = pdfmetrics.stringWidth(text, font, size) + cs * (len(text) - 1)
        t = c.beginText((A4[0] - w) / 2, A4[1] - top - size * 0.8); t.setFont(font, size); t.setCharSpace(cs)
        t.setFillColor(col); t.textOut(text); c.drawText(t)
    line("FRANÇAIS", LANG_TOP, "PopM", 8.5, 0, ACC_L)
    ci = Image.open(cover); iw, ih = ci.size
    kk = min(PEDAL_H / COVER_CH, PEDAL_MAXW / COVER_CW)
    dw, dh = iw * kk, ih * kk
    y0 = A4[1] - (PEDAL_TOP + (PEDAL_H - COVER_CH * kk) / 2) + COVER_PAD * kk
    c.drawImage(cover, (A4[0] - dw) / 2, y0 - dh, dw, dh)
    line(COVER_NAME, NAME_TOP - NAME_PT * 0.02, "Title", NAME_PT, NAME_PT * 0.5, colors.white)
    line(COVER_SUB, SUB_TOP, "Pop", SUB_PT, 1.5, colors.HexColor("#BDC4B8"))
    lw, lh = Image.open(LOGO_PNG).size
    c.drawImage(LOGO_PNG, (A4[0] - LOGO_W) / 2, LOGO_BOTTOM, LOGO_W, LOGO_W * lh / lw, mask="auto")
    c.restoreState()

doc = BaseDocTemplate(OUT, pagesize=A4, leftMargin=18 * mm, rightMargin=18 * mm, topMargin=16 * mm, bottomMargin=18 * mm,
                      title="GRINDER - Notice v" + VERSION, author="NINE HANDS | DSP")
doc.addPageTemplates([PageTemplate(id="cover", frames=[Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id="fc")], onPage=cover_bg),
                      PageTemplate(id="p", frames=[Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id="f")], onPage=deco)])
doc.build(f)
print(OUT)
