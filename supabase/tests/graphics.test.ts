import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { normalizeDocument, legacyDocumentText, storedDocument } from '../functions/lyke-solve/graphics.ts';
import { openAIRequest, validateRequest, toLCD, sha256 } from '../functions/lyke-solve/core.ts';
import { createHandler } from '../functions/lyke-solve/handler.ts';

const fixture = JSON.parse(readFileSync(new URL('../../firmware/lyke_stm32/tests/fixtures/answer_graphics.json',import.meta.url),'utf8'));
const jpeg='data:image/jpeg;base64,/9j/2Q==';
test('graphics are opt-in; schema and instructions preserve full mathematics',()=>{
  const plain=openAIRequest(validateRequest({photos:[jpeg]}));assert.ok(!('text' in plain));
  const rich=openAIRequest(validateRequest({photos:[jpeg],display:'lyke-graphics-v1'}));
  assert.equal(rich.text?.format.type,'json_schema');assert.equal(rich.text?.format.strict,true);
  assert.match(rich.instructions,/Nao substitua contas por desenhos/);
  assert.match(rich.instructions,/384 comandos/);assert.match(rich.instructions,/substituicao numerica/);
});
test('rich text keeps a single wrapping owner, paragraph separators and unambiguous math tokens',()=>{
  const text='2a) Impedancia\nZ=sqrt(36^2+27^2)=45 ohm\n\n2b) Corrente\nphi=36.87°; pi=3.14159; φ != π';
  const clean=toLCD(text,false);
  assert.match(clean,/Z=sqrt\(36\^2\+27\^2\)=45 ohm/);
  assert.match(clean,/\n\n2b\)/);assert.match(clean,/phi=36.87 deg/);assert.match(clean,/phi != pi/);
  const doc=normalizeDocument({version:1,blocks:[{kind:'text',text}]},s=>toLCD(s,false));
  assert.equal(doc.blocks[0].kind,'text');assert.equal((doc.blocks[0] as any).text,clean);
  const request=openAIRequest(validateRequest({photos:[jpeg],display:'lyke-graphics-v1'}));
  assert.match(request.instructions,/nao invente P\(m,t\)/);
  assert.match(request.instructions,/Nao pre-quebre/);
});
test('circuit, curve and angle normalize to bounded documents and readable legacy text',()=>{
  const doc=normalizeDocument(fixture,toLCD);assert.equal(doc.blocks.length,6);
  const legacy=legacyDocumentText(doc,toLCD);assert.match(legacy,/tau=R\*C/);assert.match(legacy,/FIGURA/);
  assert.ok(legacy.split('\n').every(line=>line.length<=21));
  assert.deepEqual(storedDocument(JSON.stringify(fixture),toLCD),doc);
  assert.equal(storedDocument('x=4',toLCD),undefined);
});
test('untrusted drawing commands, oversized shapes and missing solutions are rejected',()=>{
  for(const change of [
    (f:any)=>f.blocks[1].commands[0].op='javascript',
    (f:any)=>f.blocks[1].commands[0].p[0]=1e9,
    (f:any)=>f.blocks[4].commands[2].p[4]=10000,
    (f:any)=>f.blocks[1].height=513,
    (f:any)=>f.blocks=[f.blocks[1]],
    (f:any)=>f.blocks[1].commands[0].text='x'.repeat(33),
  ]){const value=structuredClone(fixture);change(value);assert.throws(()=>normalizeDocument(value,toLCD));}
});
const token='graphics-unit-test-device';
const env={supabaseUrl:'https://example.supabase.co',serviceKey:'test-service-key',openaiKey:'test-openai-key',
  tokenHash:await sha256(token),deviceId:'test-device',dailyLimit:50};
const request=(graphics=true)=>new Request('https://service.test',{method:'POST',headers:{'Content-Type':'application/json',
  Authorization:'Bearer '+token,'X-Lyke-Request-ID':'graphics-request-12345'},body:JSON.stringify({photos:[jpeg],...(graphics?{display:'lyke-graphics-v1'}:{})})});
test('structured answers are archived once and returned with legacy text plus drawing blocks',async()=>{
  let calls=0,saved:any;
  const handler=createHandler(env,async(url,init)=>{
    if(String(url).includes('/rpc/'))return Response.json({decision:'start'});
    if(String(url).includes('/storage/'))return Response.json({Key:'saved.jpg'});
    if(String(url).includes('api.openai.com')){
      ++calls;assert.equal(JSON.parse(init?.body as string).text.format.name,'lyke_answer');
      return Response.json({id:'resp_graphics',status:'completed',output:[{type:'message',content:[{type:'output_text',text:JSON.stringify(fixture)}]}]});
    }
    saved=JSON.parse(init?.body as string);return Response.json([saved]);
  });
  const response=await handler(request());const result=await response.json();
  assert.equal(response.status,200);assert.equal(calls,1);assert.equal(result.lyke_document.version,1);
  assert.match(result.output[0].content[0].text,/FIGURA/);assert.equal(saved.answer_text,JSON.stringify(fixture));
});
test('cached graphics can be read without another model call; legacy clients receive plain text',async()=>{
  let calls=0;
  const handler=createHandler(env,async url=>{++calls;assert.match(String(url),/\/rpc\//);return Response.json({decision:'cached',request:{id:'cached',model:'test',answer_text:JSON.stringify(fixture),answer_lcd:'tau=0.0001 s',photo_paths:['saved.jpg']}});});
  assert.ok((await(await handler(request())).json()).lyke_document);
  assert.ok(!(await(await handler(request(false))).json()).lyke_document);assert.equal(calls,2);
});
