# Changelog — loci-rom

Format inspiré de [Keep a Changelog](https://keepachangelog.com/).

## [Non publié] — 2026-09-10 : façade cc65 du device réseau `N:` (`$B7`)

`include/loci_net.h` + `libsrc/loci_net.c` : une URL s'ouvre, se lit et se ferme comme un
fichier depuis un programme 6502.

    loci_net_t n;
    if (loci_net_open(&n, "N:https://exemple.fr/data.bin") == 0) {
        while ((got = loci_net_read(&n, 0x8000, 512)) > 0) { ... }
        loci_net_close(&n);
    }

`open` ($14) est routé côté firmware sur le préfixe `N:`, donc un `open()` ordinaire suffit et
le fd s'utilise avec `read_xram` ($17) et `close` ($15) habituels. S'y ajoute
`loci_net_status()` (opcode **`$B7`**, `MIA_OP_NET_CONTROL`) qui rend le code HTTP,
l'avancement et les octets disponibles — de quoi distinguer un corps vide d'un `404`, ce que
`loci_net_read` seul ne dit pas.

L'en-tête documente les contraintes que le 6502 doit connaître : **une seule connexion à la
fois** (canal AT unique), **lecture seule**, **exclusivité avec l'ACIA transparente**
(`$0380`, façon OricTel), **données livrées en XRAM** et non en RAM Oric, et une **première
lecture qui peut durer plusieurs secondes** en HTTPS (handshake TLS terminé par le dongle).

⚠️ **L'ordre de dépilage de `$B7` n'est pas couvert par un test** : il est tenu à la main des
deux côtés (`libsrc/loci_net.c` ↔ `firmware/src/mia/api/net.c`). Un décalage y serait
silencieux — d'où l'avertissement en tête de la fonction. Cf. la note H de
`emul/tests/test_net.c`.

Compile et entre dans `loci.lib` (`CC65_HOME=/usr/local/share/cc65`). **Non exercé par un
programme 6502 réel** à ce stade.


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
