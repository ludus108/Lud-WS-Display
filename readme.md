\# 🎛️ LUD-WS – Display Controller

V 0. 0. 14

Firmware per display master della workstation analogica ibrida LUD-WS.  

Basato su \*\*LVGL\*\* con touch 800×480.

// Hardware: VIEWE UEDX80480050E_WB_B (ESP32-S3, 800x480)

FQBN: esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi

\- Gestione preset su SD (Synth A/B)

\- Controllo effetti e meter audio

\- Comunicazione seriale UART verso altri MCU (Router, CTRL, MOD, SynthA, SynthB, Teensy (sampler Druma + 4 audio Trahs))



📡 Protocollo `\&!` su Serial1 (pin 18/17)



🔧 Dipendenze: `lvgl` (v8), `SD`, `EEPROM`, `esp\_display\_panel`



📌 Versione 0.9xx – sviluppo attivo



\---



📖 README completo e setup: vedi sezione dedicata.

"# Lud-WS-Display" 
