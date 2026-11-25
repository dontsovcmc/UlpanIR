# /// script
# dependencies = ["olefile","matplotlib"]
# ///
import olefile,struct,sys,json,math
o=olefile.OleFileIO(sys.argv[1])
U=1/10000*0.0254  # altium unit -> mm
def recs(name):
    d=o.openstream(name+'/Data').read(); i=0
    while i<len(d):
        t=d[i]; n=struct.unpack_from('<I',d,i+1)[0]; yield t,d[i+5:i+5+n]; i+=5+n
out={'tracks':[],'arcs':[],'fills':[],'vias':[]}
for t,b in recs('Tracks6'):
    L=b[0]; net,poly,comp=struct.unpack_from('<HHH',b,3)
    x1,y1,x2,y2,w=struct.unpack_from('<iiiii',b,13)
    out['tracks'].append([L,net,poly,comp,x1*U,y1*U,x2*U,y2*U,w*U])
for t,b in recs('Arcs6'):
    L=b[0]; net,poly,comp=struct.unpack_from('<HHH',b,3)
    cx,cy,r=struct.unpack_from('<iii',b,13); a1,a2=struct.unpack_from('<dd',b,25); w=struct.unpack_from('<i',b,41)[0]
    out['arcs'].append([L,net,poly,comp,cx*U,cy*U,r*U,a1,a2,w*U])
for t,b in recs('Fills6'):
    L=b[0]; net,=struct.unpack_from('<H',b,3); comp,=struct.unpack_from('<H',b,7)
    x1,y1,x2,y2=struct.unpack_from('<iiii',b,13); rot=struct.unpack_from('<d',b,29)[0]
    out['fills'].append([L,net,comp,x1*U,y1*U,x2*U,y2*U,rot])
for t,b in recs('Vias6'):
    net,=struct.unpack_from('<H',b,3)
    x,y,dia,hole=struct.unpack_from('<iiii',b,13)
    out['vias'].append([net,x*U,y*U,dia*U,hole*U])
json.dump(out,open(sys.argv[2],'w'))
from collections import Counter
print({k:len(v) for k,v in out.items()})
print('track layers',Counter(t[0] for t in out['tracks']))
print('arc layers',Counter(a[0] for a in out['arcs']))
xs=[t[4] for t in out['tracks']]+[t[6] for t in out['tracks']]; ys=[t[5] for t in out['tracks']]+[t[7] for t in out['tracks']]
print('bbox',min(xs),max(xs),min(ys),max(ys))
