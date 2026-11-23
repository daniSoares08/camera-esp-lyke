'use strict';
const $ = id => document.getElementById(id);
let state = null, built = false, lastLog = -1, pending = false;
const pairName = p => p ? `GPIO${p.a} ↔ ${p.b === -1 ? 'GND' : `GPIO${p.b}`}` : '—';
function build(s) {
  for (let row = 0; row < 9; row++) {
    const el = document.createElement('div');
    el.className = 'keyrow' + (row === 4 ? ' enter' : row > 4 ? ' five' : '');
    s.keys.forEach((key, i) => {
      if (key.row !== row) return;
      const button = document.createElement('button');
      button.className = 'key'; button.dataset.id = key.id; button.dataset.index = i;
      button.setAttribute('aria-label', `${key.id}: ${key.label}`);
      const text = document.createElement('span'); text.textContent = key.label;
      const alpha = document.createElement('span'); alpha.className = 'alpha';
      alpha.textContent = row > 0 ? key.alpha : '';
      button.append(text, alpha); button.onclick = () => action('select', i); el.append(button);
    });
    $('keyboard').append(el);
  }
  for (const pin of s.pins) {
    const el = document.createElement('span'); el.className = 'pin'; el.dataset.pin = pin;
    el.textContent = pin; $('pinlist').append(el);
  }
  built = true;
}
function render(s) {
  state = s; if (!built) build(s);
  $('connection').textContent = s.connected ? `${s.port} · ESP conectado` : (s.port ? `${s.port} · aguardando ESP` : 'Aguardando USB/UART');
  $('dot').classList.toggle('online', s.connected);
  $('runstate').textContent = s.armed ? 'Registrando' : 'Pausado / teste';
  const key = s.keys[s.target];
  $('nextkey').textContent = key ? key.id + (key.alpha && key.label !== key.id ? ` · ${key.label}` : '') : 'Sequência concluída';
  $('position').textContent = key ? `Posição ${s.target + 1} de ${s.keys.length}` : `${s.mapped} mapeadas`;
  $('lcdkey').textContent = key ? (key.id + ' · ' + key.label) : 'SEQUÊNCIA CONCLUÍDA';
  $('lcdmode').textContent = s.armed ? 'MAPEAMENTO · APERTE E SOLTE' : 'HP 48 · PRONTO / TESTE';
  $('lcdcontact').textContent = s.live ? pairName(s.live) : 'Aguardando contato…';
  const idle = s.info.idle_low_pins || [];
  const rawContacts = (s.info.contacts || []).filter(([a,b]) => !(b === -1 && idle.includes(a)));
  $('contact').textContent = s.live ? pairName(s.live) : rawContacts.length ? `${rawContacts.length} contato(s)` : '—';
  $('contact').title = rawContacts.map(([a,b]) => pairName({a,b})).join('; ');
  $('idle-note').textContent = idle.length ? `Repouso descontado: ${idle.map(p => `GPIO${p}`).join(', ')}. Esses níveis fixos não contam como teclas.` : '';
  $('message').textContent = s.message;
  $('instruction').textContent = key ? `Fileira ${key.row + 1}, tecla ${key.col + 1} da esquerda. Aperte e solte antes de avançar.` : (s.skipped ? `${s.skipped} tecla(s) pendente(s). Selecione-as no desenho para completar.` : 'Todas as teclas registradas. Teste os contatos ou exporte seu mapa.');
  $('progresslabel').textContent = `${s.mapped} / ${s.keys.length} mapeadas` + (s.skipped ? ` · ${s.skipped} pendentes` : '');
  $('progress').max = s.keys.length; $('progress').value = s.mapped;
  $('percent').textContent = `${Math.round(s.mapped / s.keys.length * 100)}%`;
  $('savepath').textContent = s.save_path;
  $('start').disabled = !s.connected || s.armed || !key;
  $('pause').disabled = !s.armed;
  $('skip').disabled = !key;
  $('test').disabled = !s.connected;
  document.querySelectorAll('.key').forEach(el => {
    const value = s.mapping[el.dataset.id], i = Number(el.dataset.index);
    el.classList.toggle('mapped', !!value);
    el.classList.toggle('skipped', value === null);
    el.classList.toggle('target', i === s.target);
    el.classList.toggle('live', !!(value && s.live && value.a === s.live.a && value.b === s.live.b));
    el.title = `${el.dataset.id}: ${value ? pairName(value) : value === null ? 'Pendente' : 'Não mapeada'}`;
  });
  document.querySelectorAll('.pin').forEach(el => el.classList.toggle('active', !!(s.live && [s.live.a, s.live.b].includes(Number(el.dataset.pin)))));
  const logId = s.events[0]?.id || 0;
  if (logId !== lastLog) {
    $('events').replaceChildren();
    for (const event of s.events) {
      const li = document.createElement('li'), t = document.createElement('time'), text = document.createElement('span');
      li.className = event.kind; t.textContent = event.time; text.textContent = event.text;
      li.append(t, text); $('events').append(li);
    }
    if (!s.events.length) { const li = document.createElement('li'); li.textContent = 'Nenhum contato registrado.'; $('events').append(li); }
    lastLog = logId;
  }
}
async function action(name, index) {
  try {
    const response = await fetch('/api/action', {method:'POST', headers:{'Content-Type':'application/json','X-Mapper-Token':window.MAPPER_TOKEN}, body:JSON.stringify({action:name,index})});
    const body = await response.json(); if (!response.ok) throw Error(body.error); render(body);
  } catch (e) { $('message').textContent = e.message; }
}
for (const name of ['start','pause','undo','skip','test']) $(name).onclick = () => action(name);
async function poll() {
  if (pending) return; pending = true;
  try { const response = await fetch('/api/state'); if (!response.ok) throw Error(); render(await response.json()); }
  catch (e) { $('connection').textContent = 'Interface local desconectada'; $('dot').classList.remove('online'); $('start').disabled = true; }
  finally { pending = false; }
}
poll(); setInterval(poll, 140);
