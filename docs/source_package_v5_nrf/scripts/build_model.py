import os, math, struct, zipfile, json
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.path import Path
from matplotlib.tri import Triangulation
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

OUT = "/home/claude/out"
os.makedirs(OUT, exist_ok=True)

# ------------------------------------------------------------------ parameters, mm
A, B, N = 25.0, 16.5, 2.4          # outer body half-length, half-width, superellipse exponent
H, WALL, SPLIT = 10.5, 1.2, 5.25   # total height, wall, plane between shells
PCB_T, PCB_Z0 = 1.0, 4.6
PCB_X0, PCB_X1, CLR = -17.4, 22.3, 0.8
BAT_D, BAT_H, BAT_Z0, BAT_C = 20.0, 3.2, 1.3, (8.0, 0.0)
SW1, SW2 = (-12.1, 0.0), (8.7, 0.0)
SW_BODY, SW_PLUNGER_TOP = 6.2, 9.0
CAP_SMALL, CAP_BIG = 15.0, 17.0
HOLE_SMALL, HOLE_BIG = 15.8, 17.8
PCB_TOP = PCB_Z0 + PCB_T

def g(x, y, a, b, n=N):
    return (np.abs(x) / a) ** n + (np.abs(y) / b) ** n

def inner_in(x, y):
    return g(x, y, A - WALL, B - WALL) <= 1.0

def outer_in(x, y):
    return g(x, y, A, B) <= 1.0

def contour_poly(F, xr, yr, h=0.05):
    xs = np.arange(xr[0], xr[1], h); ys = np.arange(yr[0], yr[1], h)
    X, Y = np.meshgrid(xs, ys)
    fig = plt.figure(); ax = fig.add_subplot(111)
    cs = ax.contour(X, Y, F(X, Y), [0.0])
    segs = max(cs.allsegs[0], key=len)
    plt.close(fig)
    p = np.array(segs)
    if np.allclose(p[0], p[-1]):
        p = p[:-1]
    area = 0.5 * np.sum(p[:, 0] * np.roll(p[:, 1], -1) - np.roll(p[:, 0], -1) * p[:, 1])
    if area < 0:
        p = p[::-1]
    return p

def circle_poly(cx, cy, r, n=64):
    t = np.linspace(0, 2 * np.pi, n, endpoint=False)
    return np.c_[cx + r * np.cos(t), cy + r * np.sin(t)]

def stadium_poly(cx, cy, half_len_y, r, n=24):
    t = np.linspace(-np.pi / 2, np.pi / 2, n)
    right = np.c_[cx + r * np.cos(t), cy + half_len_y + r * np.sin(t + np.pi / 2) * 0 + 0]
    # build explicitly: top semicircle then bottom semicircle (CCW)
    top = np.c_[cx + r * np.cos(np.linspace(0, np.pi, n)), cy + half_len_y + r * np.sin(np.linspace(0, np.pi, n))]
    bot = np.c_[cx + r * np.cos(np.linspace(np.pi, 2 * np.pi, n)), cy - half_len_y + r * np.sin(np.linspace(np.pi, 2 * np.pi, n))]
    return np.vstack([top, bot])

def rrect_poly(x0, x1, y0, y1, r, n=10):
    pts = []
    for (cx, cy, a0) in [(x1 - r, y0 + r, -90), (x1 - r, y1 - r, 0), (x0 + r, y1 - r, 90), (x0 + r, y0 + r, 180)]:
        for a in np.linspace(a0, a0 + 90, n):
            pts.append((cx + r * math.cos(math.radians(a)), cy + r * math.sin(math.radians(a))))
    return np.array(pts)

# ------------------------------------------------------------------ mesh helpers
def quad(a, b, c, d):
    return [(a, b, c), (a, c, d)]

def box(c, s):
    cx, cy, cz = c; sx, sy, sz = s[0] / 2, s[1] / 2, s[2] / 2
    v = np.array([[cx + i * sx, cy + j * sy, cz + k * sz] for k in (-1, 1) for j in (-1, 1) for i in (-1, 1)])
    t = []
    for q in [(0, 2, 3, 1), (4, 5, 7, 6), (0, 1, 5, 4), (2, 6, 7, 3), (0, 4, 6, 2), (1, 3, 7, 5)]:
        t += quad(v[q[0]], v[q[1]], v[q[2]], v[q[3]])
    return np.array(t)

def wall(poly, z0, z1, outward=True):
    t = []
    n = len(poly)
    for i in range(n):
        p, q = poly[i], poly[(i + 1) % n]
        a = (p[0], p[1], z0); b = (q[0], q[1], z0); c = (q[0], q[1], z1); d = (p[0], p[1], z1)
        t += quad(a, b, c, d) if outward else quad(a, d, c, b)
    return np.array(t)

def fan(poly, z, up=True):
    c = poly.mean(axis=0)
    t = []
    n = len(poly)
    for i in range(n):
        p, q = poly[i], poly[(i + 1) % n]
        tri = [(c[0], c[1], z), (p[0], p[1], z), (q[0], q[1], z)]
        if not up:
            tri = [tri[0], tri[2], tri[1]]
        t.append(tri)
    return np.array(t)

def prism_convex(poly, z0, z1):
    return np.vstack([fan(poly, z1, True), fan(poly, z0, False), wall(poly, z0, z1, True)])

def cylinder(cx, cy, z0, z1, r, n=48):
    p = circle_poly(cx, cy, r, n)
    return prism_convex(p, z0, z1)

def resample(poly, step):
    pts = []
    n = len(poly)
    for i in range(n):
        p, q = poly[i], poly[(i + 1) % n]
        L = float(np.hypot(*(q - p)))
        k = max(1, int(math.ceil(L / step)))
        for j in range(k):
            pts.append(p + (q - p) * j / k)
    return np.array(pts)

def region(inside, boundaries, bbox, z, up, step=0.4, grid=0.9):
    pts = [resample(b, step) for b in boundaries]
    xs = np.arange(bbox[0], bbox[1], grid); ys = np.arange(bbox[2], bbox[3], grid)
    gx, gy = np.meshgrid(xs, ys)
    gp = np.c_[gx.ravel(), gy.ravel()]
    gp = gp[inside(gp[:, 0], gp[:, 1])]
    allp = np.vstack(pts + [gp])
    tri = Triangulation(allp[:, 0], allp[:, 1])
    T = tri.triangles
    cen = allp[T].mean(axis=1)
    T = T[inside(cen[:, 0], cen[:, 1])]
    P = allp[T]
    cr = (P[:, 1, 0] - P[:, 0, 0]) * (P[:, 2, 1] - P[:, 0, 1]) - (P[:, 1, 1] - P[:, 0, 1]) * (P[:, 2, 0] - P[:, 0, 0])
    flip = (cr < 0) if up else (cr > 0)
    T = np.where(flip[:, None], T[:, [0, 2, 1]], T)
    P = allp[T]
    return np.concatenate([P, np.full((len(T), 3, 1), z)], axis=2)

def inpoly(poly):
    path = Path(poly)
    return lambda x, y: path.contains_points(np.c_[np.ravel(x), np.ravel(y)]).reshape(np.shape(x))

# ------------------------------------------------------------------ geometry
outer = contour_poly(lambda X, Y: g(X, Y, A, B) - 1, (-A - 1, A + 1), (-B - 1, B + 1))
inner = contour_poly(lambda X, Y: g(X, Y, A - WALL, B - WALL) - 1, (-A, A), (-B, B))
pcb = contour_poly(lambda X, Y: np.maximum.reduce([g(X, Y, A - WALL - CLR, B - WALL - CLR) - 1, PCB_X0 - X, X - PCB_X1]),
                   (-A, A), (-B, B))
pcb = resample(pcb, 0.4)

hole_big = circle_poly(SW2[0], SW2[1], HOLE_BIG / 2)
hole_small = circle_poly(SW1[0], SW1[1], HOLE_SMALL / 2)
in_hb, in_hs = inpoly(hole_big), inpoly(hole_small)
BB = (-A - 0.5, A + 0.5, -B - 0.5, B + 0.5)

parts = {}   # name -> dict(tris, color, group)

def add(name, tris, color, group):
    tris = np.asarray(tris, float).reshape(-1, 3, 3)
    if name in parts:
        parts[name]["tris"] = np.vstack([parts[name]["tris"], tris])
    else:
        parts[name] = dict(tris=tris, color=color, group=group)

BLACK = (0.13, 0.13, 0.14)
# back shell
add("case_back", wall(outer, 0, SPLIT, True), BLACK, "back")
add("case_back", region(outer_in, [outer], BB, 0.0, False), BLACK, "back")
add("case_back", wall(inner, WALL, SPLIT, False), BLACK, "back")
add("case_back", region(inner_in, [inner], BB, WALL, True), BLACK, "back")
ring = lambda x, y: outer_in(x, y) & ~inner_in(x, y)
add("case_back", region(ring, [outer, inner], BB, SPLIT, True), BLACK, "back")
# top shell
add("case_top", wall(outer, SPLIT, H, True), BLACK, "top")
plate = lambda x, y: outer_in(x, y) & ~in_hb(x, y) & ~in_hs(x, y)
add("case_top", region(plate, [outer, hole_big, hole_small], BB, H, True), BLACK, "top")
plate_in = lambda x, y: inner_in(x, y) & ~in_hb(x, y) & ~in_hs(x, y)
add("case_top", region(plate_in, [inner, hole_big, hole_small], BB, H - WALL, False), BLACK, "top")
add("case_top", wall(inner, SPLIT, H - WALL, False), BLACK, "top")
add("case_top", wall(hole_big, H - WALL, H, False), BLACK, "top")
add("case_top", wall(hole_small, H - WALL, H, False), BLACK, "top")
add("case_top", region(ring, [outer, inner], BB, SPLIT, False), BLACK, "top")
# keyring lug
lug = rrect_poly(-33.0, -23.0, -6.5, 6.5, 2.5)
slot = stadium_poly(-29.0, 0.0, 2.2, 1.3)
in_lug, in_slot = inpoly(lug), inpoly(slot)
lugf = lambda x, y: in_lug(x, y) & ~in_slot(x, y)
lz0, lz1 = SPLIT - 1.6, SPLIT + 1.6
add("keyring_lug", region(lugf, [lug, slot], (-34, -22, -8, 8), lz1, True, grid=0.5), BLACK, "back")
add("keyring_lug", region(lugf, [lug, slot], (-34, -22, -8, 8), lz0, False, grid=0.5), BLACK, "back")
add("keyring_lug", wall(lug, lz0, lz1, True), BLACK, "back")
add("keyring_lug", wall(slot, lz0, lz1, False), BLACK, "back")
# button caps
GREY = (0.58, 0.60, 0.63)
add("cap_ios", cylinder(SW2[0], SW2[1], SW_PLUNGER_TOP, H + 0.2, CAP_BIG / 2), GREY, "caps")
add("cap_android", cylinder(SW1[0], SW1[1], SW_PLUNGER_TOP, H + 0.2, CAP_SMALL / 2), GREY, "caps")
# battery
add("battery_cr2032", cylinder(BAT_C[0], BAT_C[1], BAT_Z0, BAT_Z0 + BAT_H, BAT_D / 2, 72), (0.80, 0.81, 0.83), "bat")
GOLD = (0.85, 0.68, 0.22)
add("battery_contacts", box((17.2, 0.0, PCB_Z0 - 0.15), (3.4, 4.0, 0.3)), GOLD, "pcb")
# pcb
add("pcb", prism_convex(pcb, PCB_Z0, PCB_TOP), (0.10, 0.45, 0.26), "pcb")

# components (x, y, sx, sy, sz, color, ref)
SILVER = (0.78, 0.78, 0.80); DARK = (0.08, 0.08, 0.09); TAN = (0.72, 0.60, 0.42); REDP = (0.45, 0.12, 0.30)
comps = [
    ("U1", -1.8, 0.6, 5.0, 5.0, 0.9, DARK, "nRF52810"),
    ("X1", 3.0, 4.2, 2.0, 1.6, 0.6, SILVER, "X1 32 МГц"),
    ("C1", 1.6, 5.9, 1.0, 0.5, 0.35, TAN, "C1"), ("C2", 4.6, 5.9, 1.0, 0.5, 0.35, TAN, "C2"),
    ("C4", -5.4, 3.0, 1.0, 0.5, 0.35, TAN, "C4"), ("C5", -5.4, -1.6, 1.0, 0.5, 0.35, TAN, "C5"),
    ("C7", -3.4, -3.9, 1.0, 0.5, 0.35, TAN, "C7"), ("C9", -0.2, -3.9, 1.6, 0.8, 0.8, TAN, "C9"), ("C10", 2.2, -3.9, 1.6, 0.8, 0.8, TAN, "C10"), ("C8", 3.6, -2.2, 1.0, 0.5, 0.35, TAN, "C8"),
    ("L1", -3.4, 4.9, 1.0, 0.5, 0.4, TAN, "L1"), ("C3", -5.8, 5.6, 1.0, 0.5, 0.35, TAN, "C3"),
    ("D1D2", -12.5, -6.2, 2.9, 1.3, 1.1, DARK, "D1 D2"),
    ("Q1", -9.0, -8.0, 2.9, 1.3, 1.1, DARK, "Q1"),
    ("Q2", -12.5, -9.6, 2.9, 1.3, 1.1, DARK, "Q2"),
    ("R1", -7.2, -6.0, 1.0, 0.5, 0.35, TAN, "R1"), ("Cg", -8.4, -4.4, 1.0, 0.5, 0.35, TAN, "Cg"),
    ("Rg", -10.2, -11.4, 1.0, 0.5, 0.35, TAN, "Rg"), ("Rpd", -8.0, -11.4, 1.0, 0.5, 0.35, TAN, "Rpd"),
    ("Rs1", -5.9, -8.4, 1.0, 0.5, 0.35, TAN, "Rs1"), ("Rs2", -5.9, -9.8, 1.0, 0.5, 0.35, TAN, "Rs2"),
    ("Q3", 15.8, 2.6, 2.9, 1.3, 1.1, DARK, "Q3"),
    ("Cbulk", 14.8, -3.2, 3.2, 1.6, 1.8, TAN, "Cvdd 22 мкФ"),
    ("Rled1", 17.4, 6.9, 1.0, 0.5, 0.35, TAN, "R1л"), ("Rled2", 17.4, -6.9, 1.0, 0.5, 0.35, TAN, "R2л"),
    ("IR1", 20.3, 4.4, 2.51, 3.2, 1.2, REDP, "IR1 VSMY14940"), ("IR2", 20.3, -4.4, 2.51, 3.2, 1.2, REDP, "IR2 VSMY14940"),
    ("Cm", -6.4, -11.4, 1.0, 0.5, 0.35, TAN, "Cm"), ("JP1", 18.4, -3.2, 1.6, 0.8, 0.08, GOLD, "JP1"),
]
for ref, x, y, sx, sy, sz, col, _ in comps:
    add("pcb_components", box((x, y, PCB_TOP + sz / 2), (sx, sy, sz)), col, "pcb")
    parts["pcb_components"]["color"] = (0.30, 0.30, 0.32)
# per-component colours kept separately for the render
comp_tris = []
for ref, x, y, sx, sy, sz, col, _ in comps:
    comp_tris.append((box((x, y, PCB_TOP + sz / 2), (sx, sy, sz)), col))
del parts["pcb_components"]
for k, (t, col) in enumerate(comp_tris):
    add("comp_%02d" % k, t, col, "pcb")
# tact switches
for nm, (x, y) in (("sw1_android", SW1), ("sw2_ios", SW2)):
    add(nm, box((x, y, PCB_TOP + 1.3), (SW_BODY, SW_BODY, 2.6)), SILVER, "pcb")
    add(nm, cylinder(x, y, PCB_TOP + 2.6, SW_PLUNGER_TOP, 1.75, 32), (0.15, 0.15, 0.16), "pcb")
# antenna meander and SWD pads
ant = [(-6.0, 6.8), (-8.0, 6.8), (-8.0, 12.0), (-9.6, 12.0), (-9.6, 7.6), (-11.2, 7.6), (-11.2, 12.0),
       (-12.8, 12.0), (-12.8, 7.6), (-14.4, 7.6), (-14.4, 12.0)]
COPPER = (0.85, 0.55, 0.30)
for (x1, y1), (x2, y2) in zip(ant[:-1], ant[1:]):
    cx, cy = (x1 + x2) / 2, (y1 + y2) / 2
    sx, sy = abs(x2 - x1) + 0.4, abs(y2 - y1) + 0.4
    add("antenna_meander", box((cx, cy, PCB_TOP + 0.03), (sx, sy, 0.06)), COPPER, "pcb")
swd_pads = [(-6.5, -12.4), (-4.0, -12.4), (-1.5, -12.4), (1.0, -12.4)]
for x, y in swd_pads:
    add("swd_pads", cylinder(x, y, PCB_TOP, PCB_TOP + 0.06, 0.8, 24), GOLD, "pcb")
# drill marks for IR windows on the end wall
def wall_x(y):
    return A * (1 - (abs(y) / B) ** N) ** (1 / N)
for y in (4.4, -4.4):
    add("drill_marks", box((wall_x(y) + 0.05, y, 6.1), (0.25, 3.4, 1.6)), (0.95, 0.55, 0.10), "top")

# ------------------------------------------------------------------ STL export
def write_stl(path, tris, name="part"):
    tris = np.asarray(tris, np.float32)
    n = np.cross(tris[:, 1] - tris[:, 0], tris[:, 2] - tris[:, 0])
    ln = np.linalg.norm(n, axis=1, keepdims=True); ln[ln == 0] = 1
    n = n / ln
    with open(path, "wb") as f:
        f.write(name.encode()[:80].ljust(80, b" "))
        f.write(struct.pack("<I", len(tris)))
        for i in range(len(tris)):
            f.write(struct.pack("<3f", *n[i]))
            for v in tris[i]:
                f.write(struct.pack("<3f", *v))
            f.write(b"\x00\x00")

stl_dir = os.path.join(OUT, "stl"); os.makedirs(stl_dir, exist_ok=True)
groups_for_stl = {
    "case_back": ["case_back", "keyring_lug"],
    "case_top": ["case_top"],
    "button_caps": ["cap_ios", "cap_android"],
    "battery_CR2032": ["battery_cr2032", "battery_contacts"],
    "pcb_with_components": ["pcb", "sw1_android", "sw2_ios", "antenna_meander", "swd_pads"] + [k for k in parts if k.startswith("comp_")],
}
allt = []
for nm, keys in groups_for_stl.items():
    t = np.vstack([parts[k]["tris"] for k in keys])
    write_stl(os.path.join(stl_dir, nm + ".stl"), t, nm)
    allt.append(t)
write_stl(os.path.join(stl_dir, "assembly_closed.stl"), np.vstack(allt), "assembly")
zp = os.path.join(OUT, "fob_model_stl.zip")
with zipfile.ZipFile(zp, "w", zipfile.ZIP_DEFLATED) as z:
    for fn in sorted(os.listdir(stl_dir)):
        z.write(os.path.join(stl_dir, fn), fn)

# ------------------------------------------------------------------ z-buffer renderer
from PIL import Image
EXPLODE = {"back": 0, "bat": 0, "pcb": 8.0, "top": 17.0, "caps": 22.0}

def raster(T, C, elev, azim, W, Hh, scale, cx, cy):
    a, e = math.radians(azim), math.radians(elev)
    d = np.array([math.cos(e) * math.cos(a), math.cos(e) * math.sin(a), math.sin(e)])
    r = np.array([-math.sin(a), math.cos(a), 0.0])
    u = np.array([-math.sin(e) * math.cos(a), -math.sin(e) * math.sin(a), math.cos(e)])
    light = -0.45 * r + 0.55 * u + 0.75 * d; light /= np.linalg.norm(light)
    n = np.cross(T[:, 1] - T[:, 0], T[:, 2] - T[:, 0])
    ln = np.linalg.norm(n, axis=1, keepdims=True); ln[ln == 0] = 1; n = n / ln
    face = np.where((n @ d)[:, None] < 0, -n, n)
    shade = 0.38 + 0.80 * np.clip(face @ light, 0, 1) + 0.0
    col = np.clip(C * shade[:, None], 0, 1)
    sx = T @ r * scale + W / 2 - cx * scale
    sy = Hh / 2 - (T @ u * scale - cy * scale)
    dep = T @ d
    zbuf = np.full((Hh, W), -1e18); img = np.ones((Hh, W, 3))
    for i in range(len(T)):
        x0, x1, x2 = sx[i]; y0, y1, y2 = sy[i]
        minx = max(int(math.floor(min(x0, x1, x2))), 0); maxx = min(int(math.ceil(max(x0, x1, x2))), W - 1)
        miny = max(int(math.floor(min(y0, y1, y2))), 0); maxy = min(int(math.ceil(max(y0, y1, y2))), Hh - 1)
        if maxx < minx or maxy < miny:
            continue
        den = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2)
        if abs(den) < 1e-12:
            continue
        gx, gy = np.meshgrid(np.arange(minx, maxx + 1) + 0.5, np.arange(miny, maxy + 1) + 0.5)
        w0 = ((y1 - y2) * (gx - x2) + (x2 - x1) * (gy - y2)) / den
        w1 = ((y2 - y0) * (gx - x2) + (x0 - x2) * (gy - y2)) / den
        w2 = 1 - w0 - w1
        m = (w0 >= -1e-5) & (w1 >= -1e-5) & (w2 >= -1e-5)
        if not m.any():
            continue
        z = w0 * dep[i, 0] + w1 * dep[i, 1] + w2 * dep[i, 2]
        sub = zbuf[miny:maxy + 1, minx:maxx + 1]
        upd = m & (z > sub)
        sub[upd] = z[upd]
        img[miny:maxy + 1, minx:maxx + 1][upd] = col[i]
    return img

def render3d(fname, elev, azim, hide=(), explode=False, cx=-5.0, cy=0.0, W=1500, Hh=960, scale=19.0, ss=2, case_color=None):
    Ts, Cs = [], []
    for nm, p in parts.items():
        if p["group"] in hide or nm in hide:
            continue
        t = p["tris"].copy()
        if explode:
            t[:, :, 2] += EXPLODE[p["group"]]
        Ts.append(t)
        c = np.array(p["color"], float)
        if case_color is not None and nm.startswith("case"):
            c = np.array(case_color)
        Cs.append(np.tile(c, (len(t), 1)))
    T = np.vstack(Ts); C = np.vstack(Cs)
    img = raster(T, C, elev, azim, W * ss, Hh * ss, scale * ss, cx, cy)
    im = Image.fromarray((img * 255).astype(np.uint8)).resize((W, Hh), Image.LANCZOS)
    im.save(os.path.join(OUT, fname))

CASE = (0.30, 0.31, 0.34)
render3d("fob_1_closed.png", 30, -62, cx=-4, cy=1, case_color=CASE)
render3d("fob_2_exploded.png", 24, -58, explode=True, cx=-4, cy=9, scale=15.0, case_color=CASE)
render3d("fob_3_open_top.png", 90, -90, hide=("top", "caps"), cx=-4, cy=0, scale=25.0, case_color=(0.34, 0.35, 0.38))
render3d("fob_4_open_iso.png", 38, -55, hide=("top", "caps"), cx=-4, cy=0, scale=24.0, case_color=(0.34, 0.35, 0.38))

# ------------------------------------------------------------------ checks
info = {}
bat_pts = circle_poly(BAT_C[0], BAT_C[1], BAT_D / 2, 360)
info["battery_inside_cavity"] = bool(np.all(inner_in(bat_pts[:, 0], bat_pts[:, 1])))
info["pcb_inside_cavity"] = bool(np.all(inner_in(pcb[:, 0], pcb[:, 1])))
info["pcb_len_mm"] = float(pcb[:, 0].max() - pcb[:, 0].min())
info["pcb_wid_mm"] = float(pcb[:, 1].max() - pcb[:, 1].min())
info["stack_mm"] = dict(back_wall=WALL, battery=BAT_H, gap_batt_pcb=PCB_Z0 - (BAT_Z0 + BAT_H), pcb=PCB_T,
                        switch_above_pcb=SW_PLUNGER_TOP - PCB_TOP, gap_to_top_plate=(H - WALL) - SW_PLUNGER_TOP, top_wall=WALL, total=H)
info["comps_inside_pcb"] = {}
pp = Path(pcb)
for ref, x, y, sx, sy, sz, col, lab in comps:
    corners = np.array([[x - sx / 2, y - sy / 2], [x + sx / 2, y - sy / 2], [x + sx / 2, y + sy / 2], [x - sx / 2, y + sy / 2]])
    if not pp.contains_points(corners).all():
        info["comps_inside_pcb"][ref] = False
json.dump(dict(info=info, pcb=pcb.tolist(), outer=outer.tolist(), inner=inner.tolist(),
               comps=[dict(ref=c[0], x=c[1], y=c[2], sx=c[3], sy=c[4], sz=c[5], label=c[7]) for c in comps],
               ant=ant, swd=swd_pads), open(os.path.join(OUT, "geom.json"), "w"))
print(json.dumps(info, indent=1, ensure_ascii=False))
print({k: len(v["tris"]) for k, v in parts.items() if not k.startswith("comp_")})
