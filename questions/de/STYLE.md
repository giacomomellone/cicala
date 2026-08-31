# Deutscher Stil-Leitfaden

So schreibst du eine Cicala-Frage auf Deutsch. Die Messlatte: Sie passt zu
jedem gewählten Deck, ist nicht mit Ja oder Nein zu erledigen und ist konkret
genug, um eine Geschichte anzustoßen. Die Begründung für diese Regeln steht in
der [Theorie](../../docs/theory.md).

## Stimme

- Sprich eine Person direkt an: **du**, immer klein geschrieben, niemals "Sie" oder "man".
- **Keine Anglizismen in Fragen.** Schreibe "Verabredung", nicht "Date". Die
  Deck-Schlüssel und die Beschriftung des ersten Prototyps bleiben englisch.
- Eine gute deutsche Frage klingt, als hätte sie jemand am Küchentisch
  gestellt, unabhängig davon, ob sie auf Deutsch verfasst oder aus einer anderen
  Sprache übersetzt wurde.
- Maschinelle Übersetzungen sind Entwürfe. Sie müssen Sinn, Annahmen, Ton und
  mögliche Antworten des von einem Menschen verfassten Originals bewahren und
  das Original über `origin` verlinken. Klingt ein Entwurf nicht natürlich,
  überarbeite oder verwerfe ihn.
- Ein Fragezeichen, keine Ausrufezeichen, genau eine Frage pro Eintrag.

## Form

- Ziele auf 10–95 Zeichen; 140 ist die technische Obergrenze. Die Frage endet mit `?`.
- Beginne mit *Wann/Was/Wer/Wie/Welche* statt mit *Hast du/Bist du*; Letzteres lädt zu Ja/Nein ein.
- Konkret schlägt abstrakt: "Wann hast du zuletzt deine Meinung zu etwas Wichtigem geändert?" schlägt "Bist du aufgeschlossen?"
- Zweiteilige Fragen ("…, und was ist passiert?") sind gut, wenn der zweite Teil die Geschichte hervorlockt.
- Schreibe nur eine Frage, die auch die fragende Person beantworten könnte.
- Meide Superlative wie "beste", "schlimmste", "meiste" und "wenigste"; sie machen aus Erinnerung eine Rangliste.

## Deck-Zuordnung

- `new_people`: setzt keine gemeinsame Vergangenheit voraus. Keine
  Wissensprüfung über die Beziehung und keine erzwungene Lebensgeschichte.
- `close`: setzt Vertrautheit voraus. Frage eher nach Veränderung und Deutung
  als nach bekannten Eckdaten.
- `family`: muss für ein zehnjähriges Kind sicher *und interessant* sein. Generationsübergreifend: Oma und Enkel können beide antworten.
- `work`: keine erzwungene Intimität, Gerüchte, Diagnosen oder Antworten, die
  den Status am Arbeitsplatz verändern können.
- `here`: nutzt den Raum, den Tisch, die Veranstaltung oder sichtbare Umgebung.
- `wild`: düster, gewagt, makaber oder absurd. Solche Fragen erscheinen in
  keinem anderen Deck.

Eine Frage wird einmal gespeichert und kann mehreren Decks zugeordnet sein.

## Tiefe

- `1`: wenig öffentliche Preisgabe.
- `2`: verlangt eine persönliche Einordnung.
- `3`: kann Verletzlichkeit, Konflikt, Angst, Verlust oder folgenreiche
  Preisgabe betreffen.

Tiefe und Ton sind getrennt. Die normale Wiedergabe nutzt derzeit 1 und 2; 3
bleibt im Korpus und in der Stöberansicht.

## Schlagwörter

Sparsam verwenden, die meisten Fragen brauchen null oder eins. Die Schlüssel sind global englisch:

- `icebreaker`: sicher mit völlig Fremden in den ersten fünf Minuten.
- `reflective`: verlangt einen Blick nach innen; eine Denkpause ist zu erwarten.
- `spicy`: gewagt oder gesellschaftlich riskant. Erfordert `decks: [wild]`.
- `dark`: düsterer oder makaberer Ton. Erfordert `decks: [wild]`.
- `hypothetical`: Gedankenspiele ("wenn…", "was würdest du…").
- `memory`: fragt nach einem konkreten erinnerten Moment.
- `wouldyourather`: eine erzwungene Wahl zwischen genau zwei Möglichkeiten.
