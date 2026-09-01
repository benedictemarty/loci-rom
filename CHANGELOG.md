# Changelog — loci-rom

Format inspiré de [Keep a Changelog](https://keepachangelog.com/).

## [non publié]

### 2026-09-01 — Lib 6502 `loci_stream_*` + opcodes `$A7`/`$A8` (branche `feature/stream-bank-A8`)

Base : branche `feature/stream-bank-A8`, créée sur `webdisk`.

#### Added
- `src/include/loci.h` et `src/asminc/loci.inc` : constantes d'opcodes
  **`MIA_OP_SET_BANK` (`$A7`)** et **`MIA_OP_STREAM_BANK` (`$A8`)**.
- `src/include/loci_stream.h` + `src/libsrc/loci_stream.c` : bibliothèque
  read-only **`loci_stream_open/load/show/hide/close`** (niveaux 0/1 de la spec
  streamer) au-dessus de `open`/`$A8`/`$A7`/`close`. `load` = un seul fastcall
  `$A8` (`lseek`+`read` fichier → banque 16 Ko) ; `show`/`hide` = mapping `$A7`.
  Style calqué sur `libsrc/lseek.c` / `libsrc/read_xram.c` (helpers `mia.s`).

#### Notes
- Ordre de push xstack (`fd,off,dst,len`) cohérent avec le dépilage LIFO du
  handler firmware `std_api_stream_bank()`.
- **`loci_stream.c` compile proprement en isolation avec cc65** (`cc65 -c`,
  émet `lda #$A8` pour `load`, `lda #$A7` pour `show`/`hide`). ⚠️ Le **`make`
  complet de la ROM échoue dans cet environnement pour une raison PRÉ-EXISTANTE
  et sans rapport** : `main.c` utilise `strcasestr` non déclaré → precondition
  cc65 (version installée trop ancienne). À relier/valider avec la bonne
  toolchain cc65, puis end-to-end dans **Phosphoric**.
- Réf : `extensions/streamer-A8/spec-streamer-assets.md`,
  `extensions/banking-A7/spec-registre-banque.md`.
