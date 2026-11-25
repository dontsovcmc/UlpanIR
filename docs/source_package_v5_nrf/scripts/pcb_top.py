import json, numpy as np
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Polygon, Rectangle, Circle
g = json.load(open('/home/claude/out/geom.json'))
fig, ax = plt.subplots(figsize=(13, 8.6), dpi=150)
ax.add_patch(Polygon(np.array(g['outer']), closed=True, fc='#50555c', ec='k', lw=1))
ax.add_patch(Polygon(np.array(g['inner']), closed=True, fc='#e9e9e4', ec='#888', lw=0.8))
ax.add_patch(Polygon(np.array(g['pcb']), closed=True, fc='#1b6f40', ec='#0c3a22', lw=1.5))
ax.add_patch(Circle((8,0), 10, fc='none', ec='#ddd', lw=1.2, ls='--'))
ax.text(8, -11.6, 'CR2032 (на обратной стороне)', color='#ddd', ha='center', fontsize=8)
for (x1,y1),(x2,y2) in zip(g['ant'][:-1], g['ant'][1:]):
    ax.plot([x1,x2],[y1,y2], color='#e08a3a', lw=2.2, solid_capstyle='butt')
ax.text(-10.5, 9.0, 'антенна-меандр\n(по референсу Nordic — уточнить)', color='#ffd9a8', fontsize=7, ha='center')
for x,y in g['swd']: ax.add_patch(Circle((x,y), 0.8, fc='#d9b13a', ec='k', lw=.5))
for lab,x in zip(('VBAT','GND','SWDIO','SWDCLK'), [p[0] for p in g['swd']]):
    ax.text(x, g['swd'][0][1]-1.5, lab, fontsize=6, ha='center', color='#ffe9a0')
cols = {'U1':'#111','X1':'#c9c9cf','Q1':'#111','Q2':'#111','Q3':'#111','D1D2':'#111','IR1':'#a0185a','IR2':'#a0185a','Cbulk':'#b89664','JP1':'#d9b13a'}
for c in g['comps']:
    ax.add_patch(Rectangle((c['x']-c['sx']/2, c['y']-c['sy']/2), c['sx'], c['sy'], fc=cols.get(c['ref'],'#b89664'), ec='k', lw=.4))
    big = c['sx']>2.4 or c['sy']>2.4
    lab = {'D1D2':'D1/D2','U1':'U1 nRF52810\nQCAA QFN32','IR1':'LED1','IR2':'LED2','Cbulk':'Cvdd'}.get(c['ref'], c['ref'])
    if big: ax.text(c['x'], c['y'], lab, fontsize=7 if c['ref']=='U1' else 8, ha='center', va='center', color='white')
    else: ax.text(c['x'], c['y']+c['sy']/2+0.4, lab, fontsize=6, ha='center', va='bottom', color='#ffffcc')
for nm,(x,y) in (('SW1',(-12.1,0)),('SW2',(8.7,0))):
    ax.add_patch(Rectangle((x-3.1,y-3.1),6.2,6.2, fc='#c9c9cf', ec='k', lw=.6)); ax.add_patch(Circle((x,y),1.75, fc='#999', ec='k', lw=.4)); ax.text(x,y-4.2,nm,fontsize=8,ha='center',color='white')
ax.annotate('отверстия под ИК\n(торец, 2 шт.)', xy=(24.2,0), xytext=(26.5,-9), fontsize=8, arrowprops=dict(arrowstyle='->'))
ax.set_aspect('equal'); ax.set_xlim(-32, 34); ax.set_ylim(-17, 17); ax.axis('off')
ax.set_title('ИК-пульт v5 — вид сверху на плату (компоновка-эскиз, дорожки не проведены)', fontsize=11)
plt.savefig('/home/claude/out/pcb_top_view.png', bbox_inches='tight')
