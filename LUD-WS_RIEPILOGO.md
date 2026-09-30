================================================================================
LUD-WS — RIEPILOGO DI CONTINUITÀ (AGGIORNATO)
================================================================================
Documento autocontenuto. Incollalo come primo messaggio in una nuova chat
per riprendere il lavoro da dove è stato interrotto.

Ultimo aggiornamento: 2026-09-28 (Display: refactoring lvglGraf + VCF-B + DCO A/B + DRUM)
================================================================================


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
     |
     +-- SerialSynthA (SerialPIO GP2/GP3) @ 115200 -> SynthA_M [a]
     |                                                  |
     |                                                  | Serial1 @ 1 Mbps
     |                                                  v
     |                                                SynthA_V [b]
     |                                                  |
     |                                                  | Serial2 @ 1 Mbps
     |                                                  v
     |                                                SynthA_V2 [c]
     |
     +-- SerialSynthB (SerialPIO GP8/GP9) @ 115200 -> SynthB [B]
     +-- SerialCtrl   (SerialPIO GP6/GP7)  @ 115200 -> Ctrl [C]   (legacy)
     +-- SerialMod    (SerialPIO GP10/GP11) @ 115200 -> Mod [M]   (legacy)
     +-- SerialPower  (SerialPIO GP14/GP15) @ 115200 -> Power [P] (legacy)
     +-- SerialMidi   (SerialPIO GP12/GP13) @ 31250 -> MIDI IN

  Hardware target: TUTTI i synth su RP2350 (Pico 2).
  Motivo: FPU hardware Cortex-M33 -> sinf/powf ~20x più veloci.
  Patch necessaria: PWM_CLKDIV_BASE = 4.8 su RP2350 (era 4.0 su RP2040).
  Clock IRQ PWM costante: 30500 Hz su entrambe le piattaforme.


-------------------------------------------------------------------------------
2. BUS LWS v1.1
-------------------------------------------------------------------------------

  Frame: [SENDER][SEQ][CMD][LEN][PAYLOAD...][CRC8][&][!]
  CRC-8/ATM (poly 0x07, init 0x00)
  Baud: 1 Mbps UART HW, 115200 SerialPIO, 31250 MIDI
  Vincolo: '&' e '!' vietati nel payload


-------------------------------------------------------------------------------
3. CMD CODES (serial_protocol.h condiviso)
-------------------------------------------------------------------------------

  CMD base:
    CMD_PING          'p'   [target]
    CMD_PONG          'P'
    CMD_PARAM         'S'   [target][key][value]
    CMD_PARAM_REL     'R'   [target][key][value] + ACK
    CMD_PARAM_ACK     'A'   [acked_seq][acked_cmd]
    CMD_ERROR         'E'   [target][msg...]
    CMD_TIMELINE      'B'   [cur_ms i32][tot_ms i32]
    CMD_MIDI_CC       'c'   [cc][value]
    CMD_MIDI_NOTE     'n'   [onoff][pitch][vel]
    CMD_MIDI_BEND     'b'   [bend i32]
    CMD_DRUM_PATTERN  'W'   [ptn_num][name...]

  CMD estesi SynthA:
    CMD_PARAM_VOCE    'V'   [target][voice][key][value]
    CMD_PARAM_I32     'I'   [target][key][i32_le]
    CMD_PARAM_I32_V   'J'   [target][voice][key][i32_le]
    CMD_MIDI_NOTE_V   'N'   [target][voice][onoff][pitch][vel]
    CMD_MIDI_BEND_V   'M'   [target][voice][bend i32_le]
    CMD_MIDI_CC_V     'K'   [target][voice][cc][value]

  CMD preset transfer:
    CMD_PRESET_BEGIN       'j'
    CMD_PRESET_CHUNK       'h'
    CMD_PRESET_END         'e'
    CMD_PRESET_ACK         'a'
    CMD_PRESET_READ        'r'
    CMD_PRESET_DUMP_BEGIN  'Q'
    CMD_PRESET_DUMP_CHUNK  'L'
    CMD_PRESET_DUMP_END    'O'


-------------------------------------------------------------------------------
4. MCU ID
-------------------------------------------------------------------------------

  'D'  Display        ESP32-S3
  'R'  Router         Pico 2 RP2350
  'a'  SynthA_M       master, 1 voce locale (voce globale 0)
  'b'  SynthA_V       slave, 2 voci locali (voci globali 1, 2)
  'c'  SynthA_V2      slave, 2 voci locali (voci globali 3, 4)
  'B'  SynthB         parafonico 6 slot
  'T'  Teensy         drum sampler + sequencer
  'C'  Ctrl, 'M' Mod, 'P' Power  (legacy)


-------------------------------------------------------------------------------
5. LUD-WS-ROUTER (Pico 2, v0.0.5) — COMPLETO
-------------------------------------------------------------------------------

  UART mapping:
    Serial1 (UART0)   GP0/GP1   @ 1 Mbps    -> Teensy
    Serial2 (UART1)   GP4/GP5   @ 1 Mbps    -> Display
    SerialSynthA      GP2/GP3   @ 115200    -> SynthA (cascata a->b->c)
    SerialSynthB      GP8/GP9   @ 115200    -> SynthB
    SerialCtrl        GP6/GP7   @ 115200
    SerialMod         GP10/GP11 @ 115200
    SerialPower       GP14/GP15 @ 115200
    SerialMidi        GP12/GP13 @ 31250

  Nodi LWS nativi: Teensy, SynthA (a, b, c)
  Nodi legacy: SynthB, Ctrl, Mod, Power

  Funzionalità: bridge Display<->Nodi, MIDI->CMD_MIDI_*_V, forwarding
  preset, discovery PING/PONG, heartbeat 1 Hz.


-------------------------------------------------------------------------------
6. LUD-WS-TEENSY (Teensy 4.1) — REV 12
-------------------------------------------------------------------------------

6.1 PIN / BUS

  LWS   : Serial3 (TX=14, RX=15) @ 1 Mbps   -> Router
  SPI   : MISO=12, MOSI=11, SCK=13 (condiviso SD + SerialFlash)
  SD CS : 10
  Flash CS : 6
  I2S   : da Teensy Audio Shield (pin 7,8,18,19,20,21,23)
  USB   : Serial (LWS_DEBUG a 115200)
  MIDI hardware: RIMOSSO (Serial6 TX24/RX25 libero)

6.2 ARCHITETTURA AUDIO

  Sorgenti drum (SerialFlash):
    soundBd, soundSd, soundHhC, soundHhO, soundHh2,
    soundClap, soundPerc1, soundPerc2, soundPerc3
  Sorgenti tracce (SD):
    playSdWav (track 1 stereo: out0=peak, out1=mixOutMono ch0)
    playSdWav2 (track 2+3: out0=ch1, out1=ch2)

  Amplificatori di velocity (uno per voce):
    ampBd, ampSD, ampHh, ampOH, ampHH2,
    ampClap, ampPerc1, ampPerc2, ampPerc3
  Envelope di choke per HH/OH:
    envHh, envOH (release 5 ms per taglio pulito)
  Sottomixer HH/OH: mixHH

  Mixer:
    mixDrum1L/R (BD ch0, HH+OH ch1, SD ch2, CLAP ch3)
    mixDrum2L/R (HH2 ch0, PERC1 ch1, PERC2 ch2, PERC3 ch3)
    mixRevInt, mixOutRev, mixOutDly
    mixOutMono (track1, track2, track3)
    mixOutL/R (mix finale)
  Effetti: preDelay, revInt (Freeverb stereo), filter, filter2
  Output: AudioOutputI2SQuad + 2x SGTL5000

  Regola velocity: velGain[4] = {0.0, 0.4, 0.7, 1.0}

6.3 LAYOUT SEQARR / SEQFILLARR

  Voce    Idx   Mute bit   Note
  BD       0     0
  SD       1     1
  HH       2     2         HH+OH insieme
  OH       3     2
  HH2      4     3
  CLAP     5     4
  PERC1    6     5         PERC1/2/3 insieme
  PERC2    7     5
  PERC3    8     5

  Valori cella: 0=silenzio, 1=soft, 2=medium, 3=accent

6.4 PATTERN A 64 STEP (4 SEZIONI)

  A = step  0..15
  B = step 16..31
  C = step 32..47
  D = step 48..63

  In pattern mode:
    - default: A loop
    - cambio manuale a B/C/D via 'o' (pendente al boundary)
    - fill one-shot via 'h' (pendente al boundary): suona 16 step,
      poi torna alla sezione pendente (default A)
    - ultima sezione richiesta vince se cambia durante il fill

6.5 FILL

  16 fill indipendenti × 9 voci × 16 step.
  Banco separato da seqArr (array seqFillArr[16][9][16]).
  Trigger via 'h', selezione via 'i'.

6.6 SONG SEQUENCER

  16 song × 256 slot.
  songArr[16][2][256]:
    songArr[s][0][i] = tipo   (0=A 1=B 2=C 3=D 4=fill)
    songArr[s][1][i] = numero (pattern 0..15 per tipo 0..3,
                               fill 0..15 per tipo 4)
  songLen[16] = numero slot effettivi (0 = song vuota).
  Play mode selezionabile via 'y' (0=pattern, 1=song), solo a
  sequencer fermo.
  In song mode il kit resta globale; cambia solo il pattern/fill
  per ogni slot. Cambio kit pendente applicato al boundary.

6.7 FORMATI FILE SD (Teensy)

  Tutti in root SD, con header + CRC-8/ATM:
    [magic][version][rows][cols] + payload + [crc8]

  PTNxx.BIN          581 B   magic 'P' ver 0x02 rows 9 cols 64
                              accetta anche ver 0x01 (9x32 legacy),
                              upscalato in RAM (C+D a zero)
  PTN_FILLxx.BIN     149 B   magic 'F' ver 0x01 rows 9 cols 16
  SONGxx.BIN         517 B   magic 'S' ver 0x01
                              [2]=songLen [3]=reserved
                              payload: 256 × [tipo, numero]

  Naming canonico: PTN00.BIN..PTN15.BIN, PTN_FILL00.BIN..PTN_FILL15.BIN,
  SONG00.BIN..SONG15.BIN.

6.8 COMANDI LWS (target 'T')

  Trasporto:
    'R'  run
    'S'  stop

  Selezione:
    'N'  pattern select        0..15
    'K'  kit select            1..16
    'T'  track select          0..4
    'X'  sync mode             0=int, 1=SD, 2=ext

  Livelli master:
    'd'  drum level            0..20
    'f'  fx level              0..20
    'm'  track1 level          0..20
    't'  track2 level          0..20
    'u'  track3 level          0..20

  Timing:
    'b'  bpm                   60..250
    'W'  swing                 0..swingMax
    'F'  fill count            DEPRECATO (accettato, ignorato)

  Mute (bitmask):
    'M'  bit0=BD 1=SD 2=HH+OH 3=HH2 4=CLAP 5=PERC

  Edit kit:
    'V'  voice select          1..9
    'A'  sample number         0..maxArr[voice-1]
    'L'  voice level           0..20
    'P'  voice pan             0..100 (100=mono)
    'I'  rev int level         0..20

  Edit fx:
    'e'  fx preset             0..15
    '1'  revSize               0..20
    '2'  revDamp               0..20
    '3'  preDly                0..20
    '4'  cut filter1           0..20
    '5'  res filter1           0..20
    '6'  cut filter2           0..20
    '7'  res filter2           0..20
    '8'  mode (ser/par)        0/1

  Playback nuovo modello:
    'y'  play mode             0=pattern, 1=song
                               (rifiutato se seqRunning==1)
    's'  song select           0..15
    'o'  section select        0=A, 1=B, 2=C, 3=D
                               (pendente al boundary)
    'i'  fill select           0..15 (pendente)
    'h'  fill trigger          1=trigger (pendente)
                               echo: 0=idle 1=playing 2=pending

  Sistema:
    'Z'  dump status
    'C'  cpu info (CPU% + mem usage)

6.9 GESTIONE FILL PENDENTE

  - 'i' scrive preFillNum; 'h' alza fillPending.
  - Al boundary (fine di ogni blocco 16 step), advanceBlock():
      1) se playingFill: chiudi fill, vai a preSection, echo 'h'=0
      2) altrimenti se fillPending: avvia fill (fillNum=preFillNum),
         echo 'h'=1
      3) altrimenti: applica preSection
      4) applica prePtnNum (se non in fill)
      5) applica preKitNum
  - Ultima sezione richiesta durante fill vince.
  - Doppio trigger ignorato finché playingFill||fillPending.

6.10 GESTIONE BLOCCHI

  curBlock[9]: puntatori ai 16 step correnti per voce.
  setCurrentBlock(type, idx):
    type 0..3 -> &seqArr[idx][v][type*16]
    type 4    -> &seqFillArr[idx][v][0]
  seqRun() legge sempre curBlock[v][curSubStep], indipendente dal
  contesto (pattern sezione vs fill vs song).

6.11 NOTE

  - Rimuovere dalla libreria locale D:\sketchs\libraries\Audio:
    usare quella di Teensyduino.
  - Definire LWS_OUT_TARGET 'R' nel .ino per l'echo.
  - AudioMemory(260).
  - Mute HH+OH condiviso (bit 2); PERC condiviso (bit 5).


-------------------------------------------------------------------------------
7. LUD-WS-SYNTHA (3 chip da un solo sketch) — COMPLETO
-------------------------------------------------------------------------------

  MCU_ID 'a' -> NUM_VOCI=1, VOCE_BASE=0
  MCU_ID 'b' -> NUM_VOCI=2, VOCE_BASE=1
  MCU_ID 'c' -> NUM_VOCI=2, VOCE_BASE=3

  File: Lud-WS-SynthA_M.ino, synthConfig.h, synthState.h, synthEngine.h,
        synthControl.h, synthLws.h, synthPreset.h, ottaveA.h,
        serial_protocol.h, comunicazioni_mcu.h

  Modello: 1 voce monofonica con 2 sub_voci (base + sub1 con interval
           -24..+24 e subLevel). ADSR per-voce. Note stack. Glide.
           FM operator parameters globali al chip.

  Modi: Poly (broadcast automatico dal Master) / MultiMono (indipendenti).

  Porte:
    'a': UP=SerialPIO(2,3)@115200, DOWN=Serial1@1Mbps
    'b': UP=Serial1@1Mbps, DOWN=Serial2@1Mbps
    'c': UP=Serial1@1Mbps, HAS_DOWNSTREAM=0

  Chiavi uint8 per-voce (CMD_PARAM_VOCE):
    m w c F o a k d t v l s 1 3 4 u n B M S y D r R A T g G H L O C Z X
    e 9 5 6 7 8

  Chiavi int32 per-voce (CMD_PARAM_I32_V):
    i I f j J x X Y q Q W h

  Sistema (Master): Y P Q U ! O


-------------------------------------------------------------------------------
8. LUD-WS-SYNTHB (RP2350) — IN CORSO, quasi completo
-------------------------------------------------------------------------------

  Nodo singolo, parafonico 6 slot (POLIMAX=6), wavetable unica condivisa.
  Comandi LWS con voice sempre 0.

  File:
    Lud-WS-SynthB.ino           (bootstrap + driver TLC inline + setup1/loop1)
    synthB_Config.h             (config + chiavi K_* + PWM_CLKDIV_BASE)
    synthB_State.h              (extern)
    synthB_Engine.h             (IRQ PWM, note stack, alloc slot, glide, aux ADSR)
    synthB_Control.h            (LFO mod + pitch)
    synthB_AnalogEnv.h          (ADSR hardware via pin diretti)
    synthB_Lfo3.h               (terzo LFO con 3 counter + spread)
    synthB_Vcf.h                (gestione VCF Filter/Wovel)
    synthB_Lws.h                (callback, dispatch, preset entry)
    synthPreset.h               (utility preset comune)
    ottaveB.h                   (tabelle noteArr[61], voctpow[1230])

  Pin map (RP2350):
    Serial1 (GP0/GP1) @ 115200       -> LWS verso Router
    OUTPUT_A_PIN    GP15              -> PWM audio
    PIN_ANA_ATTACK  GP2               -> 4066
    PIN_ANA_DECAY   GP3               -> 4066
    PIN_ANA_RELEASE GP4               -> 4066
    PIN_4051_A      GP5               -> CD4051 A
    PIN_4051_B      GP6               -> CD4051 B
    PIN_4051_C      GP7               -> CD4051 C
    PIN_ADC_ENV     GP26 (ADC0)       -> feedback envelope
    PIN_TLC_DATA    GP8               -> TLC5628 DATA
    PIN_TLC_CLK     GP9               -> TLC5628 CLK
    PIN_TLC_LOAD    GP10              -> TLC5628 LOAD

  TLC5628 — canali usati:
    Ch 0 = OUTA -> VCF1 cutoff
    Ch 1 = OUTB -> VCF2 cutoff
    Ch 2 = OUTC -> VCF3 cutoff
    Ch 3 = OUTD -> Resonance
    Ch 4 = OUTE -> Sustain voltage (ADSR hardware)
    Ch 5,6,7 = riservati

  Motore audio:
    - Parafonico 6 slot, allocazione dinamica, steal-glide
    - Modi mono / poly (K_SYNTHB_MODE='y')
    - Slide per-slot
    - DCO: gate secco (nessun ADSR digitale applicato al DCO)
    - Aux ADSR digitale: parametri vir_* accettati (usati per Wovel env mode)
    - Wavetable unica

  ADSR hardware:
    - 3 GPIO per ATTACK/DECAY/RELEASE
    - 3 GPIO per CD4051 (resistenze A/D/R)
    - ADC GP26 per feedback
    - State machine: IDLE -> ATTACK -> DECAY -> SUSTAIN -> RELEASE
    - Modo AD: IDLE -> ATTACK -> RELEASE (no sustain, no retrigger)
    - Soglie: ANA_ATTACK_THRESHOLD=1000, ANA_ZERO_THRESHOLD=10

  LFO:
    LFO1 Mod (timbro): sinusoide, speedMod + modLev
    LFO2 Pitch (vibrato): sinusoide, speedPitchMod + modPitchLev
    LFO3 dedicato VCF: 3 counter indipendenti, spread 0..255
      8 forme: SINE, SAW, RSAW, SQR, RND, RND50, RND25, RND10
      Output bipolare in lfo3Mod[3] (-255..+255)

  VCF mode (synthB_Vcf.h):
    FILTER (K_VCF_MODE=0), sottomodi (K_VCF_SUBMODE):
      0 = Unisono : vcf2=vcf3=vcf1, cut2/cut3 ignorati
      1 = Slave   : vcf2=vcf1+(cut2-128), vcf3=vcf1+(cut3-128)
      2 = Free    : ogni VCF indipendente

    WOVEL (K_VCF_MODE=1), sottomodi (K_WOV_SUBMODE):
      0 = pot_aeiou  : pot 0..255 -> 5 transizioni A->E->I->O->U->A
      1 = env_aeiou  : vocale A (start) -> B (sustain) -> C (release)
                      guidato da ADSR digitale o hardware
                      (K_WOV_ENV_SRC: 0=vir, 1=ana)
                      vocali settabili: K_WOV_VOWEL_A, K_WOV_VOWEL_B,
                      K_WOV_ENV_VOWEL_C
      2 = random     : vocale random su NoteOn, tempo transizione
                      K_WOV_RND_TIME_MS

    3 array formanti nodeF1/F2/F3[10] per vocali A E I O U
    Offset formant: K_WOV_FORMANT (0..500)

  Chiavi LWS (uint8, CMD_PARAM):
    m=wave_mode (0=WF, 1=FM, 2=AM)
    w=wave (0..8)
    F=fm_select (0..7)
    o=ottava (1..3)
    a=attenua (0..9)
    u=bend up, n=bend down
    5/6/7/8=vir_adsr A/D/S/R
    A=ana_attack (0..7 ch 4051)
    D=ana_decay (0..7)
    R=ana_release (0..7)
    S=ana_sustain (0..255 -> 0..1023, scritto su TLC Ch4)
    E=ana_env_mode (0=ADSR, 1=AD)
    y=synthB mode (0=mono, 1=poly)
    B=vcf mode (0=Filter, 1=Wovel)
    C=vcf submode (0=Uni, 1=Slave, 2=Free)
    1=VCF1 cut, 2=VCF2 cut, 3=VCF3 cut, 4=VCF resonance
    V=wov_vowel_A (0..255), Z=wov_vowel_B (0..255)
    H=wov_morph (0..255)
    b=lfo3 wave (0..7), c=lfo3 spread (0..255)
    K=wov_submode (0=pot, 1=env, 2=random)
    L=wov_env_vowel_C (0..255)
    M=wov_env_attack (0..255, informativo)
    g=wov_env_src (0=vir, 1=ana)
    !=panic
    P=preset select, Q=preset save

  Chiavi LWS (int32, CMD_PARAM_I32):
    i=modInB, I=modLev, f=speedMod
    j=modPitchLev, J=speedPitchMod
    h=slide time (0..1000 ms)
    x/X/Y=FM sin 0/1/2 (float*100)
    q/Q/W=FM div 0/1/2 (1..600)
    k=lfo3 rate (350..80000 us)
    l=lfo3 lev (0..1023)
    G=wov_formant (0..500)
    N=wov_rnd_time_ms (0..5000)

  Dual-core:
    loop1() rigenera mod2_wavetable[] con flag g_wt_busy per race.


-------------------------------------------------------------------------------
9. LUD-WS-DISPLAY (ESP32-S3)
-------------------------------------------------------------------------------

9.1 HARDWARE

  ESP32-S3, LVGL 800x480, SD preset + SD drum names.
  Serial1 @ 1 Mbps verso Router.
  Cache locale: PresetCache cacheA[5] per SynthA, cacheB per SynthB.

9.2 FILE E ARCHITETTURA (REFACTORING RECENTE)

  Cartella:
    Lud-WS-Display/
    ├── Lud-WS-Display.ino
    ├── globals.h
    ├── comunicazioni.h
    ├── serial_protocol.h
    ├── images.c
    ├── images/
    └── src/
        ├── preset/
        │   ├── preset_sd.h / .cpp
        │   ├── preset_cache.h / .cpp
        │   ├── preset_transfer.h
        │   └── preset_ui.h
        └── grafica/
            ├── lvglGraf.h              (API pubblica)
            ├── lvglGraf_internal.h     (header privato condiviso 3 .cpp)
            ├── lvglGrafCore.cpp        (~1100 righe — widget/utility)
            ├── lvglGrafPages.cpp       (~700 righe — pagine + eventi)
            ├── lvglGrafModules.cpp     (~900 righe — drum/VCF-B/FX)
            └── lvglGrafFunc.cpp.bak    (VECCHIO — non compilare)

  Regola: NON devono coesistere lvglGrafFunc.cpp e i 3 nuovi (multiple
  definition). Il vecchio è rinominato .bak.

  Catena dipendenze:  Core  →  Pages  →  Modules

  `lvglGraf_internal.h`:
    - DBG macro con LGF_DBG (0/1)
    - `extern const uint8_t ui2fw_wave[9]` (in Core)
    - Statics condivise: DRUM, SYNTHB VCF, VCFB (RES/EnvA/EnvV), FX
    - Prototipi cross-file: drum_seq_page_create, vcf_page_synthb_create,
      fx_page_create, reset_ui_pointers, page_begin
    - `void sB_vcf_env_plot_update();`

  `lvglGrafCore.cpp` (14 sezioni):
    1. Widget base: btn, home_btn_at (statica), home_btn
    2. Slider: slider (con freccina), slider_plain (senza freccina)
    3. DCO: h_slider
    4. Plotter wave: grid_btn_click, populate_grid, update_wave_plot,
       update_plotter_by_wave
    5. LED/colori: update_leds, update_slider_color,
       update_shape_slider_color
    6. Arc: earc_changed, arc_with_image, create_pot_container
    7. Timeline: tl_set_buttons, create_timeline*, update_timeline
    8. Submenu (con suffisso A/B)
    9. Meter: meter, update_meters, stop_meters
    10. Log/toast: log_add, toast_show, create_log_widget
    11. EEPROM brightness
    12. reset_arrows
    13. Envelope plotter: create_env_plot, compute_env_points (statica),
        update_env_plot, env_plot_draw
    14. Keyboard: create_keyboard, update_keyboard_colors

  `lvglGrafPages.cpp`:
    - reset_ui_pointers(), page_begin(title)
    - create_preset_label (statica)
    - create_home(), create_page(title)
    - Event handler: eb, ebright, eslider, ecat_btn, ecat_cycle,
      fm_edit_btn_cb, ehslider, elist
    - Callback MIDI: midi_split_cb, midi_ch_a/b/d_cb
    - disc_btn_click

  `lvglGrafModules.cpp`:
    1. DRUM sequencer
    2. SYNTHB VCF page (6 stati + 5 arc + plotter ADSR)
    3. FX / FV-1

9.3 SISTEMA PRESET

  SD:
    /preset/synthA/preset_00.bin .. preset_29.bin
    /preset/synthA/nomi_presetA.txt
    /preset/synthB/preset_00.bin .. preset_29.bin
    /preset/synthB/nomi_presetB.txt

  Formato binario K-V:
    [version=0x01][key][len][value...]...[0xFF][crc8]

  Cache locale (Strada A):
    struct PresetCache { int32_t value[256]; uint8_t type[256]; }
    PresetCache cacheA[5]  (una per voce SynthA)
    PresetCache cacheB     (SynthB)

  Helper UI (preset_ui.h):
    uiSetParamU8(voice, key, value)
    uiSetParamI32(voice, key, value)
    uiSetParamB_U8(key, value)
    uiSetParamB_I32(key, value)

  Funzioni transfer:
    sendBlobToVoice, loadPresetToVoice
    loadPresetSynthA_Poly, loadPresetSynthA_MultiMono, loadPresetSynthB
    savePresetA, savePresetB

  Wrapper nel .ino:
    requestPresetLoad, requestPresetSave, requestRenameApply

9.4 NAMING DRUM SU DISPLAY SD

  Nomi per kit, song, sample drum. I nomi vivono solo sul Display,
  il Teensy lavora con indici. Il Display traduce nome <-> indice
  prima di inviare comandi LWS.

  Organizzazione file:
    /drum/kit_names.txt          16 righe UTF-8, una per kit 0..15
    /drum/song_names.txt         16 righe UTF-8, una per song 0..15
    /drum/sample_names/
        bd.txt      (12 righe)
        sd.txt      (12 righe)
        hh.txt      (14 righe)
        oh.txt      (11 righe)
        hh2.txt     (14 righe, identico a hh.txt)
        clap.txt    (5 righe)
        perc1.txt   (12 righe)
        perc2.txt   (13 righe)
        perc3.txt   (13 righe)

  Formato: UTF-8, LF, una riga per nome. Max 24 caratteri consigliati.
  Righe vuote = fallback a nome default. File mancante = tutti default.

  Editing: GUI offre text input (lv_keyboard + lv_textarea) per
  rinominare kit, song e slot sample. Al salvataggio il Display
  riscrive il file .txt corrispondente. Nessun comando LWS.

9.5 NOMI SAMPLE DI DEFAULT

  bd.txt (12):     LUD BAN MOS1 MOS2 LINN TR808 TR808L CR78 CR77
                   TR76 MPOP HAM
  sd.txt (12):     LUD KRI TR808A TR808B LINN MPOP TR76 CR78 SIMM
                   HAM1 HAM2 SIMMRIM
  hh.txt (14):     LUD LINN1 LINN2 TR808 MPOP HAM TR76 CR77 CR78
                   MAR808 CABLINN CYM808A CYM808B RIDLINN
  oh.txt (11):     LUD KRI TR808A TR808B TR909 LINN1 LINN2 MPOP
                   HAM TR76 CR78
  hh2.txt (14):    identico a hh.txt
  clap.txt (5):    KANO KANOBR KANOBL LINN TR808
  perc1.txt (12):  LC808A LC808B LC808C LC808D LT808A LT808B
                   TOMLINN SIMMLT CONGLLINN MPOMXL TR76PERC CR78RIM
  perc2.txt (13):  MC808A MC808B MC808C MT808A MT808B MT808C
                   SIMMMT CONGMLINN BNGLINN MPOPCON TR76PERC
                   CR78BLOC CR78GUI
  perc3.txt (13):  HC808A HC808B HC808C HC808D HC808E HT808A HT808B
                   HTSIMM CONGHLINN MPOPCL TR76PERC CR78BLOc CR78COW

9.6 PAGINA DCO A / DCO B

  Modifiche recenti:
  - Bottone UNICO ciclico WF → FM → AM al posto di 3 bottoni separati.
    Bordo colorato: WF=0x00AAFF, FM=0xFF8800, AM=0xAA44FF.
    Posizione: (110, 360), a destra di home.
  - Niente più LED (eliminati).
  - Griglia Wave Shape a **4 colonne** (erano 5): WF_ITEMS ora 8 item
    (SAW SAW8 TRI SQR SINE FM1 FM2 NOISE — rimosso FM3).
  - Nessuna label "Wave Shape" né "SHAPE".
  - Plotter + slider orizzontale spostati a sinistra (PLOT_X=10,
    PLOT_Y=45, PLOT_W=220, PLOT_H=105). Slider sotto il plotter.
  - Bottone "Edit" visibile **solo quando cat=FM**:
    posizione (210, 360), bordo rosso 0xFF2222, apre pagina "FM Edit".
  - Populate grid: bg non più colorato in selezione, bordo spesso 2px
    colorato con colore della categoria (cat_color()).
  - `ui2fw_wave[9] = {0, 4, 3, 2, 1, 5, 6, 7, 8}` — attenzione: SynthB
    firmware va adeguato perché WF ora ha 8 item (NOISE = ui2fw_wave[7]).

  Nuovi elementi in ShapeData:
    lv_obj_t *cat_btn;
    lv_obj_t *cat_btn_lbl;
    lv_obj_t *edit_btn;

  Nuove callback:
    ecat_cycle (cicla WF→FM→AM)
    fm_edit_btn_cb (apre sotto-pagina "FM Edit")

  Ramo `eb`: id `-4` = ritorno a DCO A/B dalla pagina FM Edit.

9.7 PAGINA VCF B (SYNTHB) — COMPLETA

  6 stati gestiti da `sB_vcf_apply_state()`:

  | Stato    | F1                    | F2          | F3                     |
  |----------|-----------------------|-------------|------------------------|
  | FLT/UNI  | top="CUT"             | top="DET"   | disabilitato (grigio)  |
  | FLT/SLV  | top="CUT"             | top="INT"   | top="INT"              |
  | FLT/FRE  | top="CUT"             | top="CUT"   | top="CUT"              |
  | WOV/POT  | lettere A E I O U A   | top="FORM"  | disabilitato           |
  | WOV/ENV  | nascosto + dropdown    | top="FORM"  | top="TIME"             |
  |          | ATT/SUS/REL           |             |                        |
  | WOV/RND  | disabilitato          | top="FORM"  | top="TIME"             |

  Struttura visiva:
  - Arc F1/F2/F3 (y=80) + arc RES/EnvA/EnvV (y=216), x = {35, 170, 305}
  - Ogni arc ha: label centrale, label valore sotto (offset -8),
    pallino target rosso con crossing, 5 tick radiali grigi
    (15×1px), top-label opzionale
  - F1/F2/F3 supportano anche 6 lettere scala (solo WOV/POT)
  - RES/EnvA/EnvV con pallino e crossing; **EnvA disabilitato in WOV**
  - Dropdown ENV (ATT/SUS/REL): 50px wide, gap 5, start_x=5
    → mappano A E I O U → 0..255 su chiavi 'V'/'Z'/'L'
  - Plotter ADSR blu (0x0088FF) a (470, 58) 280×100 (2px sopra frame)
  - Frame ENV vir 280×290 a (470, 160), label sopra il bordo (y=-20)
  - Slider A D S R dentro il frame: **senza freccina** (slider_plain),
    indicator azzurro 0x0022CC

  Bottone SUBMODE 3-stati ciclico (UNI→SLV→FRE o POT→ENV→RND):
  - Fondo scuro 0x1A1A2E, testo+bordo colorati (verde FLT, giallo WOV)
  - Al cambio di modo si aggiorna automaticamente

  LP/BP:
  - Cambio modo FLT/WOV → forzato a LP/BP rispettivamente
  - **Resta sempre cliccabile** (l'utente può sovrascrivere)

  3 bottoni in basso a x={110, 205, 300}, y=360 (accanto a home).
  Home a x=10, y=360.

  Chiavi LWS inviate a SynthB:
    'B' = sB_vcf_mode (0=Filter, 1=Wovel)
    'C' = sB_filter_submode (0=UNI, 1=SLV, 2=FRE)
    'K' = sB_wovel_submode (0=POT, 1=ENV, 2=RND)
    'e' = sB_filter_type (0=LP, 1=BP)
    '1','2','3' = sB_vcf_cut[0..2] (0..255)
    'V' = wov_vowel_A (dropdown ATT)
    'Z' = wov_vowel_B (dropdown SUS)
    'L' = wov_env_vowel_C (dropdown REL)
    **TODO**: mappatura LWS per RES/EnvA/EnvV (arc addizionali)

9.8 PAGINA FX / FV-1

  Frame FV-1 (lato destro): 258×310 a (470, 55)
  - Label "FV-1" sopra il bordo (y=-22)
  - Riga superiore:
    - Dropdown P1..P8 (65×45)
    - Bottone toggle "Rev1"/"Rev2" (80×45)
      Rev1 = azzurro 0x0099FF, Rev2 = azzurro chiaro 0x66CCFF
    - Bottone "Salva" rosso 0xAA0000 (85×45)
  - 3 slider verticali SIZE/LF/HF con label centrate sopra

9.9 PAGINA DRUM / SEQ

  Griglia 9×16:
  - Righe: BD SD HH OH H2 Cp P1 P2 P3
    (modifica recente: OH e HH scambiati → ora riga 2=HH, riga 3=OH)
  - **TUTTE le righe a 4 stati** (prima solo SD e H2)
  - Colore pallino/rettangolo = colore riga
  - **Rettangolino invece di cerchio**:
    - Larghezza = cell_width - 8 (4px sx + 4px dx)
    - Altezza proporzionale allo stato:
      - 0 = 0 (vuoto)
      - 1 = 1/3 altezza disponibile
      - 2 = 2/3
      - 3 = 3/3 (tutta)
  - **Long-press 1s → stato va a 0** (qualsiasi stato precedente)
  - Tap breve (< 1s) cicla: 0 → 1 → 2 → 3 → 0
  - 4 separatori verticali (1|2|3|4)
  - Cursore step rosso (timer 400 ms)
  - Play/Stop a (580, 360), Back-to-DRUM a (680, 360)

  Long-press: implementato con lv_timer one-shot (1000ms),
  cancellato su LV_EVENT_PRESS_LOST.

9.10 ALTA PAGINA — LAYOUT GENERALE

  HOME:
  - Titolo LUD-WS + versione, slider luminosità top-right
  - Log widget 700×240 a (50, 97), auto-hide dopo 2000ms
  - 6 bottoni: PLAY / SYNTH A / SYNTH B / DRUM / FX / SET UP

  SYNTH A/B:
  - Preset selector (dropdown + + / - / Sel / Save) + label + Rinomina
  - Submenu in basso con suffisso: "DCO A" / "DCO B", ecc.
  - Home a sinistra

  VCF A / VCF B:
  - VCF-A: slider con freccina, cornice ENV vir 320×300 a (460, 60)
  - VCF-B: come 9.7

  VCA A / VCA B:
  - 4 slider in cornice + plotter ENV a (20, 100)

  MIDI:
  - Tastiera 32 tasti (F3..C6) a (380, 5) 360×85
  - 3 righe: SynthA, SynthB, DRUM con dropdown CH
  - SPLIT dropdown (32 note)

9.11 CHIAVI EEPROM

  5 byte:
  - [0] bright 0..100
  - [1] midi_channel_A 1..16
  - [2] midi_channel_B 1..16
  - [3] midi_channel_D 1..16
  - [4] midi_split 0..31 (0=F3)

  API: load_all_settings() in setup, save_all_settings() da SET UP (id 21).

9.12 DRUM GUI — STATO ATTUALE

  Implementato:
  - Griglia 9×16 con 4 stati per ogni voce
  - Rettangolino colorato invece di cerchio
  - Long-press 1s → stato 0
  - Play/Stop con cursore step (timer 400ms)
  - Bottone Salva (invia 'Y' a Teensy)

  Da implementare / progettare:
  - Selezione sezione A/B/C/D (comando 'o')
  - Fill select ('i') + Fill trigger ('h') con griglia fill
  - Song editor (lista slot 256, rename)
  - Kit editor (voice/sample names a nome + rename)
  - Mute toggle (6 pulsanti)
  - BPM/Swing/Trasporto completi
  - Text input LVGL riutilizzabile per rename
  - Lettura/scrittura /drum/*.txt su SD


-------------------------------------------------------------------------------
10. PYTHON GENERATOR
-------------------------------------------------------------------------------

10.1 convert_ptn.py — DRUM (fatto)

  Converte PTNxx.TXT / PTN_FILLxx.TXT / SONGxx.TXT in .BIN per la SD
  del Teensy.

  Input:
    PTN00.TXT .. PTN15.TXT           -> PTN00.BIN .. PTN15.BIN (9x64)
    PTN_FILL00.TXT .. PTN_FILL15.TXT -> PTN_FILL00.BIN ..       (9x16)
    SONG00.TXT .. SONG15.TXT         -> SONG00.BIN ..           (256 slot)

  Formati:
    .BIN pattern: magic 'P' ver 0x02 rows 9 cols 64 + payload + crc8
    .BIN fill:    magic 'F' ver 0x01 rows 9 cols 16 + payload + crc8
    .BIN song:    magic 'S' ver 0x01 len + 256*(tipo,numero) + crc8

10.2 gen_sample_names.py — DRUM (fatto)

  Genera /drum/sample_names/*.txt con i nomi di default del punto 9.5.

10.3 Python generator preset — SYNTH (esistente)

  Genera 30 preset SynthA + 30 preset SynthB + 2 file nomi.

  DA AGGIORNARE:
    - Nuove chiavi SynthB: VCF mode, LFO3, ADSR hardware, Wovel, env_src
    - WF ora 8 item (rimosso FM3): aggiornare ui2fw_wave


-------------------------------------------------------------------------------
11. RP2350 — PATCH NECESSARIE
-------------------------------------------------------------------------------

  Ogni sketch .ino e Config.h:

    #if defined(PICO_RP2350) || defined(ARDUINO_ARCH_RP2350)
      #define PWM_CLKDIV_BASE   4.8f
    #else
      #define PWM_CLKDIV_BASE   4.0f
    #endif

    #define PWM_IRQ_RATE_HZ   30500.0f

  Nel .ino: const float masterFreq = PWM_CLKDIV_BASE; (era 4.0f)

  Arduino IDE 2.x:
    Board: Raspberry Pi Pico 2 (o Pico 2 W se con WiFi)
    CPU Speed: 150 MHz
    Optimize: -O3 (per FPU)
    USB Stack: Pico SDK
    PSRAM: Disabled
    Debug Port: Serial (o Disabled)

  Pin occupati da Pico 2 W (WiFi): GP23, GP24, GP25, GP29
  -> Nel progetto NON sono usati, nessun conflitto.


-------------------------------------------------------------------------------
12. ARDUINO IDE 2 — SETUP TEENSY 4.1
-------------------------------------------------------------------------------

  Board:        Teensy 4.1
  CPU Speed:    600 MHz
  Optimize:     Fastest
  USB Type:     Serial
  PSRAM:        Disabled
  Debug Port:   Serial

  Nota importante:
    - La libreria Audio deve essere quella di Teensyduino, NON una
      copia locale in <sketchbook>/libraries/Audio.
    - Se il Boards Manager di Arduino IDE 2 non installa correttamente
      la libreria Audio, scaricarla manualmente da
      https://github.com/PaulStoffregen/Audio e copiarla in
      .../packages/teensy/hardware/avr/<versione>/libraries/Audio
    - Rimuovere ogni cartella Audio locale per evitare conflitti.


-------------------------------------------------------------------------------
13. CONVENZIONI
-------------------------------------------------------------------------------

  Voce globale SynthA: 0..4
  Voce locale SynthA:  0..NUM_VOCI-1
  Sub_voce SynthA:     0..1 (2 per voce)
  POLIMAX SynthB:      6 (slot parafonici)
  Voice=0 fisso per SynthB

  Chiavi K_*: uint8 (nome 1 char) o int32 (int32_le)
  Payload CMD_MIDI_*_V: [target][voice][...]
  Poly broadcast: solo Master 'a' in modalità Poly
  Bridge relay: 'a' <-> 'b' <-> 'c'

  Teensy drum:
    Pattern 64 step in 4 sezioni A/B/C/D
    Fill 16 step, one-shot, pendente al boundary
    Song 256 slot
    Velocity 0..3
    Cambi pendenti al boundary di 16 step

  Display UI:
    Palette:
      Verde 0x00FF00    PLAY / DCO / SUB_FILTER
      Rosso 0xCC3300    SYNTH A / VCF / Salva (FX/DRUM)
      Blu   0x0099FF    SYNTH B / MOD / Rev1
      Giallo 0xFFCC33   DRUM / VCA / SUB_WOVEL
      Viola chiaro 0xBB88FF  FX / DLY
      Grigio 0x999999   SET UP
    Frame: border grigio 0x888888, label sopra il bordo (y=-20,
      bg_opa=COVER, pad_hor=6..8)
    Tick radiali: 5 per arc, angoli 135/202.5/270/337.5/405,
      pivot (0,15), rotazione 90+deg
    Top-label sopra tick superiore: offset -17
    Value-label sotto arc: offset -8


-------------------------------------------------------------------------------
14. HARDWARE ESTERNO SYNTHB
-------------------------------------------------------------------------------

  TLC5628 (DAC octal 8-bit, 3 fili DATA/CLK/LOAD):
    Parola 11 bit MSB-first [A2 A1 A0 D7..D0]
    5 canali usati: VCF1/VCF2/VCF3/Res/Sustain
    Nota: per smoothness uscita, filtro RC (R≈1kΩ, C≈4.7nF)
    + buffer (TL074 se alimentazione duale, MCP6004/OPA4353 se 5V
    singola). 100nF diretto NON va bene (filtro taglia a 1.6kHz,
    troppo lento per CV modulato).

  CD4051 (mux 8 canali):
    Seleziona resistenza A/D/R dell'ADSR hardware
    3 pin select (A, B, C) pilotati dal SynthB

  CD4066 (4 switch analogici):
    3 switch per ATTACK / DECAY / RELEASE
    Pilotati da 3 GPIO del SynthB

  ADC GP26:
    Feedback envelope hardware (0..1023)

  AD5242: NON usato (rimosso).


-------------------------------------------------------------------------------
15. COSE DA FARE (TODO)
-------------------------------------------------------------------------------

  CRITICHE:
  1. Test hardware Teensy rev 12 (BIN, fill, song, choke HH/OH)
  2. Verificare boot log SD: PTN:16/16 FILL:16/16 SONG:16/16
  3. Verificare cambio fill pendente e ritorno a sezione pendente
  4. Compilare e testare Display end-to-end dopo refactoring + VCF-B
  5. **Allineare firmware SynthB a nuovo WF 8 item** (rimosso FM3):
     aggiornare ui2fw_wave, K_WAVEFORM, ottaveB.h se serve
  6. **Aggiungere mappatura LWS per arc RES/EnvA/EnvV** in SynthB

  DISPLAY:
  7. Progettare GUI drum sequencer completa (sezione A/B/C/D,
     fill trigger, song mode)
  8. Progettare GUI kit editor (voice/sample names, rename)
  9. Progettare GUI song editor (lista slot 256, rename)
  10. Implementare text input riutilizzabile LVGL
  11. Integrare nuovi comandi Teensy ('y' 's' 'o' 'i' 'h')
  12. Implementare lettura/scrittura /drum/*.txt su SD Display

  SCRIPTS PC:
  13. convert_ptn.py (fatto)
  14. gen_sample_names.py (fatto)

  FEATURE NON IMPLEMENTATE:
  - DSP effetti (chiavi accettate, non applicate)
  - MIDI mode split (parametri accettati)
  - Preset dump (CMD_PRESET_READ stub)
  - Periferiche SynthB (encoder, bottoni, display locale?)
  - Revisione ADSR (tutti i synth)

  IN SOSPESO:
  - Test hardware end-to-end (a -> b -> c -> B -> T)
  - Preset transfer da SD reale
  - Verifica FPU su RP2350 (pitch 440 Hz)
  - Collisione 'Q' tra K_FM_DIV_1 e K_PRESET_SAVE (in namespace diversi)
  - Aggiornare Python generator preset SynthB con nuove chiavi


-------------------------------------------------------------------------------
16. PROSSIMO STEP
-------------------------------------------------------------------------------

  Se hardware Teensy montato:
    - Caricare firmware rev 12
    - Verificare boot log (PTN/FILL/SONG count)
    - Test play pattern, cambio sezione, fill, song
    - Verificare CPU/mem con 'C'

  Se hardware Display pronto:
    - Compilare dopo refactoring + nuove pagine
    - Test VCF-B con 6 stati (FLT/UNI-SLV-FRE + WOV/POT-ENV-RND)
    - Test DCO A/B (bottone ciclico WF/FM/AM, Edit FM)
    - Test DRUM/SEQ (rettangolini, long-press 1s)
    - Test FX/FV-1

  Altrimenti:
    A) Progettare GUI drum sequencer completa
    B) Implementare text input riutilizzabile LVGL
    C) Aggiornare Python generator preset SynthB
    D) Aggiungere DSP effetti
    E) Revisione ADSR (SynthA + SynthB)

================================================================================
FINE RIEPILOGO
================================================================================