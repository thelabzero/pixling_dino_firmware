// Dino face sheet v1: drawing code extracted from "Deskimon Dino Sheet.html" (Claude Design). Ground truth: do not edit by hand.
const W=128,H=64;
const mk=()=>new Uint8Array(W*H);
const px=(g,x,y,v=1)=>{x=Math.round(x);y=Math.round(y);if(x>=0&&x<W&&y>=0&&y<H)g[y*W+x]=v;};
const line=(g,x0,y0,x1,y1,v=1)=>{x0=Math.round(x0);y0=Math.round(y0);x1=Math.round(x1);y1=Math.round(y1);const dx=Math.abs(x1-x0),dy=-Math.abs(y1-y0),sx=x0<x1?1:-1,sy=y0<y1?1:-1;let e=dx+dy;for(;;){px(g,x0,y0,v);if(x0===x1&&y0===y1)break;const e2=2*e;if(e2>=dy){e+=dy;x0+=sx;}if(e2<=dx){e+=dx;y0+=sy;}}};
const clampB=(a,b,c,d)=>[Math.max(0,Math.floor(a)),Math.max(0,Math.floor(b)),Math.min(W-1,Math.ceil(c)),Math.min(H-1,Math.ceil(d))];
function area(g,f,b,v=1){const[x0,y0,x1,y1]=clampB(...b);for(let y=y0;y<=y1;y++)for(let x=x0;x<=x1;x++)if(f(x+.5,y+.5))g[y*W+x]=v;}
function ring(g,f,b){const[x0,y0,x1,y1]=clampB(...b);for(let y=y0;y<=y1;y++)for(let x=x0;x<=x1;x++){const X=x+.5,Y=y+.5;if(!f(X,Y))continue;g[y*W+x]=(!f(X+1,Y)||!f(X-1,Y)||!f(X,Y+1)||!f(X,Y-1))?1:0;}}
function edge(g,f,b){const[x0,y0,x1,y1]=clampB(...b);for(let y=y0;y<=y1;y++)for(let x=x0;x<=x1;x++){const X=x+.5,Y=y+.5;if(f(X,Y)&&(!f(X+1,Y)||!f(X-1,Y)||!f(X,Y+1)||!f(X,Y-1)))g[y*W+x]=1;}}
const ell=(cx,cy,rx,ry)=>(x,y)=>((x-cx)/rx)**2+((y-cy)/ry)**2<=1;
const rell=(cx,cy,rx,ry,a)=>{const c=Math.cos(a),s=Math.sin(a);return(x,y)=>{const dx=x-cx,dy=y-cy,u=dx*c+dy*s,v=-dx*s+dy*c;return(u/rx)**2+(v/ry)**2<=1;};};
const rr=(cx,cy,hw,hh,r)=>(x,y)=>{const ax=Math.abs(x-cx),ay=Math.abs(y-cy);if(ax>hw||ay>hh)return false;const qx=Math.max(ax-(hw-r),0),qy=Math.max(ay-(hh-r),0);return qx*qx+qy*qy<=r*r;};
const bE=(cx,cy,rx,ry,p=1)=>[cx-rx-p,cy-ry-p,cx+rx+p,cy+ry+p];
const dot=(g,x,y,r,v=1)=>{const R=Math.ceil(r);for(let dy=-R;dy<=R;dy++)for(let dx=-R;dx<=R;dx++)if(dx*dx+dy*dy<=r*r+.3)px(g,x+dx,y+dy,v);};
function nib(g,x,y,th){x=Math.round(x);y=Math.round(y);if(th<=1)px(g,x,y);else if(th===2){px(g,x,y);px(g,x+1,y);px(g,x,y+1);px(g,x+1,y+1);}else{px(g,x,y);px(g,x-1,y);px(g,x+1,y);px(g,x,y-1);px(g,x,y+1);}}
function stroke(g,pts,th){if(pts.length===1)nib(g,pts[0][0],pts[0][1],th);for(let i=0;i<pts.length-1;i++){const[a,b]=pts[i],[c,d]=pts[i+1],n=Math.max(1,Math.ceil(Math.hypot(c-a,d-b)*2));for(let k=0;k<=n;k++)nib(g,a+(c-a)*k/n,b+(d-b)*k/n,th);}}
const curve=(mx,my,hw,fn,n=24)=>{const o=[];for(let i=0;i<=n;i++){const u=-1+2*i/n;o.push([mx+u*hw,my+fn(u)]);}return o;};
const bmp=(g,rows,x0,y0,v=1)=>rows.forEach((r,y)=>{for(let x=0;x<r.length;x++)if(r[x]==='#')px(g,Math.round(x0)+x,Math.round(y0)+y,v);});
const plus=(g,x,y)=>{line(g,x-2,y,x+2,y);line(g,x,y-2,x,y+2);};
function zee(g,x,y,big){x=Math.round(x);y=Math.round(y);if(big){line(g,x,y,x+4,y);line(g,x+4,y,x,y+4);line(g,x,y+4,x+4,y+4);}else{line(g,x,y,x+3,y);line(g,x+3,y,x,y+3);line(g,x,y+3,x+3,y+3);}}
const pad2=n=>String(n).padStart(2,'0');
const HEART7=['.##.##.','#######','#######','.#####.','..###..','...#...'];
const DROP=['.#.','.#.','###','###','.#.'];
const NOTE=['..##','..#.','..#.','.##.','###.','.#..'];
const CROWN=['#..#..#','##.#.##','#######','#######'];

/* ---------- fonts ---------- */
const rot180=a=>a.slice().reverse().map(r=>r.split('').reverse().join(''));
const mirRows=a=>a.map(r=>r.split('').reverse().join(''));
const BIG={
 '0':['..#####..','.#######.','##.....##','##.....##','##.....##','##.....##','##.....##','##.....##','##.....##','##.....##','##.....##','.#######.','..#####..'],
 '1':['...##....','..###....','.####....','...##....','...##....','...##....','...##....','...##....','...##....','...##....','...##....','.######..','.######..'],
 '2':['..#####..','.#######.','##.....##','.......##','.......##','......##.','.....##..','....##...','...##....','..##.....','.##......','#########','#########'],
 '3':['..#####..','.#######.','##.....##','.......##','.......##','....####.','....####.','.......##','.......##','.......##','##.....##','.#######.','..#####..'],
 '4':['.....###.','....####.','...##.##.','..##..##.','.##...##.','##....##.','##....##.','#########','#########','......##.','......##.','......##.','......##.'],
 '5':['#########','#########','##.......','##.......','#######..','########.','.......##','.......##','.......##','.......##','##.....##','.#######.','..#####..'],
 '6':['...#####.','..######.','.##......','##.......','##.......','##.#####.','#########','##.....##','##.....##','##.....##','##.....##','.#######.','..#####..'],
 '7':['#########','#########','.......##','......##.','......##.','.....##..','.....##..','....##...','....##...','...##....','...##....','...##....','...##....'],
 '8':['..#####..','.#######.','##.....##','##.....##','##.....##','.#######.','.#######.','##.....##','##.....##','##.....##','##.....##','.#######.','..#####..'],
 ':':['..','..','..','##','##','..','..','..','##','##','..','..','..']
};
BIG['9']=rot180(BIG['6']);
const SMALL={
 '0':['.##.','#..#','#..#','#..#','#..#','#..#','.##.'],'1':['.#..','##..','.#..','.#..','.#..','.#..','###.'],
 '2':['.##.','#..#','...#','..#.','.#..','#...','####'],'3':['###.','...#','...#','.##.','...#','...#','###.'],
 '4':['..#.','.##.','#.#.','#.#.','####','..#.','..#.'],'5':['####','#...','###.','...#','...#','#..#','.##.'],
 '6':['.##.','#...','#...','###.','#..#','#..#','.##.'],'7':['####','...#','..#.','..#.','.#..','.#..','.#..'],
 '8':['.##.','#..#','#..#','.##.','#..#','#..#','.##.'],'9':['.##.','#..#','#..#','.###','...#','...#','.##.'],
 ':':['..','##','##','..','##','##','..'],'+':['.....','..#..','..#..','#####','..#..','..#..','.....'],
 '-':['....','....','....','####','....','....','....'],'!':['##','##','##','##','##','..','##'],
 '?':['.##.','#..#','...#','..#.','.##.','....','.##.'],'z':['....','....','####','..#.','.#..','####','....'],
 '♥':['.......','.##.##.','#######','#######','.#####.','..###..','...#...'],'<':['...','..#','.##','###','.##','..#','...'],
 '.':['.','.','.','.','.','.','#'],',':['.','.','.','.','.','#','#'],"'":['#','#','.','.','.','.','.'],'/':['...#','...#','..#.','.#..','#...','#...','#...'],'%':['##..#','##.#.','..#..','..#..','.#...','#.###','..###']
};
SMALL['>']=mirRows(SMALL['<']);
const TXT={};
('A.###.|#...#|#...#|#####|#...#|#...#|#...# B####.|#...#|#...#|####.|#...#|#...#|####. C.###.|#...#|#....|#....|#....|#...#|.###. D####.|#...#|#...#|#...#|#...#|#...#|####. E#####|#....|#....|####.|#....|#....|##### F#####|#....|#....|####.|#....|#....|#.... G.###.|#...#|#....|#.###|#...#|#...#|.#### H#...#|#...#|#...#|#####|#...#|#...#|#...# I###|.#.|.#.|.#.|.#.|.#.|### J..###|...#.|...#.|...#.|...#.|#..#.|.##.. K#...#|#..#.|#.#..|##...|#.#..|#..#.|#...# L#....|#....|#....|#....|#....|#....|##### M#...#|##.##|#.#.#|#.#.#|#...#|#...#|#...# N#...#|#...#|##..#|#.#.#|#..##|#...#|#...# O.###.|#...#|#...#|#...#|#...#|#...#|.###. P####.|#...#|#...#|####.|#....|#....|#.... Q.###.|#...#|#...#|#...#|#.#.#|#..#.|.##.# R####.|#...#|#...#|####.|#.#..|#..#.|#...# S.####|#....|#....|.###.|....#|....#|####. T#####|..#..|..#..|..#..|..#..|..#..|..#.. U#...#|#...#|#...#|#...#|#...#|#...#|.###. V#...#|#...#|#...#|#...#|#...#|.#.#.|..#.. W#...#|#...#|#...#|#.#.#|#.#.#|#.#.#|.#.#. X#...#|#...#|.#.#.|..#..|.#.#.|#...#|#...# Y#...#|#...#|.#.#.|..#..|..#..|..#..|..#.. Z#####|....#|...#.|..#..|.#...|#....|#####').split(' ').forEach(s=>{TXT[s[0]]=s.slice(1).split('|');});
const glyphOf=(font,ch)=>font==='big'?BIG[ch]:font==='small'?SMALL[ch]:(TXT[ch]||SMALL[ch]);
const gapOf=(font,a,b)=>font==='big'?((a===':'||b===':')?3:2):1;
function textRun(g,str,x,y,font,sc,v,draw){let cx=x,prev=null;for(const ch of String(str)){if(ch===' '){cx+=(font==='big'?5:4)*sc;prev=null;continue;}const gl=glyphOf(font,ch);if(!gl)continue;if(prev!==null)cx+=gapOf(font,prev,ch)*sc;if(draw)for(let r=0;r<gl.length;r++)for(let c=0;c<gl[r].length;c++)if(gl[r][c]==='#')for(let a=0;a<sc;a++)for(let b=0;b<sc;b++)px(g,cx+c*sc+b,y+r*sc+a,v);cx+=gl[0].length*sc;prev=ch;}return cx-x;}
const text=(g,str,x,y,font='txt',sc=1,v=1)=>textRun(g,str,x,y,font,sc,v,true);
const textW=(str,font='txt',sc=1)=>textRun(null,str,0,0,font,sc,1,false);
const textC=(g,str,cx,y,font='txt',sc=1,v=1)=>text(g,str,Math.round(cx-textW(str,font,sc)/2),y,font,sc,v);

/* ---------- face rig ---------- */
function eye(g,cx,cy,e,side,t){
  const s=e.s||1,w=e.w*s,h=e.h*s,hw=w/2,hh=h/2,r=Math.min(e.r*s,hw,hh),y0=cy-hh;
  let shape=e.shape||'slab';const lt=e.lt||0,lb=e.lb||0,sl=e.sl||0;
  if(shape==='slab'&&lt+lb>=.9)shape='shut';
  const th=s>=.8?3:2,th2=s>=.8?2:1;
  if(shape==='shut')stroke(g,curve(cx,y0+h*(lt/(lt+lb))*.85+h*.08,hw*.8,u=>1.4*s*(1-u*u)),th2);
  else if(shape==='arcDown')stroke(g,curve(cx,cy+hh*.05,hw*.78,u=>hh*.3*(1-u*u)),th2);
  else if(shape==='arcUp')stroke(g,curve(cx,cy+hh*.3,hw*.8,u=>-hh*.55*(1-u*u)),th);
  else if(shape==='chev'){const ox=cx+side*hw*.55,ix=cx-side*hw*.45;stroke(g,[[ox,cy-hh*.45],[ix,cy],[ox,cy+hh*.45]],th);}
  else if(shape==='x'){stroke(g,[[cx-hw*.5,cy-hh*.45],[cx+hw*.5,cy+hh*.45]],th);stroke(g,[[cx+hw*.5,cy-hh*.45],[cx-hw*.5,cy+hh*.45]],th);}
  else{
    const pup=e.pup||'round',pr=(pup==='tiny'?3:(e.pr||5))*s;
    const PX=Math.max(-1,Math.min(1,e.px||0)),PY=Math.max(-1,Math.min(1,e.py||0));
    const pcx=cx+PX*Math.max(0,hw-pr-3*s),pcy=cy+PY*Math.max(0,hh-pr-3*s);
    const RR=rr(cx,cy,hw,hh,r),sr=w*.7*(e.sm||0),scy=y0+h*1.05,gx=pcx-pr*.45,gy=pcy-pr*.45,rmax=Math.min(hw,hh)-2.5*s;
    area(g,(X,Y)=>{
      if(!RR(X,Y))return false;
      const inner=(X-cx)/hw*(-side);
      if(Y<y0+lt*h+sl*hh*inner)return false;
      if(Y>y0+h-lb*h)return false;
      if(sr>0&&(X-cx)**2+(Y-scy)**2<sr*sr)return false;
      const dx=X-pcx,dy=Y-pcy;
      if(pup==='round'||pup==='tiny'){if(dx*dx+dy*dy<pr*pr)return pup==='round'&&pr>=3.5&&Math.abs(X-gx)<1.05&&Math.abs(Y-gy)<1.05;}
      else if(pup==='slit'){if((dx/(1.6*s+.4))**2+(dy/(hh*.62))**2<=1)return false;}
      else if(pup==='heart'){const hx=Math.floor(dx+3.5),hy=Math.floor(dy+3);if(hx>=0&&hx<7&&hy>=0&&hy<6&&HEART7[hy][hx]==='#')return false;}
      else if(pup==='spiral'){const qx=X-cx,qy=Y-cy,d=Math.hypot(qx,qy);if(d<rmax){const v=(((Math.atan2(qy,qx)/(2*Math.PI))+d/(4.5*s)+t)%1+1)%1;if(v<.34)return false;}}
      else if(pup==='star'){const a=Math.abs(dx)/(pr*1.5),b=Math.abs(dy)/(pr*1.5);if(Math.sqrt(a)+Math.sqrt(b)<=1)return false;}
      return true;
    },[cx-hw-1,cy-hh-1,cx+hw+1,cy+hh+1]);
  }
  if(e.brow){const b=e.brow,by=y0-5*s-(b.lift||0)*s,ix=cx-side*hw*.85,ox=cx+side*hw*.85,a=(b.a||0)*s;stroke(g,[[ox,by-a*.4],[(ix+ox)/2,by-s+a*.3],[ix,by+a]],s>=.8?3:2);}
}
function mouth(g,type,mx,my,s,f){
  const hw=12*s*(f.mw||1),th=s>=.9?2:1;
  const fang=(x,y)=>{x=Math.round(x);y=Math.round(y);if(s>=.9){for(let k=-1;k<=1;k++){px(g,x+k,y);px(g,x+k,y+1);}px(g,x,y+2);}else{px(g,x,y);px(g,x+1,y);px(g,x,y+1);}};
  const C=(fn,w=1)=>stroke(g,curve(mx,my,hw*w,fn),th);
  const fangOn=(fn,w,u)=>fang(mx+u*hw*w,my+fn(u)+th);
  switch(type){
    case 'line':{const fn=u=>2*s*(1-u*u);C(fn);fangOn(fn,1,.35);break;}
    case 'smile':{const fn=u=>3.5*s*(1-u*u)-.5;C(fn);fangOn(fn,1,.35);break;}
    case 'flat':{const fn=()=>1;C(fn,.55);fangOn(fn,.55,.4);break;}
    case 'frown':C(u=>2*s-2.8*s*(1-u*u),.7);break;
    case 'grin':{const w=hw*.85,top=my-1,dep=7.5*s;ring(g,(X,Y)=>{const u=(X-mx)/w;return Math.abs(u)<=1&&Y>=top&&Y<=top+dep*(1-u*u)+.5;},[mx-w-1,top-1,mx+w+1,top+dep+2]);fang(mx+w*.38,top+1);break;}
    case 'rawr':{const rx=hw*.62,ry=6.5*s,cy=my+1;ring(g,ell(mx,cy,rx,ry),bE(mx,cy,rx,ry));fang(mx-rx*.45,cy-ry+1.5);fang(mx+rx*.45,cy-ry+1.5);break;}
    case 'chompOpen':{const rx=hw*.6,ry=5*s,cy=my+1;ring(g,ell(mx,cy,rx,ry),bE(mx,cy,rx,ry));fang(mx-rx*.4,cy-ry+1.5);fang(mx+rx*.4,cy-ry+1.5);const by=Math.round(cy+ry-1.5);for(const u of[-.15,.45]){const x=Math.round(mx+u*rx);px(g,x,by);px(g,x+1,by);px(g,x,by-1);}break;}
    case 'o':{const a=Math.max(1.6,3*s),b=Math.max(1.8,3.6*s);ring(g,ell(mx,my+1,a,b),bE(mx,my+1,a,b));break;}
    case 'oSmall':{const a=Math.max(1.3,2.2*s);ring(g,ell(mx,my+1,a,a),bE(mx,my+1,a,a));break;}
    case 'yawn':{const a=Math.max(2,4.6*s),b=Math.max(2.6,6.6*s);ring(g,ell(mx,my+2,a,b),bE(mx,my+2,a,b));break;}
    case 'grr':{const pts=[],n=Math.max(4,Math.round(hw*1.6/(2*s+.5)));for(let i=0;i<=n;i++)pts.push([mx-hw*.8+i*(hw*1.6/n),my+(i%2?2*s:0)]);stroke(g,pts,th);break;}
    case 'wobble':C(u=>1.6*s*Math.sin(u*Math.PI*2)+1,.65);break;
    case 'smirk':{const fn=u=>u>0?1-2.8*s*u*u:1;C(fn,.8);fangOn(fn,.8,-.35);break;}
    case 'tongue':{const fn=u=>2.5*s*(1-u*u);C(fn,.9);const tx=mx+hw*.25,ty=my+fn(.25/.9)+th;area(g,(X,Y)=>Y>=ty&&ell(tx,ty,3.2*s+.4,4.4*s+.4)(X,Y),[tx-5,ty,tx+5,ty+6]);px(g,tx,ty+1,0);px(g,tx,ty+2,0);break;}
    case 'chomp':{const pts=[];for(let i=0;i<=8;i++)pts.push([mx-hw*.75+i*(hw*1.5/8),my+(i%2?3*s:0)]);stroke(g,pts,th);break;}
    case 'drool':{C(()=>1,.55);const x=mx+hw*.4;for(let k=2;k<5;k++)px(g,x,my+k*s+1);dot(g,x,my+6*s+1,1);break;}
  }
}
function fx(g,f,cx,cy,s,ax,E){
  const top=y=>Math.max(1,y);
  if(f.z){zee(g,ax+1,cy-6*s,false);if(f.z>1)zee(g,ax+6,cy-15*s,true);}
  if(f.bang)for(let k=0;k<f.bang;k++)text(g,'!',ax+2+k*4,top(cy-24*s),'small');
  if(f.q)text(g,'?',ax+1,top(cy-24*s),'small');
  if(f.heart)text(g,'♥',ax-1,top(cy-24*s),'small');
  if(f.note)bmp(g,NOTE,ax+1,top(cy-22*s));
  if(f.sweat)bmp(g,DROP,ax,cy-12*s);
  if(f.tear)bmp(g,DROP,E[0].ex-2,E[0].ey+E[0].hh+3*s);
  if(f.steam)E.forEach((q,i)=>{const sd=i?1:-1,x=q.ex+sd*(q.hw+4*s),y=cy-17*s;edge(g,ell(x,y,2.4*s+.6,2*s+.6),bE(x,y,4,4));edge(g,ell(x+sd*3*s,y-4*s,1.6*s+.4,1.4*s+.4),bE(x,y-4*s,5,5));});
  if(f.sparkle){plus(g,ax+3,top(cy-18*s));plus(g,E[0].ex-E[0].hw-5*s,cy-14*s);plus(g,ax+1,cy+12*s);}
  if(f.crown)bmp(g,CROWN,cx-3,top(cy-Math.max(E[0].hh,E[1].hh)-12*s));
  if(f.dots)for(let k=0;k<f.dots;k++){px(g,ax+k*4,cy+10*s);px(g,ax+k*4+1,cy+10*s);px(g,ax+k*4,cy+10*s+1);px(g,ax+k*4+1,cy+10*s+1);}
}
function face(g,f){
  const s=f.s??1,cx=(f.x??64)+(f.dx||0),cy=(f.y??27)+(f.dy||0),sp=(f.sp??24)*s,t=f.t||0;
  const lk=f.look||[0,0],gx=lk[0]*3*s,gy=lk[1]*2*s,E=[];
  for(const side of[-1,1]){
    const e=Object.assign({},side<0?f.L:f.R);e.s=s;e.px=(e.px||0)+lk[0]*.9;e.py=(e.py??.12)+lk[1]*.9;
    const ex=cx+side*sp+gx,ey=cy+gy;
    eye(g,ex,ey,e,side,t);E.push({ex,ey,hw:e.w*s/2,hh:e.h*s/2});
    if(f.cheeks){const y=ey+e.h*s/2+3*s;for(const k of[-1,0,1]){const x=ex+k*5*s;line(g,x-1,y+1,x+1,y-1);}}
  }
  if(f.glasses){E.forEach((q,i)=>{const hw=q.hw+3*s,hh=q.hh+2.5*s,sd=i?1:-1;edge(g,rr(q.ex,q.ey,hw,hh,5*s),[q.ex-hw-1,q.ey-hh-1,q.ex+hw+1,q.ey+hh+1]);line(g,q.ex+sd*hw,q.ey-hh*.3,q.ex+sd*(hw+5*s),q.ey-hh*.45);});const y=E[0].ey-E[0].hh*.35;line(g,E[0].ex+E[0].hw+3*s,y,E[1].ex-E[1].hw-3*s,y);}
  if(f.nose!==false&&s>=.75){const ny=Math.round(cy+19*s+gy*.6),nx=cx+gx*.6;px(g,nx-5,ny);px(g,nx-4,ny);px(g,nx+4,ny);px(g,nx+5,ny);}
  if(f.m!=='none')mouth(g,f.m||'line',cx+gx*.6+(f.mx||0)*s,cy+(f.my??26)*s+gy*.5,s,f);
  fx(g,f,cx,cy,s,E[1].ex+E[1].hw+4*s,E);
}
const BL={w:28,h:30,r:10,pr:5,py:.12},BR={w:26,h:28,r:9,pr:5,py:.12};
const EX={
  idle:{m:'line'},
  happy:{E:{shape:'arcUp'},m:'grin',cheeks:1},
  love:{E:{pup:'heart'},m:'smile',cheeks:1,heart:1},
  curious:{Lx:{w:30,h:32,r:11,brow:{a:-1,lift:3}},Rx:{lt:.32,sl:-.25,h:26},m:'o',look:[1,-.3],mx:3},
  sleepy:{E:{lt:.55,sl:-.25,lb:.04,py:.5},m:'flat',y:28},
  asleep:{E:{shape:'arcDown'},m:'oSmall',y:29,z:1},
  surprised:{Lx:{w:32,h:34,r:12,pup:'tiny',brow:{a:-1,lift:3}},Rx:{w:30,h:32,r:11,pup:'tiny',brow:{a:-1,lift:3}},m:'o'},
  grumpy:{E:{lt:.38,sl:.55,lb:.1,py:.25,brow:{a:4}},m:'grr',steam:1},
  rawr:{E:{pup:'slit',lt:.2,sl:.5,brow:{a:4}},m:'rawr'},
  dizzy:{E:{pup:'spiral'},m:'wobble'},
  shy:{Lx:{w:22,h:24,r:8,lt:.1,lb:.15},Rx:{w:21,h:23,r:8,lt:.1,lb:.15},look:[-1,.7],m:'wobble',mw:.7,cheeks:1},
  unimpressed:{E:{lt:.5,lb:.15,px:.85,py:.1},m:'flat'},
  mischief:{Lx:{lt:.34,sl:.4,brow:{a:3}},Rx:{shape:'arcUp'},m:'tongue'},
  sad:{E:{lt:.18,sl:-.55,py:.5,brow:{a:-3,lift:1}},m:'frown',tear:1},
  focus:{E:{lt:.32,lb:.05,py:.6},glasses:1,m:'flat'},
  excited:{E:{pup:'star',pr:6},m:'grin',sparkle:1},
  hungry:{E:{lt:.4,sl:-.2,px:.7,py:.3},m:'drool'},
  chew:{E:{shape:'arcUp'},m:'chomp',cheeks:1},
  smug:{E:{lt:.45,lb:.12,px:.6},m:'smirk'},
  scared:{E:{pup:'tiny',brow:{a:-3,lift:2}},m:'wobble',sweat:1},
  determined:{E:{pup:'slit',lt:.15,sl:.3,brow:{a:2}},m:'flat'},
  squeeze:{E:{shape:'chev'},m:'grin',cheeks:1},
  wink:{Rx:{shape:'arcUp'},m:'smile'},
  yawn:{E:{shape:'chev'},m:'yawn'}
};
function X(n,o){const f={L:Object.assign({},BL),R:Object.assign({},BR)};const ap=q=>{for(const k in q){if(k==='E'){Object.assign(f.L,q.E);Object.assign(f.R,q.E);}else if(k==='Lx')Object.assign(f.L,q.Lx);else if(k==='Rx')Object.assign(f.R,q.Rx);else f[k]=q[k];}};ap(EX[n]||EX.idle);ap(o||{});return f;}

/* ---------- icons + props ---------- */
function mkIcon(fn){const g=mk();fn(g);const a=new Uint8Array(256);for(let y=0;y<16;y++)for(let x=0;x<16;x++)a[y*16+x]=g[y*W+x];return a;}
const B16=[0,0,15,15];
const ICON={
  clock:mkIcon(g=>{edge(g,ell(7.5,7.5,7.2,7.2),B16);px(g,7,2);px(g,7,12);px(g,2,7);px(g,12,7);line(g,7,7,7,4);line(g,7,7,10,7);}),
  alarm:mkIcon(g=>{edge(g,ell(7.5,9.5,5.6,5.6),B16);stroke(g,[[1,4],[3,2]],2);stroke(g,[[11,2],[13,4]],2);line(g,4,14,3,15);line(g,11,14,12,15);line(g,7,9,7,6);line(g,7,9,9,9);}),
  focus:mkIcon(g=>{edge(g,ell(3.5,8.5,3.2,3.2),B16);edge(g,ell(11.5,8.5,3.2,3.2),B16);line(g,7,7,8,7);line(g,0,6,0,5);line(g,15,6,15,5);}),
  games:mkIcon(g=>{edge(g,rr(7.5,8.5,7.4,4.8,3),B16);line(g,2,8,6,8);line(g,4,6,4,10);px(g,10,9);px(g,11,9);px(g,12,7);px(g,13,7);}),
  friends:mkIcon(g=>{area(g,rr(5,10,4.6,4.2,2),B16);area(g,rr(11,7,5.4,5,2.5),B16,0);area(g,rr(11,7,4.2,4,2),B16);px(g,3,9,0);px(g,6,9,0);px(g,9,6,0);px(g,12,6,0);px(g,8,2);px(g,11,2);px(g,14,2);}),
  chat:mkIcon(g=>{edge(g,rr(7.5,6.5,7.4,5.6,3),B16);for(let x=4;x<=6;x++)px(g,x,12,0);line(g,4,12,3,15);line(g,3,15,7,12);[4,7,10].forEach(x=>{px(g,x,6);px(g,x+1,6);});}),
  settings:mkIcon(g=>{[3,8,13].forEach(y=>line(g,0,y,15,y));ring(g,ell(4.5,3.5,2.4,2.4),B16);ring(g,ell(11.5,8.5,2.4,2.4),B16);ring(g,ell(6.5,13.5,2.4,2.4),B16);}),
  back:mkIcon(g=>{area(g,(x,y)=>{const a=[0,4.5],b=[5,0],c=[5,9];const s=(p,q)=>(x-q[0])*(p[1]-q[1])-(p[0]-q[0])*(y-q[1]);const d1=s(a,b),d2=s(b,c),d3=s(c,a);return!((d1<0||d2<0||d3<0)&&(d1>0||d2>0||d3>0));},B16);line(g,5,4,10,4);line(g,5,5,10,5);area(g,(x,y)=>x>=10&&ell(10,9,5.4,5.1)(x,y)&&!ell(10,9,3.4,3.1)(x,y),B16);line(g,5,13,10,13);line(g,5,14,10,14);}),
  meteor:mkIcon(g=>{area(g,ell(10.5,10.5,4.2,4.2),B16);px(g,11,9,0);px(g,9,12,0);px(g,12,12,0);line(g,1,0,6,5);line(g,0,4,5,8);line(g,4,0,8,4);}),
  bug:mkIcon(g=>{area(g,ell(8,10.5,4.4,3.2),B16);area(g,ell(13.2,9.5,2,2),B16);edge(g,ell(5.5,5.5,3,2.4),B16);edge(g,ell(10.5,5.5,3,2.4),B16);line(g,5,13,4,15);line(g,8,14,8,15);line(g,11,13,12,15);}),
  duel:mkIcon(g=>stroke(g,[[10,0],[5,8],[10,8],[5,15]],2)),
  pong:mkIcon(g=>{line(g,1,3,1,12);line(g,2,3,2,12);area(g,ell(11.5,5.5,1.7,1.7),B16);for(let y=0;y<16;y+=3)px(g,7,y);}),
  egg:mkIcon(g=>{edge(g,ell(7.5,8.5,5.6,7.4),B16);line(g,3,8,5,10);line(g,5,10,7,7);line(g,7,7,9,10);line(g,9,10,11,8);}),
  mail:mkIcon(g=>{edge(g,rr(7.5,8.5,7.4,5.4,0),B16);line(g,1,4,7,9);line(g,8,9,14,4);}),
  leaf:mkIcon(g=>{area(g,rell(8,8,7,3.6,-.78),B16);line(g,4,12,11,5,0);line(g,1,14,3,12);}),
  coffee:mkIcon(g=>{edge(g,rr(6.5,10.5,5,4.5,1.5),B16);edge(g,ell(12.5,10,2.3,2),[10,6,15,14]);stroke(g,[[5,1],[4,3],[5,5]],1);stroke(g,[[8,1],[7,3],[8,5]],1);})
};
const stampIcon=(g,ic,x0,y0,v=1)=>{for(let y=0;y<16;y++)for(let x=0;x<16;x++)if(ic[y*16+x])px(g,x0+x,y0+y,v);};
function card(g,x0,y0,w,h){const x1=x0+w-1,y1=y0+h-1;for(let y=y0;y<=y1+1;y++)for(let x=x0;x<=x1;x++){if(y===y1+1){if(x>x0&&x<x1)px(g,x,y);continue;}if((x===x0||x===x1)&&(y===y0||y===y1))continue;px(g,x,y,(x===x0||x===x1||y===y0||y===y1)?1:0);}}
function alarmProp(g,r,x=108,y=46){stampIcon(g,ICON.alarm,x+(r===1?-1:r===2?1:0),y);if(r===1){line(g,x-4,y+2,x-6,y);line(g,x-5,y+7,x-8,y+7);}if(r===2){line(g,x+18,y+1,x+20,y-1);line(g,x+18,y+6,x+21,y+6);}}
function batt(g,x,y,n){edge(g,rr(x+7,y+4.5,7,4.5,0),[x-1,y-1,x+15,y+10]);px(g,x+14,y+3);px(g,x+14,y+4);px(g,x+14,y+5);px(g,x+15,y+3);px(g,x+15,y+4);px(g,x+15,y+5);for(let k=0;k<n;k++)area(g,()=>true,[x+2+k*4,y+2,x+4+k*4,y+6]);}
function mini(g,x,yb,o={}){const hw=7,hh=5.5,cy=yb-hh;area(g,rr(x,cy,hw,hh,3),[x-hw-1,cy-hh-1,x+hw+1,cy+hh+1]);for(const k of[-3,0,3])px(g,x+k,cy-hh-1);const lk=o.look||0,ey=Math.round(cy-1);for(const ex of[Math.round(x-3+lk),Math.round(x+2+lk)]){if(o.blink){px(g,ex,ey+1,0);px(g,ex+1,ey+1,0);}else for(let a=0;a<2;a++)for(let b=-1;b<2;b++)px(g,ex+a,ey+b,0);}line(g,x-2,yb-2,x+2,yb-2,0);px(g,x+1,yb-1,0);if(o.bang)text(g,'!',x-1,Math.max(1,cy-hh-10),'small');}
function meteor(g,x,y){area(g,ell(x,y,2.8,2.8),bE(x,y,3,3));line(g,x+2,y-3,x+6,y-8);line(g,x+3,y-1,x+8,y-4);line(g,x,y-3,x+2,y-8);}
function burst(g,x,y){for(let k=0;k<8;k++){const a=k*Math.PI/4;line(g,x+Math.cos(a)*4,y+Math.sin(a)*4,x+Math.cos(a)*8,y+Math.sin(a)*8);}}
function bug(g,x,y,flap){bmp(g,['.###.','#####','.###.'],x-2,y-1);px(g,x+3,y);bmp(g,flap?['#...#','.#.#.']:['##.##'],x-2,flap?y-3:y-2);}
function egg(g,x,y,c=0){const f=ell(x,y,6,8);area(g,f,bE(x,y,6,8),0);edge(g,f,bE(x,y,6,8));if(c>=1){line(g,x+2,y-5,x+4,y-3);line(g,x+4,y-3,x+3,y-1);}if(c>=2){line(g,x-5,y,x-2,y+2);line(g,x-2,y+2,x,y);line(g,x,y,x+2,y+2);}if(c>=3)line(g,x+2,y+2,x+5,y-1);}
function eggHatch(g,x,y,k){const f=ell(x,y,6,8);const zig=X=>y-1+((Math.round(X)%3)===0?-1:1);area(g,(X,Y)=>f(X,Y)&&Y>=zig(X),bE(x,y,6,8),0);edge(g,(X,Y)=>f(X,Y)&&Y>=zig(X),bE(x,y,6,8));if(k===1){mini(g,x,y+2,{});const cap=(X,Y)=>ell(x+3,y-6,6,8)(X,Y)&&Y<y-6;area(g,cap,bE(x+3,y-6,6,8),0);edge(g,cap,bE(x+3,y-6,6,8));}if(k===2){mini(g,x,y-2,{});edge(g,(X,Y)=>ell(x+15,y+8,5,6)(X,Y)&&Y<y+8,bE(x+15,y+8,6,7));}}
const speed=(g,x,y0,y1)=>{for(let y=y0;y<=y1;y+=6)line(g,x,y,x+3,y);};
const shiftG=(g,dx,dy)=>{const o=mk();for(let y=0;y<H;y++)for(let x=0;x<W;x++)if(g[y*W+x]){const nx=x+dx,ny=y+dy;if(nx>=0&&nx<W&&ny>=0&&ny<H)o[ny*W+nx]=1;}return o;};

/* ---------- scenes ---------- */
function build(T,FN){
  T=/^\d\d:\d\d$/.test(T||'')?T:'10:24';
  FN=String(FN||'PIP').toUpperCase().replace(/[^A-Z0-9]/g,'').slice(0,6)||'PIP';
  const nextMin=t=>{let h=+t.slice(0,2),m=+t.slice(3)+1;if(m===60){m=0;h=(h+1)%24;}return pad2(h)+':'+pad2(m);};
  const groups=[];let GR=null;
  const group=(gid,name,desc)=>{GR={gid,name,desc,scenes:[]};groups.push(GR);};
  const add=(key,name,trigger,desc,frames)=>GR.scenes.push({id:GR.gid+(GR.scenes.length+1),key,name,trigger,desc,frames});
  const F=ds=>{const g=mk();ds.forEach(d=>d&&d(g));return g;};
  const fr=(ms,note,...ds)=>[F(ds),ms,note||''];
  const fa=(n,o)=>g=>face(g,X(n,o));
  const sm=(n,o)=>fa(n,Object.assign({s:.55,x:24,y:31,look:[1,0]},o||{}));
  const tx=(s,x,y,font,sc)=>g=>text(g,s,x,y,font||'txt',sc||1);
  const tc=(s,cx,y,font,sc)=>g=>textC(g,s,cx,y,font||'txt',sc||1);
  const ic=(k,x,y)=>g=>stampIcon(g,ICON[k],x,y);
  const I=o=>fa('idle',o);
  const SHUT={lt:.5,lb:.5},HALF={lt:.35,lb:.2};

  /* R · rig */
  group('R','Face rig','The parts every screen is built from. Eyes are rounded slabs with a pupil cut out and a 2 px glint. Lids are straight cuts, so they can slant, and brow ridges sit 5 px above the eye. The nostrils and a single fang make it read as a dino rather than a robot. All of it is parametric, so the same eye draws at full size and at 0.55.');
  const cellE=(o,x,y,side)=>g=>eye(g,x,y,Object.assign({w:28,h:30,r:10,pr:5,py:.1,s:.5},o),side||1,.1);
  add('rig_eyes','Eye parts','Pupils (top) · shapes (bottom), shown at 0.5',
    'Top row, pupils: round with glint (default), tiny (shock), slit (serious, games), heart (love), spiral (dizzy, rotates), star (excited). Bottom row, whole-eye shapes: happy arch, sleeping smile, squeeze chevron, blink (lids meet), half lid, and the smile cut that carves the bottom of the slab.',
    [fr(0,'pupils · shapes',...[{},{pup:'tiny'},{pup:'slit'},{pup:'heart'},{pup:'spiral'},{pup:'star',pr:6}].map((o,i)=>cellE(o,11+i*21,15)),...[{shape:'arcUp'},{shape:'arcDown'},{shape:'chev'},SHUT,{lt:.45},{sm:1}].map((o,i)=>cellE(o,11+i*21,47)))]);
  add('rig_brows','Brows and lids','Brow angle a · lift · lid slant sl',
    'The brow ridge is a 3 px stroke (2 px when small). Top row: worried (a −3, lifted), raised (lift 4), stern (a 2), angry (a 5 with a slanted lid). Bottom row, lids only: sad droop (sl −0.45), unimpressed (flat half lid), squint (bottom lid up), and glare (slanted lid with slit pupil).',
    [fr(0,'brows · lids',...[{brow:{a:-3,lift:2}},{brow:{a:0,lift:4}},{brow:{a:2}},{brow:{a:5},lt:.3,sl:.6}].map((o,i)=>cellE(o,16+i*32,22,-1)),...[{lt:.3,sl:-.45},{lt:.5,lb:.1},{lt:.2,lb:.35},{lt:.4,sl:.5,pup:'slit'}].map((o,i)=>cellE(o,16+i*32,50,-1)))]);
  const MT=['line','smile','flat','frown','grin','rawr','o','oSmall','yawn','grr','wobble','smirk','tongue','chomp','chompOpen','drool'];
  add('rig_mouths','Mouths','16 shapes · 2 px stroke at full size, 1 px below 0.9',
    'Row 1: soft smile with fang (default), wide smile, flat, frown. Row 2: open grin, RAWR (two fangs), small o, tiny o (asleep). Row 3: yawn, grumble zigzag, wobble (nervous), smirk. Row 4: tongue out, chomp closed, chomp open (teeth top and bottom), drool. Shown at 0.75.',
    [fr(0,MT.join(' · '),...MT.map((t,i)=>g=>mouth(g,t,16+(i%4)*32,3+Math.floor(i/4)*16,.75,{})))]);
  add('rig_type','Type and digits','Large 9 × 13 · small 4 × 7 · text 5 × 7',
    'New is a 5 × 7 uppercase alphabet for names and messages, with 1 px spacing and a 4 px word gap. Received messages render it at 2×.',
    [fr(0,'full set',tx('0123456789',2,1,'big'),tx('0123456789:+-!?♥',2,17,'small'),tx('ABCDEFGHIJKLM',2,27),tx('NOPQRSTUVWXYZ',2,37),tx(FN+': BOOP! ♥',2,50))]);
  const IK=['clock','alarm','focus','games','friends','chat','settings','back','meteor','bug','duel','pong','egg','mail','leaf','coffee'];
  add('rig_icons','Icons 16 × 16',IK.join(' · '),
    'Row 1 is the main menu: clock, alarm, focus glasses, games pad, friends (two little dinos), messages, settings, back. Row 2 is games and props: meteor, bug, quick-draw bolt, pong, egg, envelope, leaf (charging snack), coffee. 1 px lines, exported as 16 × 16 PNGs.',
    [fr(0,'menu row · props row',...IK.map((k,i)=>ic(k,i%8*16,i<8?10:38)))]);
  const XN=['idle','happy','love','curious','sleepy','asleep','surprised','grumpy','rawr','dizzy','shy','unimpressed','mischief','sad','focus','excited','hungry','chew','smug','scared','determined','squeeze','wink','yawn'];
  add('expressions','Expression sheet','24 presets · each is one parameter row',
    'Every preset is a set of overrides on the base face, such as lids, pupil type, brow, mouth and extras. Animations tween between rows, so any expression can blend into any other. The slideshow plays each one for 0.9 s.',
    XN.map(n=>fr(900,n,fa(n))));
  add('scale','Scale ladder','1.0 · 0.7 · 0.55 · 0.4',
    'The same idle face at 0.7, 0.55 and 0.4. At 0.55 the pupil hole is still 5 px and the glint survives. At 0.4 the pupil drops to 4 px with no glint, and brows merge into the lid, so 0.4 is never used. Below 0.75 the nostrils switch off.',
    [fr(0,'0.7 · 0.55 · 0.4 (not used)',fa('idle',{s:.7,x:27,y:30}),fa('idle',{s:.55,x:75,y:30}),fa('idle',{s:.4,x:112,y:30}))]);

  /* L · life */
  group('L','Life','What Deskimon does on its own and when handled. Each clip starts and ends on the idle face, so any of them can play between others. Reactions start 120–300 ms after the touch, a short pause that reads as noticing.');
  add('idle','Idle','Default loop · blink every 2.2–6.4 s',
    'Blinks are 60 ms down, 80 ms shut, 60 ms up. Pupils lead glances and the eyes follow 3 px. Breathing is a 1 px rise and fall. 12% of blinks are doubles.',
    [fr(2400,'idle',I()),fr(60,'blink',I({E:HALF})),fr(80,'',I({E:SHUT})),fr(60,'',I({E:HALF})),fr(1600,'',I()),fr(900,'glance right',I({look:[1,0]})),fr(700,'',I({look:[1,-.4]})),fr(1100,'glance left',I({look:[-1,.1]})),fr(1200,'breathe in',I({y:26})),fr(1200,'breathe out',I({y:28})),fr(60,'double blink',I({E:SHUT})),fr(120,'',I()),fr(60,'',I({E:SHUT})),fr(1400,'',I())]);
  add('pet','Pet','Hold touch 0.6 s',
    'Shy first: the eyes shrink and look away, with cheek scales. Then heart pupils, then a full grin with squeezed arches. The purr plays under the shy frame and the trill on the grin.',
    [fr(300,'touch',I({E:{brow:{a:-1,lift:2}}})),fr(700,'shy',fa('shy')),fr(1200,'heart eyes',fa('love')),fr(900,'grin',fa('squeeze')),fr(600,'happy',fa('happy')),fr(400,'',I())]);
  add('overpet','Over-petted','4th touch within 8 s · then ignores touch for 10 s',
    'Flat side-eye first, then brows down, a grumble zigzag and steam puffs. It holds the grudge: touches for the next 10 s get only a glance.',
    [fr(600,'side-eye',fa('unimpressed')),fr(500,'grumble',fa('grumpy')),fr(400,'',fa('grumpy',{dy:1})),fr(500,'',fa('grumpy')),fr(800,'looks away',fa('unimpressed',{look:[-1,0],E:{px:-.85}})),fr(400,'',I())]);
  add('rawr','RAWR','Idle quirk · every 2–4 min · or a firm double tap',
    'Its signature move: anticipation with slit pupils, a big RAWR with both fangs and a 2 px shake, then it breaks into giggles as if it had scared you. The buzzer growl starts on the RAWR frame.',
    [fr(500,'idle',I()),fr(400,'anticipation',fa('determined',{dy:1})),fr(140,'RAWR',fa('rawr',{dx:2,bang:1})),fr(140,'',fa('rawr',{dx:-2,bang:1})),fr(500,'',fa('rawr',{bang:1})),fr(300,'giggle',fa('squeeze')),fr(200,'',fa('happy',{dy:-1})),fr(200,'',fa('squeeze')),fr(400,'',I())]);
  add('shake','Shaken','IMU: 3 jerks over 1.2 g within 600 ms',
    'A startled jolt, then spiral pupils that spin (each frame advances the spiral a quarter turn) with a wobbly mouth. Then it slowly gets its eyes back.',
    [fr(120,'jolt',fa('surprised',{dx:3,bang:1})),fr(120,'',fa('surprised',{dx:-3,bang:1})),...[0,.25,.5,.75,0,.25,.5,.75].map((t,i)=>fr(150,i?'':'dizzy',fa('dizzy',{t,dx:i%2?1:-1}))),fr(500,'recovering',fa('sleepy')),fr(80,'',I({E:SHUT})),fr(400,'',I())]);
  add('pickup','Picked up','|a| off 1 g by 0.15 g for 150 ms, tilt over 25°',
    'Eyes go wide, then it peers down at the desk it just left, looks around, and settles into curiosity once it is held steady.',
    [fr(300,'lifted',fa('surprised',{dy:-2,bang:1})),fr(900,'looks down',fa('curious',{look:[0,1]})),fr(700,'',fa('curious',{look:[-1,.5]})),fr(700,'',fa('curious',{look:[1,.5]})),fr(600,'',fa('happy')),fr(400,'',I())]);
  add('tilt','Tilted','Roll over 20° · eyes slide downhill',
    'The face slides up to 6 px toward the low side with the pupils pinned downhill, plus a sweat drop. When levelled it does a relieved little grin.',
    [fr(700,'tilt left',fa('scared',{dx:-6,look:[-1,.4]})),fr(300,'',fa('scared',{dx:-3,look:[-.5,.2]})),fr(700,'tilt right',fa('scared',{dx:6,look:[1,.4]})),fr(300,'level',I()),fr(700,'relief',fa('happy'))]);
  add('sleep','Fall asleep','No input for 5 min · display dims once asleep',
    'Lids droop from the outer corners over a few seconds, the mouth goes slack, then the eyes curve shut and z’s rise with the breathing.',
    [fr(1500,'drowsy',fa('sleepy')),fr(1500,'',fa('sleepy',{E:{lt:.7}})),fr(400,'',fa('sleepy',{E:SHUT})),fr(1600,'asleep',fa('asleep')),fr(1600,'',fa('asleep',{y:30,z:2}))]);
  add('wake','Wake up','Any touch or motion while asleep',
    'One eye opens and checks you out, then a big squeezed yawn. It blinks twice and is back.',
    [fr(800,'asleep',fa('asleep',{z:2})),fr(700,'one eye',fa('idle',{Lx:{shape:'arcDown'},Rx:{lt:.5},m:'flat'})),fr(900,'yawn',fa('yawn')),fr(400,'',fa('sleepy')),fr(80,'',I({E:SHUT})),fr(120,'',I()),fr(80,'',I({E:SHUT})),fr(500,'',I())]);

  /* U · utility */
  group('U','Utility','Clock, alarm, focus, stopwatch, battery and settings. The face does the job itself: it shrinks aside to show the time, reacts to each menu card, glares at the alarm and puts on glasses to focus. Small-face screens all use the same anchor: scale 0.55 at x 24, y 31, looking right at the content.');
  const TB=(t,y)=>tx(t,62,y,'big');
  add('time','Time on request','Double tap',
    'Brows pop, the face shrinks to the left in two steps, and the digits drop in with a 2 px overshoot. Deskimon watches them, then gives you a smug look. The digits fall away and the face grows back.',
    [fr(500,'idle',I()),fr(200,'double tap: brows pop',I({E:{brow:{a:-1,lift:3}},look:[0,-.3]})),fr(120,'face slides aside',fa('idle',{s:.8,x:48,y:28,look:[1,0]})),fr(140,'',sm('idle')),fr(120,'digits drop in',sm('idle',{look:[1,-.6]}),TB(T,12)),fr(100,'overshoot',sm('idle'),TB(T,27)),fr(1400,'hold',sm('idle'),TB(T,25)),fr(1000,'smug at you',sm('smug',{look:[0,0],E:{px:0}}),TB(T,25)),fr(120,'digits fall',sm('idle',{look:[1,.6]}),TB(T,44)),fr(140,'face grows back',fa('idle',{s:.8,x:48,y:28})),fr(400,'',I())]);
  const T2=nextMin(T);
  add('clock','Clock mode','Always-on',
    'Small face on the left, time on the right, with a steady colon. When the minute flips, Deskimon glances at the new digits with raised brows. After a while it settles into a content half-lid, so most pixels sit still.',
    [fr(2500,'idle',sm('idle'),TB(T,25)),fr(150,'blink',sm('idle',{E:SHUT}),TB(T,25)),fr(1500,'',sm('idle',{look:[0,0]}),TB(T,25)),fr(300,'minute flips: glance',sm('idle',{E:{brow:{a:-1,lift:2}}}),TB(T2,25)),fr(1200,'',sm('idle'),TB(T2,25)),fr(1500,'content',sm('sleepy',{y:31,look:[.5,0]}),TB(T2,25))]);
  const OFF=[[0,0],[1,0],[1,1],[0,1],[-1,1],[-1,0],[-1,-1],[0,-1],[1,-1]];
  const base=F([sm('idle'),TB(T,25)]),env=mk();
  const sf=OFF.map(([dx,dy])=>{const s=shiftG(base,dx,dy);for(let k=0;k<W*H;k++)env[k]|=s[k];return[s,180000,'dx '+(dx>0?'+':'')+dx+' · dy '+(dy>0?'+':'')+dy,700];});
  sf.push([env,0,'all 9 overlaid (reference)',1600]);
  add('clockshift','Burn-in shift grid','Every 3 min · 9 positions · 27 min cycle',
    'The whole frame moves 1 px per step through a 3 × 3 grid. Stopwatch and focus countdown use the same table. The last frame overlays all nine positions. Preview plays at 0.7 s per step.',sf);
  const MENU=[['clock','CLOCK','smug'],['alarm','ALARM','surprised'],['focus','FOCUS','focus'],['games','GAMES','excited'],['friends','FRIENDS','love'],['chat','MESSAGES','curious'],['settings','SETTINGS','determined'],['back','BACK','idle']];
  const mcard=(k,dx=0,dy=0)=>g=>{card(g,58+dx,5+dy,28,26);stampIcon(g,ICON[k],64+dx,10+dy);};
  const mdots=i=>g=>{for(let k=0;k<8;k++){const x=51+k*6;if(k===i)area(g,()=>true,[x-1,54,x+2,57]);else area(g,()=>true,[x,55,x+1,56]);}};
  const arrows=g=>{text(g,'<',52,15,'small');text(g,'>',91,15,'small');};
  const mf=[];
  MENU.forEach(([k,lab,ex],i)=>{
    const o=ex==='idle'&&k==='back'?{x:23,look:[-1,0]}:{x:23,look:[1,-.2]};
    mf.push(fr(1100,lab.toLowerCase()+(i===0?' (opens here)':''),sm(ex,o),mcard(k),tc(lab,72,40),mdots(i),arrows));
    if(i<7){mf.push(fr(120,'tilt right: slide',sm('idle',{x:23,look:[1,0]}),mcard(k,5),g=>speed(g,52,9,27),mdots(i)));mf.push(fr(120,'swap',sm('idle',{x:23,look:[1,-.4]}),mdots(i+1)));mf.push(fr(120,'next pops up',sm('idle',{x:23,look:[1,-.4]}),mcard(MENU[i+1][0],0,-2),mdots(i+1)));}
  });
  add('menu','Menu','Long press 1.5 s · tilt to move · tap to choose',
    'One card at a time, with Deskimon reacting to each: smug for the clock, startled by the alarm, glasses on for focus, star eyes for games, heart eyes for friends, curious about messages, slit-eyed about settings, and a look back for back. Tilting runs slide, swap, pop. Dots show position.',mf);
  const aboard=(t,u,dy=0)=>g=>{card(g,56,12+dy,60,27);text(g,t,61,16+dy,'big');const ux=u==='h'?61:u==='m'?89:-1;if(ux>=0)area(g,()=>true,[ux,33+dy,ux+19,34+dy]);};
  const A=(t,u,ex,o,ms,note,...more)=>fr(ms,note,sm(ex,Object.assign({look:[1,-.2]},o||{})),aboard(t,u,(o&&o.bdy)||0),tc('ALARM',86,2),...more);
  add('setalarm','Set alarm','Tilt = change value · tap = next field, then confirm',
    'The time board with an underline under the field being edited. The underline is the only blinking element on the device: 20 × 2 px at 1.25 Hz. Each tilt shows an arrow and the brows flick up. On confirm the board hops and Deskimon grins with a heart.',
    [A('07:30','h','idle',{},400,'hours selected'),A('07:30',null,'idle',{},400,'underline blinks'),A('07:30','h','idle',{},400,''),A('08:30','h','idle',{E:{brow:{a:-1,lift:2}}},250,'tilt right: +1 h',tx('>',119,22,'small')),A('08:30','h','idle',{},400,''),A('08:30',null,'idle',{},400,''),A('08:30','m','idle',{look:[1,0]},400,'tap: minutes'),A('08:30',null,'idle',{look:[1,0]},400,''),A('08:35','m','idle',{E:{brow:{a:-1,lift:2}}},250,'tilt right: +5 min',tx('>',119,22,'small')),A('08:35','m','idle',{},400,''),A('08:35',null,'happy',{bdy:-2,heart:1},900,'tap: confirmed'),A('08:35',null,'happy',{},500,''),fr(140,'board away',sm('happy'),aboard('08:35',null,14)),fr(400,'idle',I())]);
  add('alarm','Alarm ringing','At alarm time · 3 stages, each loops about 20 s before escalating',
    'The alarm clock sits bottom-right and rattles 1 px. Stage 1: still asleep, a brow twitches, one eye opens and glares. Stage 2: wide awake with tiny pupils, bangs and a sweat drop, and the face shakes 1 px. Stage 3: squeezed eyes, full RAWR, steam, and a 2 px shake. Nothing flashes.',
    [fr(300,'stage 1: asleep',fa('asleep',{z:1}),g=>alarmProp(g,1)),fr(300,'brow twitch',fa('asleep',{E:{brow:{a:1}}}),g=>alarmProp(g,2)),fr(400,'one eye opens',fa('idle',{Lx:{shape:'arcDown'},Rx:{lt:.5,sl:.3,brow:{a:2}},m:'flat'}),g=>alarmProp(g,1)),fr(400,'',fa('idle',{Lx:{shape:'arcDown'},Rx:{lt:.5,sl:.3,brow:{a:2},px:.8,py:.6},m:'grr'}),g=>alarmProp(g,2)),
     fr(250,'stage 2: awake',fa('surprised',{dx:1,bang:1}),g=>alarmProp(g,1)),fr(250,'',fa('surprised',{dx:-1}),g=>alarmProp(g,2)),fr(250,'',fa('scared',{dx:1}),g=>alarmProp(g,1)),fr(250,'louder',fa('surprised',{dx:-1,bang:2}),g=>alarmProp(g,2)),
     fr(200,'stage 3: RAWR',fa('yawn',{m:'rawr',steam:1,dx:2}),g=>alarmProp(g,1)),fr(200,'',fa('yawn',{m:'rawr',steam:1,dx:-2,dy:1}),g=>alarmProp(g,2)),fr(220,'peak drama',fa('rawr',{bang:2,dy:-1}),g=>alarmProp(g,1)),fr(200,'',fa('yawn',{m:'rawr',steam:1,dx:2}),g=>alarmProp(g,2))]);
  add('snooze','Snooze','Tap while ringing',
    'The clock goes quiet. Deskimon grumbles with steam, +5 appears by the clock, and it drops straight back to sleep.',
    [fr(600,'grumble',fa('grumpy'),g=>alarmProp(g,0)),fr(800,'+5 min',fa('grumpy',{steam:0}),g=>{alarmProp(g,0);text(g,'+5',111,36,'small');}),fr(500,'eyes droop',fa('sleepy'),g=>{alarmProp(g,0);text(g,'+5',111,36,'small');}),fr(1400,'asleep',fa('asleep'),g=>alarmProp(g,0)),fr(1400,'',fa('asleep',{z:2,y:30}),g=>alarmProp(g,0))]);
  add('dismiss','Dismiss','Flip face-down or hold 2 s',
    'Alarm off. A huge squeezed yawn, a slow blink, then a grin and heart eyes to say good morning.',
    [fr(1000,'big yawn',fa('yawn'),g=>alarmProp(g,0)),fr(400,'',fa('sleepy'),g=>alarmProp(g,0)),fr(150,'',I({E:SHUT})),fr(300,'',I()),fr(900,'good morning',fa('happy')),fr(1400,'',fa('love'))]);
  const fb=(n,dy=0)=>g=>{card(g,56,10+dy,48,26);text(g,n,70,15+dy,'big');text(g,'<',59,18+dy,'small');text(g,'>',98,18+dy,'small');textC(g,'MIN',80,42+dy);};
  add('focuspick','Focus: pick length','Tilt = 25 / 45 / 60 · flip face-down = start',
    'Glasses go on as soon as focus is chosen. Tilting swings to the next length and wraps from 60 back to 25. Flipping the device face-down starts the session: Deskimon gives a serious nod and the board goes away.',
    [fr(1200,'25 min',sm('focus'),fb('25')),fr(150,'tilt right',sm('focus',{dx:1}),fb('45',-1)),fr(1000,'45 min',sm('focus'),fb('45')),fr(150,'tilt right',sm('focus',{dx:1}),fb('60',-1)),fr(1000,'60 min',sm('focus'),fb('60')),fr(150,'wraps to 25',sm('focus',{look:[-1,0]}),fb('25',-1)),fr(900,'',sm('focus'),fb('25')),fr(400,'flip: serious nod',sm('focus',{E:SHUT,dy:2}),fb('25')),fr(150,'board away',sm('focus'),fb('25',20))]);
  const fw=(o,t,ms,note,pms)=>[F([fa('focus',o),tx(t,114,1,'small')]),ms,note,pms];
  add('focus','Focus: working','During the session · countdown updates once a minute',
    'Full face with glasses, eyes lowered to the work and mouth set flat. Almost nothing moves: a slow blink now and then, a glance up at you about once a minute, and the countdown. Uses the burn-in shift table.',
    [fw({},'24',6000,'working',1400),fw({E:SHUT},'24',160,'blink'),fw({},'24',6000,'',1400),fw({E:{lt:.15,py:-.3}},'24',500,'glance at you'),fw({},'23',6000,'minute passes',1400)]);
  add('focusdone','Focus: done','Countdown reaches 00',
    'It looks up with a bang at 00, the glasses come off, then star eyes and a proud grin.',
    [fr(500,'time up',fa('surprised',{glasses:1,bang:1}),tx('00',114,1,'small')),fr(900,'stars',fa('excited')),fr(900,'',fa('happy')),fr(1200,'proud',fa('smug',{E:{px:0}}))]);
  const bx=(s,y)=>tc(s,114,y);
  add('break','Breathing break','After a focus session · 5 min · 4-4-4 breathing',
    'Deskimon breathes with you. The eyes grow 4 px and rise as you breathe in, hold, then the lids lower and the face sinks as you breathe out. The label sits bottom-right, clear of the mouth.',
    [fr(800,'looks at you',fa('happy'),bx('BREAK',56)),fr(1300,'in',I({E:{h:31,w:29},y:27}),bx('IN',56)),fr(1300,'',I({E:{h:33,w:30},y:26}),bx('IN',56)),fr(1400,'',I({E:{h:35,w:31,brow:{a:-1,lift:0}},y:26}),bx('IN',56)),fr(4000,'hold',I({E:{h:35,w:31,brow:{a:-1,lift:0}},y:26}),bx('HOLD',56)),fr(1300,'out',I({E:{lt:.15},y:26}),bx('OUT',56)),fr(1300,'',I({E:{lt:.3},y:28}),bx('OUT',56)),fr(1400,'',fa('sleepy',{y:29}),bx('OUT',56)),fr(1000,'again',fa('happy'))]);
  const sw=(t,ex,o,ms,note)=>fr(ms,note,sm(ex,o),TB(t,25));
  add('stopwatch','Stopwatch','Counting up · Deskimon nods once per minute',
    'MM:SS counts up beside the face. Just before the minute it watches the digits; on the minute it nods (blink and 2 px dip), then looks pleased. Uses the burn-in shift table while running.',
    [sw('00:58','idle',{},1000,''),sw('00:59','idle',{look:[1,0],E:{brow:{a:-1,lift:2}}},1000,'watching'),sw('01:00','idle',{E:SHUT,dy:2},200,'nod'),sw('01:00','idle',{},150,''),sw('01:00','idle',{E:SHUT,dy:2},200,'nod'),sw('01:00','happy',{},800,'pleased'),sw('01:01','idle',{},1000,''),sw('01:02','idle',{},1000,'')]);
  add('lowbatt','Low battery','Battery under 3.5 V · checks every 30 s',
    'Hungry, not alarmed: a battery with one bar appears bottom-right. Deskimon eyes it, drools, gets a little sad, and falls asleep sooner until it is fed. Below 3.3 V it says goodnight and turns off.',
    [fr(800,'',I(),g=>batt(g,106,50,1)),fr(600,'notices',I({look:[1,.8],E:{brow:{a:-1,lift:2}}}),g=>batt(g,106,50,1)),fr(1000,'hungry',fa('hungry',{look:[1,.6]}),g=>batt(g,106,50,1)),fr(900,'',fa('sad'),g=>batt(g,106,50,1)),fr(900,'drowsy',fa('sleepy'),g=>batt(g,106,50,1))]);
  const lf=(g,k)=>{if(k>0)stampIcon(g,ICON.leaf,96,44);if(k===2){area(g,()=>true,[96,44,103,59],0);}};
  add('charging','Charging','USB power detected',
    'Feeding time. A bang at plug-in, star eyes, then it chomps a leaf while the battery bars fill. When full it gets heart eyes and FULL appears.',
    [fr(300,'plugged in',fa('surprised',{bang:1}),g=>batt(g,108,2,1)),fr(500,'',fa('excited'),g=>{batt(g,108,2,1);lf(g,1);}),fr(200,'chomp',fa('chew',{m:'chompOpen',look:[.6,.4]}),g=>{batt(g,108,2,1);lf(g,1);}),fr(200,'',fa('chew'),g=>{batt(g,108,2,1);lf(g,2);}),fr(200,'',fa('chew',{m:'chompOpen'}),g=>{batt(g,108,2,2);lf(g,2);}),fr(200,'',fa('chew'),g=>batt(g,108,2,2)),fr(600,'munching',fa('chew',{dy:1}),g=>batt(g,108,2,3)),fr(1400,'full',fa('love'),g=>{batt(g,108,2,3);text(g,'FULL',104,56);})]);
  const sb=(a,b)=>g=>{card(g,52,10,72,30);textC(g,a,88,15);textC(g,b,88,27);text(g,'<',46,21,'small');text(g,'>',125,21,'small');};
  add('settings','Settings','Tilt = item · tap = change',
    'Sound, light and name. Deskimon reacts to each change: hums at sound on, goes quiet and shut-eyed at off, squints at the brightest setting. The name is what friends see.',
    [fr(900,'sound on',sm('happy',{note:1}),sb('SOUND','ON')),fr(900,'tap: off',sm('idle',{E:{lt:.45,lb:.2},m:'flat'}),sb('SOUND','OFF')),fr(120,'tilt',sm('idle'),g=>speed(g,48,14,32)),fr(900,'light 1',sm('sleepy'),sb('LIGHT','1')),fr(700,'light 2',sm('idle'),sb('LIGHT','2')),fr(900,'light 3',sm('idle',{E:{lt:.3,lb:.3}}),sb('LIGHT','3')),fr(120,'tilt',sm('idle'),g=>speed(g,48,14,32)),fr(1200,'name',sm('smug',{E:{px:0}}),sb('NAME','DESKI'))]);

  /* G · games */
  group('G','Games','Five games: two solo, three played with friends nearby. Controls are only tap and tilt. The full face plays when the game is about reactions; a 14 × 11 mini-Deskimon (same slab, crest of 3 spikes) stands in when it has to move around the screen. Results always come back to the small face so it can gloat or sulk.');
  const GM=[['meteor','METEOR','1P','excited'],['bug','SNACK','1P','hungry'],['duel','DRAW','2P','determined'],['pong','PONG','2P','curious'],['egg','HOT EGG','2-4P','scared']];
  const gf=[];
  GM.forEach(([k,lab,tag,ex],i)=>{gf.push(fr(1000,lab.toLowerCase()+' · '+tag,sm(ex,{look:[1,-.2]}),mcard(k),tc(lab,72,40),tx(tag,96,6,'small'),g=>{for(let q=0;q<5;q++){const x=60+q*6;if(q===i)area(g,()=>true,[x-1,54,x+2,57]);else area(g,()=>true,[x,55,x+1,56]);}},arrows));if(i<4)gf.push(fr(120,'tilt',sm('idle'),mcard(k,5),g=>speed(g,52,9,27)));});
  add('gamepick','Game picker','Menu > Games · tilt · tap to start',
    'Same card mechanic as the menu. The tag shows player count: 1P plays straight away, 2P and up opens the lobby and looks for friends nearby. The face previews the mood of each game.',gf);
  const ground=g=>line(g,0,62,127,62);
  const sc2=n=>tx(pad2(n),118,1,'small');
  const MS=[[64,[[30,4],[96,-6]],0],[60,[[29,14],[97,4]],0],[55,[[28,26],[98,16],[62,-4]],0],[50,[[27,38],[99,28],[61,8]],0],[47,[[26,50],[100,40],[60,20]],1],[44,[[101,52],[59,32]],2,'!'],[40,[[58,44],[84,0]],2,'!'],[40,[[57,56],[83,12]],3],[46,[[82,24]],3],[56,[[80,36],[30,-2]],3],[66,[[78,48],[31,10]],4,'hit']];
  add('meteor','Meteor dodge','Solo · tilt to run · score = meteors survived',
    'Mini-Deskimon runs along the floor while meteors fall at 1–2 px per frame and speed up every 10 points. It glances up with a bang at near misses. A hit ends in a star burst.',
    [fr(500,'3',g=>{mini(g,64,61,{});ground(g);textC(g,'3',64,18,'big');}),fr(500,'2',g=>{mini(g,64,61,{});ground(g);textC(g,'2',64,18,'big');}),fr(500,'1',g=>{mini(g,64,61,{});ground(g);textC(g,'1',64,18,'big');}),
     ...MS.map(([ax,ms,s,n])=>fr(n==='hit'?500:110,n==='!'?'near miss':n==='hit'?'hit':'',g=>{ground(g);ms.forEach(([x,y])=>meteor(g,x,y));if(n==='hit'){burst(g,ax,55);}else mini(g,ax,61,{look:ms[0][0]<ax?-1:1,bang:n==='!'});text(g,pad2(s),118,1,'small');if(s>0&&ms.some(([x,y])=>y>=50))line(g,22,61,30,61);}))]);
  add('meteorend','Meteor: game over','After a hit',
    'Spiral-eyed small face with the score. A new best gets star eyes and NEW BEST. Tap to play again, long press to leave.',
    [fr(300,'',sm('dizzy',{t:0}),tc('GAME OVER',88,6),tx('12',78,22,'big'),tc('BEST 31',88,44)),fr(300,'',sm('dizzy',{t:.33}),tc('GAME OVER',88,6),tx('12',78,22,'big'),tc('BEST 31',88,44)),fr(300,'',sm('dizzy',{t:.66}),tc('GAME OVER',88,6),tx('12',78,22,'big'),tc('BEST 31',88,44)),fr(1400,'new best variant',sm('excited'),tc('NEW BEST',88,6),tx('32',78,22,'big'),tc('TAP AGAIN',88,44))]);
  add('snack','Snack snap','Solo · tap the moment the bug appears',
    'Full face game. Deskimon stares with slit pupils and darts its eyes around. After 1.5–5 s a bug lands somewhere on screen and the eyes snap to it. Tap and it lunges and chomps. The result shows the reaction time; a tap before the bug gets a grumpy TOO SOON.',
    [fr(800,'ready',fa('determined')),fr(700,'waiting',fa('determined',{look:[-1,0]})),fr(700,'',fa('determined',{look:[1,0]})),fr(200,'bug lands',fa('determined',{look:[-1,1],bang:1}),g=>bug(g,10,56,1)),fr(150,'',fa('determined',{look:[-1,1]}),g=>bug(g,10,56,0)),fr(120,'tap: lunge',fa('rawr',{m:'chompOpen',dx:-5,look:[-1,.8],E:{pup:'slit'}}),g=>bug(g,10,56,1)),fr(150,'chomp',fa('chew',{dx:-3})),fr(400,'',fa('chew',{dy:1})),fr(1300,'result',sm('happy'),tx('214',72,18,'big'),tx('MS',104,24)),fr(1300,'too-early variant',sm('grumpy'),tc('TOO SOON',88,26))]);
  add('draw','Quick draw','2P · random GO · first tap wins',
    'Lobby finds a friend nearby. Both units count down together, then hold a stare with slit pupils and a 1 px tremble for 1.5–5 s. GO appears on both screens at the same moment. The winner gets a crown and its reaction time, the loser spins. Tapping before GO loses.',
    [fr(500,'lobby',sm('curious',{look:[1,0],dots:1}),tc('LOOKING',88,18),tc('FOR FRIENDS',88,30)),fr(500,'',sm('curious',{look:[1,0],dots:3}),tc('LOOKING',88,18),tc('FOR FRIENDS',88,30)),fr(900,'found',sm('excited'),tc('YOU VS',88,18),tc(FN,88,30)),fr(600,'3',sm('determined'),tx('3',84,25,'big')),fr(600,'2',sm('determined'),tx('2',84,25,'big')),fr(600,'1',sm('determined'),tx('1',84,25,'big')),fr(200,'stare',fa('determined')),fr(200,'',fa('determined',{dx:1})),fr(200,'',fa('determined')),fr(200,'',fa('determined',{dx:-1})),fr(500,'GO',sm('surprised',{look:[1,0]}),tc('GO!',88,18,'txt',3)),fr(1400,'win',sm('happy',{crown:1}),tc('YOU WIN',88,14),tx('182',70,28,'big'),tx('MS',102,34)),fr(1400,'lose (other unit)',sm('dizzy'),tc(FN,88,20),tc('WINS',88,32))]);
  const pad=(g,y)=>{area(g,()=>true,[2,y-6,3,y+6]);};
  const ball=(g,x,y)=>area(g,()=>true,[x-1,y-1,x+1,y+1]);
  const net=g=>{for(let y=0;y<64;y+=4)px(g,126,y);};
  const ps=(a,b)=>g=>{textC(g,a+' '+b,64,1,'small');};
  add('pong','Desk pong','2P · tilt moves paddle · ball travels between units',
    'Each unit is one half of the court. The paddle is on the left, and the dotted right edge is the friend’s side. The ball leaves your screen and arrives on theirs at the same height and speed. A miss shows a sad small face and the score.',
    [fr(500,'serve',g=>{pad(g,32);ball(g,8,32);net(g);},ps(0,0)),fr(110,'',g=>{pad(g,32);ball(g,30,26);net(g);},ps(0,0)),fr(110,'',g=>{pad(g,32);ball(g,60,18);net(g);},ps(0,0)),fr(110,'',g=>{pad(g,30);ball(g,90,10);net(g);},ps(0,0)),fr(110,'',g=>{pad(g,28);ball(g,120,6);net(g);},ps(0,0)),fr(900,'on '+FN+'’s side',g=>{pad(g,28);net(g);textC(g,FN+' >',96,28);},ps(0,0)),fr(110,'coming back',g=>{pad(g,30);ball(g,122,40);net(g);},ps(0,0)),fr(110,'tilt to meet it',g=>{pad(g,36);ball(g,92,44);net(g);},ps(0,0)),fr(110,'',g=>{pad(g,42);ball(g,62,48);net(g);},ps(0,0)),fr(110,'',g=>{pad(g,48);ball(g,32,52);net(g);},ps(0,0)),fr(150,'return',g=>{pad(g,52);ball(g,6,54);net(g);line(g,6,49,8,47);line(g,8,57,10,59);},ps(0,0)),fr(1300,'missed one (other round)',sm('sad'),tc('MISS',88,18),tx('1',76,30,'big'),tx('2',94,30,'big'))]);
  const eggTop=(o)=>fa('scared',Object.assign({s:.6,y:16,look:[0,1]},o||{}));
  add('hotegg','Hot egg','2–4P · tap to pass · whoever holds it when it hatches babysits',
    'An egg goes round a ring of friends. Each unit shows a nervous small face above the egg, which wobbles and cracks faster as time runs out. A tap slides it off to the next friend. When it hatches, a baby mini-Deskimon pops out and the holder gets heart eyes. Nobody loses.',
    [fr(200,'egg arrives',eggTop({look:[-1,1]}),g=>egg(g,24,50,0)),fr(200,'',eggTop({look:[-.5,1]}),g=>egg(g,48,50,0)),fr(400,'',eggTop(),g=>egg(g,64,50,0)),fr(150,'wobble',eggTop(),g=>egg(g,63,50,1)),fr(150,'',eggTop(),g=>egg(g,65,50,1)),fr(150,'tap: pass',eggTop({E:{brow:{a:-1,lift:2}}}),g=>egg(g,86,50,1)),fr(150,'',eggTop({look:[1,1]}),g=>{egg(g,112,50,1);text(g,'>',120,32,'small');}),fr(800,'relief',fa('happy',{s:.6,y:16}),tc('TO '+FN,64,46)),fr(200,'back again, cracking',eggTop({sweat:1}),g=>egg(g,64,50,2)),fr(150,'',eggTop({sweat:1}),g=>egg(g,63,50,3)),fr(150,'',eggTop({sweat:1}),g=>egg(g,65,50,3)),fr(300,'hatch',eggTop({look:[0,1],bang:1}),g=>eggHatch(g,64,52,1)),fr(1500,'baby!',fa('love',{s:.6,y:16}),g=>eggHatch(g,64,56,2))]);

  /* F · friends */
  group('F','Friends and messages','Units find each other directly with no network or phone, within a room or two. Friends are paired once by bumping two units together. After that they can see each other nearby, send fixed messages, tap rhythms, hearts and moods, and play the 2P games. The friend’s name comes from their settings (here '+FN+').');
  const bt=(o)=>fa('idle',Object.assign({s:.6,y:20},o||{}));
  add('pair','Pair by bump','Menu > Friends > Pair · knock two units together',
    'Both units show BUMP TO PAIR and look around. A knock seen by both within 150 ms squashes the face with impact lines, and the units swap names. The friend’s name appears with a heart.',
    [fr(600,'waiting',bt({look:[-1,0]}),tc('BUMP TO PAIR',64,52)),fr(600,'',bt({look:[1,0]}),tc('BUMP TO PAIR',64,52)),fr(160,'bump',bt({E:{w:34,h:22},dy:2,m:'o',bang:1}),g=>{line(g,8,14,14,18);line(g,6,22,13,22);line(g,8,30,14,26);line(g,120,14,114,18);line(g,122,22,115,22);line(g,120,30,114,26);},tc('BUMP!',64,52)),fr(400,'finding',bt({look:[0,-.3]}),tc('. . .',64,52)),fr(400,'',bt({look:[0,-.3]}),tc('. . . .',64,52)),fr(1300,'paired',fa('love',{s:.6,y:20}),tc(FN+' ♥',64,52)),fr(700,'saved',fa('happy',{s:.6,y:20}),tc('SAVED',64,52))]);
  add('nearby','Friend nearby','A paired friend comes into range',
    'Deskimon perks up mid-idle, brows lift and eyes dart right as if it heard someone. Then star eyes, the small face with the name, and a wink. Comes with a two-note chirp. It only fires once per hour per friend.',
    [fr(900,'idle',I()),fr(300,'perks up',I({E:{brow:{a:-1,lift:3}},look:[1,-.2]})),fr(500,'',fa('excited')),fr(1300,'',sm('happy'),tc(FN,88,20),tc('IS HERE',88,32)),fr(700,'wink',sm('wink'),tc(FN,88,20),tc('IS HERE',88,32))]);
  const FL=[[FN,'♥'],['MOMO','z'],['REX','!'],['ALL','']];
  const flist=i=>g=>{FL.forEach(([n,m],k)=>{const y=3+k*15;if(k===i)text(g,'>',4,y,'small');text(g,n,12,y);if(m)text(g,m,118,y,'small');});line(g,0,61,127,61);};
  add('friends','Friend list','Menu > Friends · tilt to move · tap to open',
    'Up to 8 friends, 4 rows per page. The right column shows each friend’s last shared mood: heart (happy), z (asleep), bang (busy). ALL sends to everyone in range.',
    [fr(1000,FN.toLowerCase(),flist(0)),fr(700,'tilt',flist(1)),fr(700,'',flist(2)),fr(700,'send to all',flist(3))]);
  const MSG=[['HI!','happy'],['BOOP','squeeze'],['COFFEE?','excited'],['LUNCH?','hungry'],['MISS U','love'],['GOOD JOB','smug'],['GN','sleepy'],['RAWR','rawr']];
  const mb=(m,dy=0)=>g=>{card(g,52,14+dy,72,22);textC(g,m,88,21+dy);text(g,'<',46,21,'small');text(g,'>',125,21,'small');};
  const cf=[];MSG.forEach(([m,ex],i)=>{cf.push(fr(900,m.toLowerCase(),sm(ex,ex==='smug'?{E:{px:0}}:{}),tc('TO '+FN,88,2),mb(m)));if(i<7)cf.push(fr(120,'tilt',sm('idle'),tc('TO '+FN,88,2),mb(MSG[i+1][0],-2),g=>speed(g,46,16,34)));});
  add('compose','Compose','Friend > Message · tilt through 8 phrases · tap to send',
    'Eight fixed phrases, each with its own face so you can read the message from Deskimon alone: HI, BOOP, COFFEE?, LUNCH?, MISS U, GOOD JOB, GN and RAWR. The phrases are fixed in firmware, so a packet is one byte.',cf);
  add('send','Send','Tap on a phrase',
    'The phrase folds into an envelope that flies up off the screen with speed lines. The eyes follow it up, then a grin and SENT.',
    [fr(300,'folds',sm('idle',{look:[1,0]}),ic('mail',80,24)),fr(120,'flies',sm('idle',{look:[1,-.6]}),ic('mail',80,12),g=>{line(g,82,32,82,36);line(g,93,32,93,36);}),fr(120,'',sm('idle',{look:[1,-1]}),ic('mail',80,-2),g=>{line(g,82,18,82,24);line(g,93,18,93,24);}),fr(200,'',sm('idle',{look:[.5,-1]})),fr(1000,'sent',sm('happy'),tc('SENT',88,26))]);
  add('receive','Receive','Message arrives · two-note ping',
    'An envelope drops into the right margin and bounces. Deskimon’s eyes snap up, then down to it, with a bang. It wiggles until tapped. If ignored, it stays as a small envelope in the corner of the clock screen.',
    [fr(600,'idle',I()),fr(150,'ping',I({note:1,E:{brow:{a:-1,lift:3}},look:[1,-.8]})),fr(110,'drops',I({look:[1,-.4]}),ic('mail',110,8)),fr(110,'',I({look:[1,.4]}),ic('mail',110,30)),fr(110,'',I({look:[1,.8]}),ic('mail',110,44)),fr(110,'bounce',I({look:[1,.8]}),ic('mail',110,40)),fr(500,'',fa('curious',{look:[1,.8],bang:1}),ic('mail',110,44)),fr(250,'wiggle',fa('curious',{look:[1,.8]}),ic('mail',111,44)),fr(250,'',fa('curious',{look:[1,.8]}),ic('mail',109,44))]);
  const rd=m=>g=>{text(g,FN+':',2,2);textC(g,m,64,24,'txt',2);};
  add('read','Read and react','Tap the envelope',
    'The sender name, then the phrase at 2× for 1.5 s. Deskimon then reacts with that phrase’s face: BOOP squeezes its eyes shut and grins, COFFEE? gets star eyes, MISS U gets heart eyes. Hold to reply with a heart.',
    [fr(1500,'boop',rd('BOOP!')),fr(1200,'reacts',fa('squeeze',{E:{w:30,h:26},dy:1})),fr(600,'',fa('happy')),fr(1500,'coffee',rd('COFFEE?')),fr(1200,'reacts',fa('excited')),fr(1500,'miss u',rd('MISS U')),fr(1400,'reacts',fa('love'))]);
  const hrt=(g,x,y,k)=>{for(let r=0;r<6;r++)for(let c=0;c<7;c++)if(HEART7[r][c]==='#')area(g,()=>true,[x+c*k,y+r*k,x+c*k+k-1,y+r*k+k-1]);};
  add('heart','Heart reply','Hold 1 s after reading',
    'While you hold, a heart grows beside the face in three steps (1×, 2×, 3×). On release it flies off to the right, and Deskimon blushes.',
    [fr(300,'hold',sm('love'),g=>hrt(g,82,28,1)),fr(300,'',sm('love'),g=>hrt(g,79,25,2)),fr(400,'',sm('love'),g=>hrt(g,77,22,3)),fr(110,'release',sm('love',{look:[1,0]}),g=>{hrt(g,98,22,3);speed(g,88,24,36);}),fr(110,'',sm('love',{look:[1,0]}),g=>hrt(g,118,22,3)),fr(1100,'sent',sm('shy'),tc('♥ SENT',88,26))]);
  const tapsD=(g,n)=>{const P=[0,0,1,0,1];let x=66;for(let k=0;k<n;k++){if(P[k]){area(g,()=>true,[x,44,x+7,46]);x+=12;}else{area(g,ell(x+1.5,45.5,1.8,1.8),[x-1,43,x+4,48]);x+=7;}}};
  add('taptalk','Tap talk','Friend > Tap talk · tap a rhythm · 2 s silence sends',
    'Record a rhythm on the crest: short taps become dots, holds become dashes, up to 12. Deskimon nods on every tap. On the friend’s unit the full face blinks the rhythm back while the buzzer beeps it.',
    [fr(700,'record',sm('curious'),tc('TAP TALK',88,18)),fr(200,'tap',sm('idle',{E:SHUT}),tc('TAP TALK',88,18),g=>tapsD(g,1)),fr(250,'',sm('idle'),tc('TAP TALK',88,18),g=>tapsD(g,1)),fr(200,'tap',sm('idle',{E:SHUT}),tc('TAP TALK',88,18),g=>tapsD(g,2)),fr(400,'hold',sm('idle',{E:{lt:.3,lb:.2}}),tc('TAP TALK',88,18),g=>tapsD(g,3)),fr(200,'tap',sm('idle',{E:SHUT}),tc('TAP TALK',88,18),g=>tapsD(g,4)),fr(400,'hold',sm('idle',{E:{lt:.3,lb:.2}}),tc('TAP TALK',88,18),g=>tapsD(g,5)),fr(900,'sent',sm('happy'),tc('SENT',88,18),g=>tapsD(g,5)),
     fr(600,'on '+FN+'’s unit',I({note:1})),fr(120,'dot',I({E:SHUT,note:1})),fr(200,'',I()),fr(120,'dot',I({E:SHUT,note:1})),fr(200,'',I()),fr(400,'dash',I({E:SHUT,note:1})),fr(200,'',I()),fr(120,'dot',I({E:SHUT,note:1})),fr(200,'',I()),fr(400,'dash',I({E:SHUT,note:1})),fr(800,'',fa('happy'))]);
  add('yawn','Contagious yawn','A nearby friend shares a mood change',
    'Units quietly share their mood every few minutes. When a friend falls asleep, Deskimon hears it, glances over, yawns too and gets drowsy. Happy and RAWR moods spread the same way, at most once per 10 min.',
    [fr(900,'',I()),fr(1100,'friend yawned',sm('curious'),tc(FN,88,18),tc('YAWNED',88,30)),fr(400,'',I({look:[1,0]})),fr(1100,'yawns too',fa('yawn')),fr(1200,'',fa('sleepy'))]);
  add('party','RAWR party','Send RAWR to ALL',
    'Every friend in range roars at the same moment. The sender sees the count of friends who joined, then everyone shakes with steam and the units growl together, then everyone giggles.',
    [fr(900,'',sm('excited'),tc('RAWR PARTY',88,18),tc('3 FRIENDS',88,32)),fr(300,'',fa('determined',{dy:1})),fr(140,'RAWR',fa('rawr',{dx:2,steam:1,bang:2})),fr(140,'',fa('rawr',{dx:-2,steam:1,bang:2})),fr(140,'',fa('rawr',{dx:2,steam:1,bang:2})),fr(500,'',fa('rawr',{bang:2})),fr(400,'giggle',fa('squeeze')),fr(400,'',fa('happy',{dy:-1}))]);
  return groups;
}
if(typeof module!=="undefined")module.exports={build,ICON,W,H,BIG,SMALL,TXT};
