"""Общее посадочное место ИК-светодиода: VSMY14940 и Amicc A-SP158IR1C-A02-2D (вид сверху на плату).
Размеры — даташиты в docs/parts/. Ось X — вдоль корпуса, Y — к линзе (наружу, к торцу платы)."""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle

PAD_W, PAD_D, GAP = 1.6, 1.5, 1.8            # общая площадка
fig, axs = plt.subplots(1, 2, figsize=(11, 4.6))
for ax, name, L, term, rec in [
    (axs[0], "VSMY14940 (Vishay)", 3.2, 0.6, (1.5, 1.5, 2.0)),
    (axs[1], "A-SP158IR1C-A02-2D (Amicc)", 3.0, 1.0, (0.9, 1.4, 1.8)),
]:
    for sx in (-1, 1):
        x0 = GAP / 2 if sx > 0 else -GAP / 2 - PAD_W
        ax.add_patch(Rectangle((x0, -PAD_D / 2), PAD_W, PAD_D, fc="#e8b04a", ec="#a06a00", lw=1.2))
        rw, rd, rg = rec
        rx0 = rg / 2 if sx > 0 else -rg / 2 - rw
        ax.add_patch(Rectangle((rx0, -rd / 2), rw, rd, fc="none", ec="#1f77b4", lw=1.2, ls="--"))
        tx0 = L / 2 - term if sx > 0 else -L / 2
        ax.add_patch(Rectangle((tx0, -0.5), term, 1.0, fc="#777", alpha=0.55, ec="k"))
    ax.add_patch(Rectangle((-L / 2, -0.5), L, 1.0, fc="none", ec="k", lw=1.8))
    ax.add_patch(Rectangle((-0.8, 0.5), 1.6, 0.9, fc="#cde", ec="k"))
    ax.text(0, 0.95, "линза", ha="center", va="center", fontsize=8)
    ax.text(-GAP / 2 - PAD_W / 2, -1.05, "+", ha="center", fontsize=13, weight="bold")
    ax.text(GAP / 2 + PAD_W / 2, -1.05, "K", ha="center", fontsize=11, weight="bold")
    ax.set_title(name)
    ax.set_xlim(-3, 3); ax.set_ylim(-1.5, 1.8); ax.set_aspect("equal"); ax.grid(alpha=0.3)
    ax.set_xlabel("мм")
fig.text(0.5, 0.01, "оранжевое — общая площадка 1,6 × 1,5 мм, зазор 1,8, длина 5,0\nпунктир — рекомендованная площадка детали;"
         "  серое — выводы; рамка — корпус (глубина корпуса без линзы — условно 1,0 мм)", ha="center", fontsize=9)
plt.tight_layout(rect=(0, 0.08, 1, 1))
plt.savefig(__file__.replace(".py", ".png"), dpi=130)
