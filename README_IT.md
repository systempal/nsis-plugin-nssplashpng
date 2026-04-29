# nsSplashPNG - Plugin NSIS per Splash Screen PNG

Plugin NSIS moderno che visualizza splash screen con supporto completo alla trasparenza alfa tramite WIC (Windows Imaging Component).

## Funzionalità

- **Formati Immagine Multipli**: PNG, JPEG, BMP, GIF, TIFF, ICO tramite WIC decoder
- **Trasparenza Alfa Completa**: Supporto canale alfa per PNG/GIF tramite WIC e layered windows
- **Velocità Fade Configurabile**: Velocità fade in/out personalizzabile (1-255 step, durata 30ms-7650ms)
- **Esecuzione Non Bloccante**: Lo script continua dopo il fade-in mentre lo splash rimane visibile
- **Timer Auto-Close**: Chiusura automatica configurabile dopo un numero specificato di millisecondi
- **Click per Chiudere**: Interazione utente opzionale per chiudere lo splash
- **No Cancel**: Impedisce all'utente di chiudere lo splash durante operazioni critiche
- **Controllo Manuale**: Show/stop programmabile con effetti fade
- **Supporto Multi-Monitor**: Centra lo splash sul monitor primario, corrente (mouse), per indice, o il più vicino a una coordinata
- **Zero Warning**: Compilazione pulita con Visual Studio 2022

## Installazione

1. Copia `nssplashpng.dll` nella cartella dei plugin NSIS:
   - `x86-unicode`: Per build NSIS Unicode a 32-bit
   - `x64-unicode`: Per build NSIS Unicode a 64-bit
   - `x86-ansi`: Per build NSIS ANSI a 32-bit

2. Includi la cartella del plugin nello script:
   ```nsis
   !addplugindir "path\to\plugins\x86-unicode"
   ```

## Compilazione dai Sorgenti

### Prerequisiti
- Visual Studio 2022 o successivo con strumenti C++
- Python 3.x

### Passaggi

```powershell
cd nsSplashPNG
python build_plugin.py                            # Compila tutte le architetture
python build_plugin.py --config x86-unicode       # Solo un'architettura (x86-ansi|x86-unicode|x64-unicode|all)
python build_plugin.py --vs-version 2026          # Versione VS specifica (2022|2026|auto)
python build_plugin.py --clean                    # Pulizia dist/ prima della build
python build_plugin.py --install-dir "C:\NSIS\Plugins"  # Copia in directory NSIS aggiuntiva
python build_plugin.py --verbosity minimal        # Verbosita MSBuild (quiet|minimal|normal|detailed|diagnostic)
```

Lo script:
- Compila per x86-unicode, x64-unicode e x86-ansi
- Copia le DLL in `dist/{x86-unicode|amd64-unicode|x86-ansi}/`
- Pulisce i file intermedi dalla cartella `src/Build/`
- Supporta build parallele con ottimizzazione automatica CPU

## Utilizzo

### Importante: Gestione Memoria del Plugin

**CRITICO**: Usare sempre il flag `/NOUNLOAD` alla prima chiamata a `show` per mantenere il plugin caricato in memoria. Senza questo, NSIS scarica il plugin dopo ogni chiamata, perdendo tutto lo stato (handle finestra, thread, ecc.), causando il fallimento di `stop`.

```nsis
; Corretto - mantiene il plugin caricato
nssplashpng::show /NOUNLOAD 5000 /FADEIN /FADEOUT "$TEMP\splash.png"
nssplashpng::stop /FADEOUT

; Sbagliato - plugin scaricato dopo show, stop fallirà
nssplashpng::show 5000 /FADEIN /FADEOUT "$TEMP\splash.png"
nssplashpng::stop /FADEOUT
```

### Sintassi

```nsis
nsSplashPNG::show [/NOUNLOAD] <millisecondi> [/FADEIN [step]] [/FADEOUT [step]] [/NOCANCEL] [/MONITOR <target>] <percorso_immagine>
nsSplashPNG::stop [/FADEOUT [step]]
```

**Nota**: Il timeout deve essere specificato subito dopo `/NOUNLOAD` (se presente), seguito da flag opzionali, con il percorso immagine come ultimo parametro.

### Parametri

#### Funzione show
- `/NOUNLOAD` - **Richiesto perché stop() funzioni**: Mantiene il plugin in memoria tra le chiamate
- `/FADEIN [step]` - Abilita effetto fade in. Step opzionale (1-255) controlla la velocità (default: 15)
  - Step più alto = fade più veloce (255=istantaneo, 51=veloce, 15=normale, 5=lento)
  - Formula durata: `(255 / step) × 30ms`
- `/FADEOUT [step]` - Abilita effetto fade out alla chiusura automatica tramite timer (step opzionale)
- `/NOCANCEL` - Disabilita click per chiudere (lo splash può essere chiuso solo programmaticamente o dal timer)
- `/MONITOR <target>` - Seleziona il monitor su cui centrare lo splash (default: monitor primario)
  - `PRIMARY` - Monitor primario (default)
  - `CURRENT` o `MOUSE` - Monitor dove si trova il cursore del mouse
  - `1`, `2`, `3`, ... - Monitor per indice 1-based (ordine di enumerazione)
  - `POINT x y` - Monitor più vicino alle coordinate schermo fornite
- `<millisecondi>` - Timer auto-close in millisecondi (0 = solo chiusura manuale)
- `<percorso_immagine>` - Percorso al file immagine (PNG, JPEG, BMP, GIF, TIFF, ICO)

#### Funzione stop
- `/FADEOUT [step]` - Abilita effetto fade out alla chiusura. Step opzionale (1-255) controlla la velocità (default: 15)

### Esempi

#### Utilizzo Base
```nsis
; Mostra splash per 3 secondi (PNG con alfa) - /NOUNLOAD non necessario per solo auto-close
nsSplashPNG::show 3000 "$EXEDIR\splash.png"

; Mostra splash JPEG (opaco, senza trasparenza)
nsSplashPNG::show 3000 "$EXEDIR\splash.jpg"
```

#### Effetti Fade
```nsis
; Velocità fade default (step 15, ~510ms)
nsSplashPNG::show 3000 /FADEIN /FADEOUT "$EXEDIR\splash.png"

; Fade veloce (step 51, ~150ms)
nsSplashPNG::show 3000 /FADEIN 51 /FADEOUT 51 "$EXEDIR\splash.png"

; Fade lento (step 5, ~1530ms)
nsSplashPNG::show 5000 /FADEIN 5 /FADEOUT 5 "$EXEDIR\splash.png"
```

#### Controllo Manuale
```nsis
; CRITICO: Usare /NOUNLOAD quando si usa stop() per chiudere manualmente
nsSplashPNG::show /NOUNLOAD 0 /NOCANCEL "$EXEDIR\splash.png"

; ... operazioni di installazione ...

; Chiudi con fade out
nsSplashPNG::stop /FADEOUT
```

## Dettagli Tecnici

### Implementazione
- **Libreria Grafica**: WIC (Windows Imaging Component) tramite windowscodecs.lib
- **Formato Immagine**: 32bppPBGRA (alfa pre-moltiplicato) per corretta trasparenza
- **Tipo Finestra**: Layered window con `WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW`
- **Trasparenza**: `UpdateLayeredWindow` con blending alfa per pixel
- **Rendering**: Accelerato hardware tramite `UpdateLayeredWindow` con `ULW_ALPHA`
- **Animazione**: Timer da 30ms per fade fluido (range alfa 0-255)
- **Threading**: Thread finestra in background per esecuzione non bloccante

### Confronto con nsAdvsplash

| Funzionalità | nsAdvsplash | nsSplashPNG |
|--------------|-------------|-------------|
| Supporto Alfa PNG | Parziale | Completo |
| Effetti Fade | Sì | Sì |
| Metodo Trasparenza | Color key | Alfa per pixel |
| API Grafica | GDI | WIC |
| Tipo Finestra | Standard | Layered |
| Click per Chiudere | Sì | Sì |
| Warning Compilazione | Alcuni | Nessuno |

### Formati Immagine Supportati

WIC rileva e decodifica automaticamente i seguenti formati:
- **PNG** - Supporto completo trasparenza alfa
- **JPEG/JPG** - Senza trasparenza (opaco)
- **BMP** - Trasparenza alfa se a 32-bit, altrimenti opaco
- **GIF** - Supporto trasparenza base
- **TIFF** - Trasparenza alfa se supportata dalla variante
- **ICO** - Formato icona con alfa
- **WMP** - Windows Media Photo

### Requisiti di Sistema
- Windows Vista o successivo (per supporto layered window)
- WIC - Windows Imaging Component (incluso da Windows Vista in poi)

## Risoluzione Problemi

### Immagine non visibile
- Verificare che il percorso del file immagine sia corretto e accessibile
- Controllare che il file sia in un formato supportato
- Verificare che WIC possa decodificare il file immagine specifico

### Nessuna trasparenza
- Confermare che il PNG abbia canale alfa (non solo RGB)
- Verificare di usare la DLL plugin corretta per la versione NSIS

### Effetti fade non fluidi
- Comportamento normale - il fade usa timer 30ms con incrementi alfa di 15 step
- Durata totale fade ≈ 450-500ms

## Licenza

Creato per il sistema di installazione NSIS.

## Crediti

- NSIS Plugin API
- WIC - Windows Imaging Component
- Windows Layered Windows API (`UpdateLayeredWindow`)
- Sistema di build ispirato al plugin nsProcess

## Cronologia Versioni

### 1.0.0 (Corrente)
- Supporto trasparenza alfa PNG completo tramite WIC
- Blending alfa per pixel con `UpdateLayeredWindow`
- Effetti fade in/out (30ms, 15 step)
- Timer auto-close con timeout configurabile
- Click per chiudere con disabilitazione opzionale (`/NOCANCEL`)
- Build x86-unicode, x64-unicode e x86-ansi
- Zero warning di compilazione
- Sistema di build ottimizzato con compilazione parallela

---

*See [README.md](README.md) for the English version.*
