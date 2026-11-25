"""Комната сбоку: человек 1,6 м, пульт в руке, проектор на 2 м в 3 м от человека.
Модель луча — как в ir_spot.py (гаусс ±9°, Ie 82·45/70 мВт/ср тип., порог 0,25 мВт/м²).
Допущение: пульт держат на высоте 1,2 м (рука согнута перед грудью)."""
import math
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Circle, Rectangle, Polygon

IE0, HW, TH = 82.0 * 45 / 70, 9.0, 0.25
RX, RY = 0.25, 1.2          # пульт
PX, PY = 3.0, 2.0           # окошко приёмника проектора

def e_at(aim_deg, x, y):
    dx, dy = x - RX, y - RY
    r = math.hypot(dx, dy)
    off = abs(math.degrees(math.atan2(dy, dx)) - aim_deg)
    return IE0 * math.exp(-math.log(2) * (off / HW) ** 2) / r**2, off, r

aim_ok = math.degrees(math.atan2(PY - RY, PX - RX))
cases = [("целится в проектор", aim_ok, "#d62728", "-"), ("держит горизонтально", 0.0, "#1f77b4", "--")]

fig, ax = plt.subplots(figsize=(12, 6))
ax.set_aspect("equal")
ax.plot([-0.6, 4.0], [0, 0], color="k", lw=2)                     # пол
ax.text(1.7, -0.12, "пол", ha="center", va="top", fontsize=9)
# человек 1,6 м
ax.add_patch(Circle((0, 1.48), 0.12, fc="none", ec="k", lw=2))
ax.plot([0, 0], [1.36, 0.8], color="k", lw=2)
ax.plot([0, -0.15], [0.8, 0], color="k", lw=2); ax.plot([0, 0.15], [0.8, 0], color="k", lw=2)
ax.plot([0, 0.12, RX - 0.03], [1.3, 1.1, RY], color="k", lw=2)
ax.add_patch(Rectangle((RX - 0.04, RY - 0.025), 0.08, 0.05, fc="#222"))
ax.annotate("", xy=(-0.35, 1.6), xytext=(-0.35, 0), arrowprops=dict(arrowstyle="<->"))
ax.text(-0.4, 0.8, "1,6 м", rotation=90, ha="right", va="center")
ax.plot([RX, RX + 0.3], [RY, RY], color="#888", lw=0.6, ls=":")
ax.text(RX + 0.02, RY - 0.08, "пульт 1,2 м", fontsize=8, va="top")
# проектор
ax.add_patch(Rectangle((PX, PY - 0.12), 0.45, 0.24, fc="#ddd", ec="k"))
ax.add_patch(Circle((PX, PY), 0.035, color="#c00"))
ax.plot([PX + 0.22, PX + 0.22], [PY + 0.12, 2.6], color="k", lw=1.5)   # крепление к потолку
ax.annotate("", xy=(PX + 0.6, PY), xytext=(PX + 0.6, 0), arrowprops=dict(arrowstyle="<->"))
ax.plot([PX + 0.45, PX + 0.65], [PY, PY], color="k", lw=0.6)
ax.text(PX + 0.65, 1.0, "2 м", rotation=90, va="center")
ax.annotate("", xy=(PX, -0.32), xytext=(0, -0.32), arrowprops=dict(arrowstyle="<->"))
ax.text(1.5, -0.36, "3 м", ha="center", va="top")

lines = []
for name, aim, col, ls in cases:
    L = 3.3
    t = math.radians(aim)
    for s in (+1, -1):
        a = t + s * math.radians(HW)
        ax.plot([RX, RX + L * math.cos(a)], [RY, RY + L * math.sin(a)], color=col, lw=1.3, ls=ls)
    ax.add_patch(Polygon([(RX, RY), (RX + L * math.cos(t + math.radians(HW)), RY + L * math.sin(t + math.radians(HW))),
                          (RX + L * math.cos(t - math.radians(HW)), RY + L * math.sin(t - math.radians(HW)))],
                         closed=True, fc=col, alpha=0.08, ec="none"))
    e, off, r = e_at(aim, PX, PY)
    lines.append(f"{name}: угол {aim:.0f}°, до проектора {r:.2f} м, отклонение {off:.0f}°, "
                 f"на приёмнике {e:.1f} мВт/м² = {e/TH:.0f}× порога")
ax.set_xlim(-0.7, 4.0); ax.set_ylim(-0.6, 3.4); ax.axis("off")
ax.set_title("Пульт → проектор, вид сбоку (модель, ±9° — половина яркости)", fontsize=12)
ax.text(-0.6, 3.35, "\n".join(lines), fontsize=9, va="top")
fig.savefig(__file__.replace(".py", ".png"), dpi=120, bbox_inches="tight")
print("\n".join(lines))
