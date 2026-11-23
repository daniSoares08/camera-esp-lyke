"""Manual do comprador. Gera PDF, Markdown, figuras e pacote portátil.

Execute na raiz: .venv\\Scripts\\python.exe scripts/manual_lyke/gerar_manual.py
Fontes funcionais: STM32 R11 + ESP Lyke R8. Nenhuma credencial é lida.
"""
from pathlib import Path
import re, json, shutil, zipfile, math, html, argparse
import urllib.request
from PIL import Image, ImageDraw, ImageFont
from reportlab.pdfgen import canvas
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import Paragraph, Table, TableStyle
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'output/pdf'
ASSETS = OUT / 'manual-assets'
OUT.mkdir(parents=True, exist_ok=True)
ASSETS.mkdir(exist_ok=True)
(ROOT/'tmp/pdfs/manual-lyke').mkdir(parents=True, exist_ok=True)
reference=ASSETS/'lyke-original-referencia.jpg'
if not reference.exists():
    urllib.request.urlretrieve('https://lyke.com.br/arquivos/produtos/imagens_adicionais/101078_01-4198.jpg',reference)
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--unit-photo',type=Path,help='Foto da unidade fornecida pelo proprietário')
args=parser.parse_args()
unit_photo=ASSETS/'unidade-lyke.png'
if args.unit_photo:shutil.copyfile(args.unit_photo,unit_photo)
elif not unit_photo.exists() and (ROOT/'docs/manual/manual-assets/unidade-lyke.png').exists():
    shutil.copyfile(ROOT/'docs/manual/manual-assets/unidade-lyke.png',unit_photo)
NAVY = '#142C3C'; TEAL = '#087F81'; INK = '#243D4B'; MUTED = '#58707D'
PALE = '#EFF6F6'; AMBER = '#FFF2D8'; WHITE = '#FFFFFF'
FONT = Path('C:/Windows/Fonts')
for name, fn in [('Body','arial.ttf'),('Bold','arialbd.ttf'),('Italic','ariali.ttf')]:
    pdfmetrics.registerFont(TTFont(name, str(FONT/fn)))
pdfmetrics.registerFontFamily('Body', normal='Body', bold='Bold', italic='Italic', boldItalic='Bold')

def font(size, bold=False):
    return ImageFont.truetype(str(FONT / ('arialbd.ttf' if bold else 'arial.ttf')), size)

def wrapped(draw, text, xy, width, size=27, fill=INK, bold=False, leading=None):
    f = font(size,bold); words=text.split(); lines=[]; current=''
    for w in words:
        nxt=(current+' '+w).strip()
        if draw.textlength(nxt,font=f)>width and current: lines.append(current); current=w
        else: current=nxt
    if current: lines.append(current)
    x,y=xy
    for line in lines:
        draw.text((x,y),line,font=f,fill=fill); y+=leading or int(size*1.35)
    return y

raw=(ROOT/'firmware/calculadora/common/font5x7.h').read_text()
glyphs=[[int(v,16) for v in re.findall(r'0x[0-9a-fA-F]+',line)] for line in raw.splitlines() if '{0x' in line]
assert len(glyphs)==95

def lcd(lines, inverse=(), photos=0, wifi=False, flags='', battery=3, header=True):
    """Visible 21 x 6 viewport; exact 5x7 firmware glyphs, enlarged nearest-neighbor."""
    base=Image.new('RGB',(128,48),'#CFD8B8'); d=ImageDraw.Draw(base)
    for row,text in enumerate(lines):
        assert len(text)<=21,(text,len(text))
        inv=row in inverse
        if inv: d.rectangle((1,row*8,127,row*8+7),fill='#233A2C')
        for col,ch in enumerate(text):
            code=ord(ch)
            bits=glyphs[code-32] if 32<=code<=126 else glyphs[ord('?')-32]
            for x,byte in enumerate(bits):
                for y in range(7):
                    if byte&(1<<y): d.point((2+col*6+x,row*8+y), fill='#CFD8B8' if inv else '#233A2C')
    # Answer view uses all six lines, without status icons or title.
    if not header:return base.resize((1024,384),Image.Resampling.NEAREST)
    for x in range(96,107):
        for y in range(8):
            if x in [96,106] or y in [0,7]:d.point((x,y),fill='#233A2C')
    for y in range(2,6):d.point((107,y),fill='#233A2C')
    for bar in range(battery):
        for y in range(2,6):d.point((98+2*bar,y),fill='#233A2C')
    # Fixed photo icons from interface.h; text line 0 is reserved for status.
    for slot in range(3):
        x=112+slot*6
        d.rectangle((x,1,x+3,4), outline='#233A2C', fill='#233A2C' if slot<photos else '#CFD8B8')
    if wifi:
        for pt in [(0,1),(4,1),(1,0),(2,0),(3,0),(1,3),(3,3),(2,2),(2,5)]: d.point(pt,fill='#233A2C')
    for ix,ch in zip([3,5,7],flags):
        if ch==' ': continue
        for x,byte in enumerate(glyphs[ord(ch)-32]):
            for y in range(7):
                if byte&(1<<y):d.point((2+ix*6+x,y),fill='#233A2C')
    return base.resize((1024,384),Image.Resampling.NEAREST)

def lcd_figure(name, screens):
    n=len(screens); width=1500; gap=45; margin=40
    im=Image.new('RGB',(width,520 if n<3 else 430),PALE); d=ImageDraw.Draw(im)
    sw=(width-2*margin-(n-1)*gap)//n; sh=int(sw*48/128)
    y=75 if n<3 else 100
    for i,s in enumerate(screens):
        x=margin+i*(sw+gap)
        d.rounded_rectangle((x-10,y-10,x+sw+10,y+sh+10),18,fill=NAVY)
        sc=(Image.open(s['frame']).convert('RGB') if s.get('frame') else
            lcd(s['lines'],s.get('inverse',()),s.get('photos',0),s.get('wifi',False),s.get('flags',''),s.get('battery',3),s.get('header',True)))
        if s.get('frame'):
            mono=Image.open(s['frame']).convert('L')
            sc=Image.new('RGB',mono.size,'#CFD8B8');sc.paste('#233A2C',mask=Image.eval(mono,lambda v:255-v))
        im.paste(sc.resize((sw,sh),Image.Resampling.NEAREST),(x,y))
        wrapped(d,s.get('label',''),(x,y+sh+28),sw,26,INK,True)
    d.text((40,22),'TELAS SIMULADAS | LCD: 21 colunas x 6 linhas úteis',font=font(24),fill=MUTED)
    p=ASSETS/f'{name}.png'; im.save(p); return p

def panel_figure(name,title,subtitle,rows,foot='',kind='browser'):
    im=Image.new('RGB',(1500,750),PALE);d=ImageDraw.Draw(im)
    d.rounded_rectangle((30,22,1470,728),24,fill=WHITE,outline='#CEDDE0',width=3)
    d.rounded_rectangle((30,22,1470,110),24,fill=NAVY)
    d.rectangle((30,80,1470,110),fill=NAVY)
    d.text((65,47),title,font=font(34,True),fill=WHITE)
    d.text((65,132),subtitle,font=font(25),fill=MUTED)
    y=194
    for label,value in rows:
        d.text((65,y),label,font=font(27,True),fill=INK)
        d.rounded_rectangle((630,y-7,1420,y+49),10,fill=PALE,outline='#D3E1E2',width=2)
        wrapped(d,value,(654,y+2),740,25,TEAL)
        y+=86
    if foot:wrapped(d,foot,(65,650),1355,24,MUTED)
    p=ASSETS/f'{name}.png';im.save(p);return p

def flow_figure(name,title,nodes,sub=''):
    im=Image.new('RGB',(1500,430),PALE);d=ImageDraw.Draw(im)
    d.text((42,25),title,font=font(34,True),fill=NAVY)
    n=len(nodes);gap=55;bw=(1416-(n-1)*gap)//n
    for i,(a,b) in enumerate(nodes):
        x=42+i*(bw+gap)
        d.rounded_rectangle((x,112,x+bw,319),20,fill=WHITE,outline='#B9D5D5',width=3)
        d.text((x+18,126),f'{i+1:02}',font=font(25,True),fill=TEAL)
        yy=wrapped(d,a,(x+18,170),bw-36,29,NAVY,True)
        wrapped(d,b,(x+18,yy+9),bw-36,24,MUTED)
        if i<n-1:
            ax=x+bw+9;ay=214
            d.line((ax,ay,ax+gap-20,ay),fill=TEAL,width=6)
            d.polygon([(ax+gap-20,ay),(ax+gap-34,ay-10),(ax+gap-34,ay+10)],fill=TEAL)
    wrapped(d,sub,(42,351),1410,25,MUTED)
    p=ASSETS/f'{name}.png';im.save(p);return p

def calculator_figure():
    im=Image.new('RGB',(1500,1350),'#F2F6F5');d=ImageDraw.Draw(im)
    x0,y0,x1,y1=435,42,1065,1292
    d.rounded_rectangle((x0+12,y0+15,x1+12,y1+15),75,fill='#D3DEDF')
    d.rounded_rectangle((x0,y0,x1,y1),75,fill='#20292E',outline='#0C151A',width=5)
    d.text((477,69),'Lyke',font=font(40,True),fill=WHITE)
    d.text((778,79),'MANUAL R11 / R8',font=font(19),fill='#C6DADB')
    d.rounded_rectangle((474,139,1026,390),20,fill='#111D22')
    sc=lcd(['','2+3*4','14','','',''])
    im.paste(sc.resize((520,195),Image.Resampling.NEAREST),(490,165))
    # The drawing shows logical key positions, not the finished enclosure dimensions.
    d.text((490,424),'SHIFT',font=font(18,True),fill='#E8B74F');d.text((586,424),'ALPHA',font=font(18,True),fill='#90CBCB')
    for xx,lab in [(490,'SHIFT'),(585,'ALPHA'),(878,'MODE'),(972,'ON')]:
        d.rounded_rectangle((xx,454,xx+74,497),15,fill='#667B85' if lab!='ON' else '#414B50')
        if lab in ('MODE','ON'):d.text((xx+10,465),lab,font=font(17,True),fill=WHITE)
    d.ellipse((700,420,848,550),fill='#788A92')
    for txt,xx,yy in [('^',766,420),('<',707,463),('>',821,463),('v',767,510)]:d.text((xx,yy),txt,font=font(27,True),fill=NAVY)
    rows=[['x^-1','nCr','Pol(','x^3'],['ab/c','sqrt','x^2','^','log','ln'],['(-)','DMS','hyp','sin','cos','tan'],['RCL','ENG','(',')',',','M+'],['7','8','9','DEL','AC'],['4','5','6','x','/'],['1','2','3','+','-'],['0','.','EXP','Ans','=']]
    yy=572
    for j,row in enumerate(rows):
        n=len(row); gap=15; kw=(540-(n-1)*gap)//n;xx=480
        for text in row:
            bg='#A7B5BB' if j>=4 else '#3D4C54'
            if text in ['DEL','AC']:bg='#B64E60'
            d.rounded_rectangle((xx,yy,xx+kw,yy+56),10,fill=bg)
            sz=25 if len(text)<=3 else 21;ff=font(sz,True)
            d.text((xx+(kw-d.textlength(text,font=ff))/2,yy+12),text,font=ff,fill=WHITE)
            xx+=kw+gap
        yy+=78
    def callout(n,text,box,tip):
        bx,by=box;d.ellipse((bx,by,bx+46,by+46),fill=TEAL)
        d.text((bx+14,by+8),str(n),font=font(25,True),fill=WHITE)
        wrapped(d,text,(bx,by+61),300,27,NAVY,True)
        edge=bx+305 if bx<400 else bx-12
        d.line((edge,by+23,tip[0],tip[1]),fill=TEAL,width=4)
        d.ellipse((tip[0]-5,tip[1]-5,tip[0]+5,tip[1]+5),fill=TEAL)
    callout(1,'Tela: conta, fotos e resposta',(60,180),(477,250))
    callout(2,'SHIFT e ALPHA: pressione e solte',(62,445),(490,475))
    callout(3,'REPLAY: navegação e edição',(1133,444),(848,475))
    callout(4,'MODE: menu e despertar',(1133,135),(915,475))
    callout(5,'DEL corrige; AC limpa ou volta',(1133,780),(970,930))
    callout(6,'= calcula ou confirma',(62,1060),(975,1220))
    d.text((65,1301),'ILUSTRAÇÃO DIDÁTICA | carcaça Lyke 101078; teclas e tela baseadas no firmware',font=font(24),fill=MUTED)
    p=ASSETS/'calculadora-guia.png';im.save(p);return p

PAGES=[]
def page(title,kicker,fig,caption,blocks):
    PAGES.append(dict(title=title,kicker=kicker,fig=fig,caption=caption,blocks=blocks))
def p(text):return ('p',text)
def h(text):return ('h',text)
def steps(*items):return ('steps',items)
def table(headers,*rows):return ('table',headers,rows)
def note(title,text):return ('note',title,text)
def S(label,lines,inverse=(),photos=0,wifi=False,flags='',battery=3,header=True):
    return dict(label=label,lines=lines,inverse=inverse,photos=photos,wifi=wifi,flags=flags,battery=battery,header=header)

def native(label,stem):
    return dict(label=label,frame=ROOT/'.build/lyke_stm32/lyke-r11/previews'/f'{stem}.pbm')

def battery_figure():
    im=Image.new('RGB',(1500,440),PALE);d=ImageDraw.Draw(im)
    d.text((40,18),'INDICADOR NO CABEÇALHO | níveis aproximados pela tensão',font=font(28,True),fill=NAVY)
    for n,(stem,label) in enumerate([('cheia','Cheia: quatro barras'),('media','Intermediária: duas barras'),
                                    ('baixa','Baixa: ! piscando'),('sem_leitura','Falha de leitura: ?')]):
        x=40+(n%2)*740;y=80+(n//2)*170
        d.text((x,y),label,font=font(27,True),fill=INK)
        raw=Image.open(ROOT/f'.build/lyke_stm32/lyke-r11/previews/bateria_{stem}.pbm').convert('L').crop((88,0,128,8))
        sc=Image.new('RGB',raw.size,'#CFD8B8');sc.paste('#233A2C',mask=Image.eval(raw,lambda v:255-v))
        im.paste(sc.resize((600,120),Image.Resampling.NEAREST),(x,y+40))
    target=ASSETS/'bateria-indicador.png';im.save(target);return target
def screens(name,*ss):return lcd_figure(name,list(ss))
def panel(name,title,subtitle,rows,foot=''):return panel_figure(name,title,subtitle,rows,foot)
def flow(name,title,nodes,sub=''):return flow_figure(name,title,nodes,sub)

calc=calculator_figure()

page('Sua calculadora, do primeiro toque à IA','MANUAL DO COMPRADOR',calc,
     'Ilustração didática da Lyke modificada. Não é fotografia da unidade montada.',[
 p('Guia de uso da calculadora Lyke 101078 modificada, com cálculo científico offline, câmera, Wi-Fi, resolução por IA e arquivo privado de fotos.'),
 note('Comece sem configurar nada','Para fazer contas, basta ligar o aparelho e usar o teclado. Para resolver fotos com IA, siga o início rápido e a conexão ao Wi-Fi.'),
 p('Edição 2.0 - 23/11/2026. Compatibilidade: STM32 Lyke R11 + ESP32-CAM Lyke R8. As telas deste guia são simulações baseadas no firmware.'),
 p('<b>Integração testada: OpenAI, usada pelos modelos do ChatGPT.</b> Claude aparece como alternativa de preparação da conta; ainda não foi integrado nem testado nesta calculadora.')])

page('Escolha o que você quer fazer','COMO USAR ESTE GUIA',flow('rotas','Três caminhos para começar',[
 ('Fazer contas','Ligue e use o teclado'),('Resolver uma foto','Conecte, capture e envie'),('Usar sua API','Prepare conta e serviço')]),
 'Fluxo ilustrativo. Os capítulos avançados podem ficar para depois.',[
 table(['Quero...','Vá às páginas'],
 ('Conhecer as teclas e ligar pela primeira vez','3 a 6'),
 ('Usar as funções científicas, memórias e estatística','7 a 14'),
 ('Conectar à internet e resolver questões fotografadas','15 a 19'),
 ('Entender onde as fotos ficam guardadas','20'),
 ('Configurar minha própria conta OpenAI','21 a 24'),
 ('Preparar uma conta Claude e entender a limitação atual','25 e 26'),
 ('Controlar gastos, Wi-Fi e atualização','27 a 29'),
 ('Resolver problemas ou consultar todas as teclas','30 a 33'),
 ('Aprender com IA e conferir compatibilidade e fontes','34 a 36')),
 h('Como ler os passos'),p('Uma sequência como <b>SHIFT, direita</b> significa: pressione SHIFT, solte e depois pressione a seta direita. Não é necessário segurar duas teclas. Nomes em maiúsculas reproduzem o visor. Os sinais x e / representam multiplicação e divisão.'),
 note('Guarde os dados da sua unidade','O fornecedor deve entregar a senha de Calc-Lyke, o local da chave de energia, a orientação da câmera, o método de recarga e, se contratado, o acesso ao serviço de IA. Esses dados variam conforme a montagem.')])

page('Conheça a calculadora','01 | O APARELHO',calc,'Desenho da frente com funções das teclas. Posição da câmera, chave e conector depende da montagem.',[
 p('O teclado e a carcaça vêm da Lyke 101078, mas o funcionamento é definido pelo firmware deste projeto. Legendas secundárias impressas na carcaça podem diferir das funções descritas aqui.'),
 table(['Parte','Para que serve'],('Tela','Mostra expressão, resultado, menus e respostas em páginas.'),('REPLAY','As quatro setas movem o cursor ou percorrem listas e respostas.'),('MODE / = / AC','Abrem o menu / confirmam / voltam ou limpam.'),('SHIFT / ALPHA','Ativam a função secundária ou uma variável para a próxima tecla.'),('Câmera e Wi-Fi','Capturam questões e acessam o serviço de IA pela internet.')),
 note('Como ligar','Use a chave física de alimentação indicada pelo fornecedor. A tecla ON está sem função nesta versão de 49 teclas. Não é necessário abrir a calculadora nem ligar fios para o uso normal.')])

page('Primeiros minutos: conta e foto','02 | INÍCIO RÁPIDO',screens('inicio',S('Sua primeira conta',['','2+3*4','14','','','']),S('Confirmar envio',['','ENVIAR 1 FOTO(S)?','= ENVIA A IA','AC CANCELA','',''],(1,),1,True)),
 'Simulações. A conta local funciona sem internet; a resolução por IA exige o serviço configurado.',[
 h('Faça a primeira conta'),steps('Ligue pela chave física. Se a tela apenas foi apagada por SHIFT, AC, pressione MODE para despertar.','Pressione AC. Digite 2 + 3 x 4 e depois =. O resultado esperado é 14.','Para uma nova conta, digite um número. Para continuar usando o resultado, pressione um operador.'),
 h('Resolva a primeira questão com foto'),steps('Conecte a calculadora a uma rede de internet 2,4 GHz pelo portal do celular: páginas 15 e 16. O fornecedor precisa ter configurado o serviço de IA.','Na tela de contas, aponte a câmera ao enunciado. Pressione SHIFT, direita. Aguarde a captura terminar e um quadradinho ficar cheio.','Abra MODE, selecione ENVIAR FOTOS IA com as setas e pressione =. Confirme com = novamente.','Aguarde a IA. Para consultar, use MODE > ULTIMA RESPOSTA > =. Avance com direita/baixo; volte com esquerda/cima. AC retorna às contas.'),
 note('Antes de desligar','Envie ou baixe as fotos que deseja guardar. As capturas ainda na fila ficam somente na memória temporária e se perdem ao cortar a energia.')])

page('Navegação, tela e atalhos','03 | CONTROLES',screens('controles',S('Menu com seleção invertida',['','MENU','PROCURAR WIFI','WIFI SALVO','CONFIG. CELULAR',''],(1,2)),S('Indicadores no cabeçalho',['','3+7','10','','',''],photos=2,wifi=True,flags='SAH')),
 'Simulações: o cabeçalho pode mostrar Wi-Fi, S, A, H e até três fotos.',[
 table(['Ação','Como fazer'],('Abrir / fechar o menu','MODE abre. AC ou MODE volta à calculadora.'),('Escolher uma opção','Setas percorrem a lista; = abre ou confirma.'),('Capturar rapidamente','Na tela de contas: SHIFT, direita.'),('Apagar a entrada atual','AC. Isso não remove as fotos.'),('Apagar a tela e desligar o Wi-Fi','SHIFT, AC. MODE desperta a tela; não corta a energia.'),('Zerar memórias locais','SHIFT, MODE. Limpa também estatística, pilha, histórico e Ans.')),
 h('Entenda os pequenos sinais'),p('A bateria à esquerda das fotos mostra de zero a quatro barras. ! pisca quando está baixa; ? indica falha na medição. <b>S</b>: SHIFT está armado; <b>A</b>: ALPHA; <b>H</b>: função hiperbólica. Os três quadrados representam fotos: vazios sem captura, preenchidos após a captura, com indicação piscante durante a operação. O ícone de Wi-Fi indica conexão à rede externa, e não apenas a existência de Calc-Lyke.'),
 note('SHIFT, AC não é desligamento elétrico','Essa combinação apaga a tela e manda o ESP desligar o Wi-Fi. Para desligar todo o aparelho, use a chave física. Ao reiniciar o ESP, Calc-Lyke volta a ficar disponível.')])

page('Contas comuns e correção','04 | CÁLCULO OFFLINE',screens('editar',S('Prioridade das operações',['','2+3*4','14','','','']),S('Use parênteses para agrupar',['','(2+3)*4','20','','',''])),
 'Simulações do modo algébrico. Use ponto para decimais e x para multiplicar.',[
 steps('Digite a expressão usando números, +, -, x, / e parênteses. Use multiplicação explícita: 2 x (3 + 4).','Pressione = para calcular. Multiplicação e divisão têm prioridade sobre soma e subtração. Parênteses mudam a ordem.','Durante a edição, esquerda/direita move o cursor. DEL apaga o elemento anterior; uma função como sin( é apagada como um elemento.','Para rever contas, use cima/baixo no REPLAY. Há até oito expressões no histórico temporário. Depois de um resultado, DEL ou esquerda/direita recupera a expressão para editar.','SHIFT, DEL alterna inserir e sobrescrever. AC limpa a entrada para começar novamente.'),
 h('Continuar com o resultado'),p('Após 14, pressione x 2 = para obter 28. O firmware usa Ans para continuar a conta. Para usar o resultado em outra posição, pressione a tecla Ans. Digitar um algarismo após um resultado inicia uma expressão nova.'),
 table(['Exemplo digitado','Resultado'],('1 . 5 + 2 . 2 5 =','3.75'),('( (-) 2 ) ^ 2 =','4'),('(-) 2 ^ 2 =','-4: o sinal fica fora da potência')),
 note('Limites de entrada','A expressão comporta até 95 elementos internos; funções contam como um elemento. Se aparecer EXPRESSAO CHEIA, divida o cálculo. Se faltar um fechamento, use ) antes de =.')])

page('Ângulo e formato dos números','05 | PREFERÊNCIAS',screens('ajustes',S('Mude com as setas e =',['','MODO CIENTIFICO','ANGULO  DEG','FORMATO NORM 6','ENTRADA ALGEBRICA',''],(1,2)),S('Exemplo com duas casas',['','MODO CIENTIFICO','ANGULO  DEG','FORMATO FIX 2','ENTRADA ALGEBRICA',''],(1,3))),
 'Simulações do menu MODO CIENTIFICO. Saia com AC ou MODE para salvar.',[
 steps('Abra MODE > MODO CIENTIFICO > =. Use cima/baixo para escolher ANGULO, FORMATO ou ENTRADA.','Na linha ANGULO, esquerda/direita ou = alterna DEG, RAD e GRA. DEG usa graus; RAD usa radianos; GRA usa grados (400 em uma volta).','Na linha FORMATO, alterne NORM, FIX, SCI e ENG. Ainda nessa linha, pressione um número de 0 a 9 para ajustar a precisão.','Na linha ENTRADA, escolha ALGEBRICA para o uso comum ou RPN para pilha. Saia com AC ou MODE para gravar as preferências.'),
 table(['Formato','O que muda'],('NORM','Exibição geral: decimal ou científica conforme a ordem de grandeza.'),('FIX 2, por exemplo','Mostra duas casas decimais, quando o valor cabe nesse formato.'),('SCI 2, por exemplo','Notação científica com duas casas na mantissa: 1.23E+4.'),('ENG','Expoente múltiplo de 3: 12.345E+3. A tecla ENG alterna ENG/NORM.')),
 p('O atalho SHIFT, Ans alterna o ângulo. Para garantir que essa mudança sobreviva a um desligamento, entre em MODO CIENTIFICO e saia salvando. O formato arredonda a apresentação; não transforma a calculadora em um sistema de precisão exata.')])

page('Potências, raízes e logaritmos','06 | FUNÇÕES CIENTÍFICAS',screens('potencias',native('Raiz sobre toda a expressão','tela_raiz'),native('Expoente elevado','tela_potencia')),
 'Telas do código do LCD: raiz desenhada com barra e expoente elevado. Feche o argumento com ) antes de calcular.',[
 table(['Função','Sequência de exemplo','Resultado'],('Quadrado / cubo','5, x^2, = / 2, x^3, =','25 / 8'),('Potência / recíproco','2, ^, 5, = / 4, x^-1, =','32 / 0.25'),('Raiz quadrada','sqrt, 8, 1, ), =','9'),('Raiz cúbica','SHIFT, x^3, 2, 7, ), =','3'),('Raiz de índice n','3, SHIFT, ^, 2, 7, =','3: raiz cúbica de 27'),('Logaritmo decimal','log, 1, 0, 0, ), =','2'),('Logaritmo natural','ln, ALPHA, ln, ), =','1: ln(e)'),('Potências 10^x e e^x','SHIFT, log / SHIFT, ln','Digite o argumento e feche )'),('Fatorial','5, SHIFT, x^-1, =','120')),
 h('Números grandes e pequenos'),p('EXP insere o expoente de base 10. Digite <b>6 . 0 2 EXP 2 3 =</b> para 6.02 x 10^23. Para 1.2 x 10^-3, use 1 . 2 EXP (-) 3 =. EXP não significa e elevado a x; essa função é SHIFT, ln.'),
 note('Quando houver erro de domínio','Raiz quadrada de número negativo e ln de zero/negativo não dão resultados reais. O fatorial aceita inteiros de 0 a 170. Esta calculadora local trabalha com números reais, não com álgebra simbólica ou números complexos.')])

page('Trigonometria e coordenadas','07 | FUNÇÕES CIENTÍFICAS',screens('trig',S('Em DEG',['','sin(30)','0.5','','','']),S('Polar: raio e ângulo',['','Pol(3,4)','5','Y=53.13010235','',''])),
 'Simulações. Em Pol, a segunda linha de resultado mostra o ângulo; em Rec, mostra y.',[
 steps('Escolha o ângulo em MODE > MODO CIENTIFICO. Para sin(30) = 0.5, use DEG.','Pressione sin, cos ou tan, digite o argumento, feche ) e pressione =. Para arco-seno, arco-cosseno ou arco-tangente, use SHIFT antes da função.','Para sinh, cosh ou tanh, pressione hyp e depois a função. Para a inversa hiperbólica, a ordem é hyp, SHIFT, sin/cos/tan. O H desaparece quando a função é inserida.'),
 table(['Conversão','Digite','Leia'],('Cartesiana para polar','Pol( 3 , 4 ) =','Resultado: r=5. Linha Y: ângulo=53.1301... em DEG.'),('Polar para cartesiana','SHIFT, Pol( 5 , 53.13010235 ) =','Resultado: x aproximadamente 3. Linha Y: y aproximadamente 4.')),
 p('Pol(x,y) calcula raio e ângulo; Rec(r,ângulo) faz a conversão inversa. Os dois valores ficam também nas variáveis X e Y. Nas funções hiperbólicas, o argumento é um número real; o modo DEG/RAD não muda sinh, cosh ou tanh.'),
 note('Verifique o modo angular','Se o resultado parecer estranho, confira DEG/RAD/GRA antes de refazer a conta. tan(90) em DEG é indefinida; arco-seno e arco-cosseno reais exigem argumento entre -1 e 1.')])

page('Frações, porcentagem e graus','08 | FORMATOS E COMBINAÇÕES',screens('formatos',native('Frações de entrada e resultado','tela_fracoes_resultado'),S('Graus, minutos e segundos',['','12DMS30DMS0DMS','12.5','','',''])),
 'Simulações; DMS representa a tecla com os símbolos de graus, minutos e segundos.',[
 table(['O que fazer','Como usar'],('Digitar uma fração','1, ab/c, 3 funciona como 1/3. Use parênteses em numeradores compostos.'),('Mostrar o resultado em fração','Depois de =, pressione ab/c. Pressione novamente para voltar. SHIFT, ab/c também alterna a apresentação.'),('Porcentagem','SHIFT, = insere %; depois = calcula. % significa dividir o valor anterior por 100.'),('Entrada DMS','12, DMS, 30, DMS, 0, DMS, = resulta em 12.5 graus.'),('Saída DMS','Após calcular 12.5, pressione DMS: aparece 12:30:00. DMS novamente retorna ao decimal.'),('Combinações e arranjos','5, nCr, 2, = dá 10. 5, SHIFT, nCr, 2, = dá 20.')),
 h('Porcentagem sem surpresa'),p('Para calcular 10% de 200: <b>200 x 10, SHIFT, =, =</b> resulta em 20. Para acrescentar 10%: <b>200 x (1 + 10, SHIFT, =, ), =</b> resulta em 220. A expressão 200 + 10% resulta em 200.1; não há acréscimo comercial automático.'),
 note('A fração é uma forma de exibir o número','O resultado é calculado numericamente. A exibição tenta encontrar uma fração com denominador até 9999; nem todo decimal aparecerá como fração. Em nCr/nPr, use n e r inteiros, com 0 <= r <= n; há limites de faixa.')])

page('Memórias, constantes e aleatórios','09 | REUTILIZE VALORES',screens('memorias',S('Guardar o valor em A',['','12','12','','STO: VARIAVEL','']),S('Escolher uma constante',['','CONSTANTES','PI','EULER','C  LUZ',''],(1,2))),
 'Simulações. RCL recupera; SHIFT, RCL prepara o armazenamento na variável escolhida.',[
 steps('Para guardar 12 em A: digite 12, pressione SHIFT, RCL e depois a tecla (-), associada a A.','Para recuperar A: pressione RCL, (-), =. Para usar A numa expressão, pressione ALPHA, (-).','M+ soma o valor atual à memória M. SHIFT, M+ subtrai. Para ler M, use RCL, M+, =.'),
 table(['Variável','Tecla depois de ALPHA, RCL ou STO'],('A / B / C','(-) / DMS / hyp'),('D / E / F','sin / cos / tan'),('X / Y / M',') / , / M+'),('Δ / θ / Ω','^ / log / EXP')),
 p('MODE > CONSTANTES oferece pi, Euler e, velocidade da luz c=299792458 m/s, gravidade padrão g0=9.80665 m/s^2, Avogadro NA=6.02214076E23 mol^-1 e Boltzmann kB=1.380649E-23 J/K. Selecione e pressione = para inserir. A legenda G GRAVIDADE significa g0, não a constante gravitacional universal G.'),
 p('SHIFT, EXP insere pi; ALPHA, ln insere e. SHIFT, ponto insere Ran#: valores pseudoaleatórios de 0.000 a 0.999. SHIFT, 0 insere Rnd(, que arredonda para cerca de dez algarismos significativos.'),
 note('Zerar tem efeitos amplos','SHIFT, MODE limpa as variáveis, Ans, estatística, histórico e pilha RPN. Essas memórias também se perdem ao cortar a energia. A sequência gravada de programa e as preferências salvas são preservadas.')])

page('Estatística e regressão','10 | ANÁLISE DE DADOS',screens('estatistica',S('Conjunto 2, 4, 6',['','ESTATISTICA','N=3','MEDIA X','4',''],(1,)),S('Pares (1,3), (2,5), (3,7)',['','ESTATISTICA','N=3','REGRESSAO B','2',''],(1,))),
 'Simulações. A estatística usa uma coleção temporária até zerar memórias ou desligar.',[
 steps('Antes de um conjunto novo, use SHIFT, MODE. Atenção: isso também limpa as outras memórias locais.','Para os valores 2, 4 e 6: digite 2 e pressione SHIFT, vírgula. AC volta à conta. Repita com 4 e depois 6. Não é necessário pressionar = antes de registrar.','Entre em MODE > ESTATISTICA. Percorra as métricas com as setas. Para esse conjunto: N=3, média=4, desvio amostral=2, desvio populacional aproximadamente 1.63299, soma=12 e soma dos quadrados=56.','Para regressão, reinicie a coleção e registre pares: 1, vírgula, 3, SHIFT, vírgula; AC; depois (2,5) e (3,7) da mesma forma. A vírgula sem SHIFT separa x e y.'),
 p('Métricas disponíveis: MEDIA X, MEDIA Y, DESVIO X AMOSTRA, DESVIO X POP., SOMA X, SOMA X^2, REGRESSAO A, REGRESSAO B, CORRELACAO R e SOMA XY. A reta é <b>y = A + B*x</b>. Nos três pares acima, A=1, B=2 e R=1.'),
 p('Nas seis primeiras métricas, = copia um valor válido para Ans e retorna à conta. SHIFT, 1 abre SOMA X; SHIFT, 2 abre MEDIA X. A regressão e a correlação são consultadas na tela, sem esse atalho de cópia.'),
 note('Se aparecer SEM DADOS','Pode faltar amostra ou variação: desvio amostral precisa de pelo menos dois valores; regressão precisa de variação em x. Não misture valores simples e pares na mesma coleção. Não há remoção individual exposta no teclado nesta revisão.')])

page('RPN: cálculo por pilha','11 | USO OPCIONAL',screens('rpn',S('Escolha RPN nas preferências',['','MODO CIENTIFICO','ANGULO  DEG','FORMATO NORM 6','ENTRADA RPN',''],(1,4)),S('Após 2 ENTER, 3 ENTER, +',['','','5','','',''])),
 'Simulações. No modo RPN, a tecla = atua como ENTER para empilhar.',[
 p('Se você está começando, mantenha ENTRADA ALGEBRICA. RPN é uma alternativa para quem prefere informar os valores antes do operador. A pilha guarda até 16 valores.'),
 steps('Abra MODE > MODO CIENTIFICO, escolha ENTRADA RPN e saia com AC para salvar.','Para 2+3: digite 2, =, 3, =, +. O topo passa a ser 5.','Para (2+3)x4, continue: 4, =, x. O resultado será 20.','Para subtração e divisão, a ordem importa: 10, =, 2, =, / dá 5. O segundo valor fica no topo; o operador usa o valor anterior como primeiro operando.','Funções de um valor, como x^2, atuam no topo. Com a entrada vazia, DEL remove o topo. SHIFT, MODE zera a pilha e as demais memórias locais.'),
 table(['Mensagem','O que significa'],('PILHA INSUFICIENTE','O operador precisa de mais valores. Empilhe-os com =.'),('PILHA CHEIA','Há 16 valores. Use operações ou DEL para liberar espaço.'),('Voltar ao modo comum','MODO CIENTIFICO > ENTRADA ALGEBRICA; AC salva.')),
 note('Observe qual modo está selecionado','O cabeçalho não mostra um indicador permanente ALG/RPN. Se = e os operadores parecerem agir de outra forma, confira ENTRADA em MODO CIENTIFICO.')])

page('Grave uma sequência de teclas','12 | PROGRAMA',screens('programa',S('Antes de gravar',['','PROGRAMA','GRAVAR NOVO','EXECUTAR','APAGAR',''],(1,2)),S('Finalize a gravação',['','PROGRAMA','SALVAR GRAVACAO','EXECUTAR','APAGAR',''],(1,2))),
 'Simulações. Há um programa salvo de até 96 teclas; ele é uma repetição de teclas, sem condições ou laços.',[
 steps('Abra MODE > PROGRAMA > GRAVAR NOVO > =. A entrada é limpa e a calculadora passa a registrar as teclas.','Faça um exemplo simples: 2 + 3 =. A indicação GRAVANDO PROGRAMA identifica a gravação.','Para terminar, pressione MODE, entre em PROGRAMA e escolha SALVAR GRAVACAO. Pressione =.','Para repetir, abra PROGRAMA > EXECUTAR > =. A sequência salva é reproduzida e o exemplo resulta em 5.','Para excluir a sequência, escolha PROGRAMA > APAGAR > =. Para substituí-la, grave uma nova e salve.'),
 p('A sequência salva permanece após desligar, assim como as preferências científicas. Ao atingir o limite de gravação, o firmware encerra e salva. Use sequências de cálculo simples e confira o resultado antes de depender delas.'),
 note('O programa não automatiza a câmera ou a internet','Atalhos de captura e apagamento de tela não são reproduzidos como ações de rede pelo programa. Esta função não é uma linguagem de programação: ela repete teclas do cálculo local.')])

page('Entenda as duas conexões Wi-Fi','13 | INTERNET E CELULAR',flow('duas-redes','A calculadora usa duas conexões diferentes',[
 ('Seu celular','Conecta em Calc-Lyke'),('Calculadora','Abre o portal local'),('Roteador 2,4 GHz','Leva a calculadora à internet'),('Serviço de IA','Recebe e resolve as fotos')],
 'O celular configura pelo portal; quem precisa de internet para enviar as fotos é a calculadora.'),
 'Diagrama conceitual. Calc-Lyke é uma rede local de configuração, não um plano de internet.',[
 table(['Conexão','Finalidade'],('Celular > Calc-Lyke','Abrir o painel em http://192.168.8.1, configurar e baixar fotos.'),('Calculadora > roteador ou hotspot','Acessar o servidor e a OpenAI. Use Wi-Fi 2,4 GHz.'),('Ícone no LCD','Indica que a calculadora está conectada à rede externa.')),
 steps('Ligue a calculadora. Em R8, Calc-Lyke abre automaticamente ao iniciar o ESP e fica disponível até você desligar o Wi-Fi pelo menu.','No celular, abra Ajustes/Configurações > Wi-Fi, escolha Calc-Lyke e informe a senha entregue pelo fornecedor.','Se aparecer Sem internet, escolha permanecer conectado. Abra o navegador e digite exatamente http://192.168.8.1, incluindo http://.','Se o celular sair dessa rede sozinho, desative temporariamente a troca automática para dados móveis ou outra rede. Tente novamente.'),
 note('Hotspot do celular','Para usar o mesmo celular como roteador e configurador, siga as possibilidades do sistema dele. É mais simples fazer o primeiro cadastro com outro celular/computador conectado a Calc-Lyke, enquanto o primeiro fornece o hotspot 2,4 GHz.')])

page('Cadastre sua rede de internet','14 | PORTAL DO CELULAR',panel('portal-wifi','Calc-Lyke | Wi-Fi','Simulação do portal local: http://192.168.8.1',[
 ('Procurar redes','Selecione uma rede encontrada'),('Nome da rede','SUA_REDE_2G'),('Senha','Digite a senha do seu roteador'),('Salvar e conectar','Depois, confirme o status da conexão'),('Rede salva / Conectar','Para reutilizar um cadastro')],
 'Campos ilustrativos. Nenhuma senha real está impressa neste guia.'),
 'Simulação da seção Wi-Fi. A aparência pode variar conforme o navegador.',[
 steps('Com o portal aberto, na seção Wi-Fi toque em Procurar redes. Aguarde a lista e selecione a sua rede. Confira Nome da rede.','Digite a senha do roteador e toque em Salvar e conectar. Aguarde a confirmação de rede externa conectada. A senha do roteador é diferente da senha de Calc-Lyke.','Para uma rede já cadastrada, escolha Rede salva e toque em Conectar. No teclado, o equivalente é MODE > WIFI SALVO > selecionar a rede > =.','Para procurar pelo teclado, use MODE > PROCURAR WIFI. A lista mostra redes encontradas; escolher uma abre a orientação para configurar no celular. A senha é cadastrada no portal.','Depois de uma conexão bem-sucedida, o ESP tenta a última rede usada ao reiniciar. Se a senha mudar, faça o cadastro novamente pelo portal.'),
 note('A rede não aparece?','Confira se ela oferece 2,4 GHz. A ESP32 clássica não se conecta ao Wi-Fi de 5 GHz. Redes com página de login, como hotéis e escolas, podem exigir configuração adicional que este portal não oferece.')])

page('Fotografe um enunciado legível','15 | CÂMERA',flow('capturar','Prepare a imagem antes de gastar uma consulta',[
 ('Enquadre','Uma questão por sessão'),('Capture','SHIFT e depois direita'),('Espere','O quadrado fica cheio'),('Confira','Abra a foto no celular')],
 'A captura é local. O envio à IA é uma etapa separada e explícita.'),
 'Fluxo ilustrativo. A orientação da lente depende da montagem fornecida.',[
 steps('Ilumine a folha sem reflexo e limpe a lente com cuidado. Posicione a calculadora para que a câmera veja o enunciado completo, incluindo unidades, tabelas e figura.','Na tela de cálculo, pressione SHIFT e depois direita. Também pode usar MODE > TIRAR FOTO > = ou Tirar foto no portal.','Aguarde CAPTURANDO FOTO terminar. Um quadradinho preenchido indica uma foto guardada. Não repita a tecla enquanto ESP OCUPADO aparecer.','Se precisar, capture uma segunda e uma terceira foto. Use-as como partes da mesma questão: enunciado, figura e detalhes. Não há obrigação de usar três.','Confira as imagens em VER FOTOS NO WIFI. Remova uma captura ruim antes de enviar. Evite texto muito pequeno, movimento, sombra e página cortada.'),
 h('Ajustes pelo celular'),p('Na seção Fotos do portal, Espelhar e Inverter na vertical corrigem a orientação; Autofoco habilita o recurso do módulo compatível. Toque em Salvar câmera e refaça a captura para avaliar. Não há prévia de vídeo ao vivo no uso normal; a câmera é acionada para a foto e depois pausada.'),
 note('Capacidade e duração','São até três JPEGs na fila, com limite de 512 KiB por foto e 1,5 MiB no conjunto. Elas ficam na RAM: desligar ou reiniciar apaga as fotos ainda não enviadas.')])

page('Veja, baixe ou descarte fotos','16 | GALERIA LOCAL',panel('portal-fotos','Calc-Lyke | Fotos','Conecte o celular a Calc-Lyke e abra http://192.168.8.1',[
 ('Tirar foto','Faz mais uma captura, até 3'),('Recarregar lista','Atualiza a galeria temporária'),('Abrir foto 1','Abre o JPEG original no navegador'),('Espelhar / Inverter na vertical','Ajuste a orientação e salve'),('Autofoco / Salvar câmera','Use conforme o módulo instalado')],
 'SIMULAÇÃO | A galeria temporária é diferente do arquivo privado na nuvem.'),
 'Simulação da seção Fotos. As imagens reais são abertas pelos links da galeria.',[
 steps('No teclado, selecione MODE > VER FOTOS NO WIFI > =. Conecte o celular a Calc-Lyke e abra o endereço mostrado.','Na seção Fotos, toque em Recarregar lista e em Abrir foto 1, 2 ou 3. Amplie para conferir se o texto pode ser lido.','Para salvar uma cópia no celular, use o comando do navegador para baixar/salvar a imagem. O nome e a posição do comando variam entre Android e iPhone.','Para apagar só a captura mais recente da fila, use MODE > REMOVER ULT. FOTO > =. Para descartar a fila toda, use MODE > LIMPAR FOTOS > =. Essas ações não são uma lixeira recuperável.','Depois de uma resolução bem-sucedida, a fila local é esvaziada. As fotos enviadas ficam no arquivo privado do serviço; veja a página 20.'),
 note('Sem internet ainda é possível baixar','A visualização local funciona pela rede Calc-Lyke sem crédito de API e sem internet externa. Baixar antes de desligar é a forma mais simples de guardar uma foto que você não quer enviar à IA.')])

page('Envie à IA e leia a resposta','17 | RESOLUÇÃO',screens('resposta',S('Uma confirmação antes do envio',['','ENVIAR 2 FOTO(S)?','= ENVIA A IA','AC CANCELA','',''],(1,),2,True),S('Seis linhas só de resolução',['2*x+3=11','2*x=11-3','2*x=8','x=8/2','x=4','2*4+3=11'],header=False),S('Avance com direita ou baixo',['CONFERENCIA','2*4+3=11','8+3=11','11=11','RESULTADO: x=4',''],header=False)),
 'Resposta ilustrativa para 2*x+3=11; não é uma transcrição do LCD do teste físico.',[
 steps('Confira conexão externa, serviço de IA e uma a três fotos da mesma questão. Abra MODE > ENVIAR FOTOS IA > =.','Leia a quantidade e confirme com =. AC cancela antes de enviar. A tela indica AGUARDANDO IA; mantenha a energia e aguarde.','Abra MODE > ULTIMA RESPOSTA > = para consultar a resposta disponível. Use direita/baixo para avançar uma página e esquerda/cima para voltar. AC ou MODE retorna ao cálculo.','Se preferir uma tela maior, no portal toque em Abrir última resposta. Para rever mais tarde, use novamente ULTIMA RESPOSTA; a última resposta fica salva no ESP.'),
 p('A resposta usa as seis linhas de 21 caracteres, sem cabeçalho, ícones ou título de página. O serviço pede fórmulas, substituições e desenvolvimento completo, sem conversa desnecessária. No leitor da IA, x^2 significa quadrado, sqrt(...) significa raiz e * significa multiplicação. Esse leitor usa texto; o desenho gráfico de fórmulas pertence à calculadora local.'),
 note('Se a IA pedir outra foto','Confira se faltou parte do enunciado ou se a imagem ficou ilegível. Remova/refaça a captura e envie novamente. Compare a transcrição dos dados com o papel: a IA pode errar leitura, unidade ou cálculo. Uma nova consulta pode gerar custo.')])

page('Onde suas fotos ficam guardadas','18 | ARQUIVO E PRIVACIDADE',flow('arquivo','Da foto temporária ao arquivo privado',[
 ('Captura','RAM do aparelho'),('Envio','Servidor recebe os JPEGs'),('Arquivo privado','Fotos são guardadas'),('Resposta','IA resolve; texto é salvo')],
 'No serviço configurado, a gravação das fotos antecede a chamada à IA.'),
 'Fluxo do serviço testado. Não há galeria pública nem compartilhamento automático de links.',[
 table(['Situação','O que fica disponível'],('Capturou, mas não enviou','Só a fila temporária. Baixe pelo portal antes de desligar.'),('Envio chegou ao arquivo','JPEGs no bucket privado lyke-images. Uma falha antes dessa etapa pode impedir o arquivo.'),('IA respondeu com sucesso','Fotos e resposta no serviço; última resposta também no ESP. A fila local é limpa.'),('IA falhou depois do arquivo','As fotos já gravadas permanecem na nuvem. A fila local é mantida para nova tentativa.'),('REMOVER ou LIMPAR no menu','Descarta a fila do ESP. Não apaga o histórico do servidor.')),
 h('Como obter as cópias'),steps('Para imagens ainda na fila, baixe pela galeria local no celular.','Se você administra seu próprio projeto Supabase: entre no projeto, abra Storage > lyke-images, localize a pasta do dispositivo e da consulta e baixe os JPEGs. A tabela lyke_requests registra a consulta e os caminhos.','Se contratou o serviço do fornecedor, solicite acesso, exportação ou exclusão a ele. O portal atual não oferece uma galeria completa de consultas antigas nem exclusão do arquivo da nuvem.'),
 p('Private significa acesso controlado, não ausência de processamento: as imagens são enviadas ao servidor e ao provedor de IA para resolução. Prefira fotografar apenas o exercício, sem rostos, documentos ou dados pessoais desnecessários. [S8]')])

page('API: entenda antes de configurar','19 | CONTA E SERVIÇO',flow('chaves','Três coisas diferentes precisam combinar',[
 ('Sua conta de IA','Crédito e chave do provedor'),('Servidor do serviço','Guarda a chave e o arquivo'),('Calculadora','Recebe endereço e acesso próprio')],
 'A chave OpenAI fica no servidor. A calculadora guarda uma credencial diferente.'),
 'Diagrama de configuração. Uma conta logada no ChatGPT, sozinha, não configura a calculadora.',[
 table(['Nome','Onde vai'],('Chave da API OpenAI','Segredo OPENAI_API_KEY no servidor do seu serviço.'),('Endereço do serviço','Campo Endereço do serviço no portal Calc-Lyke.'),('Chave de acesso da calculadora','Campo com esse nome no portal; emitida pelo serviço para o aparelho.')),
 h('Escolha seu caminho'),p('<b>Serviço já entregue pelo fornecedor:</b> você usa o endereço e a credencial recebidos. Não precisa criar um Supabase nem pagar a API diretamente, salvo o combinado na compra. Pule para a conexão da página 24.'),
 p('<b>Quero usar minha própria conta OpenAI:</b> crie a conta e a chave nas páginas seguintes. Você ou o fornecedor também precisa administrar uma instância do servidor compatível. Criar a chave e comprar crédito não substitui essa instalação.'),
 note('ChatGPT e API são produtos com cobrança própria','Estar logado no ChatGPT não cria uma chave de API. Confira a organização e o saldo na plataforma de API. Eventuais benefícios de créditos vinculados a planos exigem as condições e a vinculação oferecidas pela plataforma. [S1, S2]'),
 p('O serviço desta edição usa OpenAI Responses e armazenamento privado no Supabase. Não existe seletor de provedor no portal R8. Não cole uma chave sk-... ou sk-ant-... no campo de acesso da calculadora.')])

page('OpenAI: conta e crédito da API','20 | PASSO A PASSO OPENAI - 1/3',panel('openai-conta','OpenAI Platform | conta e cobrança','SIMULAÇÃO DIDÁTICA - nomes e menus podem mudar',[
 ('1. Entrar','platform.openai.com'),('2. Organização / projeto','Selecione onde o serviço será usado'),('3. Billing','Confira crédito de API disponível'),('4. Add payment details / Buy credits','Revise valor e recarga automática'),('5. Usage','Acompanhe o consumo')],
 'Não é uma tela real da conta. Nenhum dado de pagamento foi reproduzido.'),
 'Simulação de navegação. Consulte Billing na organização correta.',[
 steps('Abra https://platform.openai.com e entre ou crie sua conta. Conclua as verificações solicitadas.','Selecione a organização e o projeto que serão usados pelo serviço da sua calculadora. Anote o nome, sem publicar dados da conta. [S2]','Abra a seção Billing. Confira se há crédito de API utilizável ou benefício elegível já vinculado à organização. A assinatura do aplicativo ChatGPT não configura isso automaticamente. [S1]','Se necessário, adicione a forma de pagamento e compre créditos. Confira valor, moeda, impostos e a opção Use auto-reload antes de confirmar. Deixe recarga automática desligada se não desejar compras futuras automáticas. [S3]','Aguarde o saldo aparecer na mesma organização. Depois acompanhe Usage e os controles de cobrança disponíveis. Um limite de recarga não é um teto instantâneo de uso. [S3]'),
 note('Você pode já ter saldo','Não compre novamente só porque o aparelho deu erro. Primeiro confira organização, saldo, chave e serviço. Há erros de Wi-Fi e configuração que não são resolvidos com mais crédito.'),
 p('Links oficiais: platform.openai.com e help.openai.com. Não é necessário assinar o aplicativo de conversa apenas para seguir este fluxo de API.')])

page('OpenAI: crie e proteja a chave','21 | PASSO A PASSO OPENAI - 2/3',panel('openai-chave','OpenAI Platform | API keys','SIMULAÇÃO - o segredo é exibido aqui somente como campo fictício',[
 ('1. Projeto','Escolha o projeto do serviço'),('2. API keys','Criar nova chave secreta'),('3. Nome','Calculadora - serviço de IA'),('4. Secret key','[CHAVE FICTÍCIA - NÃO UTILIZÁVEL]'),('5. Destino','Servidor: OPENAI_API_KEY')],
 'Não envie a chave por uma conversa comum, não a coloque em PDF nem em repositório.'),
 'Simulação da criação de chave. A permissão para criar chaves depende do papel da sua conta.',[
 steps('No projeto correto, abra API keys. Use a ação de criar uma nova chave secreta; o nome exato do botão pode variar.','Dê um nome identificável, como Calculadora - serviço de IA. Escolha as permissões necessárias ao servidor; ele precisa chamar a API de respostas com texto e imagens. [S2, S4]','Copie a chave quando for mostrada e guarde-a em um gerenciador de segredos. Se perder o segredo, crie outra chave; não conte com recuperar o valor completo depois. [S2]','Instale a chave no servidor conforme a página 24. Para uma instância compartilhada/administrada pelo fornecedor, combine um canal seguro ou faça você mesmo o cadastro do segredo.','Se a chave vazar, revogue-a na plataforma, crie outra e atualize o servidor. Remover o texto de um arquivo público não revoga uma chave já exposta. [S2]'),
 note('Não cole aqui no aparelho','A caixa Chave de acesso da calculadora não recebe a chave OpenAI. Essa separação permite substituir a chave do provedor sem regravar cada calculadora.'),
 p('A escolha do modelo fica no serviço, não na criação da chave. Nesta edição, o principal é GPT-6.1 Terra. A página 27 explica o modo econômico e o custo.')])

page('Ative o serviço e faça o teste','22 | PASSO A PASSO OPENAI - 3/3',panel('portal-api','Calc-Lyke | IA e arquivo de fotos','SIMULAÇÃO - dados de exemplo, sem endereço de uma conta real',[
 ('Endereço do serviço','https://SEU-PROJETO.supabase.co/functions/v1/lyke-solve'),('Chave de acesso da calculadora','[CREDENCIAL DO APARELHO, NÃO DA OPENAI]'),('Modo econômico','Desmarcado para o modelo principal'),('Salvar configuração','Grava o endereço e a credencial no ESP'),('Resolver fotos guardadas','Envia a fila para o serviço')],
 'O fornecedor deve entregar um serviço já implantado e uma credencial válida para sua unidade.'),
 'Simulação do formulário real. Não há campo de seleção OpenAI/Claude na R8.',[
 h('Se o serviço já está pronto'),steps('Abra http://192.168.8.1 conectado a Calc-Lyke. Na seção IA e arquivo de fotos, confira o Endereço do serviço fornecido.','Se for configurar/substituir o acesso, informe a Chave de acesso da calculadora entregue pelo administrador. Escolha se deseja Modo econômico e toque em Salvar configuração.','Conecte a calculadora à internet. Fotografe uma folha com 2*x+3=11. Confira a foto e envie uma vez. Verifique se a resposta encontra x=4 e confere a substituição. Essa consulta usa a API e pode ter custo.','Confira também Abrir última resposta e o arquivo da consulta. Se não responder, use a página 30 antes de repetir vários envios.'),
 h('Se você administra a instância'),p('Primeiro implante a função lyke-solve, as migrações e o bucket privado do projeto. No Supabase do seu projeto, em Edge Functions > Secrets, salve OPENAI_API_KEY. Configure também a credencial própria do aparelho, seu hash no serviço e os recursos de arquivo; uma chave OpenAI sozinha não cria esse servidor. O procedimento técnico e os arquivos de implantação ficam em docs/API-E-ARQUIVO.md e supabase/ no código-fonte. [S7]'),
 note('Verificação final do fornecedor','Entregue endereço, acesso exclusivo do aparelho, política de crédito/limite, suporte e forma de recuperar fotos. O comprador não precisa receber segredos administrativos do Supabase.')])

page('Claude: prepare conta e cobrança','23 | ALTERNATIVA CLAUDE - 1/2',panel('claude-conta','Claude Console | preparação da conta','ALTERNATIVA NÃO TESTADA NESTA CALCULADORA',[
 ('1. Entrar','platform.claude.com'),('2. Organização / workspace','Escolha o ambiente do serviço'),('3. Settings > Billing','Confira saldo e benefícios elegíveis'),('4. Buy credits','Revise e confirme somente se desejar'),('5. Situação na R8','Ainda requer integração do fornecedor')],
 'Criar conta e pagar créditos não adiciona suporte Claude ao firmware/servidor atual.'),
 'Simulação didática do Console Claude. Não é captura de uma conta real.',[
 note('Só OpenAI foi testada','O serviço entregue nesta edição chama a OpenAI. Não chama Claude. As etapas abaixo preparam a conta Anthropic para uma futura integração; não constituem ativação pronta na calculadora.'),
 steps('Antes de comprar crédito, confirme com o fornecedor se ele disponibilizará um servidor com suporte Claude e quando ele será validado.','Abra https://platform.claude.com e entre ou crie uma conta. Selecione a organização/workspace que receberá o serviço. [S5]','Abra Settings > Billing, ou https://platform.claude.com/settings/billing. Confira o saldo e eventuais créditos elegíveis vinculados a um plano; não presuma que estar logado no aplicativo Claude já habilitou a API. [S6]','Se optar por financiar a API, use Buy credits, confira o valor e confirme o pagamento. A conta precisa ter papel de cobrança adequado. Revise auto-reload para evitar compras automáticas indesejadas. [S6]'),
 p('Mantenha o serviço OpenAI funcionando até a alternativa estar implementada e testada. Os nomes e a localização dos menus do Console podem mudar; os links oficiais de referência estão ao final deste manual.')])

page('Claude: chave e o que falta ativar','24 | ALTERNATIVA CLAUDE - 2/2',panel('claude-chave','Claude Console | API keys','SIMULAÇÃO - ainda não há adaptador Anthropic no serviço R8',[
 ('API keys','platform.claude.com/settings/keys'),('Create key','Escolha nome, validade e conta vinculada'),('Workspace','Use o workspace do seu serviço'),('Chave criada','[CHAVE FICTÍCIA - NÃO UTILIZÁVEL]'),('Destino futuro','Segredo no servidor compatível com Claude')],
 'Não cole uma chave Anthropic no campo Chave de acesso da calculadora.'),
 'Simulação da criação de chave Claude. Somente a integração OpenAI foi testada.',[
 steps('Abra https://platform.claude.com/settings/keys. Clique em Create key. Informe nome e validade; escolha a conta vinculada e o workspace adequado. Para um serviço compartilhado, o administrador pode usar uma conta de serviço. [S5]','Copie o segredo quando ele for mostrado e guarde-o com segurança. A chave completa só aparece na criação; se perder, crie outra. Não a inclua em PDF, GitHub ou capturas de tela. [S5]','Peça ao fornecedor uma versão do servidor que aceite Anthropic Messages, envie os JPEGs ao modelo de visão escolhido e normalize a resposta para o protocolo da calculadora. É uma implementação necessária, não uma configuração disponível nesta R8. [S9]','Somente depois dessa implementação, instale o segredo no servidor, configure o endereço/credencial de serviço apropriados no aparelho e repita os testes de foto, resposta curta, paginação e arquivo privado.','Considere Claude disponível apenas quando esse teste passar na sua unidade. Até lá, use a integração OpenAI validada.'),
 note('Trocar apenas a chave não funciona','As APIs têm formatos diferentes. Não substitua OPENAI_API_KEY por uma chave sk-ant-... e não altere só o nome do modelo. A preferência por Claude exige o adaptador e sua validação, que não foram feitos nesta edição.')])

page('Modelos, custos e uso econômico','25 | CUIDE DO CONSUMO',flow('custos','Uma boa foto evita repetir uma consulta',[
 ('Confira a foto','Nitidez e enunciado inteiro'),('Escolha o modo','Principal ou econômico'),('Envie uma vez','Aguarde terminar'),('Acompanhe','Usage, saldo e limite')]),
 'Fluxo de uso. Capturar e baixar localmente não chama a API; enviar para resolver chama.',[
 table(['Modo atual','O que o serviço usa'],('Principal','GPT-6.1 Terra, com raciocínio alto.'),('Modo econômico','GPT-6 Luna, com raciocínio médio.'),('Onde mudar','Portal > IA e arquivo de fotos > Modo econômico > Salvar configuração.'),('Quem define outros modelos','Administrador do servidor, após verificar disponibilidade e compatibilidade.')),
 p('A preferência inicial por “6.2 Terra” resultou em GPT-6.1 Terra, modelo Terra disponível no catálogo consultado. O serviço mantém esse identificador; o nome não muda automaticamente quando surgem versões novas. A disponibilidade deve ser conferida na plataforma. [S10]'),
 p('O custo depende de modelo, imagens e tokens de entrada/saída. Não há preço fixo por exercício nem quantidade garantida de questões por compra de crédito. Consulte o preço e o consumo atuais na plataforma antes de definir seu orçamento. [S11]'),
 p('A configuração inicial do serviço limita novas consultas a 50 em uma janela móvel de 24 horas por aparelho; o administrador pode ajustar. Esse limite é do projeto, separado do saldo e dos limites da OpenAI. Reenvios da mesma consulta concluída podem recuperar o resultado em cache, mas uma nova consulta pode cobrar novamente.'),
 note('Evite repetir por impaciência','Se AGUARDANDO IA aparecer, aguarde. Em caso de erro, confira a mensagem antes de reenviar. O saldo pré-pago e os avisos de orçamento não devem ser tratados como corte instantâneo garantido de gastos. [S3]')])

page('Desligar, despertar e preservar dados','26 | ENERGIA E MEMÓRIA',battery_figure(),
 'Simulações do código real: o ícone fica antes dos três quadrados de fotos, na mesma janela visível.',[
 table(['Ação / dado','O que acontece'],('MODE > DESLIGAR WIFI','Desliga o Wi-Fi do ESP nesta sessão. O cálculo local continua.'),('SHIFT, AC','Limpa a entrada, apaga a tela e pede Wi-Fi desligado. MODE desperta a tela.'),('Reabrir o portal','MODE > CONFIG. CELULAR (ou VER FOTOS NO WIFI) solicita o portal novamente.'),('Reiniciar / chave física','Ao reiniciar o ESP, Calc-Lyke volta a abrir automaticamente.'),('Bateria no cabeçalho','Quatro níveis aproximados; ! pisca abaixo de cerca de 3,45 V. ? indica leitura inválida.'),('MODE > BATERIA','Mostra a tensão aproximada; = mede de novo. A leitura automática é renovada a cada 30 segundos.')),
 h('O que permanece ao desligar'),p('Permanecem o mapeamento de teclas, as preferências salvas, o programa salvo, as redes cadastradas, a configuração do serviço e a última resposta gravada no ESP. O arquivo do servidor também permanece conforme a política do serviço.'),
 h('O que se perde'),p('Perdem-se a entrada atual, Ans, memórias A-F/X/Y/M/Δ/θ/Ω, histórico, estatística, pilha RPN e fotos ainda na fila temporária. Antes de cortar energia, conclua o envio ou baixe as imagens e pare qualquer atualização em andamento.'),
 note('Alimentação e recarga','Use apenas o conector, a fonte e o procedimento entregues para sua unidade. As barras estimam a carga pela tensão; não são uma porcentagem exata nem indicam carregamento. A montagem de referência usa uma célula e divisor de 470 kΩ + 470 kΩ corrigido e conferido pelo proprietário. Para recarregar, siga a orientação específica da unidade; não use o ícone para decidir qual entrada alimentar.')])

page('Atualize somente com o arquivo correto','27 | MANUTENÇÃO PELO WI-FI',panel('atualizar','Calc-Lyke | Atualizar firmware','SIMULAÇÃO - arquivos devem ser fornecidos para sua unidade',[
 ('STM32','lyke-stm32-ota.bin'),('Atualizar STM32','Atualização por Wi-Fi validada nesta unidade'),('ESP32-CAM','esp32cam_integrada.ino.bin'),('Atualizar ESP32-CAM','OTA já utilizada e testada nesta montagem'),('Após reiniciar','Reconecte a Calc-Lyke e confira a revisão')],
 'Não use binário da GOOUUU ESP32-S3 nem uma imagem de flash completa neste formulário.'),
 'Simulação da seção de atualização. Há dois processadores e arquivos diferentes.',[
 steps('Solicite ao fornecedor o arquivo para o processador e a versão exatos de sua calculadora. Confirme o nome e, se fornecido, o hash do arquivo.','Mantenha alimentação estável e salve as fotos pendentes. A atualização pode reiniciar o aparelho e apagar a fila temporária.','Abra MODE > ATUALIZAR FIRMWARE. No celular, conecte em Calc-Lyke e acesse http://192.168.8.1.','Na seção Atualizar firmware, escolha o arquivo na linha correta: STM32 ou ESP32-CAM. Use o botão correspondente uma vez.','Aguarde a conclusão sem desligar nem trocar de rede. Após o reinício, reconecte o celular ao portal e confira a revisão e uma conta simples. O AP volta a abrir no boot do ESP.'),
 note('Não desligue ao chegar a 100%','A barra mede o envio ao ESP. Depois ainda há verificação e gravação. Aguarde a mensagem STM32 atualizado e reiniciado. A base V2 já está instalada nesta unidade; as próximas atualizações do STM dispensam abrir a carcaça ou acessar o USB.'),
 p('A identificação de 49 teclas no portal deve aparecer com o teclado concluído quando ESP e STM32 estão funcionando juntos. Em falha de atualização, anote a mensagem e procure suporte antes de tentar arquivos de outro hardware.')])

page('Se algo não funcionar','28 | SOLUÇÃO DE PROBLEMAS',screens('erros',S('Falha na comunicação com o ESP',['','CONFIG. CELULAR','ESP SEM CONEXAO','','AC PARA VOLTAR','= TENTAR NOVAMENTE'],(1,)),S('Ainda não há captura',['','','','','TIRE UMA FOTO',''])),
 'Simulações de mensagens. Anote o texto exato para o suporte.',[
 table(['Sintoma','O que tentar'],('Tela apagada','Pressione MODE uma vez. Confira a chave física e a alimentação fornecida.'),('ON não liga','É esperado: ON não está mapeada. Use a chave física; MODE desperta tela apagada.'),('Calc-Lyke sumiu','Reabra CONFIG. CELULAR. Se necessário, reinicie fora de captura/envio/atualização; fotos da fila serão perdidas.'),('192.168.8.1 não abre','Confirme conexão a Calc-Lyke, mantenha rede sem internet e use http://, não https://.'),('Wi-Fi externo não conecta','Confira rede 2,4 GHz, senha e status no portal. Cadastre de novo se a senha mudou.'),('ESP SEM CONEXAO','Pressione = para tentar. Se persistir, confira energia e procure suporte da montagem.'),('ESP OCUPADO / AGUARDANDO IA','Aguarde a operação atual. Evite várias confirmações de envio.'),('TIRE UMA FOTO','Capture ao menos uma foto antes de enviar. A fila pode ter sido limpa ou perdida no reinício.'),('Chave de acesso inválida','Confira a credencial do aparelho e o endereço do serviço com o administrador.'),('Sem crédito / limite diário','Confira Billing da API ou a janela de limite do serviço. Comprar crédito não remove o limite local.'),('Foto ruim / resposta inventada','Abra o JPEG, confira enquadramento, nitidez e dados transcritos; refaça a questão completa.'),('SEM RESPOSTA SALVA','Ainda não há resposta disponível. Faça uma consulta válida e aguarde a conclusão.')),
 p('Em erro matemático, confira parênteses, domínio, DEG/RAD e ENTRADA ALGEBRICA/RPN. Se aparecer MEMORIAS LIMPAS, a sequência SHIFT, MODE foi acionada; não é restauração de fábrica.')])

page('Todos os itens do menu','29 | CONSULTA RÁPIDA',screens('menu-indice',S('Entre com MODE e =',['','MENU','PROCURAR WIFI','WIFI SALVO','CONFIG. CELULAR',''],(1,2)),S('Setas mudam a seleção',['','MENU','PROGRAMA','AJUDA E TECLAS','',''],(1,3))),
 'Simulações. A seleção percorre as opções e retorna ao início ao passar do fim.',[
 table(['Item do menu','Função'],('PROCURAR WIFI','Lista redes externas; cadastro de senha pelo celular.'),('WIFI SALVO','Escolhe uma rede já cadastrada e solicita a conexão.'),('CONFIG. CELULAR','Abre/reabre o portal local Calc-Lyke.'),('TIRAR FOTO','Captura um JPEG para a fila temporária.'),('ENVIAR FOTOS IA','Abre confirmação; = envia 1 a 3 fotos.'),('ULTIMA RESPOSTA','Abre o texto salvo com paginação pelas setas.'),('VER FOTOS NO WIFI','Orienta acesso ao portal e à galeria local.'),('REMOVER ULT. FOTO','Descarta a foto mais recente da fila.'),('LIMPAR FOTOS','Descarta todas as fotos da fila.'),('ATUALIZAR FIRMWARE','Abre o acesso ao portal de atualização.'),('DESLIGAR WIFI','Desliga o rádio na sessão atual; reboot reativa AP.'),('BATERIA','Consulta tensão aproximada em PA0.'),('MODO CIENTIFICO','Ângulo, formato/precisão e entrada ALG/RPN.'),('ESTATISTICA','Médias, desvios, somas e regressão.'),('CONSTANTES','Insere uma das seis constantes.'),('PROGRAMA','Grava, executa ou apaga uma sequência de teclas.'),('AJUDA E TECLAS','Mostra lembretes de operação em três páginas.'))])

page('Teclas: funções científicas','30 | REFERÊNCIA COMPLETA - 1/2',screens('atalhos-cientificos',S('S indica SHIFT armado',['','25','','','',''],flags='S'),S('H indica função hiperbólica',['','','','','',''],flags='  H')),
 'Simulações. Pressione e solte SHIFT antes da tecla; hyp permanece armado até a função trigonométrica.',[
 table(['Tecla','Sem SHIFT','Depois de SHIFT'],('x^-1','Recíproco','Fatorial !'),('nCr','Combinação','Arranjo nPr'),('Pol(','Pol(x,y)','Rec(r,ângulo)'),('x^3','Cubo','Raiz cúbica cbrt('),('ab/c','Divisão fracionária; após = alterna fração','Alterna apresentação fracionária'),('sqrt','Raiz quadrada','Mesma raiz quadrada'),('x^2','Quadrado','Mesmo quadrado'),('^','Potência','Raiz de índice n'),('log','Logaritmo base 10','10^('),('ln','Logaritmo natural','exp('),('(-)','Sinal de menos','Mesmo sinal'),('DMS','Entrada DMS; após = alterna exibição','Alterna exibição decimal/DMS'),('hyp','Arma/desarma H','Também arma/desarma H'),('sin / cos / tan','Trigonométricas; com H, hiperbólicas','Inversas; com H, inversas hiperbólicas'),('EXP','Notação E; se vazio, começa por 1E','Constante pi'),('Ans','Último resultado','Alterna DEG / RAD / GRA')),
 p('ALPHA, ln insere a constante e. ALPHA,^ insere Δ; ALPHA,log insere θ; ALPHA,EXP insere Ω. São variáveis numéricas, assim como A-F/X/Y/M (página 11). O texto e os atalhos deste guia prevalecem sobre legendas secundárias herdadas da carcaça original.')])

page('Teclas: edição, memória e controle','31 | REFERÊNCIA COMPLETA - 2/2',screens('atalhos-controle',S('Uma foto pronta na fila',['','','','','',''],photos=1),S('Memórias zeradas',['','','','','MEMORIAS LIMPAS',''])),
 'Simulações. Limpar memórias e apagar fotos são comandos diferentes.',[
 table(['Tecla','Função principal / secundária'],('0 a 9','Algarismos. SHIFT, 0: Rnd(. SHIFT, 1: SOMA X. SHIFT, 2: MEDIA X. SHIFT nos demais: mesmo algarismo.'),('Ponto .','Decimal. SHIFT, ponto: Ran# pseudoaleatório.'),('+ / - / x / /','Operações aritméticas; SHIFT mantém a operação.'),('( e )','Parênteses; SHIFT mantém o símbolo.'),('Vírgula ,','Separa valores. SHIFT, vírgula: registra dado/par estatístico.'),('RCL','Recupera variável. SHIFT, RCL: STO (guardar). Depois escolha a tecla da variável.'),('M+','Soma valor à memória M. SHIFT, M+: subtrai de M.'),('ENG','Alterna ENG/NORM, com ou sem SHIFT.'),('=','Calcula; em RPN, empilha/ENTER. SHIFT, =: %.'),('DEL','Apaga elemento; após resultado, recupera conta. SHIFT, DEL: inserir/sobrescrever.'),('AC','Limpa entrada; em menus, volta. SHIFT, AC: tela apagada e Wi-Fi OFF.'),('MODE','Abre menu; sai de subpágina. SHIFT, MODE: zera memórias locais.'),('REPLAY esquerda/direita','Move cursor. SHIFT, direita, na conta: captura foto.'),('REPLAY cima/baixo','Histórico de expressões. Em menus e respostas, navega.'),('SHIFT / ALPHA','Arma/desarma modificador para próxima tecla; um cancela o outro.'),('ON','Sem função nesta unidade. Ligue pela chave física.')),
 note('Atalhos valem na tela de cálculo','Dentro dos menus, as teclas seguem as regras de navegação. Para usar SHIFT, direita ou outra função de cálculo, volte primeiro com AC.')])

page('Aprenda com uma IA usando o .md','32 | MATERIAL PARA CONTINUAR',flow('aprender','O manual também acompanha você em texto',[
 ('Abra o .md','No editor ou aplicativo de IA'),('Diga sua dúvida','Descreva tela e objetivo'),('Peça uma ação','Uma etapa por vez'),('Confira','Compare com o aparelho')],
 'O Markdown contém o mesmo conteúdo do PDF e usa a pasta manual-assets para as imagens.'),
 'Fluxo de aprendizado. Nunca envie chaves, senhas ou dados de pagamento para obter ajuda.',[
 h('Copie este pedido para começar'),note('Prompt de aprendizagem','Estou usando a Lyke modificada com STM32 R11 e ESP R8. Use o manual anexado como referência. Sou iniciante: explique uma ação por vez, diga exatamente qual tecla pressionar e o que devo ver. Diferencie captura, envio à IA e arquivo. Se a função não existir nesta versão, diga isso. Minha dúvida é: [descreva].'),
 h('O que anexar'),p('Anexe Manual-Lyke.md ao chat e, se houver suporte a imagens, uma foto da tela ou a figura pertinente do pacote. Explique em que passo parou e transcreva a mensagem. Para manter as figuras ao abrir o Markdown localmente, deixe manual-assets na mesma pasta do .md; o ZIP já tem essa estrutura.'),
 table(['Termo','Significado simples'],('Firmware','Programa instalado dentro da calculadora.'),('API','Forma de o servidor conversar com o provedor de IA.'),('Portal / AP','Página local e rede Calc-Lyke que permitem configurar.'),('Servidor / serviço','Sistema na internet que chama a IA e guarda o arquivo.'),('RAM / arquivo privado','Memória temporária do aparelho / armazenamento com acesso controlado.'),('Token / credencial','Segredo que identifica o acesso do aparelho ao serviço.')),
 p('Para suporte, informe revisão, mensagem, quantidade de fotos, status de rede e sequência de teclas. Não publique o segredo do aparelho ou da OpenAI. A IA ajuda a entender o manual; a confirmação do resultado deve vir da tela e dos testes da sua unidade.')])

page('Compatibilidade e entrega da unidade','33 | O QUE FOI VALIDADO',flow('validacao','Versão documentada nesta edição',[
 ('STM32','Lyke R11 - teclado e LCD'),('ESP32-CAM','Lyke R8 - câmera e Wi-Fi'),('OpenAI','Resposta real aceita'),('Arquivo','JPEG recuperado e conferido')]),
 'Estado do projeto em 23/11/2026. Esquema, implementação e teste físico são coisas diferentes.',[
 table(['Situação','Evidência ou limite'],('Testado: OpenAI e arquivo','Consulta de visão com equação sintética 2*x+3=11 retornou x=4; reenvio em cache conferido.'),('Testado: envio pelo ESP','Captura real da bancada enviada pelo próprio ESP, arquivada e respondida pela IA; JPEG recuperado com hash idêntico.'),('Testado: AP e OTA do ESP','Portal automático, desligamento por comando, retorno após reinício e atualização ESP por Wi-Fi.'),('Testado: atualização e LCD','STM R10→R10a e R10a→R11 pelo Wi-Fi; mapa de 49 teclas preservado. Usuário confirmou raiz/frações na janela original.'),('Bateria e resolução completa','Dois resistores de 470 kΩ corrigidos pelo proprietário; leitura normal. API v2 conferida com três fotos de questão real e desenvolvimento completo.'),('Não integrado e não testado','Claude/Anthropic. O guia de conta não significa suporte pronto.')),
 h('Checklist para receber a calculadora pronta'),steps('Peça a demonstração de ligar/desligar, fazer 2+3x4 e ler a tela na sua unidade.','Confira senha do AP, orientação da câmera, alimentação/recarga e uma foto legível de questão real.','Peça uma resolução completa no LCD e o procedimento de recuperar/excluir fotos do serviço.','Confirme quem paga a API, os limites, contato de suporte e arquivos corretos de atualização.'),
 p('Este manual não é o manual original de fábrica da Lyke. A carcaça foi adaptada e a eletrônica é do projeto CAMERA_ESP. A ilustração da frente e as telas são simulações; há também uma foto real da unidade, enviada pelo proprietário, na página de fontes.')])

page('Fontes, imagens e versão','34 | REFERÊNCIAS',unit_photo if unit_photo.exists() else reference,
 'Foto real da unidade modificada durante o ajuste da bateria, anterior ao ícone R11. As demais telas são simulações; o produto comercial original está em [S12].' if unit_photo.exists() else 'Foto do produto comercial original [S12], não da unidade modificada.',[
 p('Links oficiais consultados em 23/11/2026. As telas de portais deste manual são desenhos explicativos: menus e condições comerciais podem mudar. As instruções de operação local foram conferidas nos fontes R11/R8.'),
 table(['Referência','Documento oficial'],
 ('S1','<link href="https://help.openai.com/en/articles/9039756-managing-billing-for-chatgpt-and-the-api-platform" color="#087F81">OpenAI: cobrança ChatGPT e plataforma API</link>'),
 ('S2','<link href="https://developers.openai.com/api/docs/guides/api-keys" color="#087F81">OpenAI: chaves de API e contas de serviço</link>'),
 ('S3','<link href="https://help.openai.com/en/articles/8264644-how-can-i-set-up-prepaid-billing" color="#087F81">OpenAI: crédito pré-pago e recarga</link>'),
 ('S4','<link href="https://developers.openai.com/api/docs/quickstart" color="#087F81">OpenAI: início rápido da API</link>'),
 ('S5','<link href="https://platform.claude.com/docs/en/get-api-key" color="#087F81">Claude: criar e administrar chave de API</link>'),
 ('S6','<link href="https://support.claude.com/en/articles/8977456-how-do-i-pay-for-my-claude-api-usage" color="#087F81">Claude: cobrança e compra de créditos</link>'),
 ('S7','<link href="https://supabase.com/docs/guides/functions/secrets" color="#087F81">Supabase: segredos de Edge Functions</link>'),
 ('S8','<link href="https://supabase.com/docs/guides/storage/security/access-control" color="#087F81">Supabase: controle de acesso ao Storage</link>'),
 ('S9','<link href="https://platform.claude.com/docs/en/api/overview" color="#087F81">Claude: API e formato de mensagens</link>'),
 ('S10','<link href="https://developers.openai.com/api/docs/models/gpt-6.1-terra" color="#087F81">OpenAI: modelo GPT-6.1 Terra</link>'),
 ('S11','<link href="https://developers.openai.com/api/docs/pricing" color="#087F81">OpenAI: preços atuais da API</link>'),
 ('S12','<link href="https://lyke.com.br/produtos/detalhes/calculadora-cientifica-10-2-digitos/" color="#087F81">Lyke: produto comercial de referência 101078</link>')),
 p('Fontes locais: firmware/lyke_stm32/interface.h e scientific.h; firmware/esp32cam_integrada/lyke/portal.h; supabase/functions/lyke-solve/; docs/API-E-ARQUIVO.md e docs/conhecimento/. As telas usam a fonte bitmap Adafruit GFX, licença BSD incluída no pacote como FONT-LICENSE.txt. Ilustrações, diagramação e simulações produzidas para este guia.')])

assert len(PAGES)==36,len(PAGES)

def clean(s):
    return s.replace('\u2011','-').replace('\u2013','-').replace('\u2014','-').replace('\u00a0',' ')

def markdown_text(s):
    s=clean(s)
    s=re.sub(r'<link href="([^"]+)"[^>]*>(.*?)</link>',lambda m:f'[{m[2]}]({m[1]})',s)
    s=s.replace('<b>','**').replace('</b>','**').replace('<br/>','<br>')
    return html.unescape(s)

md=['# Manual do comprador - Calculadora Lyke modificada','',
    '> Edição 2.0, 23/11/2026. STM32 R11 + ESP32-CAM R8. Somente OpenAI testada. Claude ainda não integrado/testado.','',
    'Este Markdown contém o conteúdo integral do PDF, com as mesmas figuras. Mantenha `manual-assets/` junto deste arquivo para exibir imagens. O ZIP inclui a pasta.','',
    '## Índice','']
for i,pg in enumerate(PAGES,1):md.append(f'- [{i:02}. {pg["title"]}](#pagina-{i:02})')
for i,pg in enumerate(PAGES,1):
    md+=['',f'<a id="pagina-{i:02}"></a>','',f'## {i:02}. {pg["title"]}','',f'*{pg["kicker"]}*','',f'![{pg["title"]}](manual-assets/{pg["fig"].name})','',pg['caption'],'']
    for block in pg['blocks']:
        tag=block[0]
        if tag=='p':md+=[markdown_text(block[1]),'']
        elif tag=='h':md+=[f'### {block[1]}','']
        elif tag=='steps':
            md += [f'{j}. {markdown_text(t)}' for j,t in enumerate(block[1],1)]+['']
        elif tag=='note':md += [f'> **{block[1]}**','>',f'> {markdown_text(block[2])}','']
        elif tag=='table':
            md += ['| '+' | '.join(block[1])+' |','| '+' | '.join(['---']*len(block[1]))+' |']
            md += ['| '+' | '.join(markdown_text(c).replace('|','\\|') for c in row)+' |' for row in block[2]]+['']
md += ['## Contexto estruturado para uma IA de apoio','',
    '```yaml','produto: Lyke 101078 modificada','data_manual: 2026-11-23','stm32: lyke-r11','esp: lyke-r8',
    'esp_chip: ESP32 clássico AI Thinker, 4 MB flash e 4 MB PSRAM','nao_confundir: GOOUUU ESP32-S3 + DevKit (montagem diferente)',
    'teclas: 49; ON não mapeada','lcd: janela 126x48; 21 colunas x 6 linhas úteis; resposta em 6 linhas sem ícones',
    'ap: Calc-Lyke','portal: http://192.168.8.1','wifi_boot: AP sempre abre; OFF só na sessão',
    'captura: SHIFT e depois direita, na tela de cálculo','fotos: 1 a 3 JPEGs em RAM; até 512 KiB cada',
    'envio: MODE > ENVIAR FOTOS IA > confirmar com =','arquivo: só após envio, no serviço privado',
    'bateria: 0-4 barras aproximadas; ! baixa; ? leitura inválida; medição a cada 30 s','stm_ota: Wi-Fi do ESP + UART existente; base V2 já instalada e testada','api_testada: OpenAI Responses','modelo_principal: gpt-6.1-terra / high','modelo_economico: gpt-6-luna / medium',
    'claude: não integrado e não testado; exige adaptador Anthropic no servidor',
    'segredo_openai: só no servidor, OPENAI_API_KEY','credencial_aparelho: diferente da chave OpenAI',
    'hardware_final: posição de chave, câmera, carregador e bateria depende da unidade',
    'nao_inventar: testes físicos, menu Claude, galeria histórica no portal, recarga e pinagem',
    '```','',
    'Para ajudar o comprador, use primeiro os procedimentos deste manual, peça a mensagem exata e oriente uma etapa por vez. Não peça que publique chaves/senhas. Não trate compra de créditos como solução para toda falha. Não mande abrir, refazer fios ou gravar imagem de outro hardware para um problema de uso.','']
(OUT/'Manual-Lyke.md').write_text('\n'.join(md),encoding='utf-8')
shutil.copyfile(ROOT/'firmware/calculadora/common/FONT-LICENSE.txt',ASSETS/'FONT-LICENSE.txt')
(ASSETS/'LEIA-ME.txt').write_text('Telas e portais: simulações didáticas baseadas nos fontes R11/R8. Fórmulas gráficas e indicador de bateria usam frames do código C testado no computador.\ncalculadora-guia.png: ilustração lógica da frente, não fotografia.\nunidade-lyke.png: foto da unidade modificada enviada pelo proprietário durante ajuste da bateria, anterior ao ícone R11.\nlyke-original-referencia.jpg e lyke-modelo-referencia.jpg: foto do produto comercial original, fabricante Lyke. Não mostra a calculadora modificada.\nFonte: https://lyke.com.br/produtos/detalhes/calculadora-cientifica-10-2-digitos/\nDownload: 23/11/2026.\nFonte bitmap das telas: Adafruit GFX; licença em FONT-LICENSE.txt.\n',encoding='utf-8')

W,H=A4;M=42;CW=W-2*M
styles={
    'p':ParagraphStyle('p',fontName='Body',fontSize=10.2,leading=14.2,textColor=colors.HexColor(INK),spaceAfter=8),
    'small':ParagraphStyle('small',fontName='Body',fontSize=8.2,leading=10.7,textColor=colors.HexColor(MUTED)),
    'h':ParagraphStyle('h',fontName='Bold',fontSize=12.2,leading=15.2,textColor=colors.HexColor(NAVY)),
    'cell':ParagraphStyle('cell',fontName='Body',fontSize=9.0,leading=12.0,textColor=colors.HexColor(INK)),
    'th':ParagraphStyle('th',fontName='Bold',fontSize=9.0,leading=12,textColor=colors.white),
}
pdf_path=OUT/'Manual-Lyke.pdf'
c=canvas.Canvas(str(pdf_path),pagesize=A4,pageCompression=1)
c.setTitle('Manual do comprador - Lyke modificada | R11 / R8')
c.setAuthor('Projeto CAMERA_ESP')
c.setSubject('Uso, funções, Wi-Fi, fotos, OpenAI e preparação Claude não integrada')
layout=[]
def para(text,y,width=CW,x=M,style='p',scale=1):
    st=styles[style]
    if scale!=1:
        st=ParagraphStyle('scaled',parent=st,fontSize=st.fontSize*scale,leading=st.leading*scale)
    text=re.sub(r'\[S(\d+)\]',r'<link href="#p36" color="#087F81">[S\1]</link>',clean(text))
    obj=Paragraph(text,st);_,hh=obj.wrap(width,H)
    obj.drawOn(c,x,y-hh)
    return y-hh

for i,pg in enumerate(PAGES,1):
    c.bookmarkPage(f'p{i}')
    c.addOutlineEntry(f'{i:02} {pg["title"]}',f'p{i}',level=0)
    c.setFillColor(colors.HexColor(TEAL));c.rect(0,H-9,W,9,fill=1,stroke=0)
    c.setFont('Bold',8.5);c.setFillColor(colors.HexColor(TEAL));c.drawString(M,H-39,pg['kicker'])
    titleStyle=ParagraphStyle('title',fontName='Bold',fontSize=25 if len(pg['title'])<43 else 23,leading=28,textColor=colors.HexColor(NAVY))
    obj=Paragraph(pg['title'],titleStyle);_,th=obj.wrap(CW,H);obj.drawOn(c,M,H-59-th)
    y=H-59-th-17
    im=Image.open(pg['fig']);iw,ih=im.size
    if i==1: maxh=292
    elif i==3:maxh=302
    elif i==36:maxh=125
    else:maxh=155 if ih/iw<.36 else 195
    scale=min(CW/iw,maxh/ih);dw,dh=iw*scale,ih*scale
    c.drawImage(str(pg['fig']),M+(CW-dw)/2,y-dh,dw,dh,mask='auto')
    y-=dh+7
    y=para(pg['caption'],y,style='small')-14
    # Tight reference pages still retain usable text sizes.
    textscale=.94 if i in [30,31,32,33,35,36] else 1
    for b in pg['blocks']:
        if b[0]=='p':y=para(b[1],y,scale=textscale)-8
        elif b[0]=='h': y=para(b[1],y-2,style='h',scale=textscale)-7
        elif b[0]=='steps':
            for j,t in enumerate(b[1],1):
                c.setFillColor(colors.HexColor(TEAL));c.circle(M+8,y-7,8,fill=1,stroke=0)
                c.setFont('Bold',8);c.setFillColor(colors.white);c.drawCentredString(M+8,y-10,str(j))
                y=para(t,y,width=CW-28,x=M+28,scale=textscale)-8
        elif b[0]=='note':
            txt=f'<b>{b[1]}</b><br/>{b[2]}'
            st=ParagraphStyle('note',parent=styles['p'],fontSize=9.8*textscale,leading=13.4*textscale)
            ob=Paragraph(clean(txt),st);_,hh=ob.wrap(CW-24,H)
            c.setFillColor(colors.HexColor(AMBER));c.roundRect(M,y-hh-19,CW,hh+19,7,fill=1,stroke=0)
            ob.drawOn(c,M+12,y-hh-8);y-=hh+29
        elif b[0]=='table':
            heads,rows=b[1],b[2];n=len(heads)
            widths=[CW*.34,CW*.66] if n==2 else [CW*.25,CW*.51,CW*.24]
            if i in [31]:widths=[CW*.37,CW*.63]
            if i==36:widths=[CW*.13,CW*.87]
            st=ParagraphStyle('cellscaled',parent=styles['cell'],fontSize=styles['cell'].fontSize*textscale,leading=styles['cell'].leading*textscale)
            cells=[[Paragraph(clean(x),styles['th']) for x in heads]]
            for row in rows:
                formatted=list(row)
                if i==2:
                    target=re.search(r'\d+',formatted[1])[0]
                    formatted[1]=f'<link href="#p{target}" color="#087F81">{formatted[1]}</link>'
                cells.append([Paragraph(clean(x),st) for x in formatted])
            tab=Table(cells,colWidths=widths,hAlign='LEFT')
            tab.setStyle(TableStyle([
                ('BACKGROUND',(0,0),(-1,0),colors.HexColor(NAVY)),
                ('ROWBACKGROUNDS',(0,1),(-1,-1),[colors.HexColor(PALE),colors.white]),
                ('VALIGN',(0,0),(-1,-1),'TOP'),('LEFTPADDING',(0,0),(-1,-1),9),('RIGHTPADDING',(0,0),(-1,-1),9),
                ('TOPPADDING',(0,0),(-1,-1),6 if i not in [31,32,33,36] else 4.5),
                ('BOTTOMPADDING',(0,0),(-1,-1),6 if i not in [31,32,33,36] else 4.5),
                ('LINEBELOW',(0,-1),(-1,-1),.5,colors.HexColor('#D6E4E4')),
            ]))
            _,hh=tab.wrap(CW,H);tab.drawOn(c,M,y-hh);y-=hh+11
    layout.append(dict(page=i,bottom=round(y,1)))
    c.setStrokeColor(colors.HexColor('#CCDADC'));c.line(M,42,W-M,42)
    c.setFont('Body',8);c.setFillColor(colors.HexColor(MUTED))
    c.drawString(M,27,'LYKE MODIFICADA | Manual do comprador | R11 / R8 | 23/11/2026')
    c.drawRightString(W-M,27,f'{i:02} / {len(PAGES)}')
    c.showPage()
c.save()
(ROOT/'tmp/pdfs/manual-lyke/layout.json').write_text(json.dumps(layout,indent=2))
bad=[r for r in layout if r['bottom']<54]
print(json.dumps({'pages':len(PAGES),'layout_overflow':bad,'pdf':str(pdf_path)},ensure_ascii=False))
if bad:raise RuntimeError('Corrigir páginas com conteúdo abaixo da margem antes de entregar.')
with zipfile.ZipFile(OUT/'Manual-Lyke-Completo.zip','w',zipfile.ZIP_DEFLATED) as z:
    for f in [OUT/'Manual-Lyke.pdf',OUT/'Manual-Lyke.md',*sorted(ASSETS.glob('*'))]:z.write(f,f.relative_to(OUT))
