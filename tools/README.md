# Ferramentas offline

`prepare_upstream.py` verifica revisões; `stage_core.py` gera overlay; `pio_build.py` integra fontes e bloqueia upload; `build_ota.py --milestone M3 --diagnostic` compila/empacota; `package_ota.py` valida imagem/manifesto. `test_native.py` executa testes de teclado, mesh, FAT, CRC/snapshots e pacote; `generate_font12.py` reproduz a fonte maior a partir da biblioteca OLED fixada do ambiente M2. Nenhuma ferramenta aceita porta serial ou grava firmware. M1/M2 continuam disponíveis pela opção `--milestone`.
