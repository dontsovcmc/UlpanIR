"""Вид сбоку: луч VSMY14940 от пульта до проектора. Модель и допущения — как в ir_spot.py."""
import math
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import FancyBboxPatch, Rectangle

IE0 = 82.0 * 45 / 70  # мВт/ср, тип., 45 мА
HW = 9.0
TH = 0.25

def E(x, y):
    r2 = x**2 + y**2
    th = np.degrees(np.arctan2(np.abs(y), x))
    return IE0 * np.exp(-math.log(2) * (th / HW) ** 2) * np.cos(np.radians(th)) / r2

xs = np.linspace(0.05, 6.0, 700)
ys = np.linspace(-2.0, 2.0, 500)
X, Y = np.meshgrid(xs, ys)
Z = E(X, Y)

fig, ax = plt.subplots(figsize=(13, 5.2))
ax.imshow(np.log10(np.clip(Z, 1e-3, None)), extent=[xs[0], xs[-1], ys[0], ys[-1]], origin="lower",
          cmap="inferno", aspect="auto", vmin=-3, vmax=2)
ax.contour(X, Y, Z, levels=[TH], colors="lime", linewidths=2, linestyles="--")
t = math.tan(math.radians(HW))
ax.plot([0, 6], [0, 6 * t], color="cyan", lw=1.5)
ax.plot([0, 6], [0, -6 * t], color="cyan", lw=1.5)

# пульт
ax.add_patch(FancyBboxPatch((-0.45, -0.17), 0.43, 0.34, boxstyle="round,pad=0.02,rounding_size=0.12",
                            fc="#222", ec="white", lw=1.5))
ax.text(-0.235, -0.32, "пульт", color="white", ha="center", va="top", fontsize=10)
# проектор в 5 м
ax.add_patch(Rectangle((5.0, -0.25), 0.55, 0.5, fc="#ddd", ec="black"))
ax.add_patch(plt.Circle((5.0, 0.0), 0.06, color="#c00"))
ax.text(5.27, 0.32, "проектор\n(5 м)", ha="center", va="bottom", fontsize=10, color="white")

for d in (1, 2, 5):
    ax.axvline(d, color="white", lw=0.6, ls=":")
    half = d * t
    ax.annotate("", xy=(d, half), xytext=(d, -half), arrowprops=dict(arrowstyle="<->", color="cyan", lw=1.2))
    ax.text(d + 0.05, -1.85, f"{d} м\n½ яркости: Ø {2*half*100:.0f} см", color="white", fontsize=9, va="bottom")

ax.set_xlim(-0.6, 6.0)
ax.set_ylim(-2.0, 2.0)
ax.set_xlabel("расстояние, м")
ax.set_ylabel("в сторону от оси, м")
ax.set_title("Луч ИК-светодиода VSMY14940 (45 мА): голубое — ±9° половина яркости, "
             "зелёный пунктир — граница приёма 0,25 мВт/м² (модель)", fontsize=11)
fig.savefig(__file__.replace(".py", ".png"), dpi=120, bbox_inches="tight")
