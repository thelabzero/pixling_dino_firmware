// Exports the dino design's frames: node tools/export_dino_reference.js [time] [friend]
// Writes art/reference/dino-v1/dino_frames.bin (1 byte per pixel, 8192 B per frame, in clip order)
// and dino_timing.json (per clip and frame: id, key, name, trigger, hold ms, note). Deterministic.
const fs=require('fs'),path=require('path');
const dir=path.join(__dirname,'..','art','reference','dino-v1');
const {build,W,H}=require(path.join(dir,'rig.js'));
const groups=build(process.argv[2]||'10:24',process.argv[3]||'SAM');
const bufs=[],clips=[];let n=0;
for(const G of groups)for(const s of G.scenes){
  const fr=s.frames.map(f=>{bufs.push(Buffer.from(f[0]));return{index:n++,ms:f[3]||f[1]||1000,note:f[2]||''};});
  clips.push({id:s.id,key:s.key,group:G.gid,name:s.name,trigger:s.trigger,frames:fr});
}
fs.writeFileSync(path.join(dir,'dino_frames.bin'),Buffer.concat(bufs));
fs.writeFileSync(path.join(dir,'dino_timing.json'),JSON.stringify({width:W,height:H,time:process.argv[2]||'10:24',friend:process.argv[3]||'SAM',frames:n,clips},null,1));
console.log(clips.length+' clips, '+n+' frames');
