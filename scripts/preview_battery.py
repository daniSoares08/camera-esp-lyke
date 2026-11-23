"""Preview actual native-test LCD frames; no photograph or image generation."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
folder = ROOT / '.build/lyke_stm32/lyke-r11/previews'
names = [('bateria_cheia','Cheia'),('bateria_media','Intermediária'),
         ('bateria_baixa','Baixa: ! pisca'),('bateria_sem_leitura','Falha de leitura: ?')]
font = ImageFont.truetype('C:/Windows/Fonts/arial.ttf',21)
out = Image.new('RGB',(660,575),'#edf4f3')
draw = ImageDraw.Draw(out)
draw.text((20,15),'Lyke R11 · indicador na janela existente',font=font,fill='#19323e')
for n,(name,label) in enumerate(names):
    y = 70+n*115
    draw.text((20,y),label,font=font,fill='#19323e')
    im = Image.open(folder/(name+'.pbm')).convert('L').crop((0,0,128,8))
    im = im.resize((512,32),Image.Resampling.NEAREST)
    screen = Image.new('RGB',im.size,'#cdd6b1')
    screen.paste('#263624',mask=Image.eval(im,lambda value:255-value))
    out.paste(screen,(20,y+32))
draw.text((20,540),'Simulação do código real do cabeçalho do LCD.',
          font=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',17),fill='#40565c')
for target in [ROOT/'output/previews/lyke-r11-bateria.png', ROOT/'docs/assets/lyke-r11-bateria.png']:
    target.parent.mkdir(parents=True,exist_ok=True)
    out.save(target)
