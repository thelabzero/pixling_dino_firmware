#!/usr/bin/env node
/* Builds the wedge-eye dino sheet (new-eyes page D) from the v1 rig.
   Reads   art/reference/dino-v1/rig.js            (never edited)
           art/reference/dino-wedge-v1/wedge_face.js (the new face, hand-authored)
           art/reference/dino-wedge-v1/sheet.template.html
   Writes  art/reference/dino-wedge-v1/rig_wedge.js
           art/reference/dino-wedge-v1/Dino Wedge Sheet.html  (self-contained)
   Run: node tools/build_wedge_sheet.js */
const fs=require('fs'),path=require('path');
const root=path.join(__dirname,'..'),v1=path.join(root,'art/reference/dino-v1'),out=path.join(root,'art/reference/dino-wedge-v1');
let rig=fs.readFileSync(path.join(v1,'rig.js'),'utf8');
const face=fs.readFileSync(path.join(out,'wedge_face.js'),'utf8');

const must=(cond,msg)=>{if(!cond){console.error('build_wedge_sheet: '+msg);process.exit(1);}};
const cut=(src,a,b)=>{const i=src.indexOf(a),j=src.indexOf(b);must(i>=0&&j>i,'marker not found: '+a+' / '+b);return[src.slice(0,i),src.slice(j)];};

/* 1. swap the face rig */
{const[head,tail]=cut(rig,'/* ---------- face rig ---------- */','/* ---------- icons + props ---------- */');rig=head+face+tail;}

/* 2. rewrite the R group parts that drew the old eye/mouth directly (R1 to R3) */
const R123=`  /* R · rig */
  group('R','Face rig','The parts every screen is built from. Eyes are flat-cut wedges: a slab whose top edge tilts to set the mood, with no pupil and no brow. A small one-pixel mouth sits 8 px under the eyes, and there is no nose. Hearts, stars, spirals and the happy arch are whole-eye shapes. All of it is parametric, so the same eye draws at full size and at 0.55.');
  const cellE=(o,x,y,side)=>g=>eye(g,x,y,Object.assign({w:18,h:14,s:.5},o),side||-1,.1);
  const cellP=(oL,oR,x,y)=>g=>{eye(g,x-8,y,Object.assign({w:18,h:14,s:.7},oL),-1,.1);eye(g,x+8,y,Object.assign({w:18,h:14,s:.7},oR||oL),1,.1);};
  add('rig_eyes','Eye parts','Wedge cuts (top) · whole-eye shapes (bottom), shown at 0.5',
    'Top row, slab cuts: flat idle, mischief (top edge slants in), grumpy (shorter, steeper), sad (slants out), sleepy bar, smug half lid, tall (surprised), and wide-short (the small eye of curious). Bottom row, whole-eye shapes: happy arch, sleeping smile, squeeze chevron, blink (a flat bar), heart, four-point star, spiral (dizzy, rotates) and cross.',
    [fr(0,'cuts · shapes',...[{},{lt:.3,sl:.6},{h:12,lt:.35,sl:.8},{lt:.1,sl:-.6},{h:4},{h:8,lt:.05},{w:12,h:19},{w:18,h:7}].map((o,i)=>cellE(o,8+i*16,15)),...[{shape:'arcUp'},{shape:'arcDown'},{shape:'chev'},SHUT,{pup:'heart'},{pup:'star'},{pup:'spiral'},{shape:'x'}].map((o,i)=>cellE(o,8+i*16,47)))]);
  add('rig_brows','Lid cuts','Top edge slant (sl) · lid height (lt) · bottom lid (lb)',
    'The whole mood sits in the top edge. Row 1: flat, mischief (lt .3, sl .6), rawr (lt .25, sl .9, bigger eye), grumpy (shorter, sl .8). Row 2: sad (sl −0.6), curious (one tall eye, one short), sleepy bar, smug. The outer top corner of a flat eye is chamfered by 2 px; slanted eyes are not.',
    [fr(0,'slants · heights',...[[{}],[{lt:.3,sl:.6}],[{w:20,h:17,lt:.25,sl:.9}],[{h:12,lt:.35,sl:.8}]].map((p,i)=>cellP(p[0],p[1],16+i*32,17)),...[[{lt:.1,sl:-.6}],[{w:12,h:19},{w:18,h:7}],[{h:4}],[{h:8,lt:.05}]].map((p,i)=>cellP(p[0],p[1],16+i*32,46)))]);
  const MT=['line','smile','flat','frown','grin','rawr','o','oSmall','yawn','grr','wobble','smirk','tongue','chomp','chompOpen','drool'];
  add('rig_mouths','Mouths','16 shapes · 1 px stroke, a filled wedge for the grin',
    'Row 1: soft curve (default), wide smile, flat, frown. Row 2: filled grin, RAWR (boxed, two teeth), small o, tiny o (asleep). Row 3: yawn, grumble zigzag, wobble (nervous), smirk. Row 4: tongue out, chomp closed, chomp open (teeth top and bottom), drool. Shown at 0.75.',
    [fr(0,MT.join(' · '),...MT.map((t,i)=>g=>mouth(g,t,16+(i%4)*32,3+Math.floor(i/4)*16,.75,{})))]);
`;
{const[head,rest]=cut(rig,'  /* R · rig */',"  add('rig_type'");rig=head+R123+rest;}

/* 3. R6 and R7 descriptions */
const rep=(a,b)=>{must(rig.includes(a),'text not found: '+a.slice(0,60));rig=rig.split(a).join(b);};
rep('such as lids, pupil type, brow, mouth and extras.','such as lid cut, whole-eye shape, mouth and extras.');
rep('The same idle face at 0.7, 0.55 and 0.4. At 0.55 the pupil hole is still 5 px and the glint survives. At 0.4 the pupil drops to 4 px with no glint, and brows merge into the lid, so 0.4 is never used. Below 0.75 the nostrils switch off.',
    'The same idle face at 0.7, 0.55 and 0.4. At 0.55 the eyes are 10 × 8 and the top-edge cut still reads. At 0.4 the eyes are 7 × 6, a slant is only one or two pixels, and the mouth is a single dash, so 0.4 is never used.');

/* 4. wording that referred to removed parts */
const swaps=[
 ['Deskimon','Dino'],['double tap: brows pop','double tap: eyes pop'],['raised brows','taller eyes'],['the brows flick up','the eyes flick taller'],
 ['a brow twitches','an eye twitches'],["'brow twitch'","'eye twitch'"],['heart pupils','heart eyes'],['slit pupils','a hard slant'],['slit-eyed','hard-eyed'],
 ['with tiny pupils','with tall eyes'],['both fangs','a boxed mouth'],['with the pupils pinned downhill','with the eyes pinned downhill']
];
swaps.forEach(([a,b])=>{rig=rig.split(a).join(b);});
rig=rig.replace(/\/\* ---------- face rig ---------- \*\/[\s\S]{0,0}/,'');

fs.writeFileSync(path.join(out,'rig_wedge.js'),'/* dino wedge rig: generated by tools/build_wedge_sheet.js from dino-v1/rig.js + wedge_face.js. Do not hand-edit. */\n'+rig);

/* 5. the sheet */
const tpl=fs.readFileSync(path.join(out,'sheet.template.html'),'utf8');
must(tpl.includes('/*__RIG__*/'),'template has no rig slot');
const inline=rig.replace(/if\(typeof module!=="undefined"\)module\.exports=[^\n]*\n?/,'');
fs.writeFileSync(path.join(out,'Dino Wedge Sheet.html'),tpl.replace('/*__RIG__*/',()=>inline));
const probe=require(path.join(out,'rig_wedge.js'));
const G=probe.build('10:24','SAM');
console.log('built: '+G.map(x=>x.gid+' '+x.scenes.length+' clips').join(', ')+' · '+G.reduce((a,x)=>a+x.scenes.reduce((b,s)=>b+s.frames.length,0),0)+' frames');
