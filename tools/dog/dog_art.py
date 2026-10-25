"""The Brittany: the family's orange-and-white dog, for both screen designs.

Drawn as SVG in a painted style (soft gradients, a thin warm-brown outline, fur
strands), with a portrait head taken from her photo, on Pippa's 260x252 stage so she stands where Pippa
stands: head in the top ~120 px, chest centre near (128, 128) where the all-done
heart floats, feet on y ~240. Six outfits match Pippa's wardrobe (dress, pants,
fall, rain, hot, snow) so the picture and the garment tiles always agree.

Based on a photo of the family's dog: copper-orange head and long feathered
ears, a white blaze running down into a white muzzle, a pink-liver nose, amber
eyes, a mostly white body with an orange saddle and hip patch, orange ticking
on white legs, a short docked tail, and a dark collar with an orange pattern.

    stage_svg(outfit)   full character, outfit in OUTFITS or "base"
    head_svg()          head and ears only, for badges and thumbnails
"""

INK = "#4A2F22"       # outline: a thin warm brown, so she reads as a painted dog
WHITE = "#FFFDF9"
ORANGE = "#D9782F"    # copper-orange coat
ORANGE_D = "#B65E1E"  # ear feathering, shading
NOSE = "#A65B4C"      # pink-liver nose
EYE = "#B8782C"       # amber iris
EYE_RIM = "#2A1A12"
ORANGE_L = "#EFA766"  # sunlit fringe
BLUSH = "#F7A9A0"
TONGUE = "#F27A9B"
W = 260
H = 252

OUTFITS = ["dress", "pants", "snow", "rain", "hot", "fall"]


DEFS = (
    "<defs>"
    f'<radialGradient id="coat" cx="0.5" cy="0.38" r="0.62"><stop offset="0" stop-color="#E58E45"/>'
    f'<stop offset="0.7" stop-color="{ORANGE}"/><stop offset="1" stop-color="{ORANGE_D}"/></radialGradient>'
    f'<linearGradient id="ear" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="{ORANGE_D}"/>'
    f'<stop offset="0.45" stop-color="{ORANGE}"/><stop offset="1" stop-color="{ORANGE_L}"/></linearGradient>'
    '<radialGradient id="muzzle" cx="0.5" cy="0.62" r="0.6"><stop offset="0" stop-color="#FFFFFF"/>'
    '<stop offset="1" stop-color="#F1E8DF"/></radialGradient>'
    '<radialGradient id="iris" cx="0.45" cy="0.4" r="0.6"><stop offset="0" stop-color="#D69A4A"/>'
    '<stop offset="1" stop-color="#7A4A1E"/></radialGradient>'
    '<linearGradient id="fur" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="#F1E8DF"/>'
    '<stop offset="0.35" stop-color="#FFFDF9"/><stop offset="1" stop-color="#EDE3D9"/></linearGradient>'
    "</defs>")


def _svg(body, w=W, h=H, vb=None):
    vb = vb or f"0 0 {w} {h}"
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="{vb}">{DEFS}'
            f'<g stroke-linejoin="round" stroke-linecap="round">{body}</g></svg>')


def s(d, fill, sw=5.0, extra=""):
    """A filled shape with an outline under the fill (drawn thinner than Pippa's
    sticker line: widths are scaled to 0.6)."""
    stroke = "" if "stroke=" in extra else f'stroke="{INK}" '
    return (f'<path d="{d}" fill="{fill}" {stroke}stroke-width="{sw * 0.6:.2f}" '
            f'paint-order="stroke" {extra}/>')


def line(d, sw=2.2, color=INK, extra=""):
    return f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{sw}" {extra}/>'


def dots(points, r=2.0, color=ORANGE):
    return "".join(f'<circle cx="{x}" cy="{y}" r="{r}" fill="{color}"/>' for x, y in points)


# ------------------------------------------------------------------ body parts

def tail():
    # Her docked tail: a short white stub with a feathered tuft, peeking out on the right.
    d = ("M156,180 C162,176 170,170 176,164 C178,158 184,156 186,160 C190,158 193,163 190,167 "
         "C193,170 190,175 186,175 C182,180 176,184 170,186 C164,188 158,188 156,184 Z")
    return s(d, "url(#fur)") + line("M164,180 C170,176 176,172 182,166", 1.2, "#D9CFC6")


def legs(sock=None):
    """Two short legs with paws. sock: None (fur), or a colour for tights."""
    out = ""
    for x0 in (104, 134):
        d = f"M{x0},180 L{x0},226 C{x0},232 {x0+22},232 {x0+22},226 L{x0+22},180 Z"
        out += s(d, sock or "url(#fur)", 4.4)
        if not sock:
            out += dots([(x0 + 6, 200), (x0 + 14, 206), (x0 + 8, 214), (x0 + 16, 218)], 1.6)
    return out


def paws_feet():
    out = ""
    for cx in (114, 146):
        out += s(f"M{cx-15},236 C{cx-15},226 {cx+15},226 {cx+15},236 C{cx+15},242 {cx-15},242 {cx-15},236 Z",
                 WHITE, 4.4)
        out += line(f"M{cx-5},233 L{cx-5},239 M{cx+5},233 L{cx+5},239", 1.6)
    return out


def torso(fur=True):
    d = ("M100,124 C100,114 112,108 130,108 C148,108 160,114 160,124 L166,180 "
         "C166,192 152,196 130,196 C108,196 94,192 94,180 Z")
    out = s(d, "url(#fur)")
    if fur:
        # orange saddle over her right shoulder and a hip patch, clipped to the torso
        out += ('<clipPath id="torso"><path d="' + d + '"/></clipPath><g clip-path="url(#torso)">'
                f'<path d="M140,104 C156,104 166,116 166,132 C164,152 154,162 146,160 C140,148 138,124 140,104 Z" fill="{ORANGE}"/>'
                f'<path d="M92,168 C100,160 114,164 116,176 C116,186 104,196 92,194 Z" fill="{ORANGE}"/>'
                '</g>')
        out += line("M108,156 C110,166 108,176 110,186 M150,156 C150,166 152,176 150,186", 1.2, "#E3D8CE")
    return out


def ruff():
    """Her fluffy white chest, spilling out under the chin."""
    d = ("M104,110 C116,114 144,114 156,110 C160,118 158,126 154,130 C152,138 146,136 144,142 "
         "C140,148 134,144 130,150 C126,144 120,148 116,142 C114,136 108,138 106,130 C102,126 100,118 104,110 Z")
    return s(d, "url(#muzzle)", 3.0) + line("M118,128 C120,134 124,136 126,140 M142,128 C140,134 136,136 134,140 "
                                             "M130,130 L130,142", 1.1, "#DCCFC3")


def arms(sleeve=None, cuff=None, mitt=None, paw_l=True, paw_r=True):
    """Arms hang at her sides. sleeve: colour of a long sleeve, or None for fur."""
    out = ""
    arms_d = {"l": "M102,124 C92,134 86,152 86,176 C86,184 98,184 99,176 C100,160 104,146 110,136 Z",
              "r": "M158,124 C168,134 174,152 174,176 C174,184 162,184 161,176 C160,160 156,146 150,136 Z"}
    for side, d in arms_d.items():
        out += s(d, sleeve or "url(#fur)", 4.6)
        if not sleeve:
            x = 94 if side == "l" else 166
            out += dots([(x, 156), (x + (2 if side == "l" else -2), 166)], 1.6)
        elif cuff:
            x0 = 84 if side == "l" else 160
            out += s(f"M{x0},170 L{x0+16},170 L{x0+16},178 L{x0},178 Z", cuff, 3.4)
    for side, cx in (("l", 92), ("r", 168)):
        if (side == "l" and not paw_l) or (side == "r" and not paw_r):
            continue
        out += s(f"M{cx-10},182 C{cx-10},172 {cx+10},172 {cx+10},182 C{cx+10},190 {cx-10},190 {cx-10},182 Z",
                 mitt or WHITE, 4.2)
        if not mitt:
            out += line(f"M{cx-3},184 L{cx-3},188 M{cx+3},184 L{cx+3},188", 1.4)
    return out


def collar():
    # Tucked up under her chin, tag to the side: the all-done heart floats on her chest
    # just below it, at the same stage spot as Pippa's (116..138, 121..134).
    d = "M108,108 C118,115 142,115 152,108 L154,114 C142,121 118,121 106,114 Z"
    out = s(d, "#1F1A1C", 3.0)   # black, with her orange-and-red pattern and a metal ring
    for x in (114, 124, 134, 144):
        out += line(f"M{x},113 L{x+4},116.5 L{x+8},113", 1.4, "#E0602E")
    out += '<circle cx="110" cy="117" r="3.4" fill="none" stroke="#A9AFB8" stroke-width="1.6"/>'
    out += s("M145,118 C145,115 151,115 151,118 C151,122 145,122 145,118 Z", "#C9CED8", 2.2)   # tag
    return out


def _mirror_tok(t):
    """Mirror one SVG path token across x = 130."""
    if "," not in t:
        return t
    cmd = t[0] if t[0].isalpha() else ""
    x, y = t[len(cmd):].split(",")
    return f"{cmd}{260 - float(x):g},{y}"


def ears():
    """Long, wavy copper ears set high on the skull, hanging past the jaw, with a
    lighter feathered fringe; fur strands drawn over the top."""
    l = ("M102,20 C88,14 72,22 66,38 C62,48 65,56 61,66 C57,76 63,84 59,94 C56,104 62,110 59,118 "
         "C64,116 65,124 70,126 C72,120 76,128 81,129 C82,122 86,126 90,122 "
         "C88,114 93,108 92,98 C91,86 95,74 96,60 C98,44 106,30 102,20 Z")
    r = " ".join(_mirror_tok(t) for t in l.split())
    out = s(l, "url(#ear)", 2.6) + s(r, "url(#ear)", 2.6)
    # just a couple of soft waves: clean shapes, like the TV pups
    strands = ("M88,30 C78,44 74,58 74,70 C74,82 68,88 70,100 C72,110 68,114 69,120 "
               "M94,36 C86,52 84,64 86,76 C88,88 80,96 82,108 C83,116 80,120 81,124")
    out += line(strands, 1.4, ORANGE_D, 'opacity="0.6"')
    out += line(" ".join(_mirror_tok(t) for t in strands.split()), 1.4, ORANGE_D, 'opacity="0.6"')
    hi = "M84,40 C78,56 76,74 77,96 M164,40 C170,56 172,74 171,96"
    out += line(hi, 1.2, ORANGE_L, 'opacity="0.8"')
    return out


SKULL = ("M130,14 C158,14 176,30 177,54 C178,68 172,80 162,88 C155,94 152,104 148,112 "
         "C144,120 138,124 130,124 C122,124 116,120 112,112 C108,104 105,94 98,88 "
         "C88,80 82,68 83,54 C84,30 102,14 130,14 Z")


def eye(cx, cy, flip):
    """A big glossy cartoon eye (the TV-pup look): amber iris filling most of it, a
    large pupil and two bright glints."""
    k = -1 if flip else 1
    out = (f'<ellipse cx="{cx}" cy="{cy}" rx="11.5" ry="13" fill="#FFFFFF" stroke="{EYE_RIM}" stroke-width="2"/>'
           f'<ellipse cx="{cx+1.5*k}" cy="{cy+1.5}" rx="9" ry="10.5" fill="url(#iris)"/>'
           f'<ellipse cx="{cx+1.5*k}" cy="{cy+2}" rx="5.4" ry="6.4" fill="#1E140F"/>'
           f'<circle cx="{cx-2*k}" cy="{cy-3.5}" r="3.4" fill="#FFFFFF"/>'
           f'<circle cx="{cx+4.5*k}" cy="{cy+6}" r="1.6" fill="#FFFFFF"/>')
    out += line(f"M{cx-12},{cy-3} C{cx-10},{cy-14} {cx+10},{cy-14} {cx+12},{cy-3}", 2.6, EYE_RIM)  # lid
    return out


def head(hat=None):
    out = s(SKULL, "url(#coat)", 2.6)
    # the white: a narrow blaze from the crown, widening below the eyes into a fully
    # white muzzle and chin; orange stays around the eyes and over the cheeks
    out += ('<clipPath id="skull"><path d="' + SKULL + '"/></clipPath><g clip-path="url(#skull)">'
            '<path d="M127,10 L133,10 C134,30 135,50 137,68 C146,72 155,80 157,90 C160,106 148,128 130,128 '
            'C112,128 100,106 103,90 C105,80 114,72 123,68 C125,50 126,30 127,10 Z" fill="url(#muzzle)"/>'
            # soft fur edge where white meets orange, and shading down the muzzle sides
            + line("M137,68 C146,72 155,80 157,90 M123,68 C114,72 105,80 103,90", 2.6, "#E9B07E", 'opacity="0.7"')
            + line("M110,96 C112,106 118,116 126,121 M150,96 C148,106 142,116 134,121", 4, "#E6D9CD", 'opacity="0.8"')
            + '</g>')
    out += f'<path d="{SKULL}" fill="none" stroke="{INK}" stroke-width="2.4"/>'
    # brow fur and the little furrow over the eyes
    out += line("M101,44 C106,40 114,40 118,43 M142,43 C146,40 154,40 159,44", 3.0, ORANGE_D)
    out += eye(110, 62, False) + eye(150, 62, True)
    # muzzle: a soft shadow under the nose, the nose, the mouth
    out += '<ellipse cx="130" cy="104" rx="16" ry="9" fill="#E8DCD2" opacity="0.8"/>'
    # nose: the simple rounded one, pink-liver with a single highlight
    out += s("M120,87 C120,81 140,81 140,87 C140,93 134,99 130,99 C126,99 120,93 120,87 Z", NOSE, 3.2)
    out += '<ellipse cx="126" cy="86" rx="3.4" ry="2" fill="#E3A79A"/>'
    out += line("M130,99 L130,105 M130,105 C127,108 122,108 119,106 M130,105 C133,108 138,108 141,106", 1.6, INK)
    out += dots([(115, 96), (118, 101), (112, 101), (145, 96), (142, 101), (148, 101)], 1.5, "#D58A52")
    out += ears()
    if hat:
        out += hat
    return out


# ------------------------------------------------------------------ clothes

HEART = "M130,140 C126,134 120,138 124,144 L130,150 L136,144 C140,138 134,134 130,140 Z"


def tee(color="#F7A8C6"):
    d = ("M100,124 C100,114 112,110 130,110 C148,110 160,114 160,124 L164,172 "
         "C150,176 110,176 96,172 Z")
    return s(d, color) + s(HEART, "#E8467C", 2.0)


def sleeves_short(color="#F7A8C6"):
    out = ""
    for d in ("M102,124 C94,130 90,138 88,148 L104,152 C106,142 108,136 112,132 Z",
              "M158,124 C166,130 170,138 172,148 L156,152 C154,142 152,136 148,132 Z"):
        out += s(d, color, 4.4)
    return out


def jeans(color="#4A78C8", cuff="#7FA3E0"):
    d = ("M96,170 L164,170 L162,226 L136,226 L131,190 L129,190 L124,226 L98,226 Z")
    out = s(d, color)
    out += line("M130,172 L130,188", 1.6, "#2F558F")
    out += s("M98,220 L124,220 L124,228 L98,228 Z", cuff, 3.2) + s("M136,220 L162,220 L162,228 L136,228 Z", cuff, 3.2)
    return out


def shorts(color="#B8A0F0"):
    return s("M96,168 L164,168 L166,198 L134,198 L130,186 L126,198 L94,198 Z", color)


def sneakers(trim="#F06C9B"):
    out = ""
    for cx in (114, 146):
        out += s(f"M{cx-16},238 C{cx-16},226 {cx+16},226 {cx+16},236 L{cx+16},240 L{cx-16},240 Z", "#FFFFFF", 4.4)
        out += s(f"M{cx-16},236 L{cx+16},236 L{cx+16},241 L{cx-16},241 Z", trim, 3.0)
        out += line(f"M{cx-6},230 L{cx+6},230", 1.6, trim)
    return out


def boots(color, top=None, h=24):
    out = ""
    for cx in (114, 146):
        y0 = 240 - h
        out += s(f"M{cx-13},{y0} L{cx+13},{y0} L{cx+13},232 C{cx+17},234 {cx+17},241 {cx+12},241 "
                 f"L{cx-14},241 C{cx-17},241 {cx-17},233 {cx-13},232 Z", color, 4.4)
        if top:
            out += s(f"M{cx-15},{y0-4} L{cx+15},{y0-4} L{cx+15},{y0+4} L{cx-15},{y0+4} Z", top, 3.4)
    return out


def mary_janes(color="#E8467C"):
    out = ""
    for cx in (114, 146):
        out += s(f"M{cx-15},236 C{cx-15},228 {cx+15},228 {cx+15},236 C{cx+15},242 {cx-15},242 {cx-15},236 Z",
                 color, 4.2)
        out += line(f"M{cx-10},231 L{cx+10},231", 2.4, "#B8204F")
    return out


def dress():
    out = s("M104,122 C104,114 116,112 130,112 C144,112 156,114 156,122 L158,150 L102,150 Z", "#F7A1C4")
    out += s("M100,148 L160,148 L176,198 C150,206 110,206 84,198 Z", "#F7A1C4")
    out += line("M108,160 L100,196 M130,160 L130,202 M152,160 L160,196", 1.6, "#E57CA6")
    out += s("M100,146 L160,146 L160,154 L100,154 Z", "#E8467C", 3.0)   # sash
    out += s("M124,150 C118,144 116,156 124,152 C128,150 132,150 136,152 C144,156 142,144 136,150 Z",
             "#E8467C", 2.2)   # bow
    out += s(HEART.replace("140", "128").replace("134", "122").replace("138", "126").replace("144", "132")
             .replace("150", "138"), "#E8467C", 1.8)
    return out


def open_jacket(color, lining=None):
    """An open jacket over the top: two front panels and long sleeves."""
    out = ""
    for d in ("M100,122 C100,114 110,110 120,110 L122,176 L94,176 Z",
              "M160,122 C160,114 150,110 140,110 L138,176 L166,176 Z"):
        out += s(d, color)
    out += line("M120,112 L118,140 M140,112 L142,140", 1.8, INK)   # lapels
    return out


def hoodie(color="#B9A3E3"):
    out = open_jacket(color)
    out += s("M100,116 C108,104 152,104 160,116 C150,112 110,112 100,116 Z", color, 3.6)   # hood rim
    out += line("M124,120 L124,138 M136,120 L136,138", 1.8, "#FFFFFF")   # drawstrings
    return out


def scarf():
    out = s("M104,112 C116,124 144,124 156,112 L158,124 C144,134 116,134 102,124 Z", "#FBE3A8", 3.6)
    out += s("M138,124 L150,126 L148,160 L136,158 Z", "#FBE3A8", 3.6)
    out += line("M140,136 L148,138 M139,146 L148,148", 2.2, "#F06C9B")
    return out


def raincoat(color="#F8D33C"):
    d = ("M98,124 C98,114 110,108 130,108 C150,108 162,114 162,124 L170,196 C150,202 110,202 90,196 Z")
    out = s(d, color)
    out += line("M130,112 L130,198", 2.0)
    out += "".join(f'<circle cx="138" cy="{y}" r="2.6" fill="{INK}"/>' for y in (130, 150, 170))
    out += s("M106,166 L122,166 L122,178 L106,178 Z", color, 2.6)   # pocket
    out += s("M138,166 L154,166 L154,178 L138,178 Z", color, 2.6)
    return out


def puffer(color="#F7A7C7"):
    d = ("M98,124 C98,112 110,106 130,106 C150,106 162,112 162,124 L168,186 C150,192 110,192 92,186 Z")
    out = s(d, color)
    # green-and-white striped sweater showing at the front
    out += ('<clipPath id="front"><path d="M122,112 L138,112 L140,188 L120,188 Z"/></clipPath>'
            '<g clip-path="url(#front)">' + '<path d="M120,112 L140,112 L140,190 L120,190 Z" fill="#FFFFFF"/>'
            + "".join(f'<path d="M118,{y} L142,{y} L142,{y+6} L118,{y+6} Z" fill="#3FB56A"/>'
                      for y in range(114, 190, 12)) + '</g>')
    out += line("M122,112 L120,188 M138,112 L140,188", 2.2)
    out += line("M100,140 L120,140 M140,140 L162,140 M98,160 L120,160 M140,160 L164,160", 1.8, "#E57CA6")
    return out


def beanie():
    out = s("M86,40 C88,10 172,10 174,40 Z", "#F7A1C4")
    out += s("M82,36 L178,36 L178,50 L82,50 Z", "#F27EAF", 4.0)
    out += line("M96,38 L96,48 M108,38 L108,48 M120,38 L120,48 M132,38 L132,48 M144,38 L144,48 M156,38 L156,48 "
                "M168,38 L168,48", 1.6, "#E05591")
    return out


def sunglasses():
    out = ""
    for cx in (106, 154):
        h = (f"M{cx},76 C{cx-4},70 {cx-20},64 {cx-18},56 C{cx-16},48 {cx-6},48 {cx},56 "
             f"C{cx+6},48 {cx+16},48 {cx+18},56 C{cx+20},64 {cx+4},70 {cx},76 Z")
        out += s(h, "#F06C9B", 3.4) + f'<path d="{h}" fill="#3B2A4A" opacity="0.55" transform="translate({cx*0.12:.1f},6.6) scale(0.88)"/>'
    out += line("M120,58 C126,54 134,54 140,58", 3.2, "#F06C9B")
    return out


def water_bottle():
    out = s("M170,150 L186,150 L186,192 C186,196 170,196 170,192 Z", "#C9B6F2", 3.8)
    out += s("M172,142 L184,142 L184,151 L172,151 Z", "#8A6FD6", 3.0)
    return out


def umbrella():
    # held in her left paw (viewer's left), a big dome up and out to the side like Pippa's
    out = line("M50,30 L90,180", 4.2, INK)
    out += line("M90,180 C93,190 84,193 81,186", 4.2, INK)
    dome = ("M2,74 C2,40 24,18 52,18 C80,18 102,40 102,74 "
            "C96,66 86,66 80,74 C74,66 62,66 56,74 C50,66 38,66 32,74 C26,66 12,66 2,74 Z")
    out += s(dome, "#F5A3C7")
    out += line("M52,20 C40,36 34,54 32,72 M52,20 C54,38 56,56 56,72 M52,20 C66,36 76,54 80,72", 1.8, "#E57CA6")
    out += s("M49,8 L55,8 L54,19 L50,19 Z", INK, 1.0)
    return out


# ------------------------------------------------------------------ outfits

def stage_svg(outfit="base"):
    o = outfit
    out = tail()
    if o == "dress":
        out += legs(sock="#2E2E3A") + mary_janes()
        out += torso(fur=False) + dress()
        out += open_jacket("#5EC2F0")
        out += arms(sleeve="#5EC2F0", cuff="#5EC2F0")
    elif o == "pants":
        out += legs() + torso(fur=False) + jeans() + sneakers()
        out += tee() + open_jacket("#5EC2F0")
        out += arms(sleeve="#5EC2F0", cuff="#8FD6F6")
    elif o == "fall":
        out += legs() + torso(fur=False) + jeans() + sneakers()
        out += tee() + hoodie()
        out += arms(sleeve="#B9A3E3", cuff="#A48BD8")
    elif o == "rain":
        out += umbrella()
        out += legs() + torso(fur=False) + jeans("#2B3A6B", "#3D4F86") + boots("#263A6B")
        out += raincoat()
        out += arms(sleeve="#F8D33C", cuff="#F0C21F")
    elif o == "hot":
        out += legs() + torso(fur=False) + shorts() + sneakers()
        out += tee() + sleeves_short()
        out += arms(paw_r=True) + water_bottle()
    elif o == "snow":
        out += legs() + torso(fur=False) + jeans("#2B3A6B", "#3D4F86") + boots("#E9E2FA", top="#D8CCF5", h=26)
        out += puffer()
        out += arms(sleeve="#F7A7C7", mitt="#A88BE8")
    else:   # base: just her
        out += legs() + paws_feet() + torso() + arms() + ruff()
    if o in ("dress", "pants", "fall", "rain", "hot", "base"):
        out += collar()
    if o == "fall":
        out += scarf()
    hat = beanie() if o == "snow" else None
    out += head(hat)
    if o == "hot":
        out += sunglasses()
    return _svg(out)


def head_svg():
    """Head and ears only, for the round badges (cropped like Pippa's head window)."""
    return _svg(head(), w=150, h=134, vb="56 4 150 134")


if __name__ == "__main__":
    import sys
    print(stage_svg(sys.argv[1] if len(sys.argv) > 1 else "base"))
