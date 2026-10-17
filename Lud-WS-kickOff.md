================================================================================
LUD-WS — KICK-OFF SESSIONE: WEB SERVER SU DISPLAY
================================================================================
Obiettivo: aggiornare via WiFi i file sulle SD di Display e Teensy.
Contesto: LUD-WS, progetto multi-MCU (vedi riepiloghi precedenti).
================================================================================


0. PREREQUISITI
-------------------------------------------------------------------------------
- Board Display: ESP32-S3 Dev Module
- Partizione attuale: app3M... modificata in "custom" via partitions.csv
  (~15 MB app, no OTA, FATFS piccola)
- SD esterna collegata al Display (SPI: CS=10, MOSI=11, CLK=12, MISO=13)
- WiFi ESP32-S3: disponibile, non ancora usato dal codice
- Router Pico 2: bridge LWS tra Display e Teensy
- Teensy 4.1: SD esterna con PTNxx.BIN, FILLxx.BIN, SONGxx.BIN, sample


1. GOAL
-------------------------------------------------------------------------------
Aggiornare via WiFi, da un PC sulla stessa rete:

A) File sulla SD del DISPLAY:
   - /preset/synthA/preset_NN.bin + nomi_presetA.txt
   - /preset/synthB/preset_NN.bin + nomi_presetB.txt
   - eventuali /drum/kit_names.txt, /fm/fmNN.txt (futuri)

B) File sulla SD del TEENSY:
   - /PTN00.BIN .. /PTN15.BIN
   - /PTN_FILL00.BIN .. /PTN_FILL15.BIN
   - /SONG00.BIN .. /SONG15.BIN
   - /sample (raw drum, già caricati su SerialFlash? — verificare)


2. ARCHITETTURA PROPOSTA (da confermare)
-------------------------------------------------------------------------------

   PC ─── WiFi (HTTP/FTP) ───▶ Display ─── LWS (Serial1 1Mbps) ───▶ Router
                                  │                                   │
                                  │                                   ▼
                                  ▼                                Teensy
                              SD Display                              │
                                                               SD Teensy
                                                               (via LWS forwarding)

   Due percorsi distinti:

   PERCORSO A — file per SD Display:
     PC → WiFi → Display → SD locale.
     Semplice. Solo lato Display.

   PERCORSO B — file per SD Teensy:
     PC → WiFi → Display (staging in RAM o SD locale)
                 → LWS → Router (bridge) → Teensy → SD Teensy.
     Complesso: serve un protocollo LWS file transfer (vedi §5).


3. LATO DISPLAY — SCELTE DA FARE
-------------------------------------------------------------------------------

3.1 Modalità di accesso
   Opzione 1: HTTP upload via browser (pagina web semplice).
     - Facile: <form enctype="multipart/form-data">
     - Upload singolo file alla volta
     - Nessun client speciale sul PC
     - Adatto a: preset, singoli file

   Opzione 2: FTP server.
     - Più ricco: puoi navigare cartelle, upload multiplo, delete
     - Serve un client FTP sul PC (FileZilla, WinSCP, ecc.)
     - Libreria: ESP-FTP-Server-Lib (https://github.com/robo8080/ESP-FTP-Server-Lib)
     - Adatto a: gestione completa SD

   Opzione 3: entrambi (HTTP per file piccoli, FTP per bulk).
     - Più codice ma più flessibile.

   SUGGERIMENTO: partire con HTTP upload, aggiungere FTP dopo.

3.2 Libreria HTTP
   - WebServer (built-in Arduino ESP32) — sincrono, semplice
   - ESPAsyncWebServer — asincrono, non blocca LVGL
     (attenzione: ESPAsyncWebServer è abbastanza matura su ESP32)
   - SUGGERIMENTO: ESPAsyncWebServer, così LVGL continua a girare fluido

3.3 SSID / password WiFi
   - Hard-coded all'inizio
   - Poi configurabile via UI (pagina SET UP)
   - Salvataggio in EEPROM (5 byte attuali → servono più byte)
   - O su SD (/config/wifi.txt)

3.4 Autenticazione
   - Rete locale: nessuna auth (rischio basso)
   - Password HTTP basic: semplice, protegge da accessi involontari
   - SUGGERIMENTO: HTTP basic con password semplice all'inizio

3.5 mDNS
   - Permette di accedere come http://lud-ws-display.local/
     invece che dover ricordare l'IP
   - Costo: pochi KB, poche righe di codice
   - SUGGERIMENTO: attivare

3.6 Integrazione con LVGL
   - Il web server gira in background (task FreeRTOS separato)
   - LVGL gira su core 1 (loop principale)
   - Attenzione a race conditions sull'accesso SD:
     * Se il Display legge preset mentre il server scrive → lock
     * Soluzione: mutex (xSemaphoreTake) su SD access


4. LATO TEENSY — FILE TRANSFER (il pezzo grosso)
-------------------------------------------------------------------------------

Il Teensy non ha WiFi. Riceve i file dal Display via LWS.

4.1 Protocollo proposto (da definire in dettaglio)

   CMD_FILE_BEGIN   'F'  [target][file_id u8][size u32_le][crc32 u32_le]
   CMD_FILE_CHUNK   'f'  [file_id u8][offset u16_le][data up to 64B]
   CMD_FILE_END     'E'  [file_id u8][crc32 u32_le][ack/nak]
   CMD_FILE_ACK     'a'  [file_id u8][status u8]
   CMD_FILE_NAK     'n'  [file_id u8][error u8]

   file_id: enum dei file conosciuti (PTN00..PTN15, FILL00..FILL15,
            SONG00..SONG15, ecc.) — oppure stringa se serve flessibilità

   MTU LWS attuale: verificare dimensione massima payload.
   Chunk da 64 byte → overhead alto. Se possibile, aumentare a 128/256.

4.2 Lato Display — flusso
   - Il server HTTP riceve il file in RAM (o streaming su SD locale)
   - Il Display lo spezza in chunk
   - Invia BEGIN → CHUNK* → END al Router → Teensy
   - Attende ACK finale
   - Se NAK: ritrasmette da un offset

4.3 Lato Teensy — ricezione
   - Buffer temporaneo in RAM (o scrittura incrementale)
   - Scrive su SD
   - Calcola CRC32 incrementale
   - Alla fine confronta con quello inviato
   - Risponde ACK/NAK

4.4 Recovery / robustezza
   - Retry su NAK (max 3 tentativi)
   - Timeout per BEGIN/CHUNK/END
   - Stato "transfer in corso" persistente: se il Teensy si riavvia
     durante un trasferimento, cosa succede?
   - SUGGERIMENTO: stato volatile (in RAM), il file parziale viene
     cancellato a timeout


5. OTA FIRMWARE (opzionale, futuro)
-------------------------------------------------------------------------------
Distinto dal file transfer. Richiede:
- Ripartizionare il Display con OTA (large_spiffs_16MB o mf_large)
- Partizione otadata + app0 + app1
- Libreria ArduinoOTA o esp_https_ota
- Upload firmware via browser o PlatformIO

Da valutare DOPO aver completato il file transfer.


6. ROADMAP PROPOSTA (milestone)
-------------------------------------------------------------------------------

M1 — WiFi connection
     - WiFi.begin(SSID, PASS) nel setup
     - Log con IP assegnato
     - Nessun server ancora

M2 — HTTP server base
     - Endpoint "/" → pagina HTML con form upload
     - Endpoint "/upload" → scrive su SD Display
     - Test: caricare un file .txt nella root SD

M3 — Upload in cartelle specifiche (Display SD)
     - Form con dropdown "destinazione" (preset/synthA, preset/synthB, ...)
     - Rinominare correttamente i file (es. preset_00.bin)
     - Refresh UI (se cambia un preset mentre visualizzato → aggiorna)

M4 — FTP server (opzionale, se M2/M3 non bastano)
     - Navigazione cartelle, upload bulk, delete
     - Stesso backend SD di M2/M3

M5 — LWS file transfer verso Teensy
     - Definire protocollo (BEGIN/CHUNK/END/ACK)
     - Implementare su Teensy (receiver)
     - Implementare su Display (sender)
     - Test end-to-end: caricare un PTN00.BIN e verificare che il
       Teensy lo suoni corretto

M6 — mDNS + auth HTTP
     - lud-ws-display.local
     - Password basic

M7 — UI su Display
     - Pagina SET UP: campo SSID, password, stato connessione,
       IP visualizzato, link al web server

M8 (opzionale) — OTA firmware
     - Ripartizionare con OTA
     - ArduinoOTA su Display


7. DOMANDE APERTE (da discutere all'inizio della sessione)
-------------------------------------------------------------------------------

1. HTTP o FTP o entrambi? → consiglio HTTP per iniziare
2. WebServer sync (Arduino) o ESPAsyncWebServer? → consiglio Async
3. SSID/password: hard-coded inizialmente, poi su UI/EEPROM?
4. Autenticazione: nessuna, basic, o digest?
5. mDNS attivo subito o dopo?
6. Il Teensy deve poter ricevere qualsiasi nome file, o solo nomi
   canonici (PTN00.BIN, ...)?
7. Il Router attuale fa già da bridge per payload binari LWS?
   (verificare codice Router)
8. Esiste già una libreria LWS file transfer nel progetto o va creata?
9. La UI Display deve mostrare stato upload (progress bar)?
10. Cosa succede se l'upload verso il Teensy fallisce a metà?
    Retry automatico o error message?


8. RIFERIMENTI CODICE
-------------------------------------------------------------------------------
- SD Display: init in Lud-WS-Display.ino (sd_init), CS=10, SPI condiviso
- SD Teensy: vedi codice Teensy (SerialFlash + SD esterna)
- LWS: serial_protocol.h, comunicazioni.h/cpp
- Router: codice separato (Lud-WS-Router)
- EEPROM Display: 5 byte attuali (bright, midi_ch_A/B/D, midi_split).
  Per WiFi servono ~64 byte in più (SSID 32 + PASS 32).
  → serve espandere EEPROM_SIZE da 5 a ~70 byte.
- LVGL: gira su core 1, attenzione a mutex SD


9. MATERIALE DA MANDARE ALL'INIZIO DELLA SESSIONE
-------------------------------------------------------------------------------
Per partire, servono:

Obbligatori:
- comunicazioni.h (Display) → come funziona l'invio LWS
- serial_protocol.h → cmd codes attuali
- Lud-WS-Display.ino → setup attuale, init SD
- Codice Router (Lud-WS-Router.ino) → come fa bridging

Opzionali (utili ma non bloccanti):
- lv_conf.h → per sapere se ci sono flag utili al WiFi
- Codice Teensy (parte ricezione LWS e scrittura SD)
- Eventuale codice FTP/HTTP già presente nel progetto (probabilmente no)


================================================================================
FINE KICK-OFF
================================================================================