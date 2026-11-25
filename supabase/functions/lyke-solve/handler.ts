import { BUCKET, MAX_BODY_BYTES, MODEL, RequestError, openAIRequest, sha256, toLCD, validateRequest } from './core.ts';
import { legacyDocumentText, normalizeDocument, storedDocument } from './graphics.ts';

export type Environment = {
  supabaseUrl: string; serviceKey: string; openaiKey: string;
  tokenHash: string; deviceId: string; dailyLimit: number;
};
const jsonHeaders = { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' };
const reply = (value: unknown, status = 200) => new Response(JSON.stringify(value), { status, headers: jsonHeaders });
const richText = (text:string) => toLCD(text,false);

function completed(row: Record<string, any>, graphics = false) {
  const document = graphics ? storedDocument(row.answer_text,richText) : undefined;
  return {
    id: row.id, object: 'response', status: 'completed', model: row.model,
    output: [{ type: 'message', role: 'assistant', content: [{ type: 'output_text', text: row.answer_lcd }] }],
    usage: { input_tokens: row.input_tokens || 0, output_tokens: row.output_tokens || 0 },
    archive: { request_id: row.id, bucket: BUCKET, photos: row.photo_paths, private: true },
    ...(document ? { lyke_document: document } : {}),
  };
}

export function createHandler(env: Environment, transport: typeof fetch = fetch) {
  const adminHeaders: Record<string, string> = { apikey: env.serviceKey, 'Content-Type': 'application/json' };
  if (env.serviceKey.startsWith('eyJ')) adminHeaders.Authorization = `Bearer ${env.serviceKey}`;

  async function db(path: string, method = 'GET', body?: unknown) {
    const response = await transport(`${env.supabaseUrl}/rest/v1/${path}`, {
      method, headers: { ...adminHeaders, Prefer: 'return=representation' },
      body: body === undefined ? undefined : JSON.stringify(body), signal: AbortSignal.timeout(12000),
    });
    if (!response.ok) throw new RequestError(503, 'archive_unavailable', 'Arquivo indisponivel. Tente novamente.');
    return response.status === 204 ? null : response.json();
  }

  return async (req: Request): Promise<Response> => {
    let activeId = '';
    let finished = false;
    try {
      if (req.method === 'GET') return reply({ service: 'lyke-solve', model: MODEL, version: '4', graphics: 'lyke-graphics-v1', private_storage: true });
      if (req.method !== 'POST') return reply({ error: { code: 'method_not_allowed', message: 'Use POST.' } }, 405);
      if (!env.tokenHash || !env.openaiKey || !env.serviceKey) throw new RequestError(503, 'not_configured', 'Servico ainda nao configurado.');
      const auth = req.headers.get('Authorization') || '';
      if (!auth.startsWith('Bearer ') || await sha256(auth.slice(7)) !== env.tokenHash)
        throw new RequestError(401, 'unauthorized', 'Chave de acesso da calculadora invalida.');
      if (!(req.headers.get('Content-Type') || '').startsWith('application/json'))
        throw new RequestError(415, 'invalid_content_type', 'Envie JSON com fotos JPEG.');
      const declared = Number(req.headers.get('Content-Length') || 0);
      if (declared > MAX_BODY_BYTES) throw new RequestError(413, 'body_too_large', 'Fotos grandes demais.');
      const reader = req.body?.getReader();
      if (!reader) throw new RequestError(400, 'invalid_request', 'Envie as fotos.');
      const chunks: Uint8Array[] = []; let bytes = 0;
      while (true) {
        const part = await reader.read(); if (part.done) break;
        bytes += part.value.length;
        if (bytes > MAX_BODY_BYTES) { await reader.cancel(); throw new RequestError(413, 'body_too_large', 'Fotos grandes demais.'); }
        chunks.push(part.value);
      }
      const raw = new Uint8Array(bytes); let offset = 0;
      for (const chunk of chunks) { raw.set(chunk, offset); offset += chunk.length; }
      let body: unknown;
      try { body = JSON.parse(new TextDecoder().decode(raw)); } catch { throw new RequestError(400, 'invalid_json', 'JSON invalido.'); }
      const valid = validateRequest(body);
      const requestId = req.headers.get('X-Lyke-Request-ID') || crypto.randomUUID();
      if (!/^[A-Za-z0-9_-]{16,64}$/.test(requestId)) throw new RequestError(400, 'invalid_request_id', 'Identificador de consulta invalido.');
      const fingerprint = await sha256(valid.model + (valid.graphics ? ':graphics-v1' : '') + ':' + (await Promise.all(valid.photos.map(p => sha256(p.bytes)))).join(':'));
      const claim = await db('rpc/lyke_claim_request', 'POST', {
        p_id: requestId, p_device_id: env.deviceId, p_fingerprint: fingerprint,
        p_model: valid.model, p_photo_count: valid.photos.length, p_daily_limit: env.dailyLimit,
      });
      if (claim.decision === 'cached') return reply(completed(claim.request,valid.graphics));
      if (claim.decision === 'failed') {
        const saved = claim.request.error || {};
        return reply({ error: { code: saved.code || 'previous_failure', message: saved.message || 'Consulta anterior falhou. Tente novamente.' } }, saved.http_status || 502);
      }
      if (claim.decision === 'limit') throw new RequestError(429, 'device_daily_limit', 'Limite diario de consultas atingido.');
      if (claim.decision !== 'start') throw new RequestError(409, 'request_in_progress', 'Consulta em andamento. Aguarde e tente novamente.');
      activeId = requestId;
      const paths: string[] = [];
      for (let i = 0; i < valid.photos.length; i++) {
        const path = `${env.deviceId}/${requestId}/${i + 1}.jpg`;
        const uploaded = await transport(`${env.supabaseUrl}/storage/v1/object/${BUCKET}/${path}`, {
          method: 'POST', headers: { ...adminHeaders, 'Content-Type': 'image/jpeg', 'x-upsert': 'false' },
          body: valid.photos[i].bytes as BodyInit, signal: AbortSignal.timeout(15000),
        });
        if (!uploaded.ok) throw new RequestError(503, 'archive_unavailable', 'Falha guardando foto. A IA nao foi chamada.');
        paths.push(path);
      }
      const selector = `lyke_requests?id=eq.${requestId}&device_id=eq.${encodeURIComponent(env.deviceId)}`;
      await db(selector, 'PATCH', { photo_paths: paths });
      const response = await transport('https://api.openai.com/v1/responses', {
        method: 'POST', headers: { Authorization: `Bearer ${env.openaiKey}`, 'Content-Type': 'application/json', 'X-Client-Request-Id': requestId },
        body: JSON.stringify(openAIRequest(valid)), signal: AbortSignal.timeout(100000),
      });
      let result: any;
      try { result = await response.json(); } catch { throw new RequestError(502, 'upstream_invalid', 'A IA retornou uma resposta invalida.'); }
      if (!response.ok) {
        const quota = response.status === 429 && ['insufficient_quota', 'credit_balance_exhausted'].includes(result.error?.code);
        throw new RequestError(response.status === 401 ? 502 : response.status, quota ? 'insufficient_quota' : 'upstream_error',
          quota ? 'Sem credito na API OpenAI.' : response.status === 429 ? 'IA ocupada. Aguarde antes de tentar novamente.' : 'Falha no servico de IA. As fotos foram guardadas.');
      }
      if (result.status !== 'completed') throw new RequestError(502, 'incomplete_answer', 'Resposta incompleta. Tente uma questao por vez.');
      const answer = (result.output || []).filter((i: any) => i.type === 'message')
        .flatMap((i: any) => i.content || []).filter((c: any) => c.type === 'output_text')
        .map((c: any) => c.text || '').join('\n');
      if (!answer.trim()) throw new RequestError(502, 'empty_answer', 'A IA nao retornou texto.');
      let answerLCD: string;
      if (valid.graphics) {
        try { answerLCD = legacyDocumentText(normalizeDocument(JSON.parse(answer),richText),toLCD); }
        catch { throw new RequestError(502,'invalid_drawing','A IA retornou um desenho invalido. As fotos foram guardadas.'); }
      } else answerLCD = toLCD(answer);
      const row = {
        id: requestId, model: valid.model, status: 'completed', photo_paths: paths,
        answer_text: answer, answer_lcd: answerLCD,
        input_tokens: result.usage?.input_tokens || 0, output_tokens: result.usage?.output_tokens || 0,
        finished_at: new Date().toISOString(), openai_response_id: result.id || null,
      };
      await db(selector, 'PATCH', row);
      finished = true;
      return reply(completed(row,valid.graphics));
    } catch (cause) {
      const error = cause instanceof RequestError ? cause : new RequestError(502, 'network_error', 'Falha de conexao. Tente novamente em instantes.');
      if (activeId && !finished) {
        try { await db(`lyke_requests?id=eq.${activeId}&device_id=eq.${encodeURIComponent(env.deviceId)}`, 'PATCH', {
          status: 'failed', finished_at: new Date().toISOString(),
          error: { code: error.code, message: error.message, http_status: error.status },
        }); } catch { /* Keep a processing reservation rather than repeat a paid request. */ }
      }
      return reply({ error: { code: error.code, message: error.message } }, error.status);
    }
  };
}
