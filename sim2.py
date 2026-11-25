import math, json
import numpy as np

RDS_Q3, RDS_Q1, IMCU = 0.3, 0.1, 3.0e-3
F, DUTY = 38e3, 1/3.0

def burst(voc, ri, rl, c, vf0=1.12, rd=2.3, t_end=6e-3, dt=0.1e-6):
    vc = voc - 0.1; n = int(t_end / dt); T = 1.0 / F; ibh = []; ilh = []; vch = []; t0 = 0.5 * t_end
    for k in range(n):
        t = k * dt; on = (t % T) < DUTY * T
        il = max(0.0, (vc - vf0) / (rd + rl + RDS_Q3)) if on else 0.0
        ib = max(0.0, (voc - vc) / (ri + RDS_Q1))
        vc += dt * (ib - il - IMCU) / c
        if t >= t0: ibh.append(ib); ilh.append(il); vch.append(vc)
    ib = np.array(ibh); il = np.array(ilh); v = np.array(vch)
    return dict(ib_avg=ib.mean(), ib_pk=ib.max(), il_pk=il.max(), vmin=v.min())

print("=== RL=27 Ом: допуски (худший случай по максимуму тока: тёплая свежая батарея 3.30 В / 8 Ом, C_эфф=10 мкФ) ===")
for rl in (27 * 0.95, 27, 27 * 1.05):
    for vf0 in (1.02, 1.12, 1.22):
        s = burst(3.30, 8, rl, 10e-6, vf0=vf0)
        print("RL=%.1f Vf0=%.2f -> Iled_pk=%.1f мА  Ibat_avg=%.1f мА  Ibat_pk=%.1f мА" % (rl, vf0, s["il_pk"] * 1e3, s["ib_avg"] * 1e3, s["ib_pk"] * 1e3))

print("\n=== RL=27 Ом: по состояниям батареи (C_эфф=10 мкФ) + оценка дальности для VSLB3940 ===")
scen = [("очень свежая 3.30 В/8 Ом", 3.30, 8), ("свежая 3.20 В/11 Ом", 3.20, 11), ("середина 3.00 В/20 Ом", 3.00, 20), ("подсела 2.80 В/40 Ом", 2.80, 40), ("почти села 2.60 В/60 Ом", 2.60, 60)]
rows = []
for nm, voc, ri in scen:
    s = burst(voc, ri, 27, 10e-6)
    ie = 65.0 * s["il_pk"] * 1e3 / 100.0      # мВт/ср, линейно по току (допущение)
    d025 = math.sqrt(ie / 0.25); d012 = math.sqrt(ie / 0.12)
    rows.append((nm, s, ie, d025, d012))
    print("%-26s Iled_pk=%5.1f мА  Ibat_avg=%5.1f  Ibat_pk=%5.1f  Vmin=%.2f В  Ie=%5.1f мВт/ср  d=%.1f..%.1f м (по оси)" % (nm, s["il_pk"] * 1e3, s["ib_avg"] * 1e3, s["ib_pk"] * 1e3, s["vmin"], ie, d025, d012))

# ---------------- заряд конденсатора при включении: Q1 со slew-control (Rg, Cm) против «голого» Q1
def inrush(voc, ri, ctot, rg, cm, g1=0.3, imcu_after=1e-3, kp=2.8, vt=0.7, cgs=300e-12, cgd=80e-12, t_end=30e-3, dt=0.05e-6):
    """P-FET Q1: исток=Vbat_term, сток=VDD (ёмкость ctot). Затвор через Rg к G1 (быстро тянется кнопкой к g1).
    Cm — от затвора к стоку (Миллер)."""
    vout = 0.0; vg = voc; ibat_prev = 0.0
    n = int(t_end / dt); ibm = 0.0; t_ramp = None; log = []
    for k in range(n):
        vs = voc - ri * ibat_prev
        vsg = vs - vg; vsd = vs - vout
        if vsg <= vt or vsd <= 0:
            idr = 0.0
        else:
            vov = vsg - vt
            idr = kp * (2 * vov * vsd - vsd * vsd) if vsd < vov else kp * vov * vov
        # ограничение сопротивлением цепи: ток не больше, чем даёт батарея при коротком замыкании на Rds
        idr = min(idr, max(0.0, (voc - vout) / (ri + 0.05)))
        iload = idr
        ibat_prev = iload + (imcu_after if vout > 1.7 else 0.0)
        ibm = max(ibm, ibat_prev)
        vout += dt * (iload - (imcu_after if vout > 1.7 else 0.0)) / ctot
        # затвор: ток через Rg от G1 → суммарно перезаряжает Cgs (к истоку) и (Cgd+Cm) (к стоку)
        ig = (g1 - vg) / rg    # ток в узел затвора (отрицательный — вытекает)
        # d(vg)/dt*(cgs+cgd+cm) = ig + cgs*d(vs)/dt + (cgd+cm)*d(vout)/dt
        dvout = (iload - (imcu_after if vout > 1.7 else 0.0)) / ctot
        dvg = (ig + (cgd + cm) * dvout) / (cgs + cgd + cm)
        vg += dt * dvg
        if t_ramp is None and vout >= 0.9 * voc: t_ramp = k * dt
        if k % 2000 == 0: log.append((k * dt, vout, ibat_prev))
    return dict(ib_max=ibm, t90=t_ramp, vout_end=vout)

print("\n=== Бросок тока при включении (C_всего = 22 мкФ ном. + 5 мкФ развязка МК = 27 мкФ) ===")
for voc, ri in ((3.30, 8), (3.20, 11), (3.00, 20)):
    r0 = inrush(voc, ri, 27e-6, rg=1.0, cm=0.0)          # «голый» Q1 (Rg=1 Ом, без Cm)
    for rg, cm in ((100e3, 22e-9), (100e3, 33e-9), (220e3, 47e-9)):
        r1 = inrush(voc, ri, 27e-6, rg=rg, cm=cm)
        print("Voc=%.2f Ri=%2d | без slew: Ibat_max=%6.1f мА | Rg=%3.0fк Cm=%2.0f нФ: Ibat_max=%5.1f мА, t(0→90%%)=%s мс" % (
            voc, ri, r0["ib_max"] * 1e3, rg / 1e3, cm * 1e9, r1["ib_max"] * 1e3, "%.1f" % (r1["t90"] * 1e3) if r1["t90"] else ">30"))

# ---------------- заряд на одну посылку NEC и ресурс
mark = 9e-3 + 32 * 0.5625e-3 + 0.5625e-3     # 27.56 мс несущей
frame = 13.5e-3 + 32 * 0.5625e-3 + 16 * 0.5625e-3 + 16 * 1.6875e-3 + 0.5625e-3   # 68.06 мс
print("\nNEC: несущая %.2f мс, кадр %.2f мс (16 нулей и 16 единиц всегда)" % (mark * 1e3, frame * 1e3))
for nm, voc, ri in scen[:3]:
    s = burst(voc, ri, 27, 10e-6)
    q_ir = s["ib_avg"] * mark                       # ИК-часть вкл. 3 мА МК
    q_mcu_rest = 3e-3 * (frame - mark)
    q_boot = 3e-3 * 15e-3                           # загрузка ~15 мс (допущение)
    q_cap = 27e-6 * voc                              # заряд конденсатора при включении
    q = q_ir + q_mcu_rest + q_boot + q_cap
    print("%-26s Q_кадра = %.2f мКл (ИК %.2f + МК %.2f + старт %.2f + конд. %.2f) = %.2f мкА·ч" % (nm, q * 1e3, q_ir * 1e3, q_mcu_rest * 1e3, q_boot * 1e3, q_cap * 1e3, q / 3.6e-3))
period = 1.0 / 3.0
s = burst(3.0, 20, 27, 10e-6)
q = s["ib_avg"] * mark + 3e-3 * (frame - mark)
print("При удержании 3 кадра/с: средний ток ≈ %.1f мА (кадр %.0f мс из %.0f мс)" % (q / period * 1e3, frame * 1e3, period * 1e3))
print("Ресурс по ИК (оценка): 100 мА·ч эфф. / %.3f мкА·ч на кадр = %.0f тыс. кадров" % (q / 3.6e-3, 100e3 / (q / 3.6e-3) / 1e3))
