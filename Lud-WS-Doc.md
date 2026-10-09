================================================================================
LUD-WS — RIEPILOGO DI CONTINUITÀ (AGGIORNATO)
================================================================================
Documento autocontenuto. Incollalo come primo messaggio in una nuova chat
per riprendere il lavoro da dove è stato interrotto.

Ultimo aggiornamento: 2026-10-03 (Display: DRUM/SONG/KIT/REV completi)
================================================================================

Ver,. 0.0.17

-------------------------------------------------------------------------------
0. STRUTTURA REPOSITORY SU GITHUB
-------------------------------------------------------------------------------
	Lud-WS (
	|
	|(submodules)
	|_____Lud-WS-Display
	|_____Lud-WS-Router
	|_____Lud-WS-Ctrl
	|_____Lud-WS-Mod
	|_____Lud-WS-SyntA_M
	|_____Lud-WS-SynthA_V
	|_____Lud-WS-SynthA_V2
	|_____Lud-WS-SynthB
	|_____Lud-WS-Teensy
	|_____Lud-WS-Power


-------------------------------------------------------------------------------
1. ARCHITETTURA GENERALE
-------------------------------------------------------------------------------

  Display (ESP32-S3, LVGL 800x480, SD preset + SD drum names)
     |  Serial1 @ 1 Mbps
     v
  Router (Pico 2 RP2350)
     |
     +-- Serial1 (GP0/GP1) @ 1 Mbps ------------> Teensy 4.1 [T]
     +-- SerialSynthA (SerialPIO GP2/GP3) @ 115200 -> SynthA_M [a]
     |   (cascata a -> b -> c, 1Mbps tra slave)
     +-- SerialSynthB (SerialPIO GP8/GP9) @ 115200 -> SynthB [B]
     +-- SerialCtrl   (SerialPIO GP6/GP7)  @ 115200 -> Ctrl [C]   (legacy)
     +-- SerialMod    (SerialPIO GP10/GP11) @ 115200 -> Mod [M]   (legacy)
     +-- SerialPower  (SerialPIO GP14/GP15) @ 115200 -> Power [P] (legacy)
     +-- SerialMidi   (SerialPIO GP12/GP13) @ 31250 -> MIDI IN

  Hardware target: TUTTI i synth su RP2350 (Pico 2).
  Patch PWM: PWM_CLKDIV_BASE = 4.8 su RP2350 (era 4.0).
  Clock IRQ PWM costante: 30500 Hz su entrambe le piattaforme.


-------------------------------------------------------------------------------
2. BUS LWS v1.1
-------------------------------------------------------------------------------

  Frame: [SENDER][SEQ][CMD][LEN][PAYLOAD...][CRC8][&][!]
  CRC-8/ATM (poly 0x07, init 0x00)
  Baud: 1 Mbps UART HW, 115200 SerialPIO, 31250 MIDI
  Vincolo: '&' e '!' vietati nel payload


-------------------------------------------------------------------------------
3. CMD CODES (serial_protocol.h)
-------------------------------------------------------------------------------

  Base:
    CMD_PING          'p'   [target]
    CMD_PONG          'P'
    CMD_PARAM         'S'   [target][key][value]
    CMD_PARAM_REL     'R'   [target][key][value] + ACK
    CMD_PARAM_ACK     'A'   [acked_seq][acked_cmd]
    CMD_ERROR         'E'   [target][msg...]
    CMD_TIMELINE      'B'
    CMD_MIDI_CC       'c'   [cc][value]
    CMD_MIDI_NOTE     'n'   [onoff][pitch][vel]
    CMD_MIDI_BEND     'b'
    CMD_DRUM_PATTERN  'W'   [ptn_num][name...]

  SynthA estesi:
    CMD_PARAM_VOCE    'V'   [target][voice][key][value]
    CMD_PARAM_I32     'I'
    CMD_PARAM_I32_V   'J'
    CMD_MIDI_NOTE_V   'N'
    CMD_MIDI_BEND_V   'M'
    CMD_MIDI_CC_V     'K'

  Preset transfer:
    'j' 'h' 'e' 'a' 'r' 'Q' 'L' 'O'

  Song bulk (Teensy rev 13):
    CMD_SONG_BEGIN    'q'   [song][len_lo][len_hi]
    CMD_SONG_CHUNK    'x'   [off_lo][off_hi][4 byte per slot] x N
    CMD_SONG_END      'k'   [crc8]


-------------------------------------------------------------------------------
4. MCU ID
-------------------------------------------------------------------------------

  'D' Display ESP32-S3     'R' Router Pico 2
  'a' SynthA_M             'b' SynthA_V       'c' SynthA_V2
  'B' SynthB               'T' Teensy
  'C' Ctrl  'M' Mod  'P' Power  (legacy)


-------------------------------------------------------------------------------
5. LUD-WS-ROUTER (Pico 2, v0.0.5) — COMPLETO
-------------------------------------------------------------------------------

  UART mapping:
    Serial1 (UART0) GP0/GP1 @ 1M    -> Teensy
    Serial2 (UART1) GP4/GP5 @ 1M    -> Display
    SerialSynthA    GP2/GP3 @ 115200 -> SynthA (cascata)
    SerialSynthB    GP8/GP9 @ 115200
    SerialCtrl      GP6/GP7  @ 115200
    SerialMod       GP10/GP11 @ 115200
    SerialPower     GP14/GP15 @ 115200
    SerialMidi      GP12/GP13 @ 31250

  Bridge Display<->Nodi, MIDI->CMD_MIDI_*_V, forwarding preset,
  discovery PING/PONG, heartbeat 1 Hz.


-------------------------------------------------------------------------------
6. LUD-WS-TEENSY (Teensy 4.1) — REV 13
-------------------------------------------------------------------------------

6.1 PIN / BUS

  LWS   : Serial3 (TX=14, RX=15) @ 1 Mbps   -> Router
  SPI   : MISO=12, MOSI=11, SCK=13 (condiviso SD + SerialFlash)
  SD CS : 10      Flash CS : 6
  I2S   : da Teensy Audio Shield
  USB   : Serial (LWS_DEBUG 115200)
  MIDI hardware: RIMOSSO

6.2 ARCHITETTURA AUDIO

  Sorgenti drum (SerialFlash): 9 voci (BD, SD, HH, OH, HH2, CLAP,
  PERC1..3). Sorgenti tracce (SD): playSdWav + playSdWav2.
  Amplificatori velocity per voce, choke env HH/OH.
  Mixer multi-livello, Freeverb, preDelay, filter/filter2.
  Output: AudioOutputI2SQuad + 2x SGTL5000.
  Regola velocity: velGain[4] = {0.0, 0.4, 0.7, 1.0}

6.3 LAYOUT SEQARR / SEQFILLARR

  Voce  Idx  Mute bit
  BD    0    0        SD  1  1        HH  2  2 (HH+OH)
  OH    3    2        HH2 4  3        CLAP 5  4
  PERC1 6    5        PERC2 7  5      PERC3 8  5

  Valori cella: 0=silenzio 1=soft 2=medium 3=accent

6.4 PATTERN 64 STEP (4 sezioni)

  A=0..15  B=16..31  C=32..47  D=48..63
  - default A loop
  - cambio manuale B/C/D via 'o' (pendente al boundary)
  - fill one-shot via 'h': 16 step poi torna a sezione pendente

6.5 FILL

  16 fill × 9 voci × 16 step. Banco seqFillArr[16][9][16].
  Trigger 'h', selezione 'i'.

6.6 SONG SEQUENCER (rev 13)

  songArr[16][4][256]:  (aggiornato a 4 colonne)
    [0] tipo   0=A 1=B 2=C 3=D 4=fill
    [1] num    pattern 0..15 o fill 0..15
    [2] kit    0..15 (indice diretto)
    [3] rev    0..15 (indice preset REV)
  songLen[16] = slot effettivi (1..256).

  Play mode via 'y' (0=pattern, 1=song), solo a sequencer fermo.

6.7 FORMATI SD (Teensy)

  PTNxx.BIN        581B  magic 'P' ver 0x02 rows 9 cols 64
  PTN_FILLxx.BIN   149B  magic 'F' ver 0x01 rows 9 cols 16
  SONGxx.BIN       517B  magic 'S' ver 0x01 (ver 0x02 per 4 colonne)

6.8 COMANDI LWS (target 'T')

  Trasporto: 'R' run  'S' stop
  Selezione: 'N' pattern 0..15  'K' kit 0..15  'T' track 0..4
             'X' sync 0/1/2
  Livelli:   'd' drum  'f' fx  'm' track1  't' track2  'u' track3
  Timing:    'b' bpm 60..250  'W' swing 0..swingMax
  Mute:      'M' bit0=BD 1=SD 2=HH+OH 3=HH2 4=CLAP 5=PERC
  Edit kit:  'V' voice 1..9  'A' sample  'L' voice level 0..20
             'P' voice pan 0..100  'I' rev int
  Edit fx:   'e' preset 0..15  '1' revSize  '2' revDamp  '3' preDly
             '4' cut1  '5' res1  '6' cut2  '7' res2  '8' ser/par
  Playback:  'y' playmode 0/1  's' song 0..15  'o' section 0..3
             'i' fill select 0..15  'h' fill trigger 1
  Sistema:   'Z' dump status  'C' cpu info

  Echo BPM:  dopo 'b' il Teensy manda echo 'b' e 'W'.
  Echo fill: 'h' 0=idle 1=playing 2=pending.
  Echo kit:  'K' ritorna 0..15 (indice diretto).

6.9-6.11 NOTE

  - CurBlock[9] con puntatori a 16 step per voce
  - advanceBlock() gestisce fill pendente al boundary
  - Libreria Audio di Teensyduino (non locale)
  - LWS_OUT_TARGET 'R' per l'echo


-------------------------------------------------------------------------------
7. LUD-WS-SYNTHA (3 chip da un solo sketch) — COMPLETO
-------------------------------------------------------------------------------

  'a': NUM_VOCI=1, VOCE_BASE=0
  'b': NUM_VOCI=2, VOCE_BASE=1
  'c': NUM_VOCI=2, VOCE_BASE=3

  Modello: 1 voce monofonica con 2 sub_voci (base + sub1 con interval
           -24..+24 e subLevel). ADSR per-voce, Note stack, Glide.
           FM op params globali al chip.
  Modi: Poly (broadcast dal Master) / MultiMono.
  Porte: UP/DOWN in cascata (115200 SerialPIO / 1Mbps UART).
  Chiavi come da specifica precedente (uint8 + int32).


-------------------------------------------------------------------------------
8. LUD-WS-SYNTHB (RP2350) — IN CORSO
-------------------------------------------------------------------------------

  Parafonico 6 slot (POLIMAX=6), wavetable unica condivisa.
  Voice sempre 0.

  Pin map:
    Serial1 GP0/GP1 @ 115200 -> Router
    OUTPUT_A_PIN    GP15 -> PWM audio
    PIN_ANA_ATTACK/DECAY/RELEASE GP2/GP3/GP4 (4066)
    PIN_4051_A/B/C  GP5/GP6/GP7 (CD4051)
    PIN_ADC_ENV     GP26 (feedback)
    PIN_TLC_DATA/CLK/LOAD  GP8/GP9/GP10 (TLC5628)

  TLC5628 canali: Ch0=VCF1, Ch1=VCF2, Ch2=VCF3, Ch3=Res,
                  Ch4=Sustain, Ch5-7 riservati.

  ADSR hardware: 3 GPIO (4066) + 3 GPIO (4051) + ADC feedback.
  State machine: IDLE -> ATTACK -> DECAY -> SUSTAIN -> RELEASE
  Modo AD: IDLE -> ATTACK -> RELEASE (no sustain, no retrigger).
  Soglie: ANA_ATTACK_THRESHOLD=1000, ANA_ZERO_THRESHOLD=10.

  LFO1 Mod (timbro), LFO2 Pitch (vibrato), LFO3 VCF (3 counter,
  spread, 8 forme, output bipolare -255..+255).

  VCF FILTER: sub 0=UNI 1=SLV 2=FRE
  VCF WOVEL:  sub 0=POT(AEIOU) 1=ENV(A->B->C) 2=RND
  Formanti nodeF1/F2/F3[10], offset K_WOV_FORMANT.

  Chiavi uint8: m w F o a u n 5 6 7 8 A D R S E y B C 1 2 3 4
                V Z H b c K L M g ! P Q
  Chiavi int32: i I f j J h x X Y q Q W k l G N

  Dual-core: loop1() rigenera mod2_wavetable[] con flag g_wt_busy.


-------------------------------------------------------------------------------
9. LUD-WS-DISPLAY (ESP32-S3, v0.17)
-------------------------------------------------------------------------------

9.1 STRUTTURA FILE (REFACTORING RECENTE)

  Lud-WS-Display/
  ├── Lud-WS-Display.ino
  ├── globals.h
  ├── comunicazioni.h / .cpp
  ├── serial_protocol.h
  ├── images.c + images/
  └── src/
      ├── preset/
      │   ├── preset_sd.h / .cpp
      │   ├── preset_cache.h / .cpp
      │   ├── preset_transfer.h
      │   └── preset_ui.h
      └── grafica/
          ├── lvglGraf.h
          ├── lvglGraf_internal.h
          ├── lvglGrafCore.cpp        (widget/utility)
          ├── lvglGrafPages.cpp       (pagine + eventi + reset)
          ├── lvglGrafDrum.cpp        (DRUM/SEQ)
          ├── lvglGrafSong.cpp        (SONG)
          ├── lvglGrafKit.cpp         (KIT)
          ├── lvglGrafRev.cpp         (REV)
          ├── lvglGrafVcfB.cpp        (VCF SynthB)
          ├── lvglGrafFx.cpp          (FX / FV-1)
          ├── lvglPreset.cpp          (dropdown preset + rename)
          └── lvglGrafModules.cpp.bak (VECCHIO — non compilare)

  Regola: NON devono coesistere lvglGrafModules.cpp e i nuovi file.

9.2 PAGINA DRUM (layout principale)

  Layout top:
    [SNG n Nome▾] [monitor: POS n KIT nome PTN n X REV n]   [BPM n]
     x=5(150)     170..~500                                  640..780

  Layout bottom (y=360):
    [← Home] [Play/Stop]  [PTN] [SONG] [KIT] [REV]
     x=10     x=110       300   410    530   640
     (55×45)  (90×90)     90×90 90×90  90×90 90×90

  Frame DRUM MIX (y=70, 710×285):
    8 slider 60px larghi, gap 30, ognuno con:
      - micro meter 22×24 in alto
      - slider al centro (range 0..20)
      - label nome (colorata) in basso
    Nomi: BD SD HH HH2 CLAP P1 P2 P3
    Colori voce: come righe PTN
    (HH unisce HH+OH nei meter)

  Slider SWING (x=723, y=70):
    - Label "SWING" top
    - Slider 60×200, range 0..swingMax
    - Label valore bottom, formato "NN %"

  BPM label/bottone in alto a destra:
    - 140×50 a (640, 10)
    - Tap → popup tastiera numerica (range 60..250)

9.3 PAGINA DRUM/PTN (editing pattern)

  Layout:
    Griglia a y=0 (top)
    Bottom row (y=400):
      [←] [PTN n▾] [A][B][C][D] [FL] [Play]      [Salva]
       x=5  x=70   175..410    415  480          705

  Griglia 9×16:
    X0=10, X1=770, Y0=0
    Label nome riga (14pt) a x=10
    Micro meter 22×24 a x=40
    Celle a x=54, quadrate 46×46
    Rettangolo colorato: larghezza = cw-22px
    4 separatori verticali (col 0,4,8,12): giallo (0,8), grigio (4,12)
    8 linee orizzontali grigio 0x666666
    Cursore step rosso (1px, period = 60000/bpm/4)

  Celle 4 stati (0=vuoto, 1=1/3, 2=2/3, 3=tutto):
    - Tap breve: 0→1→2→3→0
    - Long-press 1s: stato → 0

  Micro meter: attacco istantaneo + HOLD 100ms + DECAY 500ms.
  Parametri: DRUM_METER_HOLD_MS, DRUM_METER_DECAY_MS, DRUM_METER_TICK_MS=25.

  Sezioni A/B/C/D: bottone attivo bordo giallo.
  Bottone FL: verde se attivo, griglia mostra fillArr.
  Dropdown PTN/FLN: 95×45, bordo blu, niente freccia.

9.4 PAGINA DRUM/SONG

  Array:
    uint8_t  songArr[16][4][256];   // tipo, num, kit, rev
    uint16_t songLen[16];           // 1..256
    uint8_t  song_cur_song;         // 0..15
    uint8_t  song_cur_page;         // 0..7
    int      song_sel_slot;         // -1 = nessuno

  Layout top:
    [SNG n Nome▾] [KIT n Nome▾] [A][B][C][D] [FLN n▾] [REV n▾]
     x=5(150)      160(150)      320..510    550(95)  655(95)

  Frame griglia 4×8:
    GX=32, GY=55, GW=736, GH=352
    slot_w=92, slot_h=88, gap=4
    Bordo giallo scuro 0x886600
    Linee orizzontali grigio, verticale giallo tra col 4 e 5

  Slot 88×84 con:
    - Label "KIT n" (top-left, 14pt bianco) solo se DIVERSO dal precedente
    - Numero slot 1-based (top-right, 14pt grigio)
    - Tipo A/B/C/D/FLN n (center, 24pt)
    - Label "REV n" (bottom, 14pt rosso porpora 0xCC0066)
      solo se DIVERSO dal precedente
    - Vuoto se oltre songLen
    - Selezionato: bordo spesso 3px giallo 0xFFAA00

  Cursore play song: quadrato cw+4 × ch+4, contorno rosso 3px
    - Figlio di frame, pos = (c*slot_w, r*slot_h)

  Comportamento:
    - Tap slot: seleziona/deseleziona
    - Tap A/B/C/D: scrive tipo nel slot selezionato
    - FLN dd: scrive tipo=4 + fill num
    - KIT dd (0..15): scrive kit
    - REV dd (0..15): scrive rev
    - + : aggiunge slot (eredita kit/rev precedenti)
    - - : rimuove ultimo (min 1)
    - ◄ ►: cambia pagina (32 slot)
    - Tap su SNG/KIT/etc: aggiorna dropdown e monitor

  Bottom row (y=415):
    [←] [◄] 1/N [►] [+] [-] [Play] [Loop] [RINOMINA] [Salva]
     x=5 70 122 170 230 290 360   435     520       655

  Bottone Loop/1Shot: blu, alterna song_loop.
  Bottone Rinomina: finestra modale con tastiera.
  Bottone Salva: invia song al Teensy via 'q'/'x'/'k'.

9.5 PAGINA DRUM/KIT

  Layout top:
    [KIT n Nome▾] [REV n▾]      [RINOMINA]
     x=350(180)     x=540(120)    x=670(120)

  Griglia 3×3 di dropdown (y=70):
    CELL_W=240, CELL_H=110, GRID_X=40
    Riga 1:  [BD▾]     [SD▾]     [HH▾]
    Riga 2:  [OH▾]     [H2▾]     [CLAP▾]
    Riga 3:  [PERC1▾]  [PERC2▾]  [PERC3▾]

  Array/strutture:
    char kit_names[16][24]  (editabili in RAM)
    kitArr[10][16] copiato da lista.h (riga 9 = REV)
    maxArr[9] = {11,11,13,10,13,4,11,12,12}

  Nomi kit default (1..16):
    Lud1 Lud2 Lud3 808 909 Linn miniPop TR76 CR77 CR78 Hammond
    MPC60 DMX SP1200 RX5 User

  Nomi set per voce (da lista.h senza suffisso arr):
    BD(12) SD(12) HH(14) OH(11) H2(14) CLAP(5) PERC1(12) PERC2(13) PERC3(13)

  Comportamento:
    - Cambio KIT n → tutti i 9 dropdown + REV si aggiornano
    - Cambio singolo dropdown → solo visualizzazione
    - RINOMINA: finestra modale con tastiera LVGL
    - Persistenza su SD: TODO

9.6 PAGINA DRUM/REV

  Array revPresetArr[9][16] (copiato da specifica utente)
    riga 0 = room size → SIZE
    riga 1 = damping   → DAMP
    riga 2 = cut off   → CUT1
    riga 3 = res       → RES1
    riga 4 = pre dly   → PRED
    riga 5 = cut off2  → CUT2
    riga 6 = res 2     → RES2
    riga 7 = filtSet   → SER/PAR
    riga 8 = lev out   → LEV

  Layout top: [REV n▾] x=90 (dopo il titolo)
  8 slider (y=100, h=200, gap 30):
    PRED  SIZE  DAMP  CUT1  RES1  CUT2  RES2  LEV
  Colori:
    PRED 0x00DD00 verde    SIZE 0x00AAFF azzurro
    DAMP 0xAA44FF viola    CUT1/CUT2 0xCC0066 rosso porpora
    RES1/RES2 0xFFCC33 giallo   LEV 0xCC3300 rosso

  Bottom: [← Home]  [SER|PAR]
    Toggle SER blu 0x0099FF, PAR arancione 0xFF8800

  Slider con immagini, range 0..20, no freccina.

9.7 INTERDIPENDENZA DRUM/PTN/SONG

  Stato globale song (in lvglGrafSong.cpp):
    bool     song_playing;
    int      song_play_slot;   // 0..255
    int      song_play_step;   // 0..15
    bool     song_loop;

  Funzioni helper:
    song_current_section()  → -1 se fill o non playing
    song_current_pattern()  → -1 se non playing
    song_current_is_fill()

  Timer song: period = 60000/drum_bpm/4
  Ad ogni tick:
    1. Trigger meter MIX (DRUM)
    2. Se g_current_page == "SEQ":
         - drum_meters_update_from_slot()
         - aggiorna cursore PTN
    3. Avanza step
    4. A fine slot: aggiorna UI (dropdown), se PTN ricarica griglia
    5. song_update_cursor_position()

  Editing permesso durante play: scrive nel pattern/sezione corrente.

  Play song: Play/Stop sono in DRUM e SONG, sincronizzati (stesso stato).

9.8 ALTRE PAGINE

  HOME: titolo + slider luminosità + log + 6 bottoni.
  SYNTH A/B: preset + submenu DCO/VCF/MOD/VCA/DLY.
  DCO A/B: WF/FM/AM ciclico + grid + plotter.
  VCF A/B: VCF-B con 6 stati (FLT/UNI-SLV-FRE + WOV/POT-ENV-RND).
  VCA A/B: 4 slider + plotter ENV.
  FX: frame FV-1 (dropdown, Rev toggle, Salva, 3 slider).
  MIDI: tastiera 32 tasti + 3 righe CH + SPLIT.
  SET UP: init SD, discovery, save.

9.9 COMANDI eb (id)

  -1 HOME   -2 torna synth   -3 DRUM   -4 DCO
  0..5 pagine   10..14 submenu   20 MIDI   21 save
  30 SEQ   31 Salva pattern   32 SONG   33 KIT   34 REV

9.10 CHIAVI EEPROM (5 byte)

  [0] bright 0..100
  [1..3] midi_channel_A/B/D 1..16
  [4] midi_split 0..31

9.11 NOTE RECENTI

  - Fix griglia PTN vuota al primo accesso:
    `lv_obj_update_layout(parent)` tra grid_create e grid_update
  - Micro meter MIX: necessitano drum_meter_init() chiamato
    in create_page("DRUM"), altrimenti il timer non esiste
  - Init seqArr/fillArr/songArr: funzioni rese pubbliche
    e chiamate sia da DRUM che da SONG
  - Kit 0..15 (indice diretto), non più 1..16
  - Slot KIT/REV mostrati solo se diversi dal precedente
  - Monitor formato: "POS n KIT nome PTN n X REV n"
    (o "FLN n" al posto di "PTN n X" per i fill)


-------------------------------------------------------------------------------
10. PYTHON GENERATOR
-------------------------------------------------------------------------------

  convert_ptn.py — DRUM (fatto)
  gen_sample_names.py — DRUM (fatto)
  Python generator preset SYNTH (da aggiornare con nuove chiavi SynthB)


-------------------------------------------------------------------------------
11. RP2350 — PATCH
-------------------------------------------------------------------------------

  #if defined(PICO_RP2350) || defined(ARDUINO_ARCH_RP2350)
    #define PWM_CLKDIV_BASE   4.8f
  #else
    #define PWM_CLKDIV_BASE   4.0f
  #endif
  #define PWM_IRQ_RATE_HZ   30500.0f

  Arduino IDE 2.x: Board Pico 2, CPU 150 MHz, -O3, USB Pico SDK.


-------------------------------------------------------------------------------
12. ARDUINO IDE 2 — SETUP
-------------------------------------------------------------------------------

  Teensy 4.1: CPU 600MHz, Optimize Fastest, USB Serial,
              PSRAM Disabled, libreria Audio Teensyduino.

  ESP32-S3: FQBN esp32:esp32:esp32s3:FlashSize=16M,
            PartitionScheme=app3M_fat9M_16MB,PSRAM=opi
            CPU 240 MHz, Flash QIO.

  Problemi noti:
    - "objs.a file truncated" → reinstallare pacchetto esp32
    - Cache sketch in AppData\Local\arduino\sketches


-------------------------------------------------------------------------------
13. CONVENZIONI
-------------------------------------------------------------------------------

  Palette Display:
    Verde 0x00FF00      PLAY / DCO / SUB_FILTER
    Rosso 0xCC3300      SYNTH A / VCF / Salva
    Blu   0x0099FF      SYNTH B / MOD / Rev1
    Giallo 0xFFCC33     DRUM / VCA / SUB_WOVEL
    Viola chiaro 0xBB88FF FX / DLY
    Grigio 0x999999     SET UP
    Arancione 0xFF8800  KIT
    Rosso scuro 0xAA0000 REV
    Giallo scuro 0x886600 bordi griglia
    Rosso porpora 0xCC0066 REV label negli slot

  Frame: bordo grigio 0x888888, label sopra (y=-20, bg_opa COVER)
  Dropdown: senza freccia via lv_dropdown_set_symbol(dd, NULL)


-------------------------------------------------------------------------------
14. HARDWARE ESTERNO SYNTHB
-------------------------------------------------------------------------------

  TLC5628 (DAC octal, 3 fili): parola 11 bit MSB-first.
    5 canali usati: VCF1/VCF2/VCF3/Res/Sustain.
    Filtro RC (R≈1kΩ, C≈4.7nF) + buffer.
  CD4051 (mux 8): resistenze A/D/R ADSR.
  CD4066 (4 switch): 3 per ATTACK/DECAY/RELEASE.
  ADC GP26: feedback envelope.
  AD5242: rimosso.


-------------------------------------------------------------------------------
15. COSE DA FARE (TODO)
-------------------------------------------------------------------------------

  CRITICHE (test hardware):
  1. Verifica end-to-end Display su hardware
  2. Test PTN (sezioni, fill, long-press, cursore)
  3. Test SONG (play, cursore, loop/1shot, monitor)
  4. Test KIT (dropdown voce, rinomina)
  5. Test REV (preset, slider, SER/PAR)
  6. Test BPM/Swing (echo dal Teensy)
  7. Sync Display <-> Teensy (songArr, kit)
  8. Persistenza kit_names e song_names su SD

  DISPLAY (feature mancanti):
  9. Mute toggle (6 bottoni, cmd 'M') in PTN
  10. Pagina PLAY completa (live: fill trigger, play mode, sezioni)
  11. Kit editor completo (rename sample)
  12. Text input LVGL riutilizzabile
  13. Lettura/scrittura /drum/*.txt

  FIRMWARE:
  14. Sync songArr Teensy <-> Display
  15. Aggiornare Python generator preset SynthB
  16. Allineare SynthB a WF 8 item
  17. Mappatura LWS RES/EnvA/EnvV

  IN SOSPESO:
  - Test hardware end-to-end (a->b->c->B->T)
  - Preset transfer da SD reale
  - Verifica FPU RP2350
  - Warning static non usate (innocui)


-------------------------------------------------------------------------------
16. PROSSIMO STEP CONSIGLIATO
-------------------------------------------------------------------------------

  Priorità alta:
  A) Test end-to-end Display su hardware (tutte le pagine)
  B) Sync Display <-> Teensy per songArr (usare 'q'/'x'/'k' già pronti)
  C) Persistenza kit_names/song_names su SD

  Priorità media:
  D) Mute toggle in PTN
  E) Pagina PLAY completa
  F) Python generator preset SynthB

  Priorità bassa:
  G) Kit editor con rename sample
  H) Text input riutilizzabile
  I) Mappatura LWS RES/EnvA/EnvV

================================================================================
FINE RIEPILOGO
================================================================================