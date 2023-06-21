# comelit-esphome
Interfaccia Comelit Simplebus per Home Assistant

## Introduzione al progetto

Inizialmente volevo modificare il mio citofono Comelit mini vivavoce 6721W per interfacciarlo ad Home Assistant, per poter ricevere una notifica quando qualcuno mi citofona, e per poter aprire i due portoni comandati dal citofono comodamente da remoto.

![Comelit mini](/immagini/comelit_mini.jpg)

Il citofono lavora su bus a 2 fili Simplebus2 proprietario di Comelit.
Volevo collegarmi direttamente al circuito stampato del posto interno, il quale però utilizza lo stesso altoparlante per la suoneria e per la chiamata vocale, ha pulsanti a sfioramento, la situazione si faceva complicata.
Ho scartato l’idea di usare un Ring Intercom, perché nonostante funziona benissimo e supporta il protocollo Simplebus2, non permette di comandare l’apertura del secondo portone, è ingombrante, lavora solamente in cloud e ha il problema dell’alimentazione a batteria.

Ho quindi scoperto il meraviglioso lavoro di **[plusvic](https://github.com/plusvic/simplebus2-intercom)** che ha analizzato e decodificato il protocollo simplebus, e ha realizzato un ripetitore di suoneria basato su un PIC usato per decodificare il protocollo, chip di trasmissione wireless, ESP8266.
**[aoihaugen](https://github.com/aoihaugen/simplebus2-intercom)** ha creato un fork, e ha adattato il codice per la decodifica del segnale su arduino.
Voglio fare un enorme ringraziamento ad entrambi, senza il vostro lavoro non sarei mai arrivato al mio obiettivo, ho preso abbondantemente spunto da entrambi per hardware e software.

Nella mia realizzazione ho utilizzato un Wemos d1 mini con firmware basato su Esphome, per una facile integrazione su Home Assistant.

## Hardware

Ho adattato il circuito proposto da **[plusvic](https://github.com/plusvic/simplebus2-intercom)** modificandolo per adattarlo alla situazione del mio impianto condominiale e alle necessità di esphome. La tensione a vuoto che rilevo sui fili del bus è di circa 35V, e il segnale dati è sovrapposto alla tensione DC. 
Lo scambio di informazioni (comandi) tra posto interno e posto esterno avviene attraverso un segnale dati modulato a 25kHz, nel mio caso di ampiezza picco-picco variabile da 0.5V a 5V in base alla distanza del dispositivo che sta trasmettendo. Quando viene avviata la comunicazione audio/video si aggiunge un ulteriore segnale ad alta frequenza.

![Schema elettrico](/immagini/schema_elettrico.png)

### Sezione alimentazione

Il circuito ha la possibilità di essere alimentato direttamente dal bus. 
Ho deciso di utilizzare un modulo switching step-down DD4012SA che supporta fino a 40v in ingresso e dà in uscita 5v.
L’intero circuito assorbe circa 20mA dalla linea bus, che salgono a 160mA durante la trasmissione dati.
Ho posto in ingresso un fusibile da 250mA a protezione del circuito, seguito da un ponte raddrizzatore.
L’alimentazione del modulo switching è presa a valle di un filtro RC passa basso, formato da R1 e C2, che elimina le fluttuazioni dovute alla trasmissione dati. Dopo diverse prove il miglior compromesso tra riduzione del rumore ricevuto e indotto sul bus e potenza dissipata l’ho ottenuto con una resistenza da 220Ω, che dissipa appena 75mW in maniera continuativa.

### Sezione ricezione dati

Come nel circuito a cui mi sono ispirato, il segnale dati viene captato dal bus grazie a un filtro di tipo passa alto, nello schema composto da R5 e C1, rispettivamente da 10kΩ e 10nF. 
Il segnale ai capi della resistenza viene quindi dato in ingresso ad un doppio comparatore LM2903 con uscita di tipo open collector. La soglia con cui viene comparato il segnale è posta a circa 0.25v tramite un partitore resistivo. 
Per semplificare la ricezione del segnale da parte del Wemos ho deciso di inserire uno stadio monostabile, eliminando la portante a 25 kHz e sfruttando il secondo comparatore già a bordo dell’LM2903. 
In presenza di segnale, l’uscita del primo comparatore viene portata bassa scaricando il condensatore C3 da 10nF, in mancanza di segnale il condensatore si carica attraverso la resistenza R9 da 220kΩ.
La tensione ai capi del condensatore viene comparata con una seconda tensione fissa, ricavata dallo stesso partitore usato sul primo stadio. L’uscita del secondo comparatore viene mandata al Wemos, che quindi si ritroverà in ingresso i pacchetti dati privati dell’oscillazione della portante.
In presenza di un qualsiasi segnale in ingresso, l’uscita del secondo comparatore rimane alta per ulteriori 1.5ms oltre al tempo in cui il segnale sul bus resta alto. Per demodulare la portante dei comandi sarebbe bastato un tempo di 20µs, ma aumentare così tanto la tempistica mi ha permesso di ridurre la captazione di ulteriori segnali presenti sul bus, che disturbano la ricezione e la decodifica dei comandi.

### Sezione trasmissione dati

La trasmissione dati avviene creando un assorbimento impulsivo sul bus, modulato a 25kHz, con le tempistiche dettate dal protocollo di comunicazione. Un piedino digitale del wemos pilota un transistor NPN, che deve supportare tensioni di almeno 40v e correnti di almeno 150mA. Nel prototipo che ho realizzato ho utilizzato un BC337. 

La corrente che passa dal transistor viene imposta dal valore della resistenza posta sul collettore. Ho scelto di collegare due resistenze da 470Ω in parallelo per dividere la dissipazione di potenza. 

Nel mio caso, con valori resistivi più alti, i comandi inviati dal wemos non sempre vengono ricevuti correttamente. In base alla distanza dall’alimentatore del bus potrebbe essere necessario modificare il valore.

### Realizzazione Hardware

Inizialmente ho realizzato il circuito su breadboard, una volta messo a punto lo schema definitivo ho realizzato un pcb utilizzando componenti in tecnologia SMD, grande appena 46 x 29 mm, ovvero poco più grande del wemos D1 mini. Nel mio caso è entrato perfettamente dentro la scatoletta tonda predisposta dietro al citofono.

![pcb_3d](/immagini/pcb_3d.png) ![pcb](/immagini/pcb.png)

Se l’impianto citofonico è alimentato dal contatore condominiale potreste decidere di non alimentare il wemos dal bus, non montando il modulo switching e alimentando il wemos a 5v in maniera indipendente. In tale maniera il consumo del wemos (0.5w) non graverà sull’alimentazione elettrica condominiale

## Protocollo

Come già accennato, lo scambio di informazioni tra i vari dispositivi avviene tramite un protocollo basato su una portante a 25Khz. Tale portante viene sempre inviata per 3ms. La durata del “silenzio” tra due trasmissioni determina il valore del bit trasmesso, 3ms corrisponde a “0”, 6ms corrisponde a “1”.

La trasmissione inizia sempre con un impulso da 3ms, seguito da un silenzio di 16ms.

Successivamente vengono inviati 18 bit, così suddivisi:

- i primi 8 bit indicano l’indirizzo. Viene trasmesso per prima il bit meno significativo.
- i successivi 6 bit indicano il comando, anche qui si parte dal bit meno significativo.
- i successivi 4 bit sono di checksum, indicano semplicemente il numero di bit a 1 inviati nei campi precedenti.

Ad esempio, quando dal posto esterno chiamo un citofono interno viene inviato il comando 50. Supponiamo di chiamare il citofono n°21:
- Comando 50 = 110010
- Indirizzo 21 = 00010101
- Checksum 6 = 0110

La sequenza di bit inviata è quindi 010011 10101000 0110

Sulla linea bus vengono quindi inviati comandi così formattati a tutti i posti interni.

Viene inviato un comando anche quando dal posto interno si richiede di attivare la comunicazione video in assenza di chiamata (comando 20), quando si comanda l’apertura del portone principale (comando 16), quando si comanda l’apertura del portone secondario (comando 29), quando un posto interno riceve la chiamata “fuori porta”, quando si avvia o si interrompe la conversazione audio, quando la chiamata va in timeout, etc.

## Software

Come piattaforma software ho usato ESPhome per una veloce integrazione su home assistant.
Ho deciso di sfruttare i componenti *“remote receiver”* e *“remote transmitter”* per la decodifica dei bit ricevuti. 

Remote receiver decodifica automaticamente i protocolli più comuni relativi a telecomandi o trasmettitori radio. 

Se la decodifica nativa non è disponibile, si può ricorrere alla modalità raw: ogni volta che viene ricevuto un treno di impulsi viene eseguita una automazione a cui viene passato un array di numeri interi. Il primo numero dell’array, positivo, indica per quanti microsecondi il segnale in ingresso è stato a valore logico alto. Il secondo valore, negativo, indica quanti microsecondi il segnale in ingresso è stato a valore logico basso. I successivi valori seguono la stessa logica, alternando sempre numeri positivi e negativi.

Viene da sé che basta controllare il valore dei numeri negativi dell’array per decodificare il comando e l’indirizzo del protocollo comelit simplebus2.

L’automazione dapprima controlla se la lunghezza dell’array è quella che ci aspettiamo per un comando correttamente ricevuto, quindi viene decodificato il valore dell’indirizzo e del comando, viene eseguito il controllo del checksum, e se il checksum è corretto viene generato un evento di home assistant del tipo “esphome.comelit_ricevuto” con comando e indirizzo nel campo dati.

Basterà creare una automazione su home assistant triggerata dall’evento, discriminando solamente quelle con comando “50” e indirizzo relativo al proprio posto interno, per poter ricevere una notifica quando viene chiamato il nostro citofono.


#### esempio di trigger:

    platform: event
    event_type: esphome.comelit_ricevuto
    event_data:
      comando: "50"
      indirizzo: "11"


Per poter inviare i comandi sul bus, ho realizzato una semplice funzione che viene richiamata tramite un servizio di Home Assistant, la quale converte i campi indirizzo e comando nell’array di interi che può essere dato in pasto al componente “remote transmitter”.
Remote transmitter si occuperà di pilotare il transistor, modulandolo con una portante a 25kHz per il numero di microsecondi indicato nei valori interi dell’array.

La funzione che si occupa della codifica risiede all’interno del file comelit.h, che deve essere inserito nella cartella “esphome” della propria configurazione di home assistant, verrà richiamata tramite un include.

Si possono realizzare anche servizi con già codificati all’interno comando e indirizzo fissati precedentemente. Si potrà creare quindi un comodo widget sul proprio smartphone, il quale richiamerà semplicemente il servizio “esphome.apri_portone” per poter aprire il portone con un tocco.

![Widget](/immagini/widget.jpg)

Il sistema crea un evento sul log di home assistant per ogni comando ricevuto sul bus. Se non si vuole utilizzare questa funzione basta eliminare la parte di software relativa.

![Log](/immagini/log.png)

Il log viene "agganciato" all'entità sensor.comelit_stato

## Installazione

- Copiare dentro la cartella esphome di home assistant il file comelit.h
- Collegare il wemos al pc e aggiungerlo alla propria configurazione di esphome. Non collegarlo al pc mentre che è alimentato dal bus.
- Appuntarsi la api encription key e la ota password del file di configurazione generato automaticamente.
- Sostituire il codice base generato da esphome col codice del file esphome.yaml
- Sostituire la api encription key e la ota password appuntati precedentemente negli appositi campi.

## Acquisto materiali e pcb

Ho la possibilità di fornire il pcb, i componenti, o anche l'intero hardware già saldato e collaudato.

Se sei interessato, contattami su mansellrace@gmail.com
