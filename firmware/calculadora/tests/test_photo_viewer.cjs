const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const source = fs.readFileSync(path.join(__dirname, '../s3_calculadora/runtime.h'), 'utf8');
const page = source.match(/inline String photoPage\(\)[\s\S]*?R"PAGE\(([\s\S]*?)\)PAGE";/)[1];
const script = page.match(/<script>([\s\S]*?)<\/script>/)[1];

async function exercise(count) {
  const elements = new Map();
  const element = () => ({
    children: [], hidden: true, textContent: '', value: '0',
    appendChild(child) { this.children.push(child); },
    replaceChildren() { this.children = []; },
    classList: { toggle() {} },
  });
  const get = id => {
    if (!elements.has(id)) elements.set(id, element());
    return elements.get(id);
  };
  const photos = Array.from({length: count}, (_, i) => ({
    sequence: i + 1, width: 2560, height: 1920, bytes: 1000 + i,
  }));
  const state = {
    ...photos.at(-1), available: true, photo_count: count, photos,
    camera_active: false, camera_sensor_standby_now: true,
    wifi_connected: false, api_attempted: false, profiles: ['IPHONEPROMAX'],
    status: 'Foto pronta', wifi_disconnect_reason: 201,
  };
  const writes = [];
  const context = vm.createContext({
    document: { getElementById: get, createElement: element },
    setTimeout() {},
    async fetch(url, options) {
      if (options?.method === 'POST') {
        writes.push({url, body: JSON.parse(options.body), headers: options.headers});
        return {ok: true, async json() { return {message: 'Solicitado'}; }};
      }
      assert.equal(url, '/api/photo');
      return {async json() { return structuredClone(state); }};
    },
  });
  await vm.runInContext(script, context);
  assert.equal(get('gallery').children.length, count + 1);
  assert.match(get('info').textContent, new RegExp(`Fotos para consulta: ${count}/3`));
  assert.match(get('info').textContent, /standby confirmado/);
  assert.match(get('info').textContent, /motivo Wi-Fi 201/);
  for (let i = 0; i < count; i++) {
    get('gallery').children[i + 1].onclick();
    assert.match(get('photo').src, new RegExp(`slot=${i}&seq=${i + 1}`));
    assert.match(get('download').href, new RegExp(`download=1&slot=${i}`));
  }
  // Metadata must use the current response, including an already selected slot.
  state.photos[count - 1].bytes = 876543;
  await vm.runInContext('refresh()', context);
  assert.match(get('info').textContent, /JPEG 876543 bytes/);
  assert.equal(writes.length, 0); // Viewing never captures or sends a paid request.
  await get('consult').onclick();
  assert.deepEqual(writes.at(-1).body, {api: true, batch: true});
  await get('capture').onclick();
  assert.deepEqual(writes.at(-1).body, {api: false, slot: true});
  assert.equal(writes.at(-1).headers['X-Config-Token'], '__TOKEN__');
}

(async () => {
  for (const n of [1, 2, 3]) await exercise(n);
  console.log('PASS: gallery 1/2/3, original downloads, current metadata, standby, explicit capture/send');
})().catch(error => { console.error(error); process.exitCode = 1; });
