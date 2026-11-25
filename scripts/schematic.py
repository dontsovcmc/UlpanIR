import os
W, H = 1500, 960
o = []
INK = "#1d1d1f"; BLUE = "#1a5fb4"; RED = "#c01c28"; GRY = "#777"

def a(s): o.append(s)
def line(pts, c=INK, w=2, dash=None):
    d = ' stroke-dasharray="%s"' % dash if dash else ""
    a('<polyline points="%s" fill="none" stroke="%s" stroke-width="%s"%s stroke-linejoin="round"/>' % (" ".join("%g,%g" % p for p in pts), c, w, d))
def dot(x, y): a('<circle cx="%g" cy="%g" r="3.6" fill="%s"/>' % (x, y, INK))
def text(x, y, s, size=13, anchor="start", c=INK, bold=False, ital=False):
    a('<text x="%g" y="%g" font-size="%d" text-anchor="%s" fill="%s"%s%s>%s</text>' % (x, y, size, anchor, c, ' font-weight="700"' if bold else "", ' font-style="italic"' if ital else "", s))
def gnd(x, y):
    line([(x, y), (x, y + 8)]); line([(x - 12, y + 8), (x + 12, y + 8)]); line([(x - 7, y + 13), (x + 7, y + 13)]); line([(x - 3, y + 18), (x + 3, y + 18)])
def res_v(x, yc, name, val):
    a('<rect x="%g" y="%g" width="14" height="40" fill="#fff" stroke="%s" stroke-width="2"/>' % (x - 7, yc - 20, INK))
    text(x + 14, yc - 2, name, 13, bold=True); text(x + 14, yc + 13, val, 12, c=BLUE)
def res_h(xc, y, name, val, above=True):
    a('<rect x="%g" y="%g" width="40" height="14" fill="#fff" stroke="%s" stroke-width="2"/>' % (xc - 20, y - 7, INK))
    text(xc, y - 12, name + "  " + val if above else name, 12, anchor="middle")
def cap_v(x, y, name, val, w=26, lx=16):
    line([(x - w / 2, y), (x + w / 2, y)], w=2.5); line([(x - w / 2, y + 8), (x + w / 2, y + 8)], w=2.5)
    text(x + lx, y + 2, name, 13, bold=True); text(x + lx, y + 17, val, 12, c=BLUE)
def diode_h(x, y, name):
    a('<polygon points="%g,%g %g,%g %g,%g" fill="#fff" stroke="%s" stroke-width="2"/>' % (x + 6, y - 9, x + 6, y + 9, x + 28, y, INK))
    line([(x + 28, y - 9), (x + 28, y + 9)])
    text(x + 17, y - 14, name, 13, anchor="middle", bold=True)
def sw_v(x, y, name):
    line([(x, y), (x, y + 8)]); a('<circle cx="%g" cy="%g" r="3" fill="#fff" stroke="%s" stroke-width="2"/>' % (x, y + 11, INK))
    line([(x, y + 11 + 3), (x + 12, y + 24)]); a('<circle cx="%g" cy="%g" r="3" fill="#fff" stroke="%s" stroke-width="2"/>' % (x, y + 35, INK))
    line([(x, y + 38), (x, y + 46)])
    text(x + 18, y + 24, name, 13, bold=True)
def fet(cx, cy, kind):
    # kind 'n': drain top, source bottom. 'p': source top, drain bottom
    line([(cx - 30, cy), (cx - 14, cy)]); line([(cx - 14, cy - 15), (cx - 14, cy + 15)])
    for y0, y1 in ((cy - 15, cy - 8), (cy - 4, cy + 4), (cy + 8, cy + 15)):
        line([(cx - 8, y0), (cx - 8, y1)])
    line([(cx - 8, cy - 11), (cx, cy - 11), (cx, cy - 30)]); line([(cx - 8, cy + 11), (cx, cy + 11), (cx, cy + 30)])
    line([(cx - 8, cy), (cx, cy)])
    if kind == "n":
        line([(cx, cy), (cx, cy + 11)]); a('<polygon points="%g,%g %g,%g %g,%g" fill="%s"/>' % (cx - 8, cy, cx - 2, cy - 4, cx - 2, cy + 4, INK))
    else:
        line([(cx, cy), (cx, cy - 11)]); a('<polygon points="%g,%g %g,%g %g,%g" fill="%s"/>' % (cx - 2, cy, cx - 8, cy - 4, cx - 8, cy + 4, INK))
def led_v(x, y, name):
    a('<polygon points="%g,%g %g,%g %g,%g" fill="#fff" stroke="%s" stroke-width="2"/>' % (x - 10, y, x + 10, y, x, y + 20, INK))
    line([(x - 10, y + 20), (x + 10, y + 20)])
    for k in (0, 10):
        line([(x + 14, y + 22 - k), (x + 28, y + 8 - k)], c=RED, w=1.8)
        a('<polygon points="%g,%g %g,%g %g,%g" fill="%s"/>' % (x + 28, y + 8 - k, x + 21, y + 9 - k, x + 27, y + 15 - k, RED))
    text(x + 36, y + 30, name, 13, bold=True)
def flag(x, y, name, right=True):
    text(x, y + 4, name + (" ▸" if right else ""), 13, anchor="start", c=GRY, bold=True)

a('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%d" height="%d" font-family="DejaVu Sans, Arial, sans-serif">' % (W, H, W, H))
a('<rect width="100%" height="100%" fill="#fff"/>')
text(30, 36, "ИК-пульт на nRF52810 — принципиальная электрическая схема (v5: 2× VSMY14940, не проверена на макете)", 20, bold=True)
text(30, 58, "Серым — имена цепей (одинаковые имена = соединены). Синим — номиналы-предложения; красным — ИК-излучение.", 12, c=GRY)

# ---------------- A: soft latch
a('<rect x="30" y="80" width="640" height="410" rx="10" fill="none" stroke="#aaa" stroke-dasharray="6 5"/>')
text(44, 102, "A. Питание: батарея + «защёлка» (мягкий выключатель)", 15, bold=True)
# VBAT rail
line([(80, 125), (340, 125)]); text(86, 119, "VBAT", 12, c=GRY, bold=True)
line([(80, 125), (80, 160)])
line([(65, 160), (95, 160)], w=3); line([(72, 168), (88, 168)], w=3)
text(100, 164, "BT1", 13, bold=True); text(100, 179, "CR2032, 3 В", 12, c=BLUE); text(70, 154, "+", 14, anchor="end")
line([(80, 168), (80, 215)]); gnd(80, 215)
# Cg
line([(210, 125), (210, 150)]); cap_v(210, 150, "", "", lx=-6); text(196, 154, "Cg", 13, anchor="end", bold=True); text(196, 169, "100 нФ", 12, anchor="end", c=BLUE)
line([(210, 158), (210, 185), (260, 185)])
text(172, 170, "", 12)
# R1
line([(260, 125), (260, 135)]); res_v(260, 155, "R1", "470 кОм"); line([(260, 175), (260, 185)])
dot(210, 125); dot(260, 125)
# Q1
line([(340, 125), (340, 155)]); fet(340, 185, "p"); line([(260, 185), (266, 185)]); res_h(286, 185, "Rg", "100 кОм", above=False); line([(306, 185), (310, 185)]); dot(260, 185); dot(310, 185); line([(310, 185), (310, 200)]); cap_v(310, 200, "", "", lx=-6); line([(310, 208), (310, 224), (340, 224)]); dot(340, 224); text(298, 203, "Cm", 13, anchor="end", bold=True); text(298, 218, "33 нФ", 12, anchor="end", c=BLUE)
text(356, 172, "Q1  P-FET", 13, bold=True); text(356, 188, "логический уровень", 11, c=BLUE); text(356, 202, "(исток=VBAT, сток=VDD)", 11, c=BLUE)
line([(340, 215), (340, 232), (600, 232)]); flag(606, 232, "VDD")
# G1 line down
line([(260, 185), (260, 375)]); text(236, 200, "G1", 13, anchor="end", c=GRY, bold=True)
# D1, SW1
line([(260, 262), (300, 262)]); dot(260, 262); diode_h(300, 262, "D1"); line([(328, 262), (420, 262)]); dot(420, 262)
sw_v(420, 262, "SW1"); gnd(420, 308)
line([(420, 262), (470, 262)]); res_h(490, 262, "Rs1", "10 МОм"); line([(510, 262), (560, 262)]); flag(566, 262, "BTN1")
# D2, SW2
line([(260, 340), (300, 340)]); dot(260, 340); diode_h(300, 340, "D2"); line([(328, 340), (480, 340)]); dot(480, 340)
sw_v(480, 340, "SW2"); gnd(480, 386)
line([(480, 340), (520, 340)]); res_h(540, 340, "Rs2", "10 МОм"); line([(560, 340), (590, 340)]); flag(596, 340, "BTN2")
text(300, 366, "D1/D2: сдвоенный", 11, c=BLUE); text(300, 380, "Шоттки, общий анод", 11, c=BLUE)
# Q2
line([(260, 365), (260, 395)]); fet(260, 425, "n");
text(278, 418, "Q2  N-FET", 13, bold=True)
line([(260, 455), (260, 468)]); gnd(260, 468)
line([(230, 425), (205, 425)]); dot(205, 425)
line([(130, 425), (150, 425)]); res_h(170, 425, "R2", "100 кОм", above=True); line([(190, 425), (205, 425)])
flag(60, 429, "HOLD", right=False); text(60, 429, "", 1); line([(98, 425), (130, 425)])
line([(205, 425), (205, 427)]); res_v(205, 447, "R3", "1 МОм"); line([(205, 467), (205, 470)]); gnd(205, 470)
text(40, 240, "Как работает:", 12, bold=True)
text(40, 256, "1) нажали SW → G1 тянется вниз", 11); text(40, 270, "    через D → Q1 открывается → VDD", 11)
text(40, 286, "2) МК стартует и ставит HOLD=1 →", 11); text(40, 300, "    Q2 держит G1 внизу (защёлка)", 11)
text(40, 316, "3) МК читает BTN1/BTN2 (какая", 11); text(40, 330, "    кнопка), шлёт ИК-код", 11)
text(40, 346, "4) HOLD=0 → R1 поднимает G1 →", 11); text(40, 360, "    Q1 закрыт, ток ≈ 0", 11)
text(40, 380, "Cg на G1 держит Q1 открытым", 11, c=BLUE); text(40, 394, "десятки мс, пока МК грузится", 11, c=BLUE)
text(290, 452, "Выключать (HOLD=0) только", 11, c=RED); text(290, 466, "после отпускания кнопки", 11, c=RED)

# ---------------- B: MCU
a('<rect x="690" y="80" width="520" height="490" rx="10" fill="none" stroke="#aaa" stroke-dasharray="6 5"/>')
text(704, 102, "B. Микроконтроллер, кварц, антенна, SWD", 15, bold=True)
a('<rect x="780" y="170" width="190" height="320" fill="#fff" stroke="%s" stroke-width="2.5"/>' % INK)
text(875, 198, "U1  nRF52810", 15, anchor="middle", bold=True); text(875, 216, "QFN32 5×5", 12, anchor="middle")
for y, n, net in ((240, "VDD", "VDD"), (272, "P0.x  HOLD", "HOLD"), (302, "P0.x  BTN1", "BTN1"), (332, "P0.x  BTN2", "BTN2"), (362, "P0.x  PWM_IR", "PWM_IR")):
    line([(750, y), (780, y)]); text(786, y + 4, n, 12); text(744, y + 4, net, 12, anchor="end", c=GRY, bold=True)
line([(750, 402), (780, 402)]); text(786, 406, "VSS (GND, EP)", 12); gnd(750, 402)
text(875, 428, "Блокировочные C, DEC-выводы,", 11, anchor="middle", c=BLUE); text(875, 442, "DC/DC (по желанию):", 11, anchor="middle", c=BLUE); text(875, 456, "копировать референс Nordic,", 11, anchor="middle", c=BLUE); text(875, 470, "номиналы из его BOM", 11, anchor="middle", c=BLUE)
for y, n in ((200, "XC1"), (240, "XC2"), (330, "ANT"), (370, "SWDIO"), (400, "SWDCLK")):
    line([(970, y), (1000, y)]); text(964, y + 4, n, 12, anchor="end")
def gnd_up(x, y):
    line([(x, y), (x, y - 8)]); line([(x - 12, y - 8), (x + 12, y - 8)]); line([(x - 7, y - 13), (x + 7, y - 13)]); line([(x - 3, y - 18), (x + 3, y - 18)])
# crystal
line([(1000, 200), (1090, 200), (1090, 210)]); line([(1000, 240), (1090, 240), (1090, 226)])
line([(1078, 210), (1102, 210)], w=3); a('<rect x="1082" y="214" width="16" height="8" fill="#fff" stroke="%s" stroke-width="2"/>' % INK); line([(1078, 226), (1102, 226)], w=3)
text(1110, 222, "X1 32 МГц", 13, bold=True)
dot(1030, 200); line([(1030, 200), (1030, 178)]); cap_v(1030, 170, "Ca", "12 пФ", lx=14); line([(1030, 170), (1030, 160)]); gnd_up(1030, 160)
dot(1030, 240); line([(1030, 240), (1030, 262)]); cap_v(1030, 262, "Cb", "12 пФ", lx=14); line([(1030, 270), (1030, 282)]); gnd(1030, 282)
# antenna
line([(970, 330), (1000, 330)]); a('<rect x="1000" y="315" width="86" height="30" fill="#fff" stroke="%s" stroke-width="1.8" stroke-dasharray="5 4"/>' % INK)
text(1043, 334, "согласование", 11, anchor="middle", c=BLUE); text(1043, 360, "(по референсу)", 11, anchor="middle", c=BLUE)
line([(1086, 330), (1110, 330)])
pts = [(1110, 330), (1110, 312), (1122, 312), (1122, 348), (1134, 348), (1134, 312), (1146, 312), (1146, 348), (1158, 348), (1158, 312), (1170, 312)]
line(pts, c="#b5651d", w=2.5); text(1100, 296, "ANT1: меандр", 12, bold=True); text(1100, 310, "на плате", 12, bold=True)
# SWD pads
for y, n in ((370, "SWDIO"), (400, "SWDCLK")):
    line([(970, y), (1040, y)]); a('<circle cx="1050" cy="%g" r="8" fill="#e8c34a" stroke="%s" stroke-width="1.5"/>' % (y, INK)); text(1066, y + 4, "J1: " + n, 12, bold=True)
a('<circle cx="1050" cy="430" r="8" fill="#e8c34a" stroke="%s" stroke-width="1.5"/>' % INK); text(1066, 434, "J1: VBAT (бат. +)", 12, bold=True)
line([(1042, 430), (1020, 430)]); text(1016, 434, "VBAT", 12, anchor="end", c=GRY, bold=True)
a('<circle cx="1050" cy="460" r="8" fill="#e8c34a" stroke="%s" stroke-width="1.5"/>' % INK); line([(1042, 460), (1020, 460)]); gnd(1020, 460); text(1066, 464, "J1: GND", 12, bold=True)
text(704, 520, "Прошивка: батарею снять, на J1 подать 3 В (VBAT+GND) и замкнуть SW1,", 11, c=RED)
text(704, 535, "чтобы Q1 открылся; SWD с ESP32 на SWDIO/SWDCLK. Напрямую на VDD", 11, c=RED)
text(704, 550, "(после Q1) питание не подавать — через диод Q1 зарядит батарею.", 11, c=RED)

# ---------------- C: IR driver
a('<rect x="30" y="510" width="640" height="420" rx="10" fill="none" stroke="#aaa" stroke-dasharray="6 5"/>')
text(44, 532, "C. ИК-каскад: 2× SMD side-looker VSMY14940, LED2 за JP1, NEC", 15, bold=True)
flag(60, 600, "VDD", right=False); line([(100, 600), (160, 600)])
line([(160, 600), (470, 600)])
dot(470, 600); line([(470, 600), (470, 640)]); cap_v(470, 640, "Cvdd", "22 мкФ X5R", lx=14); line([(470, 648), (470, 668)]); gnd(470, 668)
line([(470, 600), (580, 600)]); dot(580, 600)
text(430, 586, "VIR", 12, c=GRY, bold=True)
x = 580
line([(x, 600), (x, 620)]); res_v(x, 640, "RL", "30 Ом"); line([(x, 660), (x, 690)]); led_v(x, 690, ""); text(566, 706, "LED1", 12, anchor="end", bold=True); line([(x, 710), (x, 790)]); dot(x, 790)
text(566, 722, "SMD side-looker", 11, anchor="end", c=BLUE); text(566, 736, "VSMY14940", 11, anchor="end", c=BLUE)
line([(x, 600), (640, 600), (640, 606)]); dot(640, 606); dot(640, 622); line([(631, 610), (649, 618)], c=RED, w=2); line([(640, 622), (640, 636)])
text(652, 616, "JP1", 12, bold=True)
res_v(640, 656, "RL2", "30 Ом"); line([(640, 676), (640, 700)]); led_v(640, 700, ""); line([(640, 720), (640, 790), (580, 790)]); text(662, 725, "LED2", 12, bold=True); text(566, 750, "LED2 впаян, JP1 разомкнут", 11, anchor="end", c=RED)
fet(580, 820, "n"); text(598, 806, "Q3  N-FET", 13, bold=True); text(598, 821, "лог. ур., 1,8 В", 11, c=BLUE)
line([(580, 850), (580, 872)]); gnd(580, 872)
line([(550, 820), (520, 820)]); dot(520, 820); line([(520, 820), (480, 820)]); text(474, 824, "PWM_IR", 12, anchor="end", c=GRY, bold=True)
line([(520, 820), (520, 835)]); res_v(520, 855, "R4", "100к"); line([(520, 875), (520, 885)]); gnd(520, 885)
text(44, 560, "Cvdd (на VDD после Q1) сглаживает пульсации 38 кГц;", 11, c=BLUE)
text(44, 575, "Rg+Cm у Q1 ограничивают бросок заряда Cvdd (≈20 мА).", 11, c=BLUE)
text(44, 590, "LED2: цепь разорвана JP1; замкнул → оба, ток ≈×1,7–2.", 11, c=RED)

# ---------------- notes
a('<rect x="1230" y="80" width="250" height="850" rx="10" fill="#f6f6f2" stroke="#aaa"/>')
text(1244, 104, "Оценки по току (не замер)", 14, bold=True)
notes = [
 "JP1 разомкнут: 1 LED. Замкнут:",
 "оба LED, RL=30 Ом у каждого.",
 "Свежая 3,3 В: LED 56 мА каждый,",
 "из батареи среднее ≈40 мА,",
 "пик ≈44 мА (>30 мА — риск",
 "принят). 3,0 В/20 Ом: ≈29 мА.",
 "2,6 В/60 Ом: ≈14 мА, но Vmin",
 "на МК ≈1,72 В (порог 1,7 В!).",
 "",
 "Расчёт (модель, не замер):",
 "Один LED (JP1 разомкнут),",
 "RL=30, Cvdd≈10 мкФ эфф.:",
 "3,3 В/8 Ом: LED ≈60 мА,",
 "из батареи ≈23 мА (пик 25).",
 "3,0 В/20 Ом: 45 мА, 18 мА.",
 "2,6 В/60 Ом: 24 мА, Vmin ≈1,9 В.",
 "",
 "NEC: несущая 27,6 мс из",
 "кадра 68 мс. 3 кадра/с →",
 "средний ток при удержании",
 "≈2 мА; ≈0,2–0,26 мкА·ч",
 "на кадр.",
 "",
 "Старт: Rg 100к + Cm 33 нФ →",
 "бросок ≤30 мА (23–29 мА),",
 "нарастание 7–10 мс. Без них",
 "150–410 мА. Rg=220к даёт",
 "≈17–21 мА, но ~10–15 мс.",
 "",
 "Протокол: NEC, адрес Optoma",
 "0x32 (32 CD); On = 02 FD,",
 "Off = 2E D1 — для других",
 "моделей; X341 не проверена.",
 "",
 "Открытое:",
 "– Vgs(th) Q1/Q2/Q3 при 2 В;",
 "– реальная ёмкость Cvdd;",
 "– задержка старта МК vs Cg.",
]
for i, s in enumerate(notes):
    text(1244, 130 + i * 20, s, 12, bold=s.endswith(":"))
a('</svg>')
open("/home/claude/out/schematic.svg", "w").write("\n".join(o))
