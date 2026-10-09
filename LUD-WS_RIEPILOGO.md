================================================================================
LUD-WS — RIEPILOGO DI CONTINUITÀ (AGGIORNATO)
================================================================================
Documento autocontenuto. Incollalo come primo messaggio in una nuova chat
per riprendere il lavoro da dove è stato interrotto.

Ultimo aggiornamento: 2026-10-01 (Display: split lvglGraf + DRUM completo)
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
9. LUD-WS-DISPLAY (ESP32-S3, v0.16)
-------------------------------------------------------------------------------

9.1 HARDWARE

  ESP32-S3, LVGL 800x480, SD preset + SD drum names.
  Serial1 @ 1 Mbps verso Router (GPIO 18=RX, 17=TX).
  Cache locale: PresetCache cacheA[5] per SynthA, cacheB per SynthB.

9.2 STRUTTURA FILE (REFACTORING RECENTE)

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
          ├── lvglGraf.h                  (API pubblica)
          ├── lvglGraf_internal.h         (header condiviso)
          ├── lvglGrafCore.cpp            (widget/utility ~1100 righe)
          ├── lvglGrafPages.cpp           (pagine + eventi + reset)
          ├── lvglGrafDrum.cpp            (DRUM/SEQ editing)
          ├── lvglGrafSong.cpp            (SONG editor)
          ├── lvglGrafKit.cpp             (KIT editor)
          ├── lvglGrafRev.cpp             (REV — riverbero)
          ├── lvglGrafVcfB.cpp            (VCF SynthB)
          ├── lvglGrafFx.cpp              (FX / FV-1)
          ├── lvglPreset.cpp              (dropdown preset + rename)
          └── lvglGrafModules.cpp.bak     (VECCHIO, non compilare)

  Regola: NON devono coesistere lvglGrafModules.cpp e i nuovi file
  (multiple definition). Il vecchio è rinominato .bak.

9.3 COMUNICAZIONE LWS

  - Frame: [SENDER][SEQ][CMD][LEN][PAYLOAD][CRC8][&][!]
  - CRC-8/ATM (poly 0x07, init 0x00)
  - Discovery non bloccante: start() in setup, poll() in loop,
    restart() da SET UP
  - Nodi: 'D' Display, 'R' Router, 'a' SynthA_M, 'b' SynthA_V,
    'c' SynthA_V2, 'B' SynthB, 'T' Teensy
  - CMD usati dal Display:
    - send_param_update(target, key, value) per CMD_PARAM
    - send_param_voce / send_param_i32_voce per SynthA/B
    - CMD_PRESET_* per trasferimento preset
    - CMD_DRUM_PATTERN 'W' per PTN numero/nome
    - Teensy: 'Q' = request pattern, 'Y' = save pattern

9.4 SISTEMA PRESET

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

9.5 PAGINA DRUM (layout principale)

  Layout bottom (y=360):
    [PTN]  [SONG]  [KIT]  [REV]                [← Home]
    x=300  x=410   x=530  x=640                 x=10
    90×90  90×90   90×90  90×90

  - PTN (blu 0x00AAFF) → pagina SEQ
  - SONG (verde 0x00DD00) → pagina SONG
  - KIT (arancione 0xFF8800) → pagina KIT
  - REV (rosso scuro 0xAA0000) → pagina REV
  - Home (viola 0x9B59B6) → HOME

9.6 PAGINA DRUM/PTN (editing pattern)

  Layout:
    Griglia a y=0 (top row libera)
    Bottom row (y=400):
      [←] [PTN 1▾] [A][B][C][D] [FL] [Play]    [Salva]
       x=5  x=70   175..410     415  480       705

  Griglia 9×16:
    - X0=10, X1=770 (margine destro 30px)
    - Label nome riga (14pt) a x=10
    - Micro meter 22×24 a x=40
    - Celle a x=54, quadrate 46×46 (cellW=45, cellH=45)
    - Rettangolino colorato: larghezza = cw - 22px, altezza ∝ stato
    - 4 separatori verticali giallo (col 0, 8) / grigio (col 4, 12)
    - 8 linee orizzontali grigio 0x666666 tra le righe
    - Cursore step rosso (1px, timer 400ms)

  Celle a 4 stati (0=vuoto, 1=1/3, 2=2/3, 3=tutto):
    - Tap breve: 0→1→2→3→0
    - Long-press 1s: stato → 0

  Micro meter:
    - Immagini img_micro_meter_audio_track/indicator (22×24)
    - Envelope: attacco istantaneo + HOLD 100ms + DECAY 500ms
    - Parametri in cima a lvglGrafDrum.cpp:
      #define DRUM_METER_HOLD_MS  100.0f
      #define DRUM_METER_DECAY_MS 500.0f
      #define DRUM_METER_TICK_MS  25

  Sezioni A/B/C/D:
    - Bottone attivo: bordo 3px giallo 0xFFCC33
    - Cambio sezione → carica seqArr[pattern][*][sec*16..+16]
    - Pendente al boundary sul Teensy (cmd 'o')

  Bottone FL (fill mode):
    - Verde 0x00DD00 quando attivo
    - Cambia la sorgente della griglia da seqArr a fillArr
    - Dropdown cambia da "PTN 1..16" a "FLN 1..16"

  Dropdown PTN/FLN:
    - 95×45, bordo blu 0x00AAFF
    - Niente freccia (lv_dropdown_set_symbol(dd, NULL))

  Comandi LWS inviati:
    'o' = sezione (0..3)
    'N' = pattern select (0..15)
    'i' = fill select (0..15)
    'Y' = save pattern (Salva)

9.7 PAGINA DRUM/SONG (editor song)

  Layout:
    Top row (y=3):
      [SNG 1▾] [KIT▾] [A][B][C][D] [FLN 1▾]
      x=5(95)  105(180) 300..510   545(95)

    Frame griglia 4×8:
      GX=32, GY=55, GW=736, GH=352
      slot_w=92, slot_h=88, gap=4, cw=88, ch=84
      Bordo giallo scuro 0x886600
      Linee orizzontali grigio tra le righe
      Linea verticale gialla 0x886600 tra col 4 e 5

    Bottom row (y=415):
      [←] [◄] 1/8 [►] [+] [-]
       x=5  70  122  170 230 290

  Array:
    uint8_t  songArr[16][3][256];  // [song][0=tipo 1=num 2=kit][slot]
    uint16_t songLen[16];          // slot effettivi (1..256, mai 0)
    uint8_t  song_cur_song;        // 0..15
    uint8_t  song_cur_page;        // 0..7
    int      song_sel_slot;        // -1 = nessuno

  Slot:
    - Label KIT n (top, 14pt, bianco) — visibile se kit != 0
    - Label A/B/C/D/FLN n (center, 24pt)
    - Label numero slot 1-based (bottom, 16pt, grigio)
    - Vuoto (oltre songLen): bordo grigio, non cliccabile
    - Selezionato: bordo spesso 3px giallo 0xFFAA00

  Comportamento:
    - Tap su slot: seleziona/deseleziona
    - Tap su A/B/C/D: scrive tipo (pattern = id song)
    - Cambio FLN: scrive tipo=4 + fill num
    - Cambio KIT: scrive kit (0=none, 1..16) su slot selezionato
      (slot 1: kit permanente, non modificabile)
    - +: aggiunge slot in coda (default A + kit 0)
    - −: rimuove ultimo slot (mai sotto 1)
    - ◄ ►: cambia pagina (32 slot/pagina)
    - Cambio SNG: carica song, pagina 0, selezione azzerata

  Dropdown KIT: 17 item ("NO KIT" + "n Nome" per 16 kit)
  - Larghezza 180px, font 16

9.8 PAGINA DRUM/KIT (editor kit)

  Layout:
    Top row (y=5):
      [KIT n Nome▾]  [REV n▾]     [RINOMINA]
       x=350(180)      x=540(120)   x=670(120)

    Griglia 3×3 di dropdown (y=70):
      CELL_W=240, CELL_H=110, GRID_X=40, GRID_Y=70

      Riga 1:  [BD▾]     [SD▾]     [HH▾]
      Riga 2:  [OH▾]     [H2▾]     [CLAP▾]
      Riga 3:  [PERC1▾]  [PERC2▾]  [PERC3▾]

  Array/strutture:
    char kit_names[16][24]  (nomi kit, editabili in RAM)
    kitArr[10][16]  copiato da lista.h:
      righe 0..8 = voci, riga 9 = REV
      colonne 0..10 = kit 1..11 (11 kit)
    maxArr[9]  = {11, 11, 13, 10, 13, 4, 11, 12, 12}  (da lista.h)

  Nomi kit di default:
    1 Lud1    2 Lud2   3 Lud3    4 808     5 909
    6 Linn    7 miniPop 8 TR76   9 CR77    10 CR78   11 Hammond
    12 MPC60  13 DMX   14 SP1200 15 RX5    16 User

  Nomi set per voce (estratti da lista.h, senza suffisso arr):
    BD (12):     LUDBD, BDBAN, BDMOS1, BDMOS2, BDLINN, BD808,
                 BD808L, CR78BD, CR77BD, TR76BD, MPOPBD, HAMBD
    SD (12):     LUDSD, SDKRI, SD808, SD808B, SDLINN, MPOPSD,
                 TR76SD, CR78SD, SIMMSD, HAMSDA, HAMSDB, SIMMRIM
    HH (14):     LUDHH, HHLINN, LINNHHB, CH808, MPOPHH, HAMHH,
                 TR76HH, CR77HH, CR78HH, MAR808, CABLINN,
                 CYM808A, CYM808B, RIDLINN
    OH (11):     LUDOH, OHKRI, OH808, OH808B, OH909A, OHLINN,
                 OHBLINN, MPOPHO, HAMHOB, TR76HO, CR78HO
    H2 (14):     identico a HH
    CLAP (5):    KANO, KANOBR, KANOBL, CLAPLINN, CLAP808
    PERC1 (12):  LC808A/B/C/D, LT808A/B, TOMLINN, SIMMLT,
                 CONGLLINN, MPOMXL, TR76PER1C, CR78RIM
    PERC2 (13):  MC808A/B/C, MT808A/B/C, SIMMMT, CONGMLINN,
                 BNGLINN, MPOPCON, TR76PER2C, CR78BLO1, CR78GUI
    PERC3 (13):  HCA808, HCB808, HCC808, HCD808, HCE808,
                 HT808A/B, SIMMHT, CONGHLINN, MPOPCL,
                 TR76PER3C, CR78BLO2, CR78COW

  Comportamento:
    - Cambio KIT n → tutti i 9 dropdown + dropdown REV si aggiornano
      ai valori di kitArr[voce][kit]
    - Cambio singolo dropdown voce → solo visualizzazione per ora
      (nessun comando LWS; la pagina serve per programmare)
    - Cambio REV → idem
    - Tap RINOMINA → finestra modale con textarea precompilata col
      nome del kit corrente + tastiera LVGL; OK salva in kit_names[]
      e aggiorna dropdown; Annulla chiude
    - Persistenza kit_names su SD: TODO

9.9 PAGINA DRUM/REV (riverbero)

  Layout:
    Top (y=5):
      [REV n▾]  (a x=90, dopo il titolo)
      110×45, bordo rosso scuro 0xAA0000

    8 slider (y=100, h=200):
      start_x = 55, larghezza totale 690, gap 30
      PRED  SIZE  DAMP  CUT1  RES1  CUT2  RES2  LEV
      x=55  145   235   325   415   505   595   685

      Ogni slider in container verticale (nome top 14pt,
      slider center, valore bottom 16pt).

    Bottom (y=415):
      [← Home]                [SER|PAR]
       x=5                     x=340(120)

  Colori slider:
    PRED    0x00DD00  verde
    SIZE    0x00AAFF  azzurro
    DAMP    0xAA44FF  viola
    CUT1    0xCC0066  rosso porpora
    RES1    0xFFCC33  giallo
    CUT2    0xCC0066  rosso porpora
    RES2    0xFFCC33  giallo
    LEV     0xCC3300  rosso

  Slider: immagini img_slider_track/indicator/knob, no freccina,
    range 0..20

  Toggle SER/PAR:
    - blu 0x0099FF quando SER (rev_mode=0)
    - arancione 0xFF8800 quando PAR (rev_mode=1)

  Array revPresetArr[9][16] (copiato da specifica utente):
    riga 0 = room size  → SIZE
    riga 1 = damping    → DAMP
    riga 2 = cut off    → CUT1
    riga 3 = res        → RES1
    riga 4 = pre dly    → PRED
    riga 5 = cut off2   → CUT2
    riga 6 = res 2      → RES2
    riga 7 = filtSet    → SER/PAR
    riga 8 = lev out    → LEV

  Comportamento:
    - Cambio REV n → aggiorna 8 slider + SER/PAR da revPresetArr
    - Modifica manuale slider → solo valore locale

9.10 ALTRE PAGINE (non DRUM)

  HOME:
  - Titolo LUD-WS + versione, slider luminosità top-right
  - Log widget 700×240 a (50, 97), auto-hide 2000ms
  - 6 bottoni: PLAY / SYNTH A / SYNTH B / DRUM / FX / SET UP

  SYNTH A/B:
  - Preset selector (dropdown + + / - / Sel / Save) + label + Rinomina
  - Submenu DCO/VCF/MOD/VCA/DLY in basso
  - Home a sinistra

  DCO A/B:
  - Bottone ciclico WF→FM→AM (bordo 0x00AAFF / 0xFF8800 / 0xAA44FF)
  - Griglia Wave Shape a 4 colonne (8 item WF, FM3 rimosso)
  - Plotter + slider a sinistra (PLOT_X=10, PLOT_Y=45)
  - Bottone "Edit" visibile solo quando cat=FM
  - ui2fw_wave[9] = {0, 4, 3, 2, 1, 5, 6, 7, 8}

  VCF A / VCF B:
  - VCF-A: 4 slider con freccina
  - VCF-B: 6 stati (FLT/UNI-SLV-FRE + WOV/POT-ENV-RND)
    con top-label CUT/DET/INT/FORM/TIME, arc F1/F2/F3 +
    arc RES/EnvA/EnvV, plotter ADSR blu, dropdown ENV ATT/SUS/REL

  VCA A/B: 4 slider in cornice + plotter ENV

  FX: frame FV-1 (dropdown P1..P8, Rev1/Rev2 toggle, Salva,
    3 slider verticali SIZE/LF/HF)

  MIDI: tastiera 32 tasti (F3..C6), 3 righe (SynthA/B/DRUM) con
    dropdown CH, SPLIT dropdown 32 note

9.11 COMANDI eb (id)

  -1  → HOME
  -2  → torna al synth corrente (g.synth)
  -3  → torna a DRUM
  -4  → torna a DCO A/B
  0..5 → pagine principali (PLAY, SYNTH A, SYNTH B, DRUM, FX, SET UP)
  10..14 → submenu (DCO/VCF/MOD/VCA/DLY)
  20  → MIDI
  21  → save_all_settings
  30  → SEQ
  31  → Salva pattern (send 'Y')
  32  → SONG
  33  → KIT
  34  → REV

9.12 CHIAVI EEPROM (5 byte)

  [0] bright 0..100
  [1] midi_channel_A 1..16
  [2] midi_channel_B 1..16
  [3] midi_channel_D 1..16
  [4] midi_split 0..31 (0=F3)

  API: load_all_settings() in setup, save_all_settings() da SET UP


-------------------------------------------------------------------------------
10. PYTHON GENERATOR
-------------------------------------------------------------------------------

10.1 convert_ptn.py — DRUM (fatto)

  Converte PTNxx.TXT / PTN_FILLxx.TXT / SONGxx.TXT in .BIN per la SD
  del Teensy.

  Formati:
    .BIN pattern: magic 'P' ver 0x02 rows 9 cols 64 + payload + crc8
    .BIN fill:    magic 'F' ver 0x01 rows 9 cols 16 + payload + crc8
    .BIN song:    magic 'S' ver 0x01 len + 256*(tipo,numero) + crc8

10.2 gen_sample_names.py — DRUM (fatto)

  Genera /drum/sample_names/*.txt con i nomi di default.

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

  Nel .ino: const float masterFreq = PWM_CLKDIV_BASE;

  Arduino IDE 2.x:
    Board: Raspberry Pi Pico 2
    CPU Speed: 150 MHz
    Optimize: -O3 (per FPU)
    USB Stack: Pico SDK
    PSRAM: Disabled

  Pin occupati da Pico 2 W (WiFi): GP23, GP24, GP25, GP29
  -> Nel progetto NON sono usati, nessun conflitto.


-------------------------------------------------------------------------------
12. ARDUINO IDE 2 — SETUP
-------------------------------------------------------------------------------

  Teensy 4.1:
    Board:        Teensy 4.1
    CPU Speed:    600 MHz
    Optimize:     Fastest
    USB Type:     Serial
    PSRAM:        Disabled

    Nota: usare la libreria Audio di Teensyduino, NON una copia
    locale in <sketchbook>/libraries/Audio.

  ESP32-S3 (Display):
    Board:        ESP32S3 Dev Module
    FQBN:         esp32:esp32:esp32s3:FlashSize=16M,
                  PartitionScheme=app3M_fat9M_16MB,PSRAM=opi
    CPU Speed:    240 MHz
    Flash Mode:   QIO
    PSRAM:        OPI

  Problemi noti:
    - Link error "objs.a file truncated" → reinstallare pacchetto
      esp32 dal Boards Manager (più efficace di pulire solo la cache)
    - Cache sketch corrotta:
      C:\Users\<user>\AppData\Local\arduino\sketches


-------------------------------------------------------------------------------
13. CONVENZIONI
-------------------------------------------------------------------------------

  Voce globale SynthA: 0..4
  Voce locale SynthA:  0..NUM_VOCI-1
  Sub_voce SynthA:     0..1
  POLIMAX SynthB:      6
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

  Display UI — Palette:
    Verde 0x00FF00      PLAY / DCO / SUB_FILTER
    Rosso 0xCC3300      SYNTH A / VCF / Salva
    Blu   0x0099FF      SYNTH B / MOD / Rev1
    Giallo 0xFFCC33     DRUM / VCA / SUB_WOVEL
    Viola chiaro 0xBB88FF FX / DLY
    Grigio 0x999999     SET UP
    Arancione 0xFF8800  KIT (nuovo)
    Rosso scuro 0xAA0000 REV (nuovo)

  Frame: border grigio 0x888888, label sopra il bordo (y=-20,
    bg_opa=COVER, pad_hor=6..8)

  Tick radiali VCF: 5 per arc, angoli 135/202.5/270/337.5/405,
    pivot (0,15), rotazione 90+deg

  Dropdown: senza freccia via lv_dropdown_set_symbol(dd, NULL)


-------------------------------------------------------------------------------
14. HARDWARE ESTERNO SYNTHB
-------------------------------------------------------------------------------

  TLC5628 (DAC octal 8-bit, 3 fili DATA/CLK/LOAD):
    Parola 11 bit MSB-first [A2 A1 A0 D7..D0]
    5 canali usati: VCF1/VCF2/VCF3/Res/Sustain
    Filtro RC (R≈1kΩ, C≈4.7nF) + buffer

  CD4051 (mux 8 canali): seleziona resistenza A/D/R ADSR hardware

  CD4066 (4 switch analogici): 3 switch per ATTACK/DECAY/RELEASE

  ADC GP26: feedback envelope hardware (0..1023)

  AD5242: NON usato (rimosso)


-------------------------------------------------------------------------------
15. HARDWARE ESTERNO TEENSY (lista.h)
-------------------------------------------------------------------------------

  lista.h contiene gli array dei nomi file .raw dei campioni drum
  per il Teensy:
    - 9 voci: bdArr, sdArr, hhArr, ohArr, hh2Arr, clapArr,
              perc1Arr, perc2Arr, perc3Arr
    - Ogni voce ha N "set" disponibili (vedi 9.8)
    - 11 kit definiti dal commento finale
    - maxArr[9] con gli indici massimi 0-based per voce
    - NON contiene kitArr (definito dall'utente via specifica)

  Il Display usa lista.h solo per estrarre i nomi dei set
  (copia manuale in lvglGrafKit.cpp).


-------------------------------------------------------------------------------
16. COSE DA FARE (TODO)
-------------------------------------------------------------------------------

  CRITICHE:
  1. Test hardware Teensy rev 12 (BIN, fill, song, choke HH/OH)
  2. Verificare boot log SD: PTN:16/16 FILL:16/16 SONG:16/16
  3. Verificare cambio fill pendente e ritorno a sezione pendente
  4. Test Display end-to-end dopo refactoring + DRUM completo
  5. Allineare firmware SynthB a nuovo WF 8 item (rimosso FM3)
  6. Aggiungere mappatura LWS per arc RES/EnvA/EnvV in SynthB

  DISPLAY — immediati:
  7. Sync array (seqArr/fillArr/songArr) Display <-> Teensy via LWS
     (nuovo CMD: richiesta invio blob, chunk 64B, CRC)
  8. Persistenza kit_names su SD (/drum/kit_names.txt)
  9. Mute toggle (cmd 'M' bitmask) in DRUM/PTN
     (6 bottoni, layout da progettare, striscia dedicata)
  10. BPM + Swing in DRUM/PTN (cmd 'b' 'W')

  DISPLAY — pagina PLAY:
  11. Play mode Pattern/Song ('y')
  12. Song select live ('s')
  13. Fill trigger ('h')
  14. Fill select live ('i')
  15. Sezione A/B/C/D live ('o')
  16. BPM/Swing live ('b' 'W')

  DISPLAY — kit editor:
  17. Rename sample (richiede text input LVGL + SD)
  18. Kit editor completo (invio kit a Teensy)

  SCRIPTS PC:
  19. Aggiornare Python generator preset SynthB con nuove chiavi

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
  - Collisione 'Q' tra K_FM_DIV_1 e K_PRESET_SAVE (namespace diversi)
  - Warning static non usate in preset_sd.h, preset_transfer.h,
    lvglPreset.cpp (tutti innocui)


-------------------------------------------------------------------------------
17. PROSSIMO STEP CONSIGLIATO
-------------------------------------------------------------------------------

  Priorità alta — chiudono il cerchio DRUM:

  A) Sync array Display <-> Teensy
     - Definire CMD_DRUM_READ (Display → Teensy: richiesta seqArr)
     - Teensy risponde con N chunk da 64B (payload: [idx][data])
     - Display ricostruisce seqArr/fillArr/songArr
     - Al salvataggio: Display invia i blob aggiornati al Teensy

  B) Mute toggle in DRUM/PTN
     - 6 bottoni toggle (bitmask 6 bit)
     - Layout: striscia orizzontale da progettare
     - cmd 'M' con payload [mask]
     - Echo dal Teensy per stato

  C) Pagina PLAY completa
     - Ha spazio (attualmente meter L/R/C + timeline)
     - Può ospitare: fill trigger, play mode, sezioni live,
       song select, BPM/Swing live

  Priorità media:

  D) Persistenza kit_names su SD
  E) BPM + Swing in DRUM/PTN
  F) Mappatura LWS RES/EnvA/EnvV
  G) Python generator preset SynthB

================================================================================
FINE RIEPILOGO
================================================================================