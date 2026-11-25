import math, json
import numpy as np

# ---------------- модель светодиода VSLB3940: Vf(I)=1.12+2.3*I (I в А) -> 1.35 В при 100 мА (даташит typ)
VF0, RD = 1.12, 2.3
RDS_Q3 = 0.3      # Ом, лог. N-FET при ~2 В (предположение)
RDS_Q1 = 0.1      # Ом, P-FET (предположение)
IMCU = 3.0e-3     # А, МК во время посылки (бюджет)
F, DUTY = 38e3, 1/3.0

def burst(voc, ri, rl, c, t_end=8e-3, dt=0.1e-6):
    """Моделирует пачку несущей 38 кГц; возвращает статистику по установившемуся участку."""
    vc = voc - 0.1
    n = int(t_end / dt)
    T = 1.0 / F
    ib_hist = []; il_hist = []; vc_hist = []
    t0 = 0.5 * t_end
    for k in range(n):
        t = k * dt
        on = (t % T) < DUTY * T
        il = max(0.0, (vc - VF0) / (RD + rl + RDS_Q3)) if on else 0.0
        ib = max(0.0, (voc - vc) / (ri + RDS_Q1))
        vc += dt * (ib - il - IMCU) / c
        if t >= t0:
            ib_hist.append(ib); il_hist.append(il); vc_hist.append(vc)
    ib = np.array(ib_hist); il = np.array(il_hist); v = np.array(vc_hist)
    return dict(ib_avg=ib.mean(), ib_pk=ib.max(), il_pk=il.max(), il_avg=il.mean(), vmin=v.min(), vmax=v.max(), vavg=v.mean())

scen = {
    "очень свежая (3.30 В, 8 Ом)": (3.30, 8),
    "свежая (3.20 В, 11 Ом)": (3.20, 11),
    "середина (3.00 В, 20 Ом)": (3.00, 20),
    "подсела (2.80 В, 40 Ом)": (2.80, 40),
    "почти села (2.60 В, 60 Ом)": (2.60, 60),
}
out = {}
for C in (10e-6, 22e-6):
    print("\n=== C эффективная = %g мкФ ===" % (C * 1e6))
    for rl in (10, 15, 18, 22, 27, 33, 39):
        row = {}
        for nm, (voc, ri) in scen.items():
            s = burst(voc, ri, rl, C)
            row[nm] = s
        out["C%g_RL%g" % (C * 1e6, rl)] = row
        f = row["очень свежая (3.30 В, 8 Ом)"]; g = row["свежая (3.20 В, 11 Ом)"]; h = row["почти села (2.60 В, 60 Ом)"]
        print("RL=%2d  oчень свежая: Iled_pk=%5.1f мА  Ibat_avg=%5.1f  Ibat_pk=%5.1f | свежая: Iled_pk=%5.1f Ibat_avg=%5.1f Ibat_pk=%5.1f | почти села: Iled_pk=%5.1f Vmin=%.2f" % (
            rl, f["il_pk"] * 1e3, f["ib_avg"] * 1e3, f["ib_pk"] * 1e3, g["il_pk"] * 1e3, g["ib_avg"] * 1e3, g["ib_pk"] * 1e3, h["il_pk"] * 1e3, h["vmin"]))
json.dump(out, open("/home/claude/out/sim_burst.json", "w"), default=float)
