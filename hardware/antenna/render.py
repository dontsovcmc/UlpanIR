# /// script
# dependencies = ["matplotlib"]
# ///
import json,sys,math
import matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, Arc, Circle
g=json.load(open(sys.argv[1])); box=json.loads(sys.argv[3]) if len(sys.argv)>3 else None
col={1:'red',32:'blue',33:'gold',57:'magenta',56:'purple'}
fig,ax=plt.subplots(figsize=(float(sys.argv[4]) if len(sys.argv)>4 else 30,12)); ax.set_facecolor('black')
for L in [32,1,33,57,56]:
  for t in g['tracks']:
    if t[0]!=L: continue
    lw=max(t[8],0.05)*72/25.4*(fig.get_size_inches()[0]/( (box[1]-box[0]) if box else 230))
    ax.plot([t[4],t[6]],[t[5],t[7]],color=col[L],lw=lw*1,solid_capstyle='round',alpha=0.8 if L!=32 else 0.5)
  for a in g['arcs']:
    if a[0]!=L: continue
    ax.add_patch(Arc((a[4],a[5]),2*a[6],2*a[6],theta1=a[7],theta2=a[8],color=col[L],lw=1))
for f in g['fills']:
  if f[0]==1: ax.add_patch(Rectangle((f[3],f[4]),f[5]-f[3],f[6]-f[4],color='red',alpha=0.6))
for v in g['vias']: ax.add_patch(Circle((v[1],v[2]),v[3]/2,color='silver'))
ax.set_aspect('equal')
if box: ax.set_xlim(box[0],box[1]); ax.set_ylim(box[2],box[3])
ax.grid(True,color='#444',lw=0.3)
if box and box[1]-box[0]<40:
  import numpy as np; ax.set_xticks(np.arange(math.floor(box[0]),box[1],1)); ax.set_yticks(np.arange(math.floor(box[2]),box[3],1)); ax.tick_params(labelsize=6)
plt.savefig(sys.argv[2],dpi=110,bbox_inches='tight')
