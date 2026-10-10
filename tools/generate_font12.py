#!/usr/bin/env python3
"""Generate a 20% larger bitmap from the pinned OLED driver's Arial 10 font."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
source=next((root/'.pio/libdeps/cardputer-adv-m2').glob('*/src/OLEDDisplayFonts.cpp'))
text=source.read_text().split('const uint8_t ArialMT_Plain_10[] PROGMEM = {',1)[1].split('};',1)[0]
text=re.sub(r'//[^\n]*','',text)
data=bytes(int(x,16) for x in re.findall(r'0x([0-9A-Fa-f]+)',text))
height,first,count=data[1:4]
new_height=round(height*1.2)
bitmap=bytearray();table=bytearray()
for i in range(count):
    high,low,size,width=data[4+i*4:8+i*4]
    new_width=max(1,round(width*1.2))
    if high==low==255:
        table.extend([255,255,0,new_width]);continue
    offset=(high<<8)|low
    old=data[4+count*4+offset:4+count*4+offset+size]
    pixels=bytearray(new_width*((new_height+7)//8))
    for x in range(new_width):
        for y in range(new_height):
            ox=min(width-1,x*width//new_width);oy=min(height-1,y*height//new_height)
            index=ox*((height+7)//8)+oy//8
            if index<len(old) and old[index]&(1<<(oy%8)):
                pixels[x*((new_height+7)//8)+y//8]|=1<<(y%8)
    offset=len(bitmap);table.extend([offset>>8,offset&255,len(pixels),new_width]);bitmap.extend(pixels)
result=bytes([12,new_height,first,count])+table+bitmap
out=root/'include/cardputer_adv/font12.hpp'
lines=[', '.join(f'0x{b:02x}' for b in result[i:i+16])+',' for i in range(0,len(result),16)]
out.write_text('// Derived from pinned Meshtastic OLED ArialMT_Plain_10; see UPSTREAM.md.\n#pragma once\n#include <Arduino.h>\nnamespace cardputer_adv {\ninline const uint8_t Adv_Plain_12[] PROGMEM = {\n'+ '\n'.join(lines)+'\n};\n}\n')
print(f'Font 12: raster {new_height}px, {len(result)} bytes')
