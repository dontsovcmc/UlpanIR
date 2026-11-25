"""Схема UlpanIR (CH592F) по листам. Источник номиналов и кодов — hardware/README.md, соединения — netlist.md."""
import schemdraw
import schemdraw.elements as elm

OUT = __file__.rsplit("/", 1)[0]

# ---------- Лист 1: питание и защёлка (координаты вручную) ----------
with schemdraw.Drawing(file=f"{OUT}/sheet1_power_latch.svg", show=False) as d:
    d.config(fontsize=11)
    RY = 12
    L = lambda p1, p2: d.add(elm.Line().at(p1).to(p2))
    L((-2, RY), (12, RY))
    d.add(elm.Label().at((-1.5, RY)).label("VBAT", loc="top"))
    # батарейка
    L((-2, RY), (-2, 10.5))
    d.add(elm.BatteryCell().down().at((-2, 10.5)).to((-2, 6.5)))
    d.add(elm.Label().at((-3.6, 8.5)).label("BT1\nCR2032"))
    d.add(elm.Ground().at((-2, 6.5)))
    # Q1: исток сверху (VBAT), сток снизу (VSW), затвор справа
    L((4, RY), (4, 10.5))
    q1 = d.add(elm.PFet(bulk=True).theta(0).anchor("source").at((4, 10.5)).label("Q1\nIRLML6401", loc="left", ofst=(-0.2, 0)))
    gx, gy = 6.5, q1.gate[1]
    L(q1.gate, (gx, gy))
    d.add(elm.Dot().at((gx, gy)))
    d.add(elm.Label().at((gx - 0.4, gy + 0.4)).label("G1"))
    d.add(elm.Capacitor().down().at((gx, gy)).to((gx, 7.5)))
    d.add(elm.Label().at((gx + 1.0, 8.6)).label("Cm\n47 нФ"))
    L((gx, 7.5), (4, 7.5)); L(q1.drain, (4, 7.5))
    d.add(elm.Dot().at((4, 7.5)))
    L((4, 7.5), (0, 7.5))
    d.add(elm.Dot(open=True).at((0, 7.5)).label("VSW", loc="left"))
    d.add(elm.Label().at((0.6, 8.3)).label("на VDD33 (л. 2) и ИК (л. 3)", halign="left"))
    d.add(elm.Dot().at((1.8, 7.5)))
    d.add(elm.Capacitor().down().at((1.8, 7.5)).to((1.8, 4.5)).label("C4\n22 мкФ", loc="right"))
    d.add(elm.Ground().at((1.8, 4.5)))
    # Rg, GN, R1, C3
    d.add(elm.Resistor().right().at((gx, gy)).to((10, gy)).label("Rg 100 кОм", loc="top"))
    GN = (10, gy)
    d.add(elm.Dot().at(GN))
    d.add(elm.Label().at((10.5, gy - 0.4)).label("GN"))
    d.add(elm.Resistor().up().at(GN).to((10, RY)))
    d.add(elm.Label().at((8.8, 10.9)).label("R1\n100 кОм"))
    d.add(elm.Dot().at((10, RY)))
    L(GN, (12, gy))
    d.add(elm.Dot().at((12, gy)))
    d.add(elm.Capacitor().up().at((12, gy)).to((12, RY)).label("C3\n100 нФ", loc="right"))
    # Q2
    L(GN, (10, 7.5))
    q2 = d.add(elm.NFet().theta(0).anchor("drain").at((10, 7.5)).label("Q2\nIRLML6344", loc="left", ofst=(-0.2, 0)))
    d.add(elm.Ground().at(q2.source))
    L(q2.gate, (12.3, q2.gate[1]))
    d.add(elm.Dot().at((12.3, q2.gate[1])))
    d.add(elm.Resistor().down().at((12.3, q2.gate[1])).to((12.3, 3.5)).label("R2\n1 МОм", loc="right"))
    d.add(elm.Ground().at((12.3, 3.5)))
    L((12.3, q2.gate[1]), (13.6, q2.gate[1]))
    d.add(elm.Dot(open=True).at((13.6, q2.gate[1])).label("HOLD (PA15)", loc="right"))
    # кнопки
    L((12, gy), (22, gy))
    for i, (x, nm, pin) in enumerate(((16.5, "A", "PA4"), (22, "B", "PA5"))):
        d.add(elm.Dot().at((x, gy)))
        d.add(elm.Diode().down().at((x, gy)).to((x, 6.5)).label(f"D{1+i}\n1N4148WS", loc="right"))
        d.add(elm.Dot().at((x, 6.5)))
        d.add(elm.Label().at((x, 6.5)).label(f"BTN{nm}_N", loc="left", ofst=(-0.15, 0.3)))
        d.add(elm.Button().down().at((x, 6.5)).to((x, 3.5)).label(f"SW{1+i}", loc="left"))
        d.add(elm.Ground().at((x, 3.5)))
        L((x, 6.5), (x + 1.6, 6.5))
        d.add(elm.Capacitor().down().at((x + 1.6, 6.5)).to((x + 1.6, 3.5)).label(f"C{1+i}\n100 нФ", loc="right"))
        d.add(elm.Ground().at((x + 1.6, 3.5)))
        L((x, 6.5), (x - 1.2, 6.5))
        d.add(elm.Diode().down().at((x - 1.2, 6.5)).to((x - 1.2, 1.5)).reverse())
        d.add(elm.Label().at((x - 2.7, 4.0)).label(f"D{3+i}\nRB520S30"))
        d.add(elm.Dot(open=True).at((x - 1.2, 1.5)).label(f"{pin} (BTN_{nm}_IN)", loc="bottom", ofst=(0, -0.2)))

# ---------- Лист 2: CH592F ----------
pins_l = [("1 PA11", ""), ("2 PA10", "IR_PWM"), ("3 VDCID", "VDCI"), ("4 VSW", "VDCI"), ("5 VDD33", "VSW"),
          ("6 PA8", "ISP_RX"), ("7 PA9", "ISP_TX"), ("8 PB15", "DBG_TCK"), ("9 PB14", "DBG_TIO"), ("10 PB13", ""),
          ("11 PB12", ""), ("12 PB11", "USB_D+"), ("13 PB10", "USB_D-"), ("14 PB7", "")]
pins_r = [("28 PA12", "LED_PWM"), ("27 PA13", ""), ("26 PA14", ""), ("25 PA15", "HOLD"), ("24 PA5", "BTN_B_IN"),
          ("23 PA4", "BTN_A_IN"), ("22 VDCIA", "VDCI"), ("21 ANT", "ANT"), ("20 VINTA", "VINTA"), ("19 X32MI", "XI"),
          ("18 X32MO", "XO"), ("17 PB22", "BOOT"), ("16 PB23", "RST"), ("15 PB4", "")]
with schemdraw.Drawing(file=f"{OUT}/sheet2_mcu.svg", show=False) as d:
    d.config(fontsize=10)
    ic = d.add(elm.Ic(pins=[elm.IcPin(name=n, side="left", anchorname=f"L{i}") for i, (n, _) in enumerate(pins_l)] +
                       [elm.IcPin(name=n, side="right", anchorname=f"R{i}") for i, (n, _) in enumerate(pins_r)],
                       w=4.5, pinspacing=0.8, edgepadW=0.6, label="U1 CH592F\nQFN-28, EP = GND"))
    for i, (_, net) in enumerate(pins_l):
        a = getattr(ic, f"L{i}")
        d.add(elm.Line().left(1.0).at(a))
        d.add(elm.Label().label(net or "NC", loc="left"))
    for i, (_, net) in enumerate(pins_r):
        a = getattr(ic, f"R{i}")
        d.add(elm.Line().right(1.0).at(a))
        d.add(elm.Label().label(net or "NC", loc="right"))
    # обвязка
    x, y = ic.center[0] + 7.5, ic.center[1] + 4.5
    def cap_to_gnd(net, name, val, xx, yy):
        d.add(elm.Label().at((xx, yy)).label(net, loc="top"))
        d.add(elm.Capacitor().down().at((xx, yy)).label(f"{name}\n{val}", loc="right"))
        d.add(elm.Ground())
    cap_to_gnd("VSW", "C5", "0,1 мкФ", x - 0.5, y)
    cap_to_gnd("VDCI", "C6", "1 мкФ", x + 2.5, y)
    cap_to_gnd("VDCI(22)", "C8", "0,1 мкФ", x + 5, y)
    cap_to_gnd("VINTA", "C7", "1 мкФ", x + 7.5, y)
    xt = d.add(elm.Crystal().right().at((x, y - 5)).label("X1 32 МГц\nCL 12 пФ, без C", loc="bottom"))
    d.add(elm.Label().at(xt.start).label("XI", loc="left")); d.add(elm.Label().at(xt.end).label("XO", loc="right"))
    d.add(elm.Line().right(0.8).at((x, y - 8)))
    d.add(elm.Label().at((x, y - 8)).label("ANT", loc="left"))
    d.add(elm.Dot())
    d.add(elm.Capacitor().down().label("Ca1 НЕ СТАВИТЬ", loc="left")); d.add(elm.Ground())
    d.add(elm.Resistor().right().at((x + 0.8, y - 8)).label("Ra 0 Ом", loc="top"))
    dt = d.add(elm.Dot())
    d.add(elm.Capacitor().down().at(dt.center).label("Ca2 НЕ СТАВИТЬ", loc="right")); d.add(elm.Ground())
    d.add(elm.Line().right(1).at(dt.center))
    d.add(elm.Antenna().label("антенна\nTI AN043", loc="right"))

# ---------- Лист 3: ИК и индикатор (координаты вручную) ----------
with schemdraw.Drawing(file=f"{OUT}/sheet3_ir_led.svg", show=False) as d:
    d.config(fontsize=11)
    L = lambda p1, p2: d.add(elm.Line().at(p1).to(p2))
    T = lambda xy, t: d.add(elm.Label().at(xy).label(t, halign="left"))
    L((0, 12), (5, 12)); T((0, 12.4), "VSW")
    # LED1
    d.add(elm.Resistor().down().at((0, 12)).to((0, 9))); T((0.5, 10.5), "RL1 33 Ом")
    d.add(elm.LED().down().at((0, 9)).to((0, 6))); T((0.9, 7.5), "LED1 VSMY14940")
    # LED2 через JP1
    d.add(elm.Resistor().down().at((5, 12)).to((5, 9.5))); T((5.5, 10.7), "RL2 33 Ом")
    d.add(elm.Switch().down().at((5, 9.5)).to((5, 7.5))); T((5.7, 8.5), "JP1 (по умолчанию разомкнута)")
    d.add(elm.LED().down().at((5, 7.5)).to((5, 5))); T((5.9, 6.2), "LED2 VSMY14940")
    L((5, 5), (5, 4)); L((0, 6), (0, 4)); L((0, 4), (5, 4))
    d.add(elm.Dot().at((0, 4))); T((-1.5, 4.4), "IR_K")
    q3 = d.add(elm.NFet().theta(0).anchor("drain").at((0, 4)))
    T((-3.6, q3.gate[1]), "Q3\nIRLML6344")
    d.add(elm.Ground().at(q3.source))
    L(q3.gate, (3, q3.gate[1]))
    d.add(elm.Dot().at((3, q3.gate[1])))
    d.add(elm.Resistor().down().at((3, q3.gate[1])).to((3, q3.gate[1] - 3))); T((3.5, q3.gate[1] - 1.5), "R3 100 кОм")
    d.add(elm.Ground().at((3, q3.gate[1] - 3)))
    d.add(elm.Resistor().right().at((3, q3.gate[1])).to((6.5, q3.gate[1]))); T((4.0, q3.gate[1] + 0.6), "R4 100 Ом")
    d.add(elm.Dot(open=True).at((6.5, q3.gate[1]))); T((6.8, q3.gate[1]), "PA10 (IR_PWM)")
    # индикатор
    d.add(elm.Dot(open=True).at((13, 12))); T((13.3, 12.3), "PA12 (LED_PWM)")
    d.add(elm.Resistor().down().at((13, 12)).to((13, 9))); T((13.5, 10.5), "R5 470 Ом")
    d.add(elm.LED().down().at((13, 9)).to((13, 6))); T((13.9, 7.5), "LED3 красный\nKT-0603R")
    d.add(elm.Ground().at((13, 6)))
print("ok")
