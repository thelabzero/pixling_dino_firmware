// Desktop test of imu_calibrator.html: runs the page's own script in node with a fake DOM and a simulated MPU6050
// (known offset / scale / bias / mounting), drives every calibration step through the page's real handlers, then checks
//   1. the page recovers the simulated errors,
//   2. the generated block has exactly the layout of firmware/include/config.h section 4 (same names, same line widths),
//   3. the block compiles as C++ and the values read back.
// Run from the repo root:  node tools/imu_calibrator/test_page.js
const fs = require('fs'), path = require('path'), cp = require('child_process'), os = require('os');
const root = path.join(__dirname, '..', '..');
const html = fs.readFileSync(path.join(__dirname, 'imu_calibrator.html'), 'utf8');
let src = html.slice(html.indexOf('<script>') + 8, html.lastIndexOf('</script>'));
src = src.replace(/\}\)\(\);\s*$/, 'globalThis.__t = { handleLine };\n})();');

// ---- fake DOM: every element is a stub that remembers what the page sets (onclick, textContent...)
const els = {};
const stub = () => { const t = function () {}; const px = new Proxy(t, {
  get: (t, p) => (p in t ? t[p] : p === 'classList' ? { toggle() {}, add() {}, remove() {} } : p === 'style' ? (t.style = t.style || {}) :
    p === 'value' ? '100' : p === 'textContent' ? '' : stub()),
  set: (t, p, v) => { t[p] = v; if (p === 'id') els[v] = t.__self; return true; },
  apply: () => stub(),
}); t.__self = px; return px; };
globalThis.document = { getElementById: id => (els[id] = els[id] || stub()), documentElement: stub(), createElement: () => stub(), body: stub() };
globalThis.getComputedStyle = () => ({ getPropertyValue: () => '' });
Object.defineProperty(globalThis, 'navigator', { value: {}, configurable: true });
globalThis.window = globalThis;
let clock = 0;
globalThis.performance = { now: () => clock };
globalThis.requestAnimationFrame = () => 0;
globalThis.TextEncoderStream = globalThis.TextDecoderStream = function () {};
(0, eval)(src);
const { handleLine } = globalThis.__t;

let fail = 0;
const ok = (c, m) => { if (!c) { console.log('FAIL: ' + m); fail++; } };

// ---- the simulated sensor (these are the numbers the page must find)
const TRUE = {
  accOffset: [0.0319, -0.0030, 0.2957], accScale: [0.9896, 0.9902, 1.0087], gyroBias: [-1.4967, 2.8881, -6.4404],
  up: [0.0061, 0.0242, 0.9997], front: [0.0941, -0.9953, 0.0235],
};
const unit = v => { const n = Math.hypot(...v); return v.map(x => x / n); };
const noise = () => (Math.random() - 0.5) * 0.002;
function feed(gravity, ms) {  // gravity = true acceleration in g (sensor axes); streams like the firmware: 100 Hz
  for (let t = 0; t < ms + 150; t += 10) {
    clock += 10;
    const a = gravity.map((v, i) => Math.round((v * TRUE.accScale[i] + TRUE.accOffset[i] + noise()) * 8192));
    const g = TRUE.gyroBias.map(v => Math.round((v + (Math.random() - 0.5) * 0.1) * 65.5));
    handleLine(`D,${Math.round(clock)},${a.join(',')},${g.join(',')},3150`);
  }
}
const click = id => els[id].onclick();
handleLine('I,0x68,0x68,8192.0,65.5,100');

feed([0, 0, 1], 300);  // the page needs live data before it accepts a capture
const text = id => String(els[id].textContent || '');
// guidance: before any capture the banner asks for the gyro step, and the step-2 hint names the face the board rests on
feed([0, 0, 1], 400);
ok(/step 1/.test(text('nextStep')), 'banner should ask for step 1, says: ' + text('nextStep'));
ok(/\+Z up/.test(text('sixHint')) && /press/.test(text('sixHint')), 'hint should name +Z up, says: ' + text('sixHint'));
feed([-1, 0, 0], 800);  // the page waits for the board to be still for 0.5 s before it names a face
ok(/-X up/i.test(text('sixHint')), 'hint should follow the board to -X up, says: ' + text('sixHint'));
feed([0.6, 0.6, 0.53], 800);
ok(/Not square/.test(text('sixHint')), 'hint should say "not square" for a leaning board, says: ' + text('sixHint'));
feed([0, 0, 1], 300);
click('btnGyro'); feed([0, 0, 1], 3000);
ok(/gyro ✓/.test(text('tr1')) && /step 2/.test(text('nextStep')), 'after the gyro the banner should ask for step 2, says: ' + text('nextStep'));
for (const [k, g] of [['+x', [1, 0, 0]], ['-x', [-1, 0, 0]], ['+y', [0, 1, 0]], ['-y', [0, -1, 0]], ['+z', [0, 0, 1]], ['-z', [0, 0, -1]]]) {
  click('six' + k); feed(g, 2000);
}
click('btnUp'); feed(unit(TRUE.up), 1500);
// face-down: the screen points at the table, so gravity reads along -front
click('btnFront'); feed(unit(TRUE.front).map(v => -v), 1500);

ok(/ready/.test(text('tr4')) && /All done/.test(text('nextStep')), 'banner/tracker should say ready, says: ' + text('nextStep') + ' | ' + text('tr4'));
const block = window.__imuCode;
console.log(block);

const num = n => { const m = block.match(new RegExp('#define ' + n + '\\s+(-?[0-9.]+)f')); return m ? parseFloat(m[1]) : NaN; };
const near = (name, want, tol) => ok(Math.abs(num(name) - want) <= tol, `${name} = ${num(name)}, simulated ${want}`);
['X', 'Y', 'Z'].forEach((c, i) => {
  near('IMU_ACC_OFFSET_' + c, TRUE.accOffset[i], 0.003); near('IMU_ACC_SCALE_' + c, TRUE.accScale[i], 0.003);
  near('IMU_GYRO_BIAS_' + c, TRUE.gyroBias[i], 0.1); near('IMU_UP_' + c, TRUE.up[i], 0.01); near('IMU_FRONT_' + c, TRUE.front[i], 0.02);
});

// layout identical to config.h: same names, and every line the same width as the line it replaces
const cfg = fs.readFileSync(path.join(root, 'firmware', 'include', 'config.h'), 'utf8').split('\n');
const defs = block.split('\n').filter(l => l.startsWith('#define'));
ok(defs.length === 15, defs.length + ' #define lines (want 15)');
for (const l of defs) {
  const name = l.split(/\s+/)[1], old = cfg.find(x => x.startsWith('#define ' + name + ' '));
  ok(!!old, name + ' missing from config.h');
  if (old) ok(old.replace(/\s+\/\/.*$/, '').length === l.length && old.split(/\s+/)[2].slice(-1) === 'f',
    `layout of ${name}: "${l}" vs config.h "${old.replace(/\s+\/\/.*$/, '')}"`);
}

// redoing step 2 invalidates the mounting (it was computed with the old accel correction)
click('six+z'); feed([0, 0, 1], 2000);
ok(!/#define IMU_UP_X/.test(window.__imuCode), 'mounting must be cleared after the accel is redone');
ok(/redo a\) and b\)/.test(text('mountRes')), 'mounting pill should say redo, says: ' + text('mountRes'));
ok(/step 3 a\)/.test(text('nextStep')), 'banner should ask for step 3 a) again, says: ' + text('nextStep'));

// compiles and reads back
const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'imu-'));
fs.writeFileSync(path.join(tmp, 't.cpp'), block + '\n#include <stdio.h>\nint main(){printf("%.4f %.4f\\n",IMU_ACC_OFFSET_Z,IMU_FRONT_Y);}\n');
try {
  cp.execFileSync('c++', ['-std=c++17', '-Wall', '-Werror', '-o', path.join(tmp, 't'), path.join(tmp, 't.cpp')]);
  const o = cp.execFileSync(path.join(tmp, 't')).toString().trim();
  ok(o === `${num('IMU_ACC_OFFSET_Z').toFixed(4)} ${num('IMU_FRONT_Y').toFixed(4)}`, 'compiled values differ: ' + o);
} catch (e) { ok(false, 'block does not compile: ' + e.message); }

console.log(fail ? `\n${fail} FAILED` : '\nOK: page recovered the simulated sensor, output matches config.h layout and compiles');
process.exit(fail ? 1 : 0);
