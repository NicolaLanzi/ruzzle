# Ruzzle Italiano

Clone semplificato di **Ruzzle** scritto in **C++17** con
[raylib](https://www.raylib.com/).

Il gioco mostra una griglia 4x4 di lettere: trascina il mouse sulle celle
adiacenti per comporre parole italiane, poi rilascia il mouse (oppure premi
Invio) per confermarle. Le parole valide vengono aggiunte alla lista e
assegnano punti; allo scadere del tempo la partita termina.

## Caratteristiche

- griglia 4x4 generata casualmente a ogni partita;
- dizionario italiano incluso, con oltre 660.000 parole valide;
- selezione delle lettere tramite trascinamento del mouse;
- controllo delle parole duplicate e della lunghezza minima;
- punteggio, timer, lista delle parole trovate e messaggi di feedback;
- interfaccia con barra del tempo, anteprima della parola e schermata finale;
- compilazione multipiattaforma con CMake (Windows, Linux, macOS);
- raylib usata dalla copia locale `vendor/` oppure scaricata automaticamente
  con FetchContent.

## Come funziona

### 1. Caricamento del dizionario

All'avvio il programma cerca `assets/italiano.txt` in queste posizioni:

1. `assets/italiano.txt`
2. `italiano.txt`
3. `../assets/italiano.txt`
4. `../italiano.txt`

Ogni riga viene ripulita da spazi, ritorni a capo e BOM UTF-8, convertita in
maiuscolo e inserita in un `unordered_set`. Per poter essere composta sulla
griglia, una parola deve:

- contenere almeno 3 lettere;
- non superare 16 lettere;
- essere composta solo da lettere `A-Z` (le parole con accenti, apostrofi o
  trattini non sono utilizzabili perché la griglia non contiene quei simboli).

### 2. Generazione della griglia

La griglia è composta da 16 celle. Ogni cella ha circa il 40% di probabilità di
essere una vocale; alla fine la generazione garantisce che ci siano tra 5 e 10
vocali. Le consonanti usate sono quelle dell'alfabeto italiano.

### 3. Selezione delle lettere

- Premi il tasto sinistro del mouse su una cella per iniziare.
- Trascina sulle celle adiacenti (anche in diagonale) per aggiungere lettere.
- Una cella non può essere usata due volte nella stessa parola.
- La parola corrente viene mostrata sotto la griglia e le lettere collegate
  sono evidenziate con una linea.

### 4. Conferma e punteggio

La parola viene confermata quando:

- rilasci il tasto sinistro del mouse con almeno 3 lettere selezionate;
- oppure premi `Invio`.

Il programma controlla che la parola non sia già stata trovata e che sia
presente nel dizionario. Ogni lettera vale **2 punti**, quindi una parola di 5
lettere vale 10 punti. Non ci sono penalità per le parole non valide.

### 5. Timer e fine partita

La partita dura **90 secondi**. Il timer viene aggiornato a ogni frame e non
può scendere sotto zero. Quando il tempo termina:

- la selezione corrente viene annullata;
- viene mostrata la schermata con punteggio finale e numero di parole trovate;
- puoi premere `R` per ricominciare o `ESC`/`Q` per uscire.

## Struttura del progetto

```text
ruzzle/
├── CMakeLists.txt       # configurazione principale e dipendenza raylib
├── src/
│   ├── CMakeLists.txt   # sorgenti del target ruzzle
│   └── main.cpp         # logica e interfaccia del gioco
├── assets/
│   └── italiano.txt     # dizionario italiano
├── README.md
└── .gitignore
```

La cartella `vendor/` non è versionata: se è presente localmente viene usata
per la compilazione offline; altrimenti CMake scarica raylib 5.5.

## Ottenere il progetto

Clona il repository:

```bash
git clone https://github.com/NicolaLanzi/ruzzle.git
cd ruzzle
```

In alternativa puoi scaricare l'archivio ZIP dalla pagina GitHub del progetto.

## Requisiti

- CMake 3.20 o superiore
- Un compilatore C++17
- Una GPU/driver con supporto OpenGL 3.3
- Connessione a Internet alla prima configurazione, a meno che raylib non sia
  già presente in `vendor/raylib-master`

### Windows

Serve Visual Studio 2019 o 2022 con il carico di lavoro
**"Sviluppo di applicazioni desktop con C++"** e CMake. Visual Studio include
già una versione di CMake sufficiente.

### Linux

Su Debian/Ubuntu installa i prerequisiti con:

```bash
sudo apt update
sudo apt install build-essential cmake git \
    libasound2-dev libx11-dev libxrandr-dev libxi-dev \
    libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev
```

### macOS

Installa i Command Line Tools di Xcode e CMake:

```bash
xcode-select --install
brew install cmake
```

## Compilazione

### Windows (PowerShell)

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Con il generatore Visual Studio l'eseguibile si trova in:

```text
build\Release\ruzzle.exe
```

Se usi Ninja o NMake, il percorso è invece `build\ruzzle.exe`.

Per forzare Visual Studio 2022:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### Linux e macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/ruzzle
```

La cartella `assets/` viene copiata automaticamente accanto all'eseguibile
durante la build, quindi il dizionario viene trovato anche se il programma
viene avviato dalla cartella di build.

## Esecuzione

Da Windows:

```powershell
.\build\Release\ruzzle.exe
```

Da Linux/macOS:

```bash
./build/ruzzle
```

Puoi anche avviare l'eseguibile dalla cartella principale del progetto: il
programma cerca il dizionario nei percorsi di fallback descritti sopra.

## Controlli

| Azione | Comando |
| --- | --- |
| Seleziona una lettera | Tasto sinistro del mouse |
| Aggiungi lettere adiacenti | Trascina con il mouse |
| Conferma la parola | Rilascia il mouse (minimo 3 lettere) oppure `Invio` |
| Rimuovi l'ultima lettera | `Backspace` o tasto destro |
| Annulla la selezione | `ESC` |
| Nuova partita | `R` |
| Esci dal gioco | `Q` oppure chiudi la finestra |

Nella schermata di game over, `ESC` e `Q` chiudono il programma e `R` avvia una
nuova partita.

## Dizionario

Il dizionario incluso si trova in `assets/italiano.txt` e pesa circa 8 MB.
Contiene una parola per riga; dopo il caricamento e i filtri restano
**661.382 parole valide**.

Per sostituirlo basta creare un file di testo con una parola per riga e
salvarlo come `assets/italiano.txt`. Le righe vuote, i duplicati e le parole non
compatibili con la griglia vengono ignorati.

Il programma carica il dizionario in memoria all'avvio: su computer molto lenti
possono essere necessari alcuni istanti.

## Personalizzazione

Le costanti principali si trovano all'inizio di `src/main.cpp`:

- `ROUND_TIME`: durata della partita (90 secondi);
- `MIN_WORD_LENGTH`: lunghezza minima delle parole (3);
- `POINTS_PER_LETTER`: punti per lettera (2);
- dimensioni e colori dell'interfaccia.

Per cambiare la dimensione della griglia bisogna invece modificare anche le
strutture dati e la logica di selezione, perché il gioco è pensato per una
griglia 4x4.

## Risoluzione dei problemi

### "Impossibile aprire assets/italiano.txt"

Avvia il programma dalla cartella del progetto o dalla cartella di build, dove
si trova la copia di `assets/`. Verifica anche che il file esista e non sia
vuoto.

### CMake non scarica raylib

Controlla la connessione a Internet e che Git/CMake possano accedere a GitHub.
In alternativa, estrai raylib in `vendor/raylib-master`: se il file
`vendor/raylib-master/CMakeLists.txt` esiste, CMake usa quella copia senza
scaricare nulla.

### Errori di link su Linux

Assicurati di aver installato tutti i pacchetti di sviluppo elencati nella
sezione requisiti, in particolare quelli per OpenGL, X11 e ALSA.

### La finestra non si apre o è nera

Aggiorna i driver grafici e verifica che la scheda video supporti OpenGL 3.3.

## Note tecniche

- Il progetto usa C++17 e non richiede librerie aggiuntive oltre a raylib.
- Il dizionario è filtrato sulle lettere `A-Z`: le parole accentate non sono
  utilizzabili perché la griglia non mostra accenti.
- Non è presente un sistema di salvataggio dei punteggi: ogni partita parte da
  zero.
- Il progetto è stato compilato e avviato su Windows con Visual Studio 2019
  Build Tools e CMake. Le istruzioni per Linux e macOS sono incluse, ma non
  sono state testate su quelle piattaforme.
- raylib è distribuita con licenza zlib; consulta il suo repository per i
  dettagli.

## Crediti

- [raylib](https://www.raylib.com/) per la grafica, l'input e la finestra.
- Dizionario italiano incluso nel progetto.
