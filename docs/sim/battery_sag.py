"""VSW при кадре NEC: батарейка (ЭДС E, внутр. R), конденсатор C, нагрузка: ИК через 33 Ом (38 кГц, 1/3) + МК."""
def frame():
    seg=[(9000,1),(4500,0)]
    for b in range(32): seg+= [(562.5,1),(1687.5 if b%2 else 562.5,0)]
    seg+=[(562.5,1),(20000,0)]
    return seg
DUTY=1/3
def run(E,R,C,nled,imcu=3e-3,dt=0.5e-6,esr=0.01):
    v=E; vmin=E; per=1/38000
    t=0.0
    for dur,mark in frame():
        n=int(dur*1e-6/dt)
        for i in range(n):
            on = mark and ((t % per) < per*DUTY)
            # LED: I=(v-VF)/33 per LED, VF ~1.35 (тип. при ~40 мА, даташит Vishay 1.32@20, 1.48@70)
            il = nled*max(v-1.38,0)/33.0 if on else 0.0
            i_load = il+imcu
            i_bat=(E-v)/R
            v += (i_bat-i_load)*dt/C
            vmin=min(vmin,v); t+=dt
    return vmin
print("E=ЭДС без нагрузки; R=внутр. сопротивление; Vmin VSW за кадр NEC")
for E,R,tag in [(3.0,20,"свежая (R~20 Ом — гипотеза, Duracell AC 20 Ом)"),(2.8,30,"середина (TI: предел 30 мА — 30 Ом)"),(2.7,60,"к концу (TI: 60 Ом)"),(2.7,100,"холод/конец, 100 Ом")]:
    for n in (1,2):
        row=[f"{C*1e6:.0f}мкФ:{run(E,R,C,n):.2f}" for C in (22e-6,47e-6,100e-6,220e-6)]
        print(f"{tag:45s} LED×{n}  "+"  ".join(row))
