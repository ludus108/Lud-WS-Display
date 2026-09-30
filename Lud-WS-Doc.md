================================================================================
LUD-WS-DISPLAY — RIEPILOGO DI CONTINUITÀ (AGGIORNATO)
================================================================================
Documento autocontenuto. Incollalo come primo messaggio in una nuova chat
per riprendere il lavoro da dove è stato interrotto.

Ultimo aggiornamento: 2026-09-26 (refactoring lvglGraf + pagina VCF-B completa)

Refactoring lvglGrafFunc.cpp → 3 file (Core, Pages, Modules) + lvglGraf_internal.h, con reset_ui_pointers() e page_begin() fattorizzati

VCF-B: 6 stati (FLT/UNI-SLV-FRE, WOV/POT-ENV-RND) con top-label dinamiche (CUT/DET/INT/FORM/TIME) correttamente centrate

Arc F1/F2/F3 + RES/EnvA/EnvV con pallino target, crossing, tick radiali, label valore sotto

Plotter ADSR blu sopra il frame ENV vir

Slider A D S R senza freccina e con indicator azzurro

FX/FV-1 con frame compatto, dropdown, Rev toggle, Salva
================================================================================


--------------------------------------------------------------------------------
0. STRUTTURA CARTELLA DISPLAY
--------------------------------------------------------------------------------

Lud-WS-Display/
├── Lud-WS-Display.ino
├── globals.h
├── comunicazioni.h
├── serial_protocol.h
├── images.c
├── images/
└── src/
    ├── preset/
    │   ├── preset_sd.h / .cpp          (30 preset, formato binario)
    │   ├── preset_cache.h / .cpp       (cache locale)
    │   ├── preset_transfer.h           (load/save/poly/multimono)
    │   └── preset_ui.h                 (helper: uiSetParamU8/I32, uiSetParamB_*)
    └── grafica/
        ├── lvglGraf.h                  (API pubblica)
        ├── lvglGraf_internal.h         (NUOVO — header condiviso 3 .cpp)
        ├── lvglGrafCore.cpp            (NUOVO — widget/utility)
        ├── lvglGrafPages.cpp           (NUOVO — pagine + eventi)
        ├── lvglGrafModules.cpp         (NUOVO — drum/VCF-B/FX)
        └── lvglGrafFunc.cpp.bak        (VECCHIO — non compilare, tenere .bak)


--------------------------------------------------------------------------------
1. REFACTORING lvglGraf — 3 FILE
--------------------------------------------------------------------------------

Motivo: file unico >2900 righe, difficile da mantenere.

Regola: NON devono esistere lvglGrafFunc.cpp e i 3 nuovi contemporaneamente
(multiple definition). Il vecchio è rinominato .bak.

Catena dipendenze:  Core  →  Pages  →  Modules
Tutti includono: globals.h, lvglGraf.h, lvglGraf_internal.h

## lvglGraf_internal.h — header privato
- `#define DBG(...)` con LGF_DBG (0/1)
- `extern const uint8_t ui2fw_wave[9]`
- Statics condivise:
  - DRUM: `drum_cell/dot/pattern_data/cursor/step_counter/playing/timer/...`
  - SYNTHB VCF: `sB_vcf_btn/_lbl/_mode`, `sB_sub_btn/_lbl`,
    `sB_filter_submode/_wovel_submode`, `sB_filter_type_*`, `sB_vcf_arc[3]`,
    `sB_vcf_arc_dot[3]`, `sB_vcf_arc_lbl[3]`, `sB_vcf_val_lbl[3]`,
    `sB_vcf_top_lbl[3]`, `sB_vcf_tick[3][5]`, `sB_vcf_scale_lbl[3][6]`,
    `sB_vcf_cut[3]`, `sB_wov_env_*_dd/_lbl`, `sB_vcf_env_chart/_serie`
  - VCFB (RES/EnvA/EnvV): `sB_vcfb_arc[3]`, `_lbl`, `_val_lbl`, `_dot`,
    `_tick[3][5]`, `_target[3]`, `_last[3]`, `_crossed[3]`
  - FX: `pot_size_FV1`, `pot_LF_FV1`, `pot_HF_FV1`, `fx_rev_btn/_lbl`,
    `fx_preset_dd`, `fx_save_btn`
- Prototipi cross-file: `drum_seq_page_create`, `vcf_page_synthb_create`,
  `fx_page_create`, `reset_ui_pointers`, `page_begin`

## lvglGrafCore.cpp — widget/utility (~1100 righe)
Sezioni:
1. Widget base: `btn()`, `home_btn_at()` (statica), `home_btn()`
2. Slider: `slider()` (con freccina), `slider_plain()` (senza freccina)
3. DCO: `h_slider()`
4. Plotter wave: `grid_btn_click`, `populate_grid`, `update_wave_plot`,
   `update_plotter_by_wave`
5. LED/colori: `update_leds`, `update_slider_color`, `update_shape_slider_color`
6. Arc: `earc_changed`, `arc_with_image`, `create_pot_container`
7. Timeline: `tl_set_buttons`, callback, `create_timeline*`, `update_timeline`
8. Submenu: `submenu()` (DCO/VCF/MOD/VCA/DLY)
9. Meter: `meter`, `update_meters`, `stop_meters`
10. Log/toast: `log_add`, `toast_show`, `create_log_widget`
11. EEPROM: `set_bright`, `load_bright`, `save_bright`
12. `reset_arrows()`
13. Envelope plotter: `create_env_plot`, `compute_env_points` (statica),
    `update_env_plot`, **`env_plot_draw()`** (NUOVA, usata da VCF-B)
14. Keyboard: `create_keyboard`, `update_keyboard_colors`

## lvglGrafPages.cpp — pagine + eventi (~700 righe)
Sezioni:
1. Lifecycle:
   - **`reset_ui_pointers()`** — azzera tutti i puntatori di pagina
   - **`page_begin(title)`** — prologo comune: cancellazione timer,
     close_rename_window, stop_meters, reset_ui_pointers
2. `create_preset_label()` (statica, la usa create_page)
3. `create_home()` — chiama `page_begin(nullptr)`
4. `create_page(title)` — chiama `page_begin(title)`, poi switch:
   - PLAY, SYNTH A/B, DCO A/B, MOD A/B, SET UP, MIDI, DRUM, SEQ,
     VCF A/B, VCA A/B, **FX**, DLY, fallback
   - Per VCF-B chiama `vcf_page_synthb_create(m)`
   - Per FX chiama `fx_page_create(m)`
   - Per SEQ chiama `drum_seq_page_create(m, t)`
5. Event handler: `eb`, `ebright`, `eslider`, `ecat_btn`, `ehslider`, `elist`
6. Callback MIDI: `midi_split_cb`, `midi_ch_a/b/d_cb`
7. `disc_btn_click`

`eslider()` gestisce entrambi i tipi di slider:
- Con freccina (`arr[idx]` valido) → logica crossing classica
- Senza freccina (`arr[idx]==nullptr`) → invia subito `update_slider_parameter`
- Chiama `update_env_plot(isA)` per idx 4..7 (VCA)
- Chiama `sB_vcf_env_plot_update()` per idx 0..3 (VCF-B)

## lvglGrafModules.cpp — sottosistemi (~900 righe)
Tre sezioni indipendenti:

### 1) DRUM SEQUENCER
- `drum_grid_create()`, `drum_grid_update()`
- `drum_step_timer_cb`, `drum_play_btn_cb` (statiche)
- `drum_seq_page_create(parent, title_lbl)` — entry point
- Griglia 9×16, 4 separatori verticali (1|2|3|4), cursore step
- Righe a 4 stati: SD (row 1), H2 (row 4). Altre: toggle 0/1.

### 2) SYNTHB VCF PAGE
- 6 stati gestiti da `sB_vcf_apply_state()`:

| Stato | F1 | F2 | F3 |
|---|---|---|---|
| FLT/UNI | top="CUT" | top="DET" | disabilitato |
| FLT/SLV | top="CUT" | top="INT" | top="INT" |
| FLT/FRE | top="CUT" | top="CUT" | top="CUT" |
| WOV/POT | lettere A E I O U A | top="FORM" | disabilitato |
| WOV/ENV | nascosto | nascosto | top="TIME" + dropdown ATT/SUS/REL |
| WOV/RND | disabilitato | disabilitato | top="TIME" |

- Ogni arc ha: label centrale (F1/F2/F3 o RES/EnvA/EnvV),
  label valore sotto (offset -8), pallino target rosso + crossing,
  5 tick radiali grigi (15×1px), top-label (opzionale)
- F1/F2/F3 supportano anche 6 lettere scala (WOV/POT)
- Dropdown ENV: ATT → 'V', SUS → 'Z', REL → 'L' (mappa 0..4 → 0..255)
- RES/EnvA/EnvV: arc addizionali con pallino e crossing,
  **EnvA disabilitato quando WOV**
- Plotter ADSR (blu `0x0088FF`) sopra frame ENV vir, 2px gap

### 3) FX / FV-1
- Frame compatto 258×310 a (470, 55)
- Label "FV-1" sopra il bordo (`y=-22`)
- Riga top: dropdown P1..P8 (65px), Rev1/Rev2 toggle (80px),
  Salva rosso (85px)
- 3 slider verticali SIZE/LF/HF con label centrate sopra


--------------------------------------------------------------------------------
2. LAYOUT PAGINE
--------------------------------------------------------------------------------

## HOME
- Titolo "LUD-WS" in alto a sx, versione sotto
- Slider luminosità in alto a dx
- Log widget 700×240 a (50, 97)
- 6 bottoni: PLAY / SYNTH A / SYNTH B / DRUM / FX / SET UP
- Auto-hide log dopo 2000 ms (LOG_TIMEOUT)

## SYNTH A / SYNTH B
- Preset selector a dx, label rinomina
- Submenu in basso: DCO/VCF/MOD/VCA/DLY (id 10..14)
- Home a sinistra (id -1)

## VCF A / VCF B
- VCF-A: 4 slider con freccina (idx 0..3), cornice ENV vir a (460, 60) 320×300
- VCF-B: 4 slider **senza freccina** (azzurro `0x0088FF`),
  cornice ENV vir compattata 280×290 a (470, 160),
  plotter ADSR 280×100 a (470, 58),
  F1/F2/F3 a y=80, x={35, 170, 305},
  RES/EnvA/EnvV a y=216, x={35, 170, 305},
  3 bottoni in basso (LP/BP, FLT/WOV, SUBMODE) a x={110, 205, 300}, y=360,
  Home a x=10, y=360

## VCA A / VCA B
- 4 slider in cornice, plotter ENV a (20, 100)
- ENV vir label sopra il bordo (`y=-20`)

## DRUM
- Label PTN, bottone SEQ a (580, 360)

## SEQ
- Label PTN + Salva (rosso) in alto a dx
- Griglia 9×16
- Play/Stop a (580, 360), Home (back a DRUM) a (680, 360)

## FX
- Placeholder sx
- Frame FV-1 a dx: dropdown + Rev + Salva + 3 slider

## MIDI
- Tastiera 32 tasti (F3..C6) a (380, 5) 360×85
- 3 righe: SynthA, SynthB, DRUM con CH dropdown
- SPLIT dropdown (32 note)


--------------------------------------------------------------------------------
3. PRESET SYSTEM
--------------------------------------------------------------------------------

- SD: /preset/synthA/preset_00..29.bin + nomi_presetA.txt
      /preset/synthB/preset_00..29.bin + nomi_presetB.txt
- Formato: [version 0x01][key][len][value]...[0xFF][crc8]
- Cache locale: `PresetCache { int32_t value[256]; uint8_t type[256]; }`
  - `cacheA[5]` (5 voci SynthA), `cacheB`
- Helper UI (in preset_ui.h):
  - `uiSetParamU8(voice, key, value)`
  - `uiSetParamI32(voice, key, value)`
  - `uiSetParamB_U8(key, value)`
  - `uiSetParamB_I32(key, value)`
- Wrapper nel .ino:
  - `requestPresetLoad(synth, id)`
  - `requestPresetSave(synth, id)`
  - `requestRenameApply(synth, id, newName)`
- Funzioni transfer: `loadPresetSynthA_Poly`, `loadPresetSynthA_MultiMono`,
  `loadPresetSynthB`, `savePresetA`, `savePresetB`


--------------------------------------------------------------------------------
4. COMUNICAZIONE LWS
--------------------------------------------------------------------------------

- Serial1 @ 1 Mbps verso Router (GPIO 18=RX? — verificare, attuale in .ino:
  `Serial1.begin(LWS_BAUD, SERIAL_8N1, 18, 17)`)
- Frame: [SENDER][SEQ][CMD][LEN][PAYLOAD][CRC8][&][!]
- CRC-8/ATM (poly 0x07, init 0x00)
- Discovery non bloccante: `discover_all_mcu_start()` in setup,
  `discover_all_mcu_poll()` in loop, `discover_request_restart()` da SET UP
- Nodi: 'D' Display, 'R' Router, 'a' SynthA_M, 'b' SynthA_V, 'c' SynthA_V2,
  'B' SynthB, 'T' Teensy
- CMD usati dal Display:
  - `send_param_update(target, key, value)` per CMD_PARAM
  - `CMD_PRESET_*` per trasferimento preset
  - `CMD_DRUM_PATTERN 'W'` per PTN numero/nome
  - Teensy: 'Q' = request pattern, 'Y' = save pattern


--------------------------------------------------------------------------------
5. VCF SYNTHB — STATO E CHIAVI LWS
--------------------------------------------------------------------------------

Stati UI (3+3 = 6 combinazioni):
- FLT/UNI → F1=CUT, F2=DET, F3=off
- FLT/SLV → F1=CUT, F2=INT, F3=INT
- FLT/FRE → F1=CUT, F2=CUT, F3=CUT
- WOV/POT → F1=A E I O U A, F2=FORM, F3=off
- WOV/ENV → F1,F2 nascosti + 3 dropdown, F3=TIME
- WOV/RND → F1,F2 disabilitati, F3=TIME

Invii a SynthB:
- 'B' = sB_vcf_mode (0=Filter, 1=Wovel)
- 'C' = sB_filter_submode (0=UNI, 1=SLV, 2=FRE)
- 'K' = sB_wovel_submode (0=POT, 1=ENV, 2=RND)
- 'e' = sB_filter_type (0=LP, 1=BP)
- '1','2','3' = sB_vcf_cut[0..2] (0..255)
- 'V' = wov_vowel_A (dropdown ATT)
- 'Z' = wov_vowel_B (dropdown SUS)
- 'L' = wov_env_vowel_C (dropdown REL)
- **TODO**: mappatura LWS per RES/EnvA/EnvV (arc addizionali).


--------------------------------------------------------------------------------
6. EEPROM SETTINGS
--------------------------------------------------------------------------------

5 byte:
- [0] bright 0..100
- [1] midi_channel_A 1..16
- [2] midi_channel_B 1..16
- [3] midi_channel_D 1..16
- [4] midi_split 0..31 (0=F3)

API: `load_all_settings()` in setup, `save_all_settings()` da SET UP (id 21).


--------------------------------------------------------------------------------
7. COSE FATTE NELL'ULTIMA SESSIONE
--------------------------------------------------------------------------------

1. ✅ Refactoring lvglGrafFunc.cpp → 3 file + internal.h
2. ✅ `reset_ui_pointers()` e `page_begin()` fattorizzati
3. ✅ Pagina FX con frame FV-1 (dropdown, Rev, Salva, 3 slider)
4. ✅ Pagina VCF-B completa con 6 stati
5. ✅ Arc F1/F2/F3 con pallino target, crossing, tick radiali,
   top-label, lettere scala
6. ✅ Arc RES/EnvA/EnvV (sotto F1/F2/F3) con pallino, crossing, tick
7. ✅ Plotter ADSR blu sopra frame ENV vir
8. ✅ Slider A D S R senza freccina (VCF-B) + azzurri
9. ✅ Bottone SUBMODE a 3 stati ciclico (UNI→SLV→FRE / POT→ENV→RND)
10. ✅ `slider_plain()` e `env_plot_draw()` in lvglGrafCore.cpp


--------------------------------------------------------------------------------
8. COSE DA FARE (TODO)
--------------------------------------------------------------------------------

CRITICHE:
1. Compilare e testare end-to-end su hardware.
2. Verificare che la pagine VCF-B non abbia regressioni con VCF-A
   (VCF-A usa ancora `slider()` con freccina e non ha top-label).
3. Collegare i 3 arc RES/EnvA/EnvV a SynthB via LWS
   (chiavi da definire con firmware).
4. Nel ramo VCF/VCA di create_page, spostare i `#define` fuori dalla
   funzione (ora c'è `#define VCF_B_SLIDER_BLUE` inline).

FEATURE NON IMPLEMENTATE:
- DSP effetti (chiavi accettate, non applicate)
- MIDI mode split (parametri accettati, tastiera colorata)
- Preset dump (CMD_PRESET_READ stub)
- Drum pattern load/save su SD (solo LWS a Teensy)
- Modifica valori drum con drag (solo toggle)

IN SOSPESO:
- Test hardware end-to-end
- Test preset transfer da SD reale
- Verifica FPU su RP2350 (lato SynthB)
- Collisione 'Q' tra K_FM_DIV_1 e K_PRESET_SAVE (namespace diversi)


--------------------------------------------------------------------------------
9. CONVENZIONI
--------------------------------------------------------------------------------

- `g.synth` : "SYNTH A" o "SYNTH B" (impostato in create_page)
- Voce globale SynthA: 0..4; voce locale: 0..NUM_VOCI-1
- Voce SynthB: sempre 0 (parafonico 6 slot)
- Palette UI:
  - Verde `0x00FF00` PLAY / DCO / SUB_FILTER
  - Rosso `0xCC3300` SYNTH A / VCF / Salva (FX/DRUM)
  - Blu `0x0099FF` SYNTH B / MOD / Rev1
  - Giallo `0xFFCC33` DRUM / VCA / SUB_WOVEL
  - Viola chiaro `0xBB88FF` FX / DLY
  - Grigio `0x999999` SET UP
- Frame border grigio `0x888888`, label sopra il bordo con
  offset `-20` e `bg_opa=COVER` + `pad_hor=6..8`
- Tick radiali: 5 per arc, angoli 135/202.5/270/337.5/405
  (uniformi su 270°), pivot (0,15), rotazione `90 + deg`
- Top-label sopra tick superiore: offset `-17` da arc top-mid
- Value-label sotto arc: offset `-8` da arc bottom-mid


--------------------------------------------------------------------------------
10. PROSSIMO STEP
--------------------------------------------------------------------------------

Se hardware disponibile:
- Test pagina VCF-B con tutti i 6 stati in sequenza
- Verifica top-label sopra i tick radiali (CUT/DET/INT/FORM/TIME)
- Verifica crossing pallini F1/F2/F3 + RES/EnvA/EnvV
- Verifica disabilitazione F3 in FLT/UNI + WOV/POT
- Verifica EnvA disabilitato in WOV
- Verifica slider A D S R azzurri senza freccina

Se hardware non disponibile:
A) Compilare e pulire warning
B) Aggiungere collegamento LWS a RES/EnvA/EnvV
C) Spostare `#define VCF_B_SLIDER_BLUE` fuori dal corpo di create_page
D) Riscrivere pagina DRUM (drag per valori SD/H2)

================================================================================
FINE RIEPILOGO
================================================================================