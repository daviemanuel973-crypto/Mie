"""Original 32px industrial textures/icons; deterministic, no external art."""
from pathlib import Path
from PIL import Image, ImageDraw
import random
ROOT=Path(__file__).resolve().parents[1]/'resources/assets'
BLOCKS=['fuelGenerator','energyAccumulator','electricFurnace','oreCrusher','metalPress','woodSawmill','powerCable','itemPipe','itemExtractor']
ITEMS=['copperPowder','leadPowder','ironPowder','silverPowder','goldPowder','copperPlate','ironPlate','copperWire']
for index,name in enumerate(BLOCKS):
 im=Image.new('RGBA',(32,32),(47,53,59,255)); d=ImageDraw.Draw(im)
 rng=random.Random(index)
 for y in range(0,32,2):
  for x in range(0,32,2):
   v=rng.randrange(-7,8);d.rectangle((x,y,x+1,y+1),fill=(61+v,67+v,73+v,255))
 d.rectangle((1,1,30,30),outline=(25,29,34),width=2)
 for x,y in [(4,4),(26,4),(4,26),(26,26)]:d.rectangle((x,y,x+1,y+1),fill=(180,187,183))
 copper=(200,121,68);lit=(91,194,158);dark=(22,30,34)
 if name=='fuelGenerator':
  d.rectangle((7,8,24,25),fill=dark);d.rectangle((10,16,21,23),fill=(209,93,35));d.rectangle((13,13,18,22),fill=(255,188,56));d.rectangle((8,8,23,11),fill=(121,127,127))
 elif name=='energyAccumulator':
  d.rectangle((9,6,22,26),fill=dark);d.rectangle((13,3,18,5),fill=copper)
  for y in [10,15,20]:d.rectangle((12,y,19,y+2),fill=lit)
 elif name=='electricFurnace':
  d.rectangle((6,7,25,25),fill=dark);d.rectangle((9,10,22,22),outline=copper,width=2);d.line((11,16,14,19,20,12),fill=lit,width=2)
 elif name=='oreCrusher':
  d.rectangle((6,9,25,23),fill=dark)
  for x in [9,20]:
   d.rectangle((x-2,10,x+2,22),fill=(171,180,187))
   for y in range(11,22,4):d.rectangle((x-3,y,x+3,y+1),fill=(100,108,119))
 elif name=='metalPress':
  d.rectangle((8,6,23,10),fill=(162,173,182));d.rectangle((14,10,17,20),fill=copper);d.rectangle((9,20,22,23),fill=(183,194,199));d.rectangle((7,25,24,27),fill=dark)
 elif name=='woodSawmill':
  d.rectangle((6,18,26,24),fill=(139,97,56));d.ellipse((10,7,23,20),fill=(191,200,205));d.line((16,8,16,20),fill=dark,width=2);d.line((11,14,22,14),fill=dark,width=2)
 elif name=='powerCable':
  d.rectangle((0,11,31,20),fill=dark);d.rectangle((0,14,31,17),fill=copper);d.rectangle((11,0,20,31),fill=dark);d.rectangle((14,0,17,31),fill=copper)
 elif name=='itemPipe':
  d.rectangle((0,10,31,21),fill=copper);d.rectangle((0,13,31,18),fill=dark);d.rectangle((10,0,21,31),fill=copper);d.rectangle((13,0,18,31),fill=dark);d.rectangle((12,12,19,19),fill=(85,126,137))
 else:
  d.rectangle((5,8,26,24),fill=copper);d.rectangle((8,11,23,21),fill=dark);d.polygon([(11,14),(17,14),(17,11),(22,16),(17,21),(17,18),(11,18)],fill=lit)
 p=ROOT/'blocks/industry'/f'{name}.png';p.parent.mkdir(parents=True,exist_ok=True);im.save(p,optimize=True)
colors=[(197,119,66),(116,108,148),(164,169,167),(199,220,224),(236,182,65),(197,119,66),(164,169,167),(197,119,66)]
for name,c in zip(ITEMS,colors):
 im=Image.new('RGBA',(32,32));d=ImageDraw.Draw(im);shade=tuple(max(0,v-55) for v in c)
 if 'Powder' in name:
  d.polygon([(4,24),(10,18),(14,11),(20,14),(27,24),(24,27),(8,27)],fill=shade)
  d.polygon([(6,23),(11,18),(15,13),(19,15),(25,23),(23,25),(9,25)],fill=c)
  for x,y in [(12,20),(16,17),(20,21)]:d.rectangle((x,y,x+1,y+1),fill=tuple(min(255,v+35) for v in c))
 elif 'Plate' in name:
  d.polygon([(4,13),(21,7),(28,17),(11,24)],fill=shade);d.polygon([(4,10),(21,4),(28,14),(11,21)],fill=c);d.line((6,10,21,6,26,14),fill=tuple(min(255,v+35) for v in c),width=2)
 else:
  d.rectangle((8,7,24,25),fill=shade)
  for y in range(9,24,4):d.line((9,y,23,y),fill=c,width=2)
  d.line((23,21,28,24,28,28),fill=c,width=2)
 p=ROOT/'items/industry'/f'{name}.png';p.parent.mkdir(parents=True,exist_ok=True);im.save(p,optimize=True)
print('Generated 17 original industry assets, all 32x32 RGBA')
