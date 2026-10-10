#!/usr/bin/env python3
"""Exercise the actual pinned LVGL renderer and UI input, without hardware."""
from pathlib import Path
import argparse,subprocess,tempfile
from concurrent.futures import ThreadPoolExecutor
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sdk');args=p.parse_args()
sdk=['-isysroot',args.sdk] if args.sdk else []
lib=root/'.pio/libdeps/cardputer-adv-m7/lvgl'
if not (lib/'lvgl.h').exists():raise RuntimeError('Resolve cardputer-adv-m7 dependencies first.')
flags=sdk+['-fsanitize=address,undefined','-Iinclude','-I'+str(lib),'-DLV_CONF_INCLUDE_SIMPLE','-DCARDPUTER_ADV_UI_V2']
with tempfile.TemporaryDirectory(prefix='adv-lvgl-') as tmp:
    sources=sorted((lib/'src').rglob('*.c'))
    def compile(item):
        i,src=item;obj=str(Path(tmp)/f'{i}.o');subprocess.run(['cc',*flags,'-c',str(src),'-o',obj],cwd=root,check=True);return obj
    with ThreadPoolExecutor(max_workers=8) as pool:objects=list(pool.map(compile,enumerate(sources)))
    binary=str(Path(tmp)/'renderer')
    subprocess.run(['c++',*flags,'-std=c++17','-Itests/ui_fakes','tests/lvgl_surface_test.cpp','src/ui/lvgl_surface.cpp',*objects,'-o',binary],cwd=root,check=True)
    subprocess.run([binary],cwd=root,check=True)
    binary=str(Path(tmp)/'keyboard')
    subprocess.run(['c++',*flags,'-std=c++17','tests/keyboard_v2_test.cpp','src/board/keyboard_decoder.cpp','-o',binary],cwd=root,check=True)
    subprocess.run([binary],cwd=root,check=True)
    binary=str(Path(tmp)/'shell')
    subprocess.run(['c++',*flags,'-std=c++17','-Itests/ui_fakes','-Ivendor/meshtastic-firmware/src/mesh/generated','-I.pio/libdeps/cardputer-adv-m7/Nanopb','-DCARDPUTER_ADV_UI_NATIVE','-DCARDPUTER_ADV_M4','-DCARDPUTER_ADV_M5','-DCARDPUTER_ADV_M6','-DCARDPUTER_ADV_M7','-DCARDPUTER_ADV_M7_NODES','tests/m5_integration_test.cpp','src/ui/m4_shell.cpp','src/ui/ui_shell.cpp','src/mesh/mesh_validation.cpp','src/mesh/node_tools.cpp','-o',binary],cwd=root,check=True)
    subprocess.run([binary,'--ui-v2'],cwd=root,check=True)
print('LVGL renderer/color, keyboard and Enter-first chat tests passed.')
