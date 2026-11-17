/* ---------- face rig: wedge (new-eyes sheet, page D) ----------
   Eyes and a small mouth only. No pupils, no brows, no nose.
   Flat-cut wedge eyes: the top edge tilts to set the mood.
   Hand-authored source: spliced into rig_wedge.js by tools/build_wedge_sheet.js. */
const LEGACY_W=18/28,LEGACY_H=14/30;           /* old slab size -> wedge size, for scene overrides */
function lidWedge(lt,sl){                       /* top-edge drop (fraction of h): [outer,inner] */
  let o=lt-sl/2,i=lt+sl/2;const m=Math.min(o,i);if(m<0){o-=m;i-=m;}
  return [Math.min(o,.95),Math.min(i,.95)];
}
function eye(g,cx,cy,e,side,t){
  const s=e.s||1;let h0=e.h;
  let lt=e.lt||0,lb=e.lb||0,sl=e.sl||0;
  if(e.brow){const b=e.brow;if((b.lift||0)>0)h0+=b.lift*.8;if((b.a||0)>=2&&!sl&&!lt){lt=.15;sl=.5;}}
  const w=e.w*s,h=h0*s,hw=w/2,hh=h/2,y0=cy-hh;
  let shape=e.shape||'slab';
  if(shape==='slab'){if(e.pup==='heart')shape='heart';else if(e.pup==='star')shape='star';else if(e.pup==='spiral')shape='spiral';}
  if(shape==='slab'&&lt+lb>=.9)shape='shut';
  const th=s>=.8?3:2,th2=s>=.8?2:1;
  if(shape==='shut'){const yy=Math.round(y0+h*(lt/(lt+lb))*.85+h*.08);area(g,()=>true,[cx-hw*.95,yy,cx+hw*.95,yy+th2-1]);}
  else if(shape==='arcDown')stroke(g,curve(cx,cy-1*s,hw*1.05,u=>4*s*(1-u*u)),th2);
  else if(shape==='arcUp')stroke(g,[[cx-hw*1.1,cy+4*s],[cx,cy-4*s],[cx+hw*1.1,cy+4*s]],th);
  else if(shape==='chev'){const ox=cx+side*hw*.55,ix=cx-side*hw*.45;stroke(g,[[ox,cy-hh*.45],[ix,cy],[ox,cy+hh*.45]],th);}
  else if(shape==='x'){stroke(g,[[cx-hw*.5,cy-hh*.45],[cx+hw*.5,cy+hh*.45]],th);stroke(g,[[cx+hw*.5,cy-hh*.45],[cx-hw*.5,cy+hh*.45]],th);}
  else if(shape==='heart'){const k=3*s,x0=cx-3.5*k,yy=cy-3*k;area(g,(X,Y)=>{const c=Math.floor((X-x0)/k),r=Math.floor((Y-yy)/k);return c>=0&&c<7&&r>=0&&r<6&&HEART7[r][c]==='#';},[x0-1,yy-1,x0+7*k+1,yy+6*k+1]);}
  else if(shape==='star'){const a=8.5*s,b=10*s;area(g,(X,Y)=>(Math.abs(X-cx)/a)**.6+(Math.abs(Y-cy)/b)**.6<=1,[cx-a-1,cy-b-1,cx+a+1,cy+b+1]);}
  else if(shape==='spiral'){const R=10*s,turns=2.1,N=38,ph=(t||0)*2*Math.PI,pts=[];for(let k=0;k<=N;k++){const q=k/N*turns*2*Math.PI,r=R*q/(turns*2*Math.PI);pts.push([cx+r*Math.cos(q+ph),cy+r*Math.sin(q+ph)]);}stroke(g,pts,th2);}
  else{
    const [od,id]=lidWedge(lt,sl),ch=(od===0&&id===0)?Math.max(1,Math.round(2*s)):0,bot=y0+h-lb*h;
    area(g,(X,Y)=>{
      const u=(X-cx)/hw;if(Math.abs(u)>1||Y<y0||Y>y0+h)return false;
      const inner=-side*u,f=od+(id-od)*(inner+1)/2;
      if(Y<y0+f*h||Y>bot)return false;
      if(ch){const dxo=side<0?X-(cx-hw):(cx+hw)-X;if(dxo+(Y-y0)<ch+.5)return false;}
      return true;
    },[cx-hw-1,cy-hh-1,cx+hw+1,cy+hh+1]);
  }
}
function mouth(g,type,mx,my,s,f){
  const hw=5*s*(f.mw||1),th=1;
  const C=(fn,w=1)=>stroke(g,curve(mx,my,hw*w,fn),th);
  const box=(rx,ry)=>{edge(g,rr(mx,my+ry,rx,ry,1.2*s),[mx-rx-1,my-1,mx+rx+1,my+2*ry+1]);};
  switch(type){
    case 'line':C(u=>1.6*s*(1-u*u));break;
    case 'smile':C(u=>2.4*s*(1-u*u),1.2);break;
    case 'flat':C(()=>1,.8);break;
    case 'frown':C(u=>1.8*s-2.2*s*(1-u*u),.9);break;
    case 'grin':{const w=hw*1.1,dep=4.5*s;area(g,(X,Y)=>{const u=Math.abs((X-mx)/w);return u<=1&&Y>=my&&Y<=my+dep*(1-u)+.4;},[mx-w-1,my-1,mx+w+1,my+dep+2]);break;}
    case 'rawr':{const rx=hw*1.35,ry=3*s+.5;box(rx,ry);const tx=Math.round(rx*.5);for(const sd of[-1,1]){px(g,mx+sd*tx,my+1);if(s>=.9)px(g,mx+sd*tx,my+2);}break;}
    case 'chompOpen':{const rx=hw*1.3,ry=3*s+.5;box(rx,ry);const tx=Math.round(rx*.5),by=Math.round(my+2*ry-1);for(const sd of[-1,1]){px(g,mx+sd*tx,my+1);px(g,mx+sd*Math.round(tx*.3),by-1);}break;}
    case 'o':{const a=Math.max(2.4,2.8*s),b=Math.max(2.2,2.5*s);ring(g,ell(mx,my+2*s,a,b),bE(mx,my+2*s,a,b));break;}
    case 'oSmall':{const a=Math.max(2.2,2.4*s);ring(g,ell(mx,my+1,a,a),bE(mx,my+1,a,a));break;}
    case 'yawn':{const a=Math.max(2.6,3.8*s),b=Math.max(3.2,5.4*s);ring(g,ell(mx,my+3*s,a,b),bE(mx,my+3*s,a,b));break;}
    case 'grr':{const pts=[],n=6;for(let i=0;i<=n;i++)pts.push([mx-hw*.9+i*(hw*1.8/n),my+(i%2?1.6*s:0)]);stroke(g,pts,th);break;}
    case 'wobble':C(u=>1.1*s*Math.sin(u*Math.PI*3)+1,1.25);break;
    case 'smirk':C(u=>u>0?1-2*s*u*u:1,.9);break;
    case 'tongue':{const fn=u=>2*s*(1-u*u);C(fn,1.05);const tx=Math.round(mx+hw*.3),ty=Math.round(my+fn(.3)+1);area(g,()=>true,[tx-1,ty,tx+1,ty+Math.max(1,Math.round(2*s))]);break;}
    case 'chomp':{const pts=[],n=8;for(let i=0;i<=n;i++)pts.push([mx-hw*.95+i*(hw*1.9/n),my+(i%2?2*s:0)]);stroke(g,pts,th);break;}
    case 'drool':{C(()=>1,.7);const x=mx+hw*.5;for(let k=2;k<5;k++)px(g,x,my+k*s+1);dot(g,x,my+6*s+1,1);break;}
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
const FACE_K=1.25;  /* wedge faces are drawn 25 % bigger than page D (owner request 2026-11-12); firmware: FACE_K in face.cpp */
function face(g,f){
  const s=(f.s??1)*FACE_K,cx=(f.x??64)+(f.dx||0),cy=(f.y??27)+(f.dy||0),sp=(f.sp??16)*s,t=f.t||0;
  const lk=f.look||[0,0],gx=lk[0]*3*s,gy=lk[1]*2*s,E=[];
  for(const side of[-1,1]){
    const e=Object.assign({},side<0?f.L:f.R);e.s=s;
    const ex=cx+side*sp+gx,ey=cy+gy;
    eye(g,ex,ey,e,side,t);E.push({ex,ey,hw:e.w*s/2,hh:e.h*s/2});
    if(f.cheeks){const y=ey+e.h*s/2+3*s;for(const k of[-1,0,1]){const x=ex+k*5*s;line(g,x-1,y+1,x+1,y-1);}}
  }
  if(f.glasses){E.forEach((q,i)=>{const hw=q.hw+3*s,hh=q.hh+2.5*s,sd=i?1:-1;edge(g,rr(q.ex,q.ey,hw,hh,5*s),[q.ex-hw-1,q.ey-hh-1,q.ex+hw+1,q.ey+hh+1]);line(g,q.ex+sd*hw,q.ey-hh*.3,q.ex+sd*(hw+5*s),q.ey-hh*.45);});const y=E[0].ey-E[0].hh*.35;line(g,E[0].ex+E[0].hw+3*s,y,E[1].ex-E[1].hw-3*s,y);}
  if(f.m!=='none')mouth(g,f.m||'line',cx+gx*.6+(f.mx||0)*s,cy+(f.my??15)*s+gy*.5,s,f);
  fx(g,f,cx,cy,s,E[1].ex+E[1].hw+4*s,E);
}
const BL={w:18,h:14},BR={w:18,h:14};
const EX={
  idle:{m:'line'},
  happy:{E:{shape:'arcUp'},m:'grin'},
  love:{E:{pup:'heart'},m:'wobble',mw:.7,heart:1},
  curious:{Lx:{w:12,h:19},Rx:{w:18,h:7},m:'oSmall',my:16},
  sleepy:{E:{h:4},m:'flat',mw:.6,y:28,my:13},
  asleep:{E:{shape:'arcDown'},m:'oSmall',y:29,my:14,z:1},
  surprised:{E:{w:12,h:19},m:'oSmall',my:17},
  grumpy:{E:{h:12,lt:.35,sl:.8},m:'flat',mw:.8,my:13,steam:1},
  rawr:{E:{w:20,h:17,lt:.25,sl:.9},m:'rawr',my:17},
  dizzy:{E:{pup:'spiral'},m:'wobble',mw:1.1,my:17},
  shy:{E:{w:12,h:9},look:[-1,.7],m:'wobble',mw:.9,my:11},
  unimpressed:{E:{h:12,lt:.45},m:'flat'},
  mischief:{E:{lt:.3,sl:.6},m:'line'},
  sad:{E:{lt:.1,sl:-.6},m:'frown',my:15},
  focus:{E:{h:10,lt:.3},glasses:1,m:'flat'},
  excited:{E:{pup:'star'},m:'grin',my:17},
  hungry:{E:{lt:.35,sl:-.2},m:'drool'},
  chew:{E:{shape:'arcUp'},m:'chomp'},
  smug:{E:{h:8,lt:.05},m:'flat',mw:.9},
  scared:{E:{w:12,h:19},m:'wobble',mw:1,my:17,sweat:1},
  determined:{E:{lt:.25,sl:.5},m:'flat'},
  squeeze:{E:{shape:'chev'},m:'grin'},
  wink:{Lx:{},Rx:{shape:'arcUp'},m:'grin',mw:.7},
  yawn:{E:{shape:'chev'},m:'yawn',my:13}
};
/* scene overrides were written for the old 28 x 30 slab: scale their w/h to the wedge size */
const scaleWH=q=>{if(!q)return q;const o=Object.assign({},q);if(o.w!=null)o.w=Math.round(o.w*LEGACY_W);if(o.h!=null)o.h=Math.round(o.h*LEGACY_H);return o;};
function X(n,o){
  const f={L:Object.assign({},BL),R:Object.assign({},BR)};
  const ap=(q,sc)=>{for(const k in q){if(k==='E'){const v=sc?scaleWH(q.E):q.E;Object.assign(f.L,v);Object.assign(f.R,v);}else if(k==='Lx')Object.assign(f.L,sc?scaleWH(q.Lx):q.Lx);else if(k==='Rx')Object.assign(f.R,sc?scaleWH(q.Rx):q.Rx);else f[k]=q[k];}};
  ap(EX[n]||EX.idle,false);ap(o||{},true);return f;
}

