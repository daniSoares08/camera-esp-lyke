export type GraphicCommand = { op: string; p: number[]; text: string };
export type AnswerBlock = { kind: 'text'; text: string } |
  { kind: 'drawing'; width: number; height: number; alt: string; commands: GraphicCommand[] };
export type GraphicDocument = { version: 1; blocks: AnswerBlock[] };
const ops: Record<string, number> = { line: 4, rect: 4, circle: 3, arc: 5, text: 2, resistor: 4, capacitor: 4, source: 4, arrow: 4 };
const object = (properties: Record<string, unknown>) => ({ type: 'object', properties, required: Object.keys(properties), additionalProperties: false });
export const GRAPHIC_SCHEMA = object({ version: { type: 'integer', enum: [1] }, blocks: {
  type: 'array', minItems: 1, maxItems: 32, items: { anyOf: [
    object({ kind: { type: 'string', enum: ['text'] }, text: { type: 'string', maxLength: 12000 } }),
    object({ kind: { type: 'string', enum: ['drawing'] }, width: { type: 'integer', enum: [126,192,256] },
      height: { type: 'integer', minimum: 48, maximum: 512 }, alt: { type: 'string', maxLength: 160 },
      commands: { type: 'array', minItems: 1, maxItems: 128, items: object({
        op: { type: 'string', enum: Object.keys(ops) }, p: { type: 'array', minItems: 2, maxItems: 5, items: { type: 'integer', minimum: -1024, maximum: 1024 } },
        text: { type: 'string', maxLength: 32 },
      }) },
    }),
  ] },
} });

export const GRAPHIC_INSTRUCTIONS = `
Nesta consulta, retorne o objeto JSON definido pelo schema, com version=1 e blocos em ordem.
O desenvolvimento completo exigido acima vai nos blocos kind=text, em portugues ASCII.
Nao pre-quebre o texto em 21 caracteres: o visor conta simbolos e faz a unica quebra.
Uma linha por igualdade/grupo matematico; uma linha vazia antes de cada subitem.
Entre os passos, inclua kind=drawing quando a questao pedir desenho ou quando
ele for necessario para explicar uma transformacao, referencia ou topologia.
Nao redesenhe automaticamente o circuito de entrada se o pedido e apenas calcular
potencias/correntes e as equacoes ja bastam. Nao substitua contas por desenhos.
Para questoes puramente algebricas, use somente blocos de texto.
Cada desenho e monocromatico, origem no canto superior esquerdo, x cresce para direita,
y para baixo. Prefira width=126 para caber de frente; use 192/256 para desenhos maiores.
Altura 48..512. O leitor permite rolar e nao reduz a escala. Letras 5x7, avancando 6 pixels;
deixe espaco entre legendas/componentes e rotule eixos, unidades, valores e referencias.
Nao desenhe fotografias ou tente sombrear. Use no maximo seis desenhos e 384 comandos no total.
Comandos: line/arrow/resistor/capacitor/source p=[x1,y1,x2,y2]; rect p=[x,y,largura,altura];
circle p=[cx,cy,raio]; arc p=[cx,cy,raio,angulo_inicial,angulo_final]; text p=[x,y].
Arcos em graus, zero para direita e positivo anti-horario; amplitude maxima 360, raio 1..256.
Nos comandos sem legenda, text="". Para legendas use op=text, ate 32 caracteres ASCII.
Alt descreve o desenho, ate 160 caracteres, para leitores de texto sem suporte grafico.
Confira conexoes e polaridades do circuito, escala/eixos do grafico e coerencia com as contas.
Nao copie a disposicao de um circuito se isso conectar nos errados; represente a topologia correta.
Nao invente dimensoes, curvas, pontos, valores ou angulos ausentes do enunciado.`;

export function normalizeDocument(value: unknown, lcd: (text: string) => string): GraphicDocument {
  const v = value as any;
  if (!v || v.version !== 1 || !Array.isArray(v.blocks) || !v.blocks.length || v.blocks.length > 32) throw Error('invalid document');
  let textBytes = 0, figures = 0, commands = 0;
  const label = (text: unknown, max: number) => {
    if (typeof text !== 'string') throw Error('invalid text');
    const out = lcd(text).replace(/\n/g, ' ');
    if (out.length > max) throw Error('text too long');
    return out;
  };
  const blocks: AnswerBlock[] = v.blocks.map((b: any) => {
    if (b?.kind === 'text' && typeof b.text === 'string') {
      const text = lcd(b.text); textBytes += text.length;
      if (textBytes > 12000) throw Error('text too long');
      return { kind: 'text', text };
    }
    if (b?.kind !== 'drawing' || ++figures > 6 || ![126,192,256].includes(b.width) || !Number.isInteger(b.height) ||
        b.height < 48 || b.height > 512 || !Array.isArray(b.commands) || !b.commands.length || b.commands.length > 128) throw Error('invalid drawing');
    const result = b.commands.map((c: any) => {
      if (++commands > 384 || !c || !Object.hasOwn(ops,c.op) || !Array.isArray(c.p) || c.p.length !== ops[c.op] ||
          c.p.some((n: unknown) => !Number.isInteger(n) || (n as number) < -1024 || (n as number) > 1024)) throw Error('invalid command');
      if (['circle','arc'].includes(c.op) && (c.p[2] < 1 || c.p[2] > 256)) throw Error('invalid radius');
      if (c.op === 'arc' && (Math.abs(c.p[3]) > 360 || Math.abs(c.p[4]) > 360 || Math.abs(c.p[4]-c.p[3]) > 360)) throw Error('invalid arc');
      if (c.op === 'rect' && (c.p[2] < 1 || c.p[3] < 1 || c.p[2] > 512 || c.p[3] > 512)) throw Error('invalid rectangle');
      return { op: c.op, p: c.p, text: label(c.text,32) };
    });
    return { kind: 'drawing', width: b.width, height: b.height, alt: label(b.alt,160), commands: result };
  });
  if (!blocks.some(b => b.kind === 'text' && b.text.trim())) throw Error('missing mathematical answer');
  const doc: GraphicDocument = { version: 1, blocks };
  if (JSON.stringify(doc).length > 28000) throw Error('document too large');
  return doc;
}

export function legacyDocumentText(doc: GraphicDocument, lcd: (text: string) => string) {
  return lcd(doc.blocks.map(b => b.kind === 'text' ? b.text : `FIGURA: ${b.alt}`).join('\n'));
}

export function storedDocument(text: unknown, lcd: (text: string) => string): GraphicDocument | undefined {
  if (typeof text !== 'string' || !text.trimStart().startsWith('{')) return;
  try { return normalizeDocument(JSON.parse(text),lcd); } catch { return; }
}
