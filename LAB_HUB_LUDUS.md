================================================================
RIEPILOGO SISTEMA — LAB HUB LUDUS
================================================================

CONTESTO
--------
Evoluzione di un vecchio gestionale magazzino componenti elettronici
basato su Microsoft Access (.mdb) in un sistema moderno multi-servizio,
ospitato su Raspberry Pi 5, accessibile via web da LAN e Internet.

Organizzazione: LudusLab
Utente: luduspi
Hostname: Pilud
OS: Raspberry Pi OS Bookworm 64-bit
IP LAN: 192.168.1.5
IP VPN (WireGuard): 10.10.0.1
DDNS: luduslab.ddns.net (No-IP)
Dominio personale: registrato su Aruba (non ancora usato)
IP pubblico attuale: 82.89.16.85 (dinamico TIM)

================================================================
HARDWARE E RETE
================================================================

- Raspberry Pi 5 con Raspberry Pi OS Bookworm 64-bit
- Collegato via cavo Ethernet al router TIM Smart Modem
- WiFi (wlan0) DISABILITATO per evitare routing asimmetrico
- Modem TIM:
  * Port forwarding NON funzionante ("Configurazione NAT non eseguibile")
  * DMZ ATTIVA su 192.168.1.5 (workaround, espone tutto)
  * DDNS No-IP configurato e funzionante
  * Accesso pannello: http://192.168.1.1/
- WireGuard server in ascolto su UDP 51820

================================================================
SERVIZI ATTIVI SUL PI
================================================================

compmanager      porta 8080   systemd+pwsh   Gestionale magazzino
pi-panel         porta 8081   systemd+py3    Dashboard + Hub + Netstat
strumenti        porta 8082   systemd+pwsh   Archivio strumenti
AdGuard Home     porta 80     systemd        Blocco annunci DNS
WireGuard wg0    porta 51820  systemd        VPN (UDP)
Samba smbd       139/445      systemd        Condivisione file
SSH              22           systemd        Accesso remoto
VNC RealVNC      5900         systemd        Desktop grafico
wayvnc           -            disabilitato   (conflitto con RealVNC)

================================================================
MAGAZZINO COMPONENTI (compmanager)
================================================================

ORIGINE DATI
Vecchio .mdb con tabelle:
  comp (Ludus), compherve, compmrl, compnicco
  + tipo (12 categorie), subtipo (61 sottocategorie)
Solo la tabella comp (Ludus) è stata convertita: 759 componenti.

SCHEMA SQLITE (comp.sqlite)
CREATE TABLE comp (
  ID INTEGER PRIMARY KEY,
  comp TEXT,        -- nome/codice componente
  info TEXT,        -- note
  Qta INTEGER,      -- quantita
  datasheet TEXT,   -- nome file PDF/immagine
  crossref TEXT,    -- codici equivalenti
  tipo TEXT,        -- categoria (testo, non FK)
  subtipo TEXT,     -- sottocategoria (testo)
  dove TEXT,        -- ubicazione fisica
  user TEXT         -- utente (Ludus)
);
CREATE TABLE tipo (ID INTEGER PRIMARY KEY, tipo TEXT);
CREATE TABLE subtipo (ID INTEGER PRIMARY KEY, subtipo TEXT, tipo TEXT);

CONVERSIONE .mdb -> SQLite (con mdbtools)
  mdb-export comp.mdb comp > comp.csv
  mdb-export comp.mdb tipo > tipo.csv
  mdb-export comp.mdb subtipo > subtipo.csv
  # rimuovere intestazioni CSV con: tail -n +2
  # import in sqlite3 comp.sqlite

SERVER (Server.ps1 -> compmanager.service)
- PowerShell 7 + P/Invoke diretto verso libsqlite3.so
  (PSSQLite NON compatibile con ARM64)
- Porta 8080
- API REST:
    /api/components (GET/POST)
    /api/components/{id} (GET/PUT/DELETE)
    /api/tipos, /api/subtipos (CRUD)
    /api/tipos/full, /api/subtipos/full
    /api/upload (base64)
    /api/backup, /api/export/csv, /api/restart
- Filtri: ?search= ?tipo= ?subtipo= ?lowStock=
- Parametro -User "Ludus" per filtrare (opzionale)

GUI (index.html)
- Responsive desktop+mobile
- Ricerca live, filtri, ordinamento
- Colonne ridimensionabili (drag), larghezze in localStorage
- Sidebar ridimensionabile
- Modal aggiungi/modifica, Tipi & Subtipi
- Datasheet cliccabili e caricabili
- Pulsanti backup / export CSV / riavvio

================================================================
STRUMENTI ELETTRONICI (strumenti)
================================================================

SCHEMA SQLITE (strumenti.sqlite)
CREATE TABLE strumenti (
  ID INTEGER PRIMARY KEY AUTOINCREMENT,
  nome TEXT NOT NULL
);
Documenti su filesystem: ~/BStage/strumenti/{ID}/{file}

SERVER (Strumenti.ps1 -> strumenti.service)
- Stesso wrapper SQLite del Magazzino (P/Invoke)
- Porta 8082
- API:
    /api/strumenti (GET/POST)
    /api/strumenti/{id} (GET/PUT/DELETE - cancella anche docs)
    /api/strumenti/{id}/docs (GET/POST)
    /api/strumenti/{id}/docs/{filename} (DELETE)
    /strumenti/{id}/{filename} (GET - download)
    /api/backup

GUI (strumenti.html)
- Lista strumenti con conteggio documenti
- Drag&drop documenti (PDF, immagini, archivi)
- Upload multiplo con conferma overwrite
- Download/apri in nuova scheda
- Eliminazione singolo documento
- Responsive

================================================================
PI PANEL (pi-panel)
================================================================

SERVER PYTHON (pi-panel.py)
- Python 3 + http.server
- Porta 8081
- Gira come ROOT (systemd) per controllare servizi
- API:
    /api/status     stato servizi
    /api/wg         output wg show
    /api/net        reti, route, DNS, porte
    /api/sys        temp, disco, RAM, load
    /api/info       OS, kernel, arch
    /api/adguard/test
    /api/netstat    analisi rete completa
    /api/backup (POST)
    /api/power (POST reboot/shutdown/cancel)
    /api/service/{name}/{action} (POST start/stop/restart)
- Serve file statici: pi-panel.html, hub.html, netstat.html, favicon

PAGINE HTML
1. pi-panel.html   dashboard controllo
2. hub.html        contenitore multi-tab
3. netstat.html    analisi rete dettagliata

================================================================
HUB (hub.html)
================================================================

SERVIZI NEL MENU (ordine attuale)
#  Tab         Icona  URL                               Note
1  Magazzino   📦     http://HOST:8080/
2  Strumenti   🔧     http://HOST:8082/
3  Pi Panel    🎛️     http://HOST:8081/
4  AdGuard     🛡️     http://HOST/
5  Netstat     📡     http://HOST:8081/netstat
6  Modem TIM   📶     http://192.168.1.1/    external:true

CARATTERISTICHE
- Tab con icone + pallino di stato (verde/rosso/grigio)
- Auto-refresh stato ogni 15s
- Ricorda ultimo tab in localStorage
- Scorciatoie: 1-6 cambia tab, R ricarica, F schermo intero
- Pulsanti: ricarica, apri nuova scheda, schermo intero
- external:true apre in nuova scheda (Modem TIM)
- frame.removeAttribute('srcdoc') prima di ogni cambio tab
- Responsive mobile

================================================================
ADGUARD HOME
================================================================

- Installato in /opt/AdGuardHome/
- Ascolta su PORTA 80 (non 3000)
- Config: /opt/AdGuardHome/AdGuardHome.yaml
- Upstream DNS (DoH):
    https://dns.cloudflare.com/dns-query
    https://dns.quad9.net/dns-query
- Bootstrap DNS: Quad9 IP
    9.9.9.10, 149.112.112.10, 2620:fe::10, 2620:fe::fe:10
- Doppio scopo: blocco annunci DNS + resolver per VPN

================================================================
WIREGUARD
================================================================

SERVER /etc/wireguard/wg0.conf
[Interface]
Address = 10.10.0.1/24
ListenPort = 51820
PrivateKey = <server-private>
PostUp   = iptables ... MASQUERADE -o eth0
PostDown = ...

[Peer]
# Cellulare
PublicKey = 5GVASdeRbDv+yCA6bbY/q+7EJeCOIDXF1+azy30RVRI=
AllowedIPs = 10.10.0.2/32

[Peer]
# PC Windows
PublicKey = <pcwindows-public>
AllowedIPs = 10.10.0.4/32

CLIENT
Cellulare   10.10.0.2   luduslab.ddns.net:51820
PC Windows  10.10.0.4   luduslab.ddns.net:51820
(Libero)    10.10.0.3

CHIAVI
Server public key: 4425FF4vzPrTtUyHCxc0JrFrG5ElX64DdwWyRNz1tkI=
File chiavi in /etc/wireguard/*.key

CLIENT CONFIG ESEMPIO
[Interface]
PrivateKey = <client-private>
Address = 10.10.0.x/32
DNS = 10.10.0.1

[Peer]
PublicKey = 4425FF4vzPrTtUyHCxc0JrFrG5ElX64DdwWyRNz1tkI=
Endpoint = luduslab.ddns.net:51820
AllowedIPs = 0.0.0.0/0, ::/0
PersistentKeepalive = 25

PROBLEMI NOTI
- TIM forza DNS su porta 53: query in chiaro intercettate
  e riscritte (.homenet.telecomitalia.it -> 127.0.0.1)
- Soluzione: tunnel WireGuard cifra tutto
- Con tunnel attivo su PC usare:
    AllowedIPs = 0.0.0.0/1, 128.0.0.0/1, ::/0
  per escludere la LAN
- Accesso ai servizi con tunnel: usare 10.10.0.1 (non 192.168.1.5)

================================================================
SAMBA
================================================================

CONFIGURAZIONE /etc/samba/smb.conf
- Ascolta su TUTTE le interfacce (0.0.0.0) - necessario per wg0
- Condivisioni:
    [PiHome]  -> /home/luduspi
    [BStage]  -> /home/luduspi/BStage
- Utente: luduspi (password separata via smbpasswd)
- Via LAN: \\192.168.1.5\...
- Via VPN: \\10.10.0.1\...

PROBLEMI NOTI
- Samba non supporta interfaces=wg0 -> ascolta su 0.0.0.0
- Su PC Windows: route manuale se tunnel cattura traffico LAN

================================================================
DDNS - NO-IP
================================================================

Account No-IP, hostname: luduslab.ddns.net
Chiave DDNS: pzsn8cx
Server chiave: all.ddnskey.com
Configurato nel modem TIM (pagina DynDNS) come provider No-IP
Il modem aggiorna automaticamente l'IP quando cambia

================================================================
MODEM TIM - CRITICITA
================================================================

- Port forwarding: "Configurazione NAT momentaneamente non eseguibile"
- Hard reset tentato -> port forwarding ancora non funzionante
- DMZ ATTIVA su 192.168.1.5 come workaround
  ATTENZIONE: espone tutti i servizi del Pi su Internet
- Pagina DynDNS nativa per No-IP (funziona)
- Accesso pannello: http://192.168.1.1/

SICUREZZA ATTUALE
Con DMZ tutti i servizi sono esposti. Da mettere in sicurezza:
- UFW sul Pi (permettere solo 51820/UDP da fuori,
  porte web solo da LAN e VPN)
- Oppure passare a Tailscale e disattivare DMZ

================================================================
SCRIPT DI GESTIONE
================================================================

gestione.sh (Pi) - Menu bash interattivo:
1. Stato sistema
2. AdGuard Home
3. WireGuard
4. VNC
5. SSH
6. Rete e diagnostica
7. Backup e manutenzione
8. Info e comandi rapidi
9. Spegni/Riavvia

Alias consigliato:
  echo "alias pan='~/gestione.sh'" >> ~/.bashrc

================================================================
FILE E DIRECTORY
================================================================

/home/luduspi/
  BStage/
    comp.sqlite              (759 componenti Ludus)
    Server.ps1               (server magazzino)
    index.html               (GUI magazzino)
    Strumenti.ps1            (server strumenti)
    strumenti.html           (GUI strumenti)
    strumenti.sqlite
    datasheet/               (PDF componenti)
    strumenti/               (documenti per ID)
    backup/                  (backup DB magazzino)
    backup-strumenti/
  pi-panel.py                (server Pi Panel)
  pi-panel.html              (dashboard)
  hub.html                   (multi-tab)
  netstat.html               (analisi rete)
  pcwindows.conf             (client WireGuard PC)
  gestione.sh                (menu bash)
  favicon.*                  (favicon LLab)

================================================================
SERVIZI SYSTEMD
================================================================

compmanager.service           /snap/bin/pwsh Server.ps1       8080  luduspi
strumenti.service             /snap/bin/pwsh Strumenti.ps1    8082  luduspi
pi-panel.service              /usr/bin/python3 pi-panel.py    8081  root
AdGuardHome.service           /opt/AdGuardHome/AdGuardHome    80    root
wg-quick@wg0.service          wg-quick up wg0                 51820 root
smbd.service / nmbd.service   -                               139/445 root
ssh.service                   -                               22    root
vncserver-x11-serviced        -                               5900  root

================================================================
COMANDI UTILI
================================================================

GESTIONE SERVIZI
  sudo systemctl restart compmanager strumenti pi-panel AdGuardHome
  sudo systemctl restart wg-quick@wg0
  sudo systemctl status pi-panel --no-pager | head -5
  journalctl -u pi-panel -n 30 --no-pager

WIREGUARD
  sudo wg show
  sudo systemctl restart wg-quick@wg0

RETE
  hostname -I
  ip -br addr
  sudo ss -tlnp
  sudo netstat -tuapnc | grep -v LISTEN

BACKUP MANUALE
  cp ~/BStage/comp.sqlite ~/BStage/comp.sqlite.bak-$(date +%Y%m%d)
  cp ~/BStage/strumenti.sqlite ~/BStage/strumenti.sqlite.bak-$(date +%Y%m%d)
  cp /etc/wireguard/wg0.conf ~/backup-wg0.conf
  cp /opt/AdGuardHome/AdGuardHome.yaml ~/backup-adguard.yaml

================================================================
PROBLEMATICHE NOTE
================================================================

RISOLTE
- Conversione .mdb -> SQLite (759 componenti)
- P/Invoke SQLite nativo per ARM64 (no PSSQLite)
- DNS forzato TIM aggirato con WireGuard + DoH
- WireGuard funzionante su cellulare + PC Windows
- Samba su LAN + VPN
- DDNS No-IP funzionante
- Hub multi-tab con 6 servizi
- Netstat dashboard completa

APERTE / TODO
- Port forwarding TIM non funziona -> usare DMZ o Tailscale
- DMZ espone tutti i servizi -> attivare UFW sul Pi
- Cloudflare DDNS per dominio Aruba (alternativa a No-IP)
- Backup automatico con systemd timer
- Autenticazione pagine web esposte
- Sicurezza SSH: usare chiave invece di password
- HTTPS per i servizi web
- Tailscale come alternativa a WireGuard+port forwarding

================================================================
CHIAVI E ENDPOINT SENSIBILI
================================================================

WireGuard server public key:  4425FF4vzPrTtUyHCxc0JrFrG5ElX64DdwWyRNz1tkI=
No-IP DDNS key:               pzsn8cx
No-IP hostname:               luduslab.ddns.net
AdGuard web:                  http://192.168.1.5/ (o http://10.10.0.1/)
Modem TIM:                    http://192.168.1.1/

================================================================
ACCESSO AI SERVIZI
================================================================

DA LAN (WiFi di casa)
  Magazzino:   http://192.168.1.5:8080/
  Strumenti:   http://192.168.1.5:8082/
  Pi Panel:    http://192.168.1.5:8081/
  Hub:         http://192.168.1.5:8081/hub
  Netstat:     http://192.168.1.5:8081/netstat
  AdGuard:     http://192.168.1.5/
  Samba:       \\192.168.1.5\PiHome
  SSH:         ssh luduspi@192.168.1.5
  VNC:         192.168.1.5:5900 (TigerVNC)

DA VPN (WireGuard attivo, anche da fuori casa)
  Stessi servizi ma con IP 10.10.0.1:
    http://10.10.0.1:8080/
    http://10.10.0.1:8081/hub
    http://10.10.0.1/
    \\10.10.0.1\PiHome
    ssh luduspi@10.10.0.1

DA INTERNET (DMZ attiva)
  http://luduslab.ddns.net:8080/
  http://luduslab.ddns.net:8081/hub
  http://luduslab.ddns.net/
  ATTENZIONE: con DMZ tutti i servizi sono esposti

================================================================
NOTE TECNICHE IMPORTANTI
================================================================

1. PowerShell 7 installato via Snap: /snap/bin/pwsh
2. P/Invoke SQLite: script PowerShell usano Add-Type con C# inline
   per chiamare libsqlite3.so direttamente (PSSQLite NON funziona
   su ARM64)
3. File HTML: serviti staticamente, NO restart necessario
4. File .py e .ps1: richiedono systemctl restart dopo modifica
5. UTF-8: tutto il sistema usa UTF-8
6. Doppio IP: evitare eth0 + wlan0 attivi insieme
   (causa routing asimmetrico)
7. DNS forzato TIM: query in chiaro su 53 intercettate
   -> tutto deve passare per tunnel WireGuard

================================================================
RIFERIMENTI
================================================================

Raspberry Pi OS: Bookworm 64-bit
Kernel: 6.12.109+rpt (aarch64)
PowerShell: 7.6.5 (Snap, Canonical)
AdGuard Home: installato via script ufficiale
WireGuard: modulo kernel + wireguard-tools
SQLite: libsqlite3.so.0 in /usr/lib/aarch64-linux-gnu/

Documento generato il 14 ottobre 2026.
Per riprendere il lavoro in un'altra chat,
incollare questo riepilogo come contesto iniziale.

================================================================