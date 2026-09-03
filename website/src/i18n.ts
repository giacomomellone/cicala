// All UI strings, one object per shipped language (spec §7.3).

export type Lang = "en" | "de" | "it";

const en = {
  // header / nav
  "nav.browse": "browse",
  "nav.suggest": "suggest",
  "nav.device": "device",
  "nav.skip": "skip to content",

  // Physical category labels remain English in every UI language.
  "deck.all": "all",
  "deck.new_people": "new people",
  "deck.close": "close",
  "deck.family": "family",
  "deck.work": "work",
  "deck.here": "here",
  "deck.wild": "wild",

  // editorial metadata; the player does not expose a depth control
  "depth.1": "depth 1",
  "depth.2": "depth 2",
  "depth.3": "depth 3",

  // tags
  "tag.icebreaker": "icebreaker",
  "tag.reflective": "reflective",
  "tag.spicy": "spicy",
  "tag.dark": "dark",
  "tag.hypothetical": "hypothetical",
  "tag.memory": "memory",
  "tag.wouldyourather": "would you rather",

  // question provenance
  "question.translated": "automatically translated · human-reviewed",
  "question.edit": "suggest an edit →",

  // play
  "play.next": "next question →",
  "play.category": "category",
  "play.skip": "not every question is for every moment. feel free to skip.",
  "play.disclaimer":
    "Original questions are written by people. Machine translations are marked and reviewed by a fluent editor before publication.",
  "play.save": "save",
  "play.saved": "saved",
  "play.share": "share",
  "play.copied": "copied",
  "play.empty": "no questions in this deck yet.",
  "play.suggest": "have one worth asking? suggest a question →",

  // browse
  "browse.title": "browse",
  "browse.search": "search questions…",
  "browse.alltags": "all tags",
  "browse.sort.newest": "newest",
  "browse.sort.random": "random",
  "browse.more": "show more",
  "browse.empty": "No question matches. Maybe yours is missing?",
  "browse.empty.cta": "suggest it →",
  "browse.suggest": "something missing? suggest a question →",

  // native suggestion
  "suggest.title": "suggest a question",
  "suggest.pitch": "Write a question you would be willing to answer yourself.",
  "suggest.tips.eyebrow": "tip",
  "suggest.tips.prev": "previous tip",
  "suggest.tips.next": "next tip",
  "suggest.tip.build":
    "Make people build an answer instead of repeating one. If the answer is already finished in their head, the question is boring.",
  "suggest.tip.rankings":
    "Avoid rankings and superlatives: best, favourite, top, most. Ask only what you would answer yourself, at that table.",
  "suggest.tip.concrete":
    "Ask about one concrete thing, not a whole subject or a principle. A scene gives the table people, places and choices to ask about next.",
  "suggest.tip.assumptions":
    "Do not invent someone's life. Keep the assumption that helps and drop the details you cannot know about their family, work, health or history.",
  "suggest.tip.onejob":
    "Give the question one job, and keep it short. Leave the follow-up to the people at the table.",
  "suggest.lang": "language",
  "suggest.question": "your question",
  "suggest.question.hint": "10–140 characters, one question, ends with ?",
  "suggest.rule.short": "a bit longer: at least 10 characters",
  "suggest.rule.long": "too long: 140 characters max",
  "suggest.rule.mark": "must end with ?",
  "suggest.rule.multiline": "one question per entry, on one line",
  "suggest.rule.duplicate": "this question is already in the database",
  "suggest.rule.duplicate.link": "read the one we have →",
  "suggest.rule.ok": "looks good",
  "suggest.style.ranking":
    "“best” and “favourite” ask for a ranking, which turns a memory into a contest. What did you notice instead?",
  "suggest.style.stacked":
    "one question per entry. Leave the follow-up to the people at the table.",
  "suggest.style.long": "under 95 characters reads better out loud",
  "suggest.review":
    "Questions are reviewed before they are added. Editors assign decks, depth, and tags.",
  "suggest.name": "name for public credit (optional)",
  "suggest.cc0":
    "I dedicate this question to the public domain (CC0). Anyone may use it for any purpose, forever, without attribution.",
  "suggest.cc0.link": "what CC0 means",
  "suggest.submit": "send question →",
  "suggest.sending": "sending…",
  "suggest.unavailable": "native submissions are temporarily unavailable",
  "suggest.verification.error": "verification failed; please try again",
  "suggest.failed": "the question could not be sent; please try again",
  "suggest.received": "question received",
  "suggest.received.body":
    "A language maintainer will review it. If accepted, it will appear on the website and in a future device update.",
  "suggest.reference": "reference",
  "suggest.return": "return to questions",
  "suggest.again": "suggest another",

  // question edit
  "edit.title": "suggest an edit",
  "edit.pitch":
    "Propose a change to a question already in the database. Rewriting is Human Reserved: use your own words.",
  "edit.current": "the question today",
  "edit.notfound": "that question is not in the database",
  "edit.wording": "proposed wording (optional)",
  "edit.wording.hint": "leave empty for a metadata-only change",
  "edit.metadata": "metadata to reconsider (optional)",
  "edit.metadata.decks": "deck eligibility",
  "edit.metadata.depth": "depth",
  "edit.metadata.tags": "tags",
  "edit.metadata.translation": "translation provenance or origin",
  "edit.reason": "why should it change?",
  "edit.reason.hint":
    "explain the flaw, changed meaning, unnatural translation, or classification problem",
  "edit.reason.short": "a bit longer: at least 10 characters",
  "edit.reason.long": "too long: 1000 characters max",
  "edit.nothing": "propose wording or tick at least one box above",
  "edit.same": "that is the current wording",
  "edit.human":
    "I wrote this wording myself and did not use generative AI to create or rewrite it.",
  "edit.cc0": "I dedicate any wording I contribute here to the public domain (CC0-1.0).",
  "edit.review":
    "A maintainer who speaks the language reviews every request. Existing question IDs never change.",
  "edit.submit": "send edit →",
  "edit.unavailable": "native edits are temporarily unavailable",
  "edit.failed": "the edit could not be sent; please try again",
  "edit.received": "edit received",
  "edit.received.body":
    "A language maintainer will compare it with the current wording and decide. Nothing changes on the site until they do.",
  "edit.github": "prefer GitHub? open the issue form →",

  // deck
  "deck.title": "your deck",
  "deck.shared.title": "a shared deck",
  "deck.empty": "Your deck is empty. Heart questions while you play.",
  "deck.share": "share deck",
  "deck.share.copied": "link copied",
  "deck.share.truncated": "deck too large to share, link carries the first 150",
  "deck.export": "export",
  "deck.import": "import",
  "deck.saveall": "save all to my deck",
  "deck.saved": "saved to your deck",
  "deck.count": "questions",

  // device page
  "device.title": "the device",
  "device.p1":
    "A pocket-sized object for the middle of the table: one e-paper display, a small Category button, and a larger Next button. The display shows the active category and one conversation question.",
  "device.p2":
    "The category and question remain on e-paper while the device sleeps. There are no menus, modes, notifications, or hidden session state.",
  "device.p3":
    "The six decks are new people, close, family, work, here, and wild. Ordinary questions may belong to several decks. Wild is a tone choice for dark, spicy, or absurd prompts; it is not a depth level.",
  "device.how": "how it works",
  "device.t.input": "input",
  "device.t.action": "action",
  "device.t.feedback": "feedback",
  "device.t.r1a": "press Category",
  "device.t.r1b": "advance to the next category",
  "device.t.r1c": "e-paper shows the category name; the fifth press wraps to the first",
  "device.t.r2a": "press Next",
  "device.t.r2b": "draw another question",
  "device.t.r2c": "one e-paper refresh; a long press does exactly the same thing",
  "device.t.r3a": "hold Category and Next during startup",
  "device.t.r3b": "open service setup",
  "device.t.r3c": "Wi-Fi and language setup move to a phone; no table-facing menu",
  "device.t.r4a": "hands off",
  "device.t.r4b": "sleep",
  "device.t.r4c": "the category and question remain readable at zero display power",
  "device.build": "build one",
  "device.build.text":
    "Hardware (CERN-OHL-S) and firmware (MIT) live in this repository. The breadboard firmware works; the PCB and enclosure files are design contracts, not production files.",
  "device.link.firmware": "firmware sources",
  "device.link.hardware": "hardware / PCB",
  "device.link.guide": "build guide",
  "device.link.releases": "releases",
  "device.sync": "sync",
  "device.sync.text":
    "The device pulls signed per-language question bundles built from this same repository, so the site and the device consume the same release. It only ever downloads the languages you keep on it.",
  "device.sync.link": "how sync works",
  // 404
  "notfound.q": "Where do you go when you don't know where you're going?",
  "notfound.meta": "#404 · lost",
  "notfound.home": "take me home →",

  // footer
  "footer.questions": "questions",
  "footer.provenance": "human originals · reviewed translations",
  "footer.license": "questions are public domain (CC0)",
  "footer.source": "source",
} as const;

export type StringKey = keyof typeof en;

const de: Record<StringKey, string> = {
  "nav.browse": "stöbern",
  "nav.suggest": "vorschlagen",
  "nav.device": "gerät",
  "nav.skip": "zum Inhalt springen",

  "deck.all": "alle",
  "deck.new_people": "new people",
  "deck.close": "close",
  "deck.family": "family",
  "deck.work": "work",
  "deck.here": "here",
  "deck.wild": "wild",

  "depth.1": "tiefe 1",
  "depth.2": "tiefe 2",
  "depth.3": "tiefe 3",

  "tag.icebreaker": "eisbrecher",
  "tag.reflective": "nachdenklich",
  "tag.spicy": "gewagt",
  "tag.dark": "düster",
  "tag.hypothetical": "was wäre wenn",
  "tag.memory": "erinnerung",
  "tag.wouldyourather": "entweder oder",

  "question.translated": "automatisch übersetzt · von Menschen geprüft",
  "question.edit": "änderung vorschlagen →",

  "play.next": "nächste frage →",
  "play.category": "kategorie",
  "play.skip": "nicht jede frage passt zu jedem moment. überspring sie ruhig.",
  "play.disclaimer":
    "Originalfragen werden von Menschen geschrieben. Maschinelle Übersetzungen werden gekennzeichnet und vor der Veröffentlichung von einer sprachkundigen Person geprüft.",
  "play.save": "merken",
  "play.saved": "gemerkt",
  "play.share": "teilen",
  "play.copied": "kopiert",
  "play.empty": "in diesem deck gibt es noch keine fragen.",
  "play.suggest": "hast du eine gute frage? schlag sie vor →",

  "browse.title": "stöbern",
  "browse.search": "fragen durchsuchen…",
  "browse.alltags": "alle schlagwörter",
  "browse.sort.newest": "neueste",
  "browse.sort.random": "zufällig",
  "browse.more": "mehr anzeigen",
  "browse.empty": "Keine Frage passt. Vielleicht fehlt deine?",
  "browse.empty.cta": "schlag sie vor →",
  "browse.suggest": "fehlt etwas? schlag eine frage vor →",

  "suggest.title": "frage vorschlagen",
  "suggest.pitch": "Schreib eine Frage, die du selbst beantworten würdest.",
  "suggest.tips.eyebrow": "tipp",
  "suggest.tips.prev": "vorheriger tipp",
  "suggest.tips.next": "nächster tipp",
  "suggest.tip.build":
    "Stelle eine Frage, deren Antwort nicht schon fertig im Kopf liegt. Liegt sie fertig da, ist die Frage langweilig.",
  "suggest.tip.rankings":
    "Meide Superlative wie beste, schlimmste und meiste; sie machen aus Erinnerung eine Rangliste. Frage nur, was du selbst am Tisch beantworten würdest.",
  "suggest.tip.concrete":
    "Konkret schlägt abstrakt: frage nach einer Situation, nicht nach einer Haltung.",
  "suggest.tip.assumptions":
    "Erfinde niemandes Leben. Behalte die Annahme, die hilft, und lass die Details weg, die du nicht wissen kannst.",
  "suggest.tip.onejob":
    "Eine Frage pro Eintrag, kurz gehalten. Die Anschlussfrage stellen die Leute am Tisch.",
  "suggest.lang": "sprache",
  "suggest.question": "deine frage",
  "suggest.question.hint": "10–140 zeichen, eine frage, endet mit ?",
  "suggest.rule.short": "etwas länger: mindestens 10 zeichen",
  "suggest.rule.long": "zu lang: höchstens 140 zeichen",
  "suggest.rule.mark": "muss mit ? enden",
  "suggest.rule.multiline": "eine frage pro eintrag, in einer zeile",
  "suggest.rule.duplicate": "diese frage steht schon in der datenbank",
  "suggest.rule.duplicate.link": "die vorhandene ansehen →",
  "suggest.rule.ok": "sieht gut aus",
  "suggest.style.ranking":
    "„beste“ und „schlimmste“ verlangen eine Rangliste. Was ist dir stattdessen aufgefallen?",
  "suggest.style.stacked": "eine frage pro eintrag. Die Anschlussfrage stellen die Leute am Tisch.",
  "suggest.style.long": "unter 95 zeichen liest sich besser vor",
  "suggest.review":
    "Fragen werden vor der Aufnahme geprüft. Decks, Tiefe und Schlagwörter ordnet die Redaktion zu.",
  "suggest.name": "name für die öffentliche nennung (optional)",
  "suggest.cc0":
    "Ich übergebe diese Frage der Gemeinfreiheit (CC0). Alle dürfen sie für immer und jeden Zweck nutzen, ohne Namensnennung.",
  "suggest.cc0.link": "was CC0 bedeutet",
  "suggest.submit": "frage senden →",
  "suggest.sending": "wird gesendet…",
  "suggest.unavailable": "direkte einreichungen sind vorübergehend nicht verfügbar",
  "suggest.verification.error": "prüfung fehlgeschlagen; bitte versuch es erneut",
  "suggest.failed": "die frage konnte nicht gesendet werden; bitte versuch es erneut",
  "suggest.received": "frage erhalten",
  "suggest.received.body":
    "Eine Betreuungsperson für diese Sprache prüft sie. Wird sie angenommen, erscheint sie auf der Website und in einem zukünftigen Geräte-Update.",
  "suggest.reference": "referenz",
  "suggest.return": "zurück zu den fragen",
  "suggest.again": "noch eine vorschlagen",

  "edit.title": "änderung vorschlagen",
  "edit.pitch":
    "Schlage eine Änderung an einer Frage vor, die bereits in der Datenbank steht. Umformulieren bleibt Menschen vorbehalten: nutze deine eigenen Worte.",
  "edit.current": "die frage heute",
  "edit.notfound": "diese frage steht nicht in der datenbank",
  "edit.wording": "vorgeschlagener wortlaut (optional)",
  "edit.wording.hint": "leer lassen, wenn sich nur die metadaten ändern sollen",
  "edit.metadata": "metadaten zum überdenken (optional)",
  "edit.metadata.decks": "deck-zuordnung",
  "edit.metadata.depth": "tiefe",
  "edit.metadata.tags": "schlagwörter",
  "edit.metadata.translation": "übersetzungsherkunft oder original",
  "edit.reason": "warum sollte sich das ändern?",
  "edit.reason.hint":
    "erkläre den fehler, die verschobene bedeutung, die unnatürliche übersetzung oder das einordnungsproblem",
  "edit.reason.short": "etwas länger: mindestens 10 zeichen",
  "edit.reason.long": "zu lang: höchstens 1000 zeichen",
  "edit.nothing": "schlage einen wortlaut vor oder wähle oben mindestens ein feld",
  "edit.same": "das ist der aktuelle wortlaut",
  "edit.human":
    "Ich habe diesen Wortlaut selbst geschrieben und keine generative KI dafür verwendet.",
  "edit.cc0": "Ich gebe jeden Wortlaut, den ich hier beitrage, gemeinfrei frei (CC0-1.0).",
  "edit.review":
    "Eine Betreuungsperson für diese Sprache prüft jede Anfrage. Bestehende Fragen-IDs ändern sich nie.",
  "edit.submit": "änderung senden →",
  "edit.unavailable": "direkte änderungen sind vorübergehend nicht verfügbar",
  "edit.failed": "die änderung konnte nicht gesendet werden; bitte versuch es erneut",
  "edit.received": "änderung erhalten",
  "edit.received.body":
    "Eine Betreuungsperson für diese Sprache vergleicht sie mit dem aktuellen Wortlaut und entscheidet. Bis dahin ändert sich auf der Website nichts.",
  "edit.github": "lieber auf GitHub? formular dort öffnen →",

  "deck.title": "dein deck",
  "deck.shared.title": "ein geteiltes deck",
  "deck.empty": "Dein Deck ist leer. Merke dir Fragen mit dem Herz, während du spielst.",
  "deck.share": "deck teilen",
  "deck.share.copied": "link kopiert",
  "deck.share.truncated": "deck zu groß zum Teilen, der Link enthält die ersten 150",
  "deck.export": "exportieren",
  "deck.import": "importieren",
  "deck.saveall": "alle in mein deck übernehmen",
  "deck.saved": "in dein deck übernommen",
  "deck.count": "fragen",

  "device.title": "das gerät",
  "device.p1":
    "Ein kleines Objekt für die Tischmitte: ein E-Papier-Bildschirm, eine kleine Kategorie-Taste und eine größere Weiter-Taste. Der Bildschirm zeigt die aktive Kategorie und eine Gesprächsfrage.",
  "device.p2":
    "Kategorie und Frage bleiben im Schlaf auf dem E-Papier stehen. Es gibt keine Menüs, Modi, Benachrichtigungen oder verborgenen Sitzungszustände.",
  "device.p3":
    "Die sechs Decks heißen new people, close, family, work, here und wild. Gewöhnliche Fragen können zu mehreren Decks gehören. Wild bezeichnet düstere, gewagte oder absurde Töne und keine Tiefe.",
  "device.how": "so funktioniert es",
  "device.t.input": "eingabe",
  "device.t.action": "aktion",
  "device.t.feedback": "rückmeldung",
  "device.t.r1a": "Kategorie drücken",
  "device.t.r1b": "zur nächsten Kategorie wechseln",
  "device.t.r1c": "das E-Papier zeigt den Kategorienamen; der fünfte Druck springt zur ersten",
  "device.t.r2a": "Weiter drücken",
  "device.t.r2b": "eine andere Frage ziehen",
  "device.t.r2c": "eine E-Papier-Aktualisierung; langes Drücken verhält sich genauso",
  "device.t.r3a": "Kategorie und Weiter beim Start gedrückt halten",
  "device.t.r3b": "Service-Einrichtung öffnen",
  "device.t.r3c": "WLAN und Sprache werden am Telefon eingerichtet; kein Tischmenü",
  "device.t.r4a": "Hände weg",
  "device.t.r4b": "schlafen",
  "device.t.r4c": "Kategorie und Frage bleiben ohne Displaystrom lesbar",
  "device.build": "selbst bauen",
  "device.build.text":
    "Hardware (CERN-OHL-S) und Firmware (MIT) liegen in diesem Repository. Die Breadboard-Firmware funktioniert; Platine und Gehäuse sind Entwurfsverträge, keine Produktionsdateien.",
  "device.link.firmware": "firmware-quellen",
  "device.link.hardware": "hardware / platine",
  "device.link.guide": "bauanleitung",
  "device.link.releases": "veröffentlichungen",
  "device.sync": "synchronisierung",
  "device.sync.text":
    "Das Gerät lädt signierte Fragenpakete je Sprache, gebaut aus genau diesem Repository. Seite und Gerät nutzen dieselbe Veröffentlichung. Es lädt nur die Sprachen herunter, die du darauf behältst.",
  "device.sync.link": "wie die synchronisierung funktioniert",
  "notfound.q": "Wohin gehst du, wenn du nicht weißt, wohin du gehst?",
  "notfound.meta": "#404 · verlaufen",
  "notfound.home": "bring mich heim →",

  "footer.questions": "fragen",
  "footer.provenance": "menschliche Originale · geprüfte Übersetzungen",
  "footer.license": "alle fragen sind gemeinfrei (CC0)",
  "footer.source": "quellcode",
};

const it: Record<StringKey, string> = {
  "nav.browse": "sfoglia",
  "nav.suggest": "proponi",
  "nav.device": "dispositivo",
  "nav.skip": "vai al contenuto",

  "deck.all": "tutti",
  "deck.new_people": "new people",
  "deck.close": "close",
  "deck.family": "family",
  "deck.work": "work",
  "deck.here": "here",
  "deck.wild": "wild",

  "depth.1": "profondità 1",
  "depth.2": "profondità 2",
  "depth.3": "profondità 3",

  "tag.icebreaker": "rompighiaccio",
  "tag.reflective": "riflessiva",
  "tag.spicy": "audace",
  "tag.dark": "cupa",
  "tag.hypothetical": "e se",
  "tag.memory": "ricordo",
  "tag.wouldyourather": "preferiresti",

  "question.translated": "tradotta automaticamente · verificata da una persona",
  "question.edit": "proponi una modifica →",

  "play.next": "prossima domanda →",
  "play.category": "categoria",
  "play.skip": "non ogni domanda va bene in ogni momento. saltala pure.",
  "play.disclaimer":
    "Le domande originali le scrivono le persone. Le traduzioni automatiche sono segnalate e verificate da chi conosce la lingua prima della pubblicazione.",
  "play.save": "salva",
  "play.saved": "salvata",
  "play.share": "condividi",
  "play.copied": "copiato",
  "play.empty": "in questo mazzo non ci sono ancora domande.",
  "play.suggest": "ne hai una che vale la pena? proponi una domanda →",

  "browse.title": "sfoglia",
  "browse.search": "cerca fra le domande…",
  "browse.alltags": "tutte le etichette",
  "browse.sort.newest": "più recenti",
  "browse.sort.random": "casuale",
  "browse.more": "mostra altre",
  "browse.empty": "Nessuna domanda corrisponde. Forse manca la tua?",
  "browse.empty.cta": "proponila →",
  "browse.suggest": "manca qualcosa? proponi una domanda →",

  "suggest.title": "proponi una domanda",
  "suggest.pitch": "Scrivi una domanda a cui risponderesti anche tu.",
  "suggest.tips.eyebrow": "consiglio",
  "suggest.tips.prev": "consiglio precedente",
  "suggest.tips.next": "consiglio successivo",
  "suggest.tip.build":
    "Fai costruire una risposta invece di farne ripetere una. Se la risposta è già pronta in testa, la domanda è noiosa.",
  "suggest.tip.rankings":
    "Evita classifiche e superlativi: migliore, preferito, massimo, di più. Chiedi solo ciò a cui risponderesti tu, a quel tavolo.",
  "suggest.tip.concrete":
    "Chiedi di una cosa concreta, non di un tema intero o di un principio. Una scena dà al tavolo persone, luoghi e scelte su cui continuare.",
  "suggest.tip.assumptions":
    "Non inventare la vita di nessuno. Tieni il presupposto che serve e lascia perdere i dettagli che non puoi sapere su famiglia, lavoro, salute o passato.",
  "suggest.tip.onejob":
    "Dai alla domanda un compito solo, e tienila corta. La domanda successiva lasciala alle persone al tavolo.",
  "suggest.lang": "lingua",
  "suggest.question": "la tua domanda",
  "suggest.question.hint": "10–140 caratteri, una sola domanda, finisce con ?",
  "suggest.rule.short": "un po' più lunga: almeno 10 caratteri",
  "suggest.rule.long": "troppo lunga: massimo 140 caratteri",
  "suggest.rule.mark": "deve finire con ?",
  "suggest.rule.multiline": "una domanda per voce, su una riga sola",
  "suggest.rule.duplicate": "questa domanda è già nel database",
  "suggest.rule.duplicate.link": "leggi quella che abbiamo →",
  "suggest.rule.ok": "va bene",
  "suggest.style.ranking":
    "«migliore» e «preferito» chiedono una classifica, e trasformano un ricordo in una gara. Che cosa hai notato, invece?",
  "suggest.style.stacked":
    "una domanda per voce. La domanda successiva lasciala alle persone al tavolo.",
  "suggest.style.long": "sotto i 95 caratteri si legge meglio ad alta voce",
  "suggest.review":
    "Le domande sono esaminate prima di essere aggiunte. Chi cura la lingua assegna mazzi, profondità ed etichette.",
  "suggest.name": "nome per il credito pubblico (facoltativo)",
  "suggest.cc0":
    "Dedico questa domanda al pubblico dominio (CC0). Chiunque può usarla per qualsiasi scopo, per sempre, senza attribuzione.",
  "suggest.cc0.link": "che cosa significa CC0",
  "suggest.submit": "invia la domanda →",
  "suggest.sending": "invio in corso…",
  "suggest.unavailable": "gli invii dal sito sono temporaneamente non disponibili",
  "suggest.verification.error": "verifica non riuscita; riprova",
  "suggest.failed": "non è stato possibile inviare la domanda; riprova",
  "suggest.received": "domanda ricevuta",
  "suggest.received.body":
    "Chi cura la lingua la esaminerà. Se accettata, comparirà sul sito e in un futuro aggiornamento del dispositivo.",
  "suggest.reference": "riferimento",
  "suggest.return": "torna alle domande",
  "suggest.again": "proponine un'altra",

  "edit.title": "proponi una modifica",
  "edit.pitch":
    "Proponi un cambiamento a una domanda già nel database. La riscrittura è riservata alle persone: usa parole tue.",
  "edit.current": "la domanda oggi",
  "edit.notfound": "quella domanda non è nel database",
  "edit.wording": "formulazione proposta (facoltativa)",
  "edit.wording.hint": "lascia vuoto per una modifica ai soli metadati",
  "edit.metadata": "metadati da riconsiderare (facoltativo)",
  "edit.metadata.decks": "mazzi ammessi",
  "edit.metadata.depth": "profondità",
  "edit.metadata.tags": "etichette",
  "edit.metadata.translation": "provenienza della traduzione o origine",
  "edit.reason": "perché dovrebbe cambiare?",
  "edit.reason.hint":
    "spiega il difetto, il senso cambiato, la traduzione poco naturale o il problema di classificazione",
  "edit.reason.short": "un po' più lungo: almeno 10 caratteri",
  "edit.reason.long": "troppo lungo: massimo 1000 caratteri",
  "edit.nothing": "proponi una formulazione o spunta almeno una casella qui sopra",
  "edit.same": "è la formulazione attuale",
  "edit.human":
    "Ho scritto io questa formulazione e non ho usato IA generativa per crearla o riscriverla.",
  "edit.cc0": "Dedico al pubblico dominio (CC0-1.0) ogni formulazione che contribuisco qui.",
  "edit.review":
    "Ogni richiesta è esaminata da chi cura quella lingua. Gli ID delle domande non cambiano mai.",
  "edit.submit": "invia la modifica →",
  "edit.unavailable": "le modifiche dal sito sono temporaneamente non disponibili",
  "edit.failed": "non è stato possibile inviare la modifica; riprova",
  "edit.received": "modifica ricevuta",
  "edit.received.body":
    "Chi cura la lingua la confronterà con la formulazione attuale e deciderà. Fino ad allora sul sito non cambia nulla.",
  "edit.github": "preferisci GitHub? apri il modulo →",

  "deck.title": "il tuo mazzo",
  "deck.shared.title": "un mazzo condiviso",
  "deck.empty": "Il tuo mazzo è vuoto. Metti un cuore alle domande mentre giochi.",
  "deck.share": "condividi il mazzo",
  "deck.share.copied": "link copiato",
  "deck.share.truncated": "mazzo troppo grande da condividere, il link porta le prime 150",
  "deck.export": "esporta",
  "deck.import": "importa",
  "deck.saveall": "salva tutte nel mio mazzo",
  "deck.saved": "salvate nel tuo mazzo",
  "deck.count": "domande",

  "device.title": "il dispositivo",
  "device.p1":
    "Un oggetto tascabile per il centro del tavolo: un display e-paper, un piccolo tasto Category e un tasto Next più grande. Il display mostra la categoria attiva e una domanda.",
  "device.p2":
    "La categoria e la domanda restano sull'e-paper mentre il dispositivo dorme. Non ci sono menu, modalità, notifiche né stato di sessione nascosto.",
  "device.p3":
    "I sei mazzi sono new people, close, family, work, here e wild. Una domanda può appartenere a più mazzi. Wild è una scelta di tono per le domande cupe, audaci o assurde; non è un livello di profondità.",
  "device.how": "come funziona",
  "device.t.input": "comando",
  "device.t.action": "effetto",
  "device.t.feedback": "riscontro",
  "device.t.r1a": "premi Category",
  "device.t.r1b": "passa alla categoria successiva",
  "device.t.r1c":
    "l'e-paper mostra il nome della categoria; alla quinta pressione si torna alla prima",
  "device.t.r2a": "premi Next",
  "device.t.r2b": "estrai un'altra domanda",
  "device.t.r2c":
    "un solo aggiornamento dell'e-paper; una pressione lunga fa esattamente lo stesso",
  "device.t.r3a": "tieni premuti Category e Next all'accensione",
  "device.t.r3b": "apri la configurazione di servizio",
  "device.t.r3c": "Wi-Fi e lingua si impostano dal telefono; nessun menu sul tavolo",
  "device.t.r4a": "mani lontane",
  "device.t.r4b": "sonno",
  "device.t.r4c": "categoria e domanda restano leggibili con il display a consumo zero",
  "device.build": "costruiscine uno",
  "device.build.text":
    "L'hardware (CERN-OHL-S) e il firmware (MIT) stanno in questo repository. Il firmware su breadboard funziona; i file di PCB e scocca sono contratti di progetto, non file di produzione.",
  "device.link.firmware": "sorgenti del firmware",
  "device.link.hardware": "hardware / PCB",
  "device.link.guide": "guida alla costruzione",
  "device.link.releases": "rilasci",
  "device.sync": "sincronizzazione",
  "device.sync.text":
    "Il dispositivo scarica pacchetti di domande firmati, uno per lingua, costruiti da questo stesso repository: sito e dispositivo usano lo stesso rilascio. Scarica solo le lingue che ci tieni sopra.",
  "device.sync.link": "come funziona la sincronizzazione",
  "notfound.q": "Dove vai quando non sai dove stai andando?",
  "notfound.meta": "#404 · perso",
  "notfound.home": "riportami a casa →",

  "footer.questions": "domande",
  "footer.provenance": "originali umani · traduzioni verificate",
  "footer.license": "tutte le domande sono di pubblico dominio (CC0)",
  "footer.source": "codice sorgente",
};

export const strings: Record<Lang, Record<StringKey, string>> = { en, de, it };

export function t(lang: Lang, key: StringKey): string {
  return strings[lang]?.[key] ?? en[key];
}
