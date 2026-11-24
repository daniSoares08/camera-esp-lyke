"""Contact sheet from real native renderer frames; no AI call or hardware access."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont

ROOT=Path(__file__).resolve().parents[1]
folder=ROOT/'.build/lyke_stm32/lyke-r12/previews'
font=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',25)
small=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',19)
out=Image.new('RGB',(1260,1800),'#edf4f3');draw=ImageDraw.Draw(out)
draw.text((30,20),'R12: texto e desenhos no mesmo documento',font=font,fill='#183442')
draw.text((30,59),'Simulações do renderer real. Janela do LCD: 126 x 48; ainda sem teste físico.',font=small,fill='#47636d')
def screen(name,width,height):
    raw=Image.open(folder/(name+'.pbm')).convert('L')
    image=Image.new('RGB',raw.size,'#ced8b5');image.paste('#263624',mask=Image.eval(raw,lambda x:255-x))
    return image.resize((width,height),Image.Resampling.NEAREST)
out.paste(screen('resposta_completa',576,1152),(30,115))
labels=[('circuito_esquerda','Circuito: parte superior esquerda'),('circuito_direita','Circuito: rolado para a direita'),
        ('grafico_topo','Gráfico: região superior'),('grafico_base','Gráfico: região inferior direita'),('angulo_base','Ângulo: região inferior')]
for n,(name,label) in enumerate(labels):
    y=115+n*310;draw.text((660,y),label,font=small,fill='#183442');out.paste(screen(name,504,192),(660,y+38))
for n,text in enumerate(['À esquerda: documento completo de teste.','À direita: recortes de 126 x 48 pixels.',
                         'Cima/baixo: rolar 16 pixels.','Esquerda/direita: mover no desenho largo.',
                         'Só texto: esquerda/direita muda 48 pixels.']):
    draw.text((30,1320+n*35),text,font=small,fill='#47636d')
for target in [ROOT/'output/previews/lyke-r12-desenhos.png',ROOT/'docs/assets/lyke-r12-desenhos.png']:
    target.parent.mkdir(parents=True,exist_ok=True);out.save(target)
