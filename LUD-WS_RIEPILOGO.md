================================================================================
LUD-WS — RIEPILOGO DI CONTINUITÀ (AGGIORNATO)
================================================================================
Documento autocontenuto. Incollalo come primo messaggio in una nuova chat
per riprendere il lavoro da dove è stato interrotto.

Ultimo aggiornamento: 2026-10-14
Focus sessione: refactoring Display + nuove pagine (DLY, CHORUS, PHASER,
VCF A, FM Edit, LFO B) + ottimizzazione flash + fix layout
================================================================================


-------------------------------------------------------------------------------
0. STRUTTURA REPOSITORY
-------------------------------------------------------------------------------
	Lud-WS (
	|(submodules)
	|_____Lud-WS-Display      ← ESP32-S3, LVGL 800×480
	|_____Lud-WS-Router       ← Pico 2 RP2350
	|_____Lud-WS-Ctrl    ← Pico 2 RP2350
	|_____Lud-WS-Mod          ←  Pico RP2040 riceverà comandi CHORUS/PHASER/DLY
	|_____Lud-WS-SyntA_M      ← RP2350, 1 voce (voce globale 0)
	|_____Lud-WS-SynthA_V     ← RP2350, 2 voci (voci 1,2)
	|_____Lud-WS-SynthA_V2    ← RP2350, 2 voci (voci 3,4)
	|_____Lud-WS-SynthB       ← RP2350, parafonico 6 slot
	|_____Lud-WS-Teensy       ← drum sampler + sequencer
	|_____Lud-WS-Power    ← Pico RP2040

Target: tutti i synth su RP2350 (Pico 2).
Bus: LWS v1.1, frame [SENDER][SEQ][CMD][LEN][PAYLOAD][CRC8][&][!].


-------------------------------------------------------------------------------
1. ARCHITETTURA E BUS — rimando al riepilogo precedente
-------------------------------------------------------------------------------
Invariate le sezioni del riepilogo precedente relative a:
- Architettura generale e cascata SynthA
- Bus LWS v1.1 (CRC-8/ATM, cmd codes, MCU ID)
- Router (Pico 2, v0.0.5)
- Teensy rev 12 (layering drum, seqArr/fillArr/songArr, comandi LWS)
- SynthA (a/b/c), SynthB (motore, LFO3, ADSR, VCF, Wovel)
- Formati SD, Python generator, patch RP2350, setup Arduino IDE
- Hardware esterno (TLC5628, CD4051, CD4066, ADC)
- lista.h (nomi sample drum)




-------------------------------------------------------------------------------
2. LUD-WS-DISPLAY — STATO ATTUALE
-------------------------------------------------------------------------------
2.0 Arduino 2 . ESP32-S3 board :
	FQBN: esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=custom,PSRAM=opi
	custom (in partitions.csv) :
	"# Name,   Type, SubType, Offset,   Size,     Flags
nvs,      data, nvs,     0x9000,   0x5000,
otadata,  data, ota,     0xE000,   0x2000,
app0,     app,  ota_0,   0x10000,  0xF00000,
spiffs,   data, spiffs,  0xF10000, 0x0E0000,
coredump, data, coredump,0xFF0000, 0x10000,"
.

2.1 HARDWARE
  ESP32-S3, LVGL 8.4, 800×480, SD preset + SD drum names.
  Serial1 @ 1 Mbps verso Router (GPIO 18=RX, 17=TX).
  Display: VIEWE UEDX80480050E_WB_B, touch GT911.

2.2 STRUTTURA FILE (aggiornata)

  Lud-WS-Display/
  ├── Lud-WS-Display.ino
  ├── globals.h
  ├── comunicazioni.h / .cpp
  ├── serial_protocol.h
  ├── images.c + images/
  └── src/
      ├── preset/
      │   ├── preset_sd.h / .cpp        (fix static→inline, .cpp ricreato)
      │   ├── preset_cache.h / .cpp
      │   ├── preset_transfer.h
      │   └── preset_ui.h
      └── grafica/
          ├── lvglGraf.h
          ├── lvglGraf_internal.h
          ├── lvglGrafCore.cpp
          ├── lvglGrafPages.cpp
          ├── lvglGrafDrum.cpp
          ├── lvglGrafSong.cpp
          ├── lvglGrafKit.cpp
          ├── lvglGrafRev.cpp
          ├── lvglGrafVcfB.cpp      (unificato con ArcSet)
          ├── lvglGrafVcfA.cpp      ← NUOVO
          ├── lvglGrafFx.cpp
          ├── lvglGrafDly.cpp       ← NUOVO
          ├── lvglGrafChorus.cpp    ← NUOVO
          ├── lvglGrafPhaser.cpp    ← NUOVO
          ├── lvglGrafFmEdit.cpp    ← NUOVO
          ├── lvglGrafLfoB.cpp      ← NUOVO
          ├── lvglPreset.cpp
          └── lvglGrafModules.cpp.bak   (VECCHIO, non compilare)

2.3 HELPER IN CORE (lvglGrafCore.cpp + lvglGraf.h)
  Aggiunti in questa sessione:
  - mkbtn(parent, x, y, w, h, border_color, text, font, cb, user_data,
           radius=8, border_width=3) → bottone standard
  - styled_dropdown(parent, x, y, w, h, border_color, font, options)
  - open_rename_modal(title, initial_text, max_len, border_color,
                       on_apply, user_data) / close_rename_modal() /
    rename_modal_is_open() → finestra modale unificata per rename
  - typedef RenameApplyCb(const char *new_name, void *user_data)

2.4 REFACTORING COMPLETATO
  - reset_ui_pointers() splittata in 10 sotto-funzioni static
    (reset_core_widgets, reset_play_page, reset_keyboard_widget,
     reset_drum_grid, reset_drum_ptn_controls, reset_drum_main_controls,
     reset_drum_song_page, reset_drum_kit_page, reset_drum_rev_page,
     reset_vcfb_page, reset_fx_page) + chiamate:
     dly_reset_pointers(), chorus_reset_pointers(), phaser_reset_pointers(),
     fmEdit_reset_pointers(), lfoB_reset_pointers(), vcfA_reset_pointers()
  - Migrazione chiamanti a mkbtn/styled_dropdown/open_rename_modal su:
    Kit, Song, Preset, Pages(DRUM), Drum, Rev, Fx
  - lvglGrafVcfB.cpp: unificato con struct ArcSet (10 func → 5 func)
  - preset_sd.h: static → inline su getPresetPath/getNamesPath/ensureDirectory

2.5 FIX PADDING (importante!)
  Il parent "m" creato in create_page()/create_home() ha padding default
  20px del tema LVGL. Aggiunto lv_obj_set_style_pad_all(m, 0, 0) ovunque.
  Da ora: x=10 nel codice = 10px reali dal bordo schermo.

2.6 OTTIMIZZAZIONE FLASH (completata)
  - lv_conf.h: font non usati disabilitati (attivi solo 12,14,16,18,20,24,
    32,40). Aggiornato anche LV_FONT_DEJAVU_16_PERSIAN_HEBREW, SIMSUN,
    UNSCII, MONTSERRAT_12_SUBPX, MONTSERRAT_28_COMPRESSED.
  - LV_USE_LOG: TODO → suggerito di mettere a 0 (risparmio 30-60 KB).
  - LV_BUILD_EXAMPLES, LV_USE_DEMO_*: TODO → suggerito di mettere a 0.
  - Immagini rimosse:
    * img_bott_init/stop/play (72 KB) → sostituite con LV_SYMBOL_*
    * home.c (3.3 KB) → sostituito con LV_SYMBOL_LEFT
    * freccina.c / freccina_oriz.c (1.2 KB) → LV_SYMBOL_RIGHT / LV_SYMBOL_UP
    * img_slider_indicator.c / img_slider_oriz_indicator.c (67 KB) →
      unificate via #define img_slider_indicator img_slider_track
    * img_slider_oriz_* ancora presenti (prossimo step opzionale, -72 KB)
  - Immagini attuali: ~240 KB (era ~300 KB). Ulteriori ~110 KB disponibili
    se si eliminano anche gli slider orizzontali e i meter comp.

2.7 PAGINE — STATO DETTAGLIATO

  HOME:
    - Titolo LUD-WS + versione, slider luminosità top-right
    - Log widget 700×240 a (50, 97)
    - 6 bottoni: PLAY / SYNTH A / SYNTH B / DRUM / FX / SET UP

  PLAY:
    - 3 meter L/R/C
    - Timeline + controlli: [⏮ Rewind] [▶/■ Play/Stop]  (2 bottoni)
    - Rewind: LV_SYMBOL_PREV, azzera timeline_demo_ms
    - Play/Stop: toggle con label LV_SYMBOL_PLAY/LV_SYMBOL_STOP

  SYNTH A / SYNTH B:
    - Preset selector: [Preset A/B] + [dropdown] + [Rinomina] +
      [Salva] (rosso, y=0) + [Sel] (blu, y=66) — NO +/-
    - CHORUS (Synth A) / PHASER (Synth B) a (655, 170), verde scuro /
      arancione
    - Submenu: [DCO][VCF][LFO][VCA][DLY] (rinominato MOD→LFO)

  DCO A / DCO B:
    - Bottone ciclico WF→FM→AM
    - Griglia Wave Shape 4 colonne
    - Plotter + slider SHAPE (spostato +35px a destra → SL_X = PLOT_X+40)
    - Plotter: replica completa del firmware SynthB (wavefold, FM 3-op,
      AM). Funzioni helper:
      plot_gen_base, plot_apply_wavefold, plot_apply_fm + dispatch in
      update_wave_plot().
      NB: fmSetSin/fmSetDiv sono uint8_t[8][3] modificabili in Core.
      FM_PLOT_SCALE = 12 (esagerato per visibilità, non fedele al bit)
    - Bottone Edit visibile solo in FM → apre "FM Edit"

  FM Edit (DCO A / DCO B):
    - Dropdown "FM 1..8" a sinistra (x=10, y=5)
    - Salva rosso a destra (x=680, y=5)
    - 6 arc senza pallino: Sin1/2/3 (azzurro 0x00AAFF),
      Div1/2/3 (verde 0x00DD00) in basso (y=370)
      - Sin range 0..20, Div range 1..20
      - Modificano fmSetSin[preset][i] / fmSetDiv[preset][i] live
    - Plotter grande 780×285 (x=10, y=60) → ridisegna ad ogni modifica
    - Home bottom-left (x=10, y=375)
    - Vedi lvglGrafFmEdit.cpp

  LFO A (ex MOD A):
    - 6 pot P1..P6 invariati (create_pot_container)
    - Titolo "LFO A"

  LFO B (ex MOD B) — NUOVO:
    - 8 bottoni wave (4×2): SINE/SAW/RSAW/SQR (riga 1),
      RND/RND50/RND25/RND10 (riga 2)
      - x=10, w=95, h=55, gap=5/10
      - Off: bg 0x004488, bordo 0x0066AA spesso 2
      - On (selezionato): bg 0x00AA00, bordo 0x00FF44 spesso 3
      - Selettore a 8 posizioni → lfo3Wave, invia 'b'
    - 3 slider verticali con freccina:
      - RATE (azzurro 0x00AAFF) → invia 'k' (int32 350..80000 µs)
      - LEV (viola 0xAA44FF) → invia 'l' (int32 0..1023, slider ×4)
      - SPREAD (arancione 0xFF8800) → invia 'c' (uint8 0..255)
      - x=[430,530,630], y=60, w=60, h=200
      - Slider allineato TOP_MID con offset 20 (nome top, slider centro,
        label bottom)
    - Home bottom-left
    - Vedi lvglGrafLfoB.cpp

  VCF A:
    - Slider ADSR blu 0x0022CC senza freccina, in cornice "ENV vir"
      a (470, 160, 280, 290)
    - Plotter ADSR sopra la cornice (470, 58, 280, 100)
    - 5 arc/pot con pallino (slot 0..4 di g.arc_*):
      - Riga 1: CUT (125, 90), RES (255, 90)
      - Riga 2: EnvA (70, 230), EnvV (180, 230), LFO (290, 230)
    - Vedi lvglGrafVcfA.cpp
    - Nota: mappatura LWS verso SynthA da completare

  VCF B:
    - Unificato con ArcSet (struct in lvglGrafVcfB.cpp)
    - 6 slider ADSR, plotter, arc F1/F2/F3 + RES/EnvA/EnvV,
      dropdown wov env, SER/PAR
    - Invio LWS: 'B','C','e','1','2','3','V','Z','L','K','M','g'

  VCA A / VCA B: invariati

  DLY A / DLY B — NUOVO (lvglGrafDly.cpp):
    - Array dlyA_arr[10][16], dlyB_arr[10][16]:
      [0]=TIME, [1]=FB, [2]=CUT, [3]=RES, [4]=VCFLFO_depth,
      [5]=VCFLFO_rate, [6]=LFO_depth, [7]=LFO_rate, [8]=VCF_MODE,
      [9]=PREPOST
    - Dropdown "DLY A 1..16" / "DLY B 1..16" in alto a sinistra
    - Frame "DLY": bottone PRE/POST (0x00AAFF / 0xFF8800) + 4 arc
      (TIME, FB, LFO, RATE)
    - Frame "VCF": bottone LP/BP (0x0088FF / 0xFF2222) + 4 arc
      (CUT, RES, LFO, RATE)
    - 5 elementi per frame equidistanti:
      ELEM_X0=53, ELEM_DX=133, ELEM_W=80, ELEM_Y=30
    - FR_X=40, FR_W=720, FR_H=150, FR1_Y=65, FR2_Y=230
    - Home bottom-left a (10, 375, 90, 85)
    - Target MCU: 'M' (Lud-WS-Mod) — TODO invio LWS

  CHORUS — NUOVO (lvglGrafChorus.cpp):
    - Array chorus_arr[8][12]:
      [0]=MODE, [1]=OFFSET, [2]=LFO_SLOW_TIME, [3]=LFO_SLOW_GAIN,
      [4]=LFO_FAST_TIME, [5]=LFO_FAST_GAIN, [6..11]=EQ_220/510/730/
      1000/3500/6000
    - Dropdown "CRS 1..8" a sinistra (x=130, y=5), bordo verde 0x00AA00
    - Mode button SOL/EQ a (20, 100, 100, 100)
      - SOL: 0x00AA00, EQ: 0xFF6600
      - In SOL: slider EQ invisibili (indicator + knob + touch disabilitati)
      - In EQ: slider EQ visibili + touch attivi
    - Frame "EQ" (140, 65, 490, 200):
      6 slider (gradiente rosso→azzurro: 0xFF0000, 0xFF8800, 0xFFDD00,
      0x88DD00, 0x00DDAA, 0x00AAFF)
    - Slider "Offset" viola (0xAA44FF / passed 0x442266) a (660, 65, 60, 140)
      - Freccina rossa LV_SYMBOL_RIGHT, comportamento crossing come VCA
      - Label nome: arancione (0xFFA500) → bianca al crossing
    - Frame "LFO" (110, 320, 500, 115):
      4 arc (SlowTime, SlowGain, FastTime, FastGain)
      ARC_X0=45, ARC_DX=110
    - Salva rosso a (680, 5, 110, 45)
    - Home a (5, 360, 90, 90)
    - Target MCU: 'M' — TODO invio LWS

  PHASER — NUOVO (lvglGrafPhaser.cpp):
    - Array phaser_arr[8][3]:
      [0]=RATE, [1]=DEPTH, [2]=FB
    - Dropdown "PHS 1..8" (x=130, y=5), bordo 0xFF6600
    - Frame "PHASER" (100, 100, 600, 250):
      3 slider verticali (RATE verde 0x00DD00, DEPTH azzurro 0x00AAFF,
      FB viola 0xAA44FF)
    - Salva rosso a (680, 5, 110, 45)
    - Home a (5, 360, 90, 90)
    - Target MCU: 'M' — TODO invio LWS

  DRUM (principale):
    - Dropdown SNG (top-left)
    - Monitor label "POS x KIT nome PTN nn REV nn"
    - BPM button + Swing slider (top-right)
    - DRUM MIX frame (10, 70, 710, 315):
      8 container, ognuno: meter TOP (22×24), slider CENTER (60×180),
      nome BOTTOM
    - Bottoni bottom (y=390, h=60):
      [Home 60×60 @25] [Play/Stop 90×60 @110] [PTN 90×60 @300]
      [SONG 90×60 @410] [KIT 90×60 @530] [REV 90×60 @640]
    - Home usa LV_SYMBOL_LEFT, Play/Stop usa LV_SYMBOL_PLAY/STOP

  DRUM/PTN (SEQ): invariata, layout come da riepilogo precedente
    - Griglia a y=0, bottoni a y=400
    - Play/Stop con LV_SYMBOL_PLAY/STOP, font 24

  DRUM/SONG: layout come da riepilogo precedente
    - Play/Stop con LV_SYMBOL_PLAY/STOP, font 24
    - Rinomina: usa open_rename_modal (bordo verde 0x00DD00)

  DRUM/KIT:
    - Dropdown KIT 1..16 (350, 5, 180, 45) bordo 0xFF8800
    - Dropdown REV (540, 5, 120, 45) bordo 0xAA0000
    - Rinomina (670, 5, 120, 45) → open_rename_modal (bordo 0xFF8800)
    - Griglia 3×3 dropdown voci (40, 70)
    - Salva (680, 415, 110, 45) rosso → stub log+toast
    - Home (5, 415) LV_SYMBOL_LEFT
    - TODO: invio LWS, persistenza SD kit_names

  DRUM/REV:
    - Dropdown REV 1..16 (90, 5, 110, 45) bordo 0xAA0000
    - 8 slider (colorati per gruppo) start_x=55, gap=30
      PRED verde, SIZE azzurro, DAMP viola, CUT1/CUT2 rosso porpora,
      RES1/RES2 giallo, LEV rosso
    - SER/PAR toggle (340, 415, 120, 45)
      SER: blu 0x0099FF / PAR: arancione 0xFF8800
    - Salva (680, 415, 110, 45) rosso → stub log+toast
    - Home (5, 415) LV_SYMBOL_LEFT

  SET UP: invariata + MIDI
  MIDI: tastiera 32 tasti, 3 righe
  FX: frame FV-1, dropdown, toggle Rev1/Rev2, 3 slider

2.8 FONT USATI (8)
  Montserrat 12, 14, 16, 18, 20, 24, 32, 40.
  In lv_conf.h solo questi sono abilitati (=1).
  Altri: montserrat_8, 10, 22, 26, 28, 30, 34, 36, 38, 42, 44, 46, 48,
  subpx, compressed, dejavu, simsun, unscii → tutti a 0.

2.9 IMMAGINI ATTUALI (in images.c)
  Ancora usate:
  - img_slider_track (usato anche per indicator via #define)
  - img_slider_knob
  - img_slider_oriz_track / _knob (oriz_indicator via #define)
  - img_meter_audio_track / _indicator
  - img_meter_comp_track / _indicator
  - img_micro_meter_audio_track / _indicator
  - img_arc_bg / img_arc_indic
  Rimosse:
  - img_bott_init/stop/play  → LV_SYMBOL_PREV / PLAY / STOP
  - home.c                   → LV_SYMBOL_LEFT
  - freccina.c               → LV_SYMBOL_RIGHT (rosso)
  - freccina_oriz.c          → LV_SYMBOL_UP    (rosso)
  - img_slider_indicator.c   → #define img_slider_indicator img_slider_track
  - img_slider_oriz_indicator.c → #define img_slider_oriz_indicator
                                   img_slider_oriz_track


-------------------------------------------------------------------------------
3. COMANDI LWS USATI DAL DISPLAY
-------------------------------------------------------------------------------
  send_param_update(target, key, value)       → CMD_PARAM
  uiSetParamB_U8 / uiSetParamB_I32            → SynthB (target 'B')
  uiSetParamU8 / uiSetParamI32                → SynthA voce corrente
  send_param_voce / send_param_i32_voce       → voice-specific
  CMD_DRUM_PATTERN 'W'
  Teensy: 'Q' request pattern, 'Y' save pattern

  Chiavi SynthB già cablate dalla UI:
    DCO: wave_mode, wave, shape, fm_select
    VCF-B: B (mode), C (submode), e (filter type), 1/2/3 (cut),
           4 (res), V/Z/L (wov vowels), K (wov submode),
           M (wov attack), g (env src)
    LFO B: b (wave), c (spread), k (rate int32), l (lev int32)
    FM Edit (futuro LWS): modifiche a fmSetSin/Div → da mappare

  Chiavi SynthA già cablate:
    DCO A: via uiSetParamU8/I32 (voce corrente)


-------------------------------------------------------------------------------
4. TODO PRIORITARI
-------------------------------------------------------------------------------

DISPLAY — immediati:
  1. Invio LWS DLY A/B, CHORUS, PHASER → target MCU 'M'
     (attualmente solo array locali, stub log)
  2. Mappatura LWS VCF A (5 arc CUT/RES/EnvA/EnvV/LFO) verso SynthA
  3. Persistenza SD kit_names, chorus_arr, phaser_arr, dlyA/B_arr,
     fmSetSin/fmSetDiv
  4. Mute toggle (cmd 'M' bitmask) in DRUM/PTN
  5. BPM + Swing in DRUM/PTN (cmd 'b' 'W') — UI già presente
  6. Pagina PLAY completa: fill trigger, play mode, sezioni live,
     song select, BPM/Swing live
  7. Plotter LFO3 animato in tempo reale (opzionale)
  8. Mappatura LWS SynthB per lfo3 (rate/lev/spread) — già inviate,
     da confermare ricezione

SCRIPTS PC:
  9. Python generator preset SynthB aggiornato con nuove chiavi
     (VCF mode, LFO3, ADSR hardware, Wovel, env_src)

HARDWARE / TEST:
  10. Test hardware Teensy rev 12
  11. Test Display end-to-end dopo refactoring
  12. Collisione 'Q' K_FM_DIV_1 vs K_PRESET_SAVE (già nota)

SYNTHB:
  13. Invio valori reali fmSetSin/fmSetDiv al SynthB (attualmente
      valori provvisori hard-coded in lvglGrafCore.cpp)


-------------------------------------------------------------------------------
5. OTTIMIZZAZIONI FLASH ANCORA POSSIBILI
-------------------------------------------------------------------------------
  - LV_USE_LOG 0                          ~30-60 KB  (una riga!)
  - LV_BUILD_EXAMPLES 0, LV_USE_DEMO_* 0  igiene
  - img_slider_oriz_* → slider nativi      ~72 KB
    (attenzione: cambio visivo, serve decisione)
  - img_meter_comp_* → rotazione runtime   ~20 KB
  - img_arc_* → lv_arc nativi              ~38 KB
  - img_micro_meter_audio_* → lv_bar       ~3 KB
  - Widget non usati in lv_conf.h          0-5 KB (igiene)

  Stato attuale: ~240 KB di immagini.
  Potenziale risparmio residuo: ~130 KB.


-------------------------------------------------------------------------------
6. QUESTIONI APERTE / DECISIONI DA PRENDERE
-------------------------------------------------------------------------------
  1. LV_USE_LOG: mettere a 0?
  2. Slider orizzontali DCO: tenere le immagini (attuali) o sostituire
     con slider nativi? (cambio visivo)
  3. CHORUS/PHASER/DLY: quando inviare LWS al Mod? Serve definire il
     protocollo (probabilmente riuso pattern SONG 'q'/'x'/'k' con cmd
     dedicati)
  4. fmSetSin/fmSetDiv: come/editare/salvare su SD? Forse da "FM Edit"
     stesse con un pulsante Salva che scrive /fm/fmNN.txt
  5. Plotter FM in DCO A/B: attualmente usa FM_PLOT_SCALE 12 per
     visibilità; accettare o tornare a 1 (fedele al firmware)?


-------------------------------------------------------------------------------
7. PROSSIMO STEP CONSIGLIATO
-------------------------------------------------------------------------------
  Priorità alta — chiudono il cerchio SynthA/B:
  A) Invio LWS per CHORUS/PHASER/DLY (target 'M')
  B) Mappatura LWS VCF A (5 arc → SynthA)
  C) Persistenza SD dei nuovi array (chorus, phaser, dly, fm)

  Priorità media:
  D) LV_USE_LOG 0 (una riga, -30/60 KB)
  E) Pagina PLAY completa
  F) Mute toggle in PTN

  Priorità bassa:
  G) Ottimizzazione ulteriore immagini
  H) Python generator aggiornato


-------------------------------------------------------------------------------
8. FILE MODIFICATI IN QUESTA SESSIONE (per riferimento git)
-------------------------------------------------------------------------------
  Nuovi:
    src/grafica/lvglGrafDly.cpp
    src/grafica/lvglGrafChorus.cpp
    src/grafica/lvglGrafPhaser.cpp
    src/grafica/lvglGrafVcfA.cpp
    src/grafica/lvglGrafFmEdit.cpp
    src/grafica/lvglGrafLfoB.cpp
  Modificati:
    Lud-WS-Display.ino (pad_all in create_home)
    globals.h (rimossi extern img freccina, home, bott_*)
    src/grafica/lvglGraf.h (helper mkbtn, styled_dropdown, rename modal)
    src/grafica/lvglGraf_internal.h (prototipi nuovi moduli)
    src/grafica/lvglGrafCore.cpp (helper, submenu MOD→LFO, h_slider +35,
                                   plotter firmware replica, fmSetSin/Div)
    src/grafica/lvglGrafPages.cpp (DRUM bottom, FM Edit, CHORUS, PHASER,
                                    LFO A/B, DLY, pad_all=0, reset split,
                                    eb() aggiornato per LFO)
    src/grafica/lvglGrafDrum.cpp (layout MIX, sec/fl/play con mkbtn)
    src/grafica/lvglGrafSong.cpp (rename modal, play/stop simboli,
                                   bottoni con mkbtn)
    src/grafica/lvglGrafKit.cpp (rename modal, styled_dropdown, Salva)
    src/grafica/lvglGrafRev.cpp (styled_dropdown, Salva, home mkbtn)
    src/grafica/lvglGrafVcfB.cpp (ArcSet unificato)
    src/grafica/lvglGrafFx.cpp (styled_dropdown, Salva mkbtn)
    src/grafica/lvglPreset.cpp (rimossi +/-, Salva rosso top, Sel sotto,
                                 rename modal)
    src/preset/preset_sd.h (static→inline)
    src/preset/preset_sd.cpp (ricreato)
    images.c (rimosse bott_*, home, freccina*, indicator)

================================================================================
FINE RIEPILOGO
================================================================================