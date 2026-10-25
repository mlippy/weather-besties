"""Pictures for the Routine page's steps (2.0 screens), in the sticker style of the rest of
the art: a navy outline drawn under flat fills, 100 x 100 units.

    ICONS = {name: svg}   names: potty, breakfast, teeth, hair, dress, pajamas, snack, books, snuggle
"""

INK = "#2B3450"
WHITE = "#FFFFFF"


def s(d, fill, sw=5.0, extra=""):
    return (f'<path d="{d}" fill="{fill}" stroke="{INK}" stroke-width="{sw}" '
            f'stroke-linejoin="round" stroke-linecap="round" paint-order="stroke" {extra}/>')


def line(d, sw=4.0, color=INK):
    return f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{sw}" stroke-linecap="round" stroke-linejoin="round"/>'


def svg(body, size=100):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{size}" height="{size}" viewBox="0 0 100 100">'
            f'{body}</svg>')


SHIRT = ("M30,20 L42,13 C44,20 56,20 58,13 L70,20 L88,36 L78,48 L70,42 L70,88 L30,88 L30,42 "
         "L22,48 L12,36 Z")


def potty():
    out = s("M30,10 L70,10 C73,10 75,12 75,15 L75,38 L25,38 L25,15 C25,12 27,10 30,10 Z", WHITE)   # tank
    out += s("M64,18 L70,18", WHITE, 4)                                                              # handle
    out += s("M18,46 C18,40 82,40 82,46 C82,68 70,80 60,82 L62,92 L38,92 L40,82 C30,80 18,68 18,46 Z", WHITE)
    out += s("M24,46 C24,42 76,42 76,46 C76,50 24,50 24,46 Z", "#CFE3F7", 3)                        # water
    return svg(out)


def breakfast():
    out = line("M64,50 L86,16", 7, INK) + line("M64,50 L86,16", 3, "#C9CED8")                     # spoon
    out += s("M12,50 L88,50 C88,74 72,88 50,88 C28,88 12,74 12,50 Z", "#F7A8C6")                    # bowl
    out += s("M12,50 C12,42 88,42 88,50 C88,57 12,57 12,50 Z", "#FFF7E8", 4)                        # milk
    out += "".join(f'<circle cx="{x}" cy="{y}" r="4.5" fill="#F2C230" stroke="{INK}" stroke-width="2"/>'
                   for x, y in ((30, 49), (44, 52), (56, 47), (70, 51), (38, 46)))
    out += s("M40,68 C36,62 30,66 34,72 L40,77 L46,72 C50,66 44,62 40,68 Z", "#E8467C", 2.5)     # heart
    return svg(out)


def teeth():
    g = s("M8,52 L64,52 C68,52 68,62 64,62 L8,62 C4,62 4,52 8,52 Z", "#5EC2F0")                     # handle
    g += s("M62,48 L88,48 C92,48 92,64 88,64 L62,64 Z", "#5EC2F0")                                  # head
    g += s("M64,32 L88,32 L88,48 L64,48 Z", WHITE, 4)                                               # bristles
    g += line("M70,34 L70,46 M76,34 L76,46 M82,34 L82,46", 2.2, "#9AA6BF")
    g += s("M62,30 C64,20 72,24 74,18 C78,24 86,20 90,28 C86,32 66,34 62,30 Z", "#8EE0B8", 3)      # paste
    return svg(f'<g transform="rotate(-28 50 50)">{g}</g>')


def hair():
    g = s("M50,6 C66,6 74,18 74,32 C74,46 64,56 50,56 C36,56 26,46 26,32 C26,18 34,6 50,6 Z", "#B9A3E3")
    g += "".join(f'<circle cx="{x}" cy="{y}" r="2.6" fill="{WHITE}"/>'
                 for x in (38, 50, 62) for y in (20, 32, 44))
    g += s("M43,56 L57,56 L57,92 C57,97 43,97 43,92 Z", "#B9A3E3")                                 # handle
    return svg(f'<g transform="rotate(32 50 50)">{g}</g>')


def dress():
    out = s(SHIRT, "#F7A8C6")
    out += s("M50,52 C46,45 38,49 43,56 L50,63 L57,56 C62,49 54,45 50,52 Z", "#E8467C", 2.5)
    return svg(out)


def pajamas():
    out = s(SHIRT, "#9DB4F0")
    out += s("M44,46 C36,50 36,64 46,68 C40,64 40,52 48,48 C47,47 46,46 44,46 Z", "#F8D33C", 2.5)   # moon
    star = "M60,54 L62,59 L67,59 L63,62 L65,67 L60,64 L55,67 L57,62 L53,59 L58,59 Z"
    out += s(star, "#F8D33C", 2.2)
    out += s("M58,30 L59,33 L62,33 L60,35 L61,38 L58,36 L55,38 L56,35 L54,33 L57,33 Z", "#F8D33C", 2)
    return svg(out)


def snack():
    out = line("M50,30 C50,22 52,16 56,10", 5)                                                     # stem
    out += s("M56,18 C62,8 76,10 78,14 C72,22 62,24 56,18 Z", "#4CB860", 3)                       # leaf
    out += s("M50,30 C40,22 16,24 16,52 C16,76 34,92 50,86 C66,92 84,76 84,52 C84,24 60,22 50,30 Z",
             "#E5483C")
    out += '<ellipse cx="34" cy="46" rx="5" ry="9" fill="#FFFFFF" opacity="0.6"/>'
    return svg(out)


def books():
    out = s("M14,70 L86,70 L86,88 L14,88 Z", "#5EC2F0")
    out += s("M20,52 L82,52 L82,70 L20,70 Z", "#F2C230")
    out += s("M12,34 L80,34 L80,52 L12,52 Z", "#F7A8C6")
    out += line("M74,40 L74,46 M80,58 L80,64 M80,76 L80,82", 3, WHITE)
    out += s("M60,12 L70,12 L70,34 L65,29 L60,34 Z", "#E8467C", 3)                                # bookmark
    return svg(out)


def snuggle():
    bear = "#C98A5A"
    out = s("M22,22 C22,10 38,10 38,22 Z", bear) + s("M62,22 C62,10 78,10 78,22 Z", bear)          # ears (back)
    out += '<circle cx="28" cy="22" r="10" fill="#C98A5A" stroke="#2B3450" stroke-width="5" paint-order="stroke"/>'
    out += '<circle cx="72" cy="22" r="10" fill="#C98A5A" stroke="#2B3450" stroke-width="5" paint-order="stroke"/>'
    out += '<circle cx="50" cy="50" r="34" fill="#C98A5A" stroke="#2B3450" stroke-width="5" paint-order="stroke"/>'
    out += '<circle cx="28" cy="22" r="4.5" fill="#E8B48C"/><circle cx="72" cy="22" r="4.5" fill="#E8B48C"/>'
    out += s("M36,62 C36,52 64,52 64,62 C64,72 36,72 36,62 Z", "#F1D2B4", 3)                       # muzzle
    out += '<circle cx="38" cy="44" r="4" fill="#2B3450"/><circle cx="62" cy="44" r="4" fill="#2B3450"/>'
    out += s("M45,56 C45,52 55,52 55,56 C55,59 45,59 45,56 Z", "#2B3450", 1)                      # nose
    out += line("M50,59 L50,63 M50,63 C47,66 44,66 42,64 M50,63 C53,66 56,66 58,64", 2.4)
    out += s("M80,74 C77,68 70,71 73,77 L80,84 L87,77 C90,71 83,68 80,74 Z", "#E8467C", 2.5)     # heart
    return svg(out)


ICONS = {"potty": potty(), "breakfast": breakfast(), "teeth": teeth(), "hair": hair(), "dress": dress(),
         "pajamas": pajamas(), "snack": snack(), "books": books(), "snuggle": snuggle()}
