"""Пятно ИК-светодиода VSMY14940 на стене в 1, 2, 5 м.

Модель (допущения помечены):
- Ie на оси из даташита Vishay (Rev 1.4): 82 мВт/ср тип., 50 мин. при 70 мА; линейно по току (допущение).
- Ток одного светодиода 45 мА при 3,0 В и RL 33 Ом (оценка из расчёта v5, docs/source_package_v5_nrf).
- Диаграмма: гаусс с полушириной по половине интенсивности ±9° (даташит даёт только ±9°; форма — допущение).
- Плоская стена перпендикулярно оси: E = I(θ)·cos(θ)/r², r = d/cos(θ).
- Порог приёмника: 0,12 (типично) и 0,25 мВт/м² (худший) — оценка из README v5, по методике Vishay.
"""
import math
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

IE70_TYP, IE70_MIN = 82.0, 50.0  # мВт/ср при 70 мА
I_LED = 45.0                       # мА, оценка
HW = 9.0                           # градусов, половина интенсивности
TH_TYP, TH_WORST = 0.12, 0.25      # мВт/м²

def irradiance(ie0, x, y, d):
    r2 = x**2 + y**2 + d**2
    cos_t = d / np.sqrt(r2)
    th = np.degrees(np.arccos(cos_t))
    i = ie0 * np.exp(-math.log(2) * (th / HW) ** 2)
    return i * cos_t / r2

def radius_where(ie0, d, level):
    rs = np.linspace(0, d * 2, 20001)
    e = irradiance(ie0, rs, 0, d)
    ok = rs[e >= level]
    return ok.max() if ok.size else 0.0

ie_typ = IE70_TYP * I_LED / 70
ie_min = IE70_MIN * I_LED / 70
rows = []
fig, axs = plt.subplots(1, 3, figsize=(15, 5))
for ax, d in zip(axs, [1, 2, 5]):
    half = d * math.tan(math.radians(HW))
    span = max(half * 3, radius_where(ie_typ, d, TH_WORST) * 1.15)
    g = np.linspace(-span, span, 401)
    X, Y = np.meshgrid(g, g)
    E = irradiance(ie_typ, X, Y, d)
    im = ax.imshow(np.log10(E), extent=[-span*100, span*100, -span*100, span*100], cmap="inferno", origin="lower")
    ax.contour(X*100, Y*100, E, levels=[E.max()/2], colors="cyan", linewidths=1.5)
    ax.contour(X*100, Y*100, E, levels=[TH_WORST], colors="lime", linewidths=1.5, linestyles="--")
    ax.set_title(f"{d} м: центр {E.max():.1f} мВт/м²")
    ax.set_xlabel("см"); ax.set_ylabel("см")
    rows.append((d, E.max(), half*2, radius_where(ie_typ, d, TH_WORST)*2, radius_where(ie_min, d, TH_WORST)*2))
fig.suptitle("VSMY14940, 45 мА: голубой — половина яркости (±9°), зелёный пунктир — порог приёмника 0,25 мВт/м²")
fig.colorbar(im, ax=axs, label="log10(мВт/м²)", shrink=0.8)
fig.savefig(__file__.replace(".py", ".png"), dpi=110, bbox_inches="tight")
print("d,м | центр, мВт/м² | пятно ½ яркости, см | зона приёма (тип. LED), м | зона приёма (мин. LED), м")
for d, c, h, z, zm in rows:
    print(f"{d} | {c:.1f} | {h*100:.0f} | {z:.2f} | {zm:.2f}")
