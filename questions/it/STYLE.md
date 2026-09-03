# Guida di stile italiana

Come si scrive una domanda Cicala in italiano. Il criterio: si adatta a ogni
mazzo scelto, non si esaurisce con un sì o un no, ed è abbastanza concreta da
far partire un racconto. Il ragionamento dietro queste regole sta nella
[teoria](../../docs/theory.md).

## Voce

- Rivolgiti a una persona sola: **tu**, mai "Lei", mai "si" impersonale.
- **Niente anglicismi nelle domande.** Scrivi "appuntamento", non "date". Le
  chiavi dei mazzi e le scritte del primo prototipo restano in inglese.
- Una buona domanda italiana suona come se qualcuno l'avesse posta a tavola,
  che sia stata scritta in italiano o tradotta da un'altra lingua.
- Le traduzioni automatiche sono bozze. Devono conservare senso, presupposti,
  tono e ventaglio di risposte plausibili dell'originale scritto da una
  persona, e collegare l'originale con `origin`. Se una bozza non suona
  naturale, riscrivila o scartala.
- Un punto interrogativo, nessun punto esclamativo, esattamente una domanda per
  voce.

## Forma

- Punta a 10–95 caratteri; 140 è il limite tecnico. La domanda finisce con `?`.
- Comincia con *Quando/Che cosa/Chi/Come/Quale* invece di *Hai/Sei*; questi
  ultimi invitano a un sì o un no.
- Concreto batte astratto: "Quando hai cambiato idea su qualcosa che contava?"
  batte "Sei una persona di mentalità aperta?"
- Le domande in due parti ("…, e com'è andata?") funzionano quando la seconda
  parte tira fuori il racconto.
- Scrivi solo domande a cui risponderesti anche tu.
- Evita i superlativi come "migliore", "peggiore", "di più" e "di meno":
  trasformano un ricordo in una classifica.

## Assegnazione dei mazzi

- `new_people`: non presuppone un passato in comune. Niente verifiche su quanto
  ci si conosce, niente biografie forzate.
- `close`: presuppone familiarità. Chiedi di cambiamenti e interpretazioni più
  che di dati anagrafici già noti.
- `family`: deve essere sicura *e interessante* per una persona di dieci anni.
  Intergenerazionale: nonna e nipote devono poter rispondere entrambe.
- `work`: niente intimità forzata, pettegolezzi, diagnosi o risposte che
  possano cambiare la posizione di qualcuno sul lavoro.
- `here`: usa la stanza, il tavolo, l'occasione o ciò che si vede intorno.
- `wild`: cupa, azzardata, macabra o assurda. Domande così non compaiono in
  nessun altro mazzo.

Una domanda si archivia una volta sola e può appartenere a più mazzi.

## Profondità

- `1`: chiede poca esposizione pubblica.
- `2`: chiede una costruzione personale.
- `3`: può toccare vulnerabilità, conflitto, paura, perdita o rivelazioni che
  hanno conseguenze.

Profondità e tono sono indipendenti. La riproduzione normale usa per ora 1 e 2;
il 3 resta nel corpus e nella vista di consultazione.

## Etichette

Usale con parsimonia, la maggior parte delle domande ne vuole zero o una. Le
chiavi restano in inglese ovunque:

- `icebreaker`: sicura con estranei nei primi cinque minuti.
- `reflective`: chiede uno sguardo all'interno; è normale una pausa di
  riflessione.
- `spicy`: azzardata o socialmente rischiosa. Richiede `decks: [wild]`.
- `dark`: tono cupo o macabro. Richiede `decks: [wild]`.
- `hypothetical`: giochi mentali ("se…", "che cosa faresti…").
- `memory`: chiede un momento preciso che si ricorda.
- `wouldyourather`: una scelta forzata fra due sole possibilità.
