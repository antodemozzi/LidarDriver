# LidarDriver

Applicazione C++17 che acquisisce scansioni LiDAR da file CSV o da UDP e le
gestisce tramite un buffer circolare FIFO. Il progetto è una base leggera per
integrare una sorgente reale mantenendo separati acquisizione, validazione e
logica del driver.

## Funzionalità

- sorgente da file (`FileScanSource`) oppure da datagrammi UDP (`UdpScanSource`);
- scansioni con timestamp, angolo iniziale e risoluzione;
- valori mancanti o non validi rappresentati da una `Measurement` con
  `valid == false`, mai come `0.0` (zero resta una misura valida);
- normalizzazione alla dimensione attesa, con troncamento delle misure in più
  e valori mancanti per quelle assenti;
- buffer FIFO circolare con capacità configurabile;
- query della misura più vicina a un angolo;
- test automatici tramite CTest.
- timeout configurabile sulla ricezione UDP;
- eccezioni dedicate per buffer, parsing, file e UDP;
- logging con livelli `info`, `warning` ed `error`;
- frame UDP testuali con sequenza e timestamp oppure frame binari `LD1`.

## Formato dati

Ogni riga del file o ogni datagramma UDP contiene una scansione CSV:

```text
1.03,0.17,4.89,,nan,3.08
```

I valori devono essere finiti e non negativi. Un campo vuoto o `nan` è una
misura non disponibile. Le scansioni possono avere dimensioni diverse: il
driver le adatta alla risoluzione configurata.

## Compilazione

```powershell
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Su Windows viene collegata automaticamente `ws2_32` per il supporto UDP.

## Utilizzo

Modalità file, con termine alla fine della sorgente:

```powershell
.\build\LidarDriver.exe --file data\data.txt --resolution 0.5 --capacity 10
```

Per riutilizzare ciclicamente le scansioni:

```powershell
.\build\LidarDriver.exe --file data\data.txt --repeat
```

Per terminare le scansioni alla fine del file:

```powershell
.\build\LidarDriver.exe --file data\data.txt --no-repeat
```

Modalità UDP:

```powershell
.\build\LidarDriver.exe --udp 9000 --resolution 0.5 --start-angle 0
```

Un sensore o un programma esterno deve inviare datagrammi UDP verso la porta
indicata, con una riga CSV per datagramma. La lettura UDP è bloccante: il
comando `1` attende la prossima scansione con un timeout configurabile.

Per impostare il timeout:

```powershell
.\build\LidarDriver.exe --udp 9000 --udp-timeout-ms 500
```

È supportato anche il frame testuale:

```text
LIDAR1;seq=42;timestamp_ms=1700000000000;start=0;resolution=0.5;sensor=lidar-1|1.03,0.17,4.89
```

Il formato binario usa il magic `LD1`, seguito da sequenza a 32 bit,
timestamp in millisecondi a 64 bit, angolo iniziale e risoluzione come
`double`, conteggio a 32 bit e misure `double` in little-endian.

Opzioni disponibili:

| Opzione | Descrizione |
|---|---|
| `--file PATH` | Usa un file CSV come sorgente |
| `--udp PORT` | Ascolta datagrammi UDP |
| `--resolution DEGREES` | Risoluzione tra `0.1` e `1.0` |
| `--start-angle DEGREES` | Angolo iniziale della scansione |
| `--capacity N` | Numero massimo di scansioni nel buffer |
| `--no-repeat` | Non ricomincia il file quando raggiunge la fine |
| `--repeat` | Ricomincia ciclicamente il file quando raggiunge la fine |
| `--udp-timeout-ms N` | Timeout in millisecondi per una ricezione UDP |

Comandi interattivi:

| Comando | Comportamento |
|---|---|
| `1` | Acquisisce e inserisce una scansione |
| `2` | Stampa la scansione più recente |
| `3` | Estrae la scansione meno recente |
| `4` | Cerca la distanza a un angolo |
| `5` | Svuota il buffer |
| `6` | Esce |

Se la misura richiesta è assente, il programma restituisce esplicitamente
`No valid measurement at this angle` invece di confonderla con una distanza
pari a zero.

## Struttura

```text
LidarDriver/
├── CMakeLists.txt
├── data/data.txt
├── include/
│   ├── DataSource.h
│   ├── FileUtils.h
│   └── LidarDriver.h
├── src/
│   ├── DataSource.cpp
│   ├── FileUtils.cpp
│   ├── LidarDriver.cpp
│   └── main.cpp
└── tests/LidarDriverTests.cpp
```

## Limiti attuali

Il protocollo UDP non fornisce una coda affidabile: pacchetti duplicati, fuori
ordine o persi devono essere gestiti dal sensore o da un adattatore dedicato.
Per integrare un sensore specifico è sufficiente aggiungere un parser o una
nuova implementazione di `ScanSource`, senza modificare `LidarDriver`.

Il progetto non esegue ancora filtraggio del rumore, sincronizzazione tra
sensori o visualizzazione grafica; queste funzionalità possono essere aggiunte
sopra la struttura attuale.

## Autori

- Larisa Apostol - matricola 2101805
- Anna Cazzadore - matricola 2101825
- Giada Ruggero - matricola 2101765
