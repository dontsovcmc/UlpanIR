"""Оптическая схема VSMY14940 сбоку, как рисуют линзы: кристалл, полусферическая линза, лучи ±9°.
Масштаб корпуса — мм, дальше — условный, расстояния подписаны. Геометрия линзы — схематично."""
import math
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Arc, Rectangle, Polygon

HW = 9.0
fig, (a1, a2) = plt.subplots(1, 2, figsize=(14, 5), gridspec_kw={"width_ratios": [1, 1.6]})

# --- левая часть: корпус и линза (мм) ---
a1.set_aspect("equal")
a1.add_patch(Rectangle((-3.2, -0.6), 3.2, 1.2, fc="#e8e8e8", ec="k", lw=1.2))          # корпус 3,2 мм
a1.add_patch(Rectangle((-3.2, -0.75), 3.2, 0.15, fc="#c9a227", ec="k", lw=0.8))         # контакты
a1.add_patch(Rectangle((-0.55, -0.12), 0.24, 0.24, fc="#333", ec="k"))                   # кристалл
R = 0.55
a1.add_patch(Arc((0, 0), 2 * R, 2 * R, theta1=-90, theta2=90, lw=1.8, color="#1f77b4"))  # линза
a1.plot([0, 0], [-R, R], color="#1f77b4", lw=1.0, ls=":")
a1.text(-1.7, 0.0, "корпус\n3,2 × 2,5 × 1,2 мм", ha="center", va="center", fontsize=9)
a1.text(-0.43, -0.32, "кристалл", ha="center", fontsize=8)
a1.text(0.62, 0.62, "полусферическая\nлинза", color="#1f77b4", fontsize=9)
a1.text(-1.6, -1.05, "плата", ha="center", fontsize=9, color="#666")
a1.plot([-3.6, 3.0], [-0.75, -0.75], color="#0a6", lw=3)
# лучи из кристалла: выходят почти параллельно, расходимость ±9°
for y0 in (-0.35, -0.18, 0, 0.18, 0.35):
    a1.plot([-0.31, 0.5], [y0 * 0.25, y0], color="#d62728", lw=1)
    ang = math.radians(HW) * (y0 / 0.35)
    a1.annotate("", xy=(2.8, y0 + (2.8 - 0.5) * math.tan(ang)), xytext=(0.5, y0),
                arrowprops=dict(arrowstyle="->", color="#d62728", lw=1))
a1.plot([0.5, 2.8], [0, 0], color="k", lw=0.6, ls="-.")
a1.text(2.85, 0.02, "ось", fontsize=8, va="bottom")
a1.set_xlim(-3.7, 3.4); a1.set_ylim(-1.3, 1.4); a1.axis("off")
a1.set_title("Светодиод сбоку (мм): кристалл → линза → узкий пучок", fontsize=11)

# --- правая часть: конус до проектора (условный масштаб) ---
xs = {0: 0.0, 1: 1.6, 2: 3.2, 5: 6.4}
t = math.tan(math.radians(HW))
def h(d): return d * t * 1.6  # 1 м = 1,6 ед. по x, тот же масштаб по y
a2.add_patch(Polygon([(0, 0), (xs[5] + 0.6, h(5) + 0.6 * t), (xs[5] + 0.6, -h(5) - 0.6 * t)],
                     closed=True, fc="#d62728", alpha=0.12, ec="none"))
a2.plot([0, xs[5] + 0.6], [0, h(5) + 0.6 * t], color="#d62728", lw=1.4)
a2.plot([0, xs[5] + 0.6], [0, -h(5) - 0.6 * t], color="#d62728", lw=1.4)
a2.plot([0, xs[5] + 0.6], [0, 0], color="k", lw=0.6, ls="-.")
a2.add_patch(Arc((0, 0), 1.6, 1.6, theta1=0, theta2=HW, color="k"))
a2.text(0.85, 0.08, "9°", fontsize=10)
for d in (1, 2, 5):
    x = xs[d]
    a2.annotate("", xy=(x, h(d)), xytext=(x, -h(d)), arrowprops=dict(arrowstyle="<->", color="k", lw=1))
    a2.text(x + 0.08, h(d) + 0.05, f"{d} м\nØ {2 * d * t * 100:.0f} см", fontsize=10, va="bottom")
a2.add_patch(Rectangle((xs[5] + 0.15, -0.35), 0.7, 0.7, fc="#ddd", ec="k"))
a2.text(xs[5] + 0.5, -0.5, "проектор", ha="center", va="top", fontsize=9)
a2.add_patch(Rectangle((-0.35, -0.15), 0.35, 0.3, fc="#222", ec="k"))
a2.text(-0.18, -0.25, "пульт", ha="center", va="top", fontsize=9)
a2.set_xlim(-0.6, 7.6); a2.set_ylim(-2.0, 2.2); a2.set_aspect("equal"); a2.axis("off")
a2.set_title("Конус половины яркости ±9° (масштаб по осям одинаковый)", fontsize=11)
fig.savefig(__file__.replace(".py", ".png"), dpi=120, bbox_inches="tight")
