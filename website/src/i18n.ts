// All UI strings, one object per shipped language (spec §7.3).

export type Lang = "en" | "de";

const en = {
  // header / nav
  "nav.browse": "browse",
  "nav.contribute": "contribute",
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

  // play
  "play.next": "next question →",
  "play.category": "category",
  "play.hint": "space for next, c for category",
  "play.save": "save",
  "play.saved": "saved",
  "play.share": "share",
  "play.copied": "copied",
  "play.empty": "no questions in this deck yet.",

  // browse
  "browse.title": "browse",
  "browse.search": "search questions…",
  "browse.alltags": "all tags",
  "browse.sort.newest": "newest",
  "browse.sort.random": "random",
  "browse.more": "show more",
  "browse.empty": "No question matches. Maybe yours is missing?",
  "browse.empty.cta": "add it",

  // contribute
  "contribute.title": "contribute",
  "contribute.pitch":
    "Good questions are written by people. Add yours to the open database. It ships to every device and this site.",
  "contribute.lang": "language",
  "contribute.lang.other": "another language…",
  "contribute.decks": "decks",
  "contribute.decks.hint": "Choose every category where the question fits.",
  "contribute.decks.required": "Choose at least one deck.",
  "contribute.decks.wild": "Dark and spicy questions belong only in Wild.",
  "contribute.depth": "depth",
  "contribute.depth.1": "1 - little public exposure",
  "contribute.depth.2": "2 - a personal construction",
  "contribute.depth.3": "3 - consequential disclosure",
  "contribute.question": "your question",
  "contribute.question.hint": "10–140 characters, one question, ends with ?",
  "contribute.rule.short": "a bit longer: at least 10 characters",
  "contribute.rule.long": "too long: 140 characters max",
  "contribute.rule.mark": "must end with ?",
  "contribute.rule.multiline": "one question per entry, on one line",
  "contribute.rule.duplicate": "this question is already in the database",
  "contribute.rule.duplicate.link": "read the one we have →",
  "contribute.rule.ok": "looks good",
  "contribute.tags": "tags (optional)",
  "contribute.name": "name for credit (optional)",
  "contribute.cc0":
    "I dedicate this question to the public domain (CC0). Anyone may use it for any purpose, forever, without attribution. That's what lets it ship everywhere.",
  "contribute.cc0.link": "what CC0 means",
  "contribute.submit": "open the submission on GitHub →",
  "contribute.fineprint":
    "Submitting opens GitHub (free account required). A maintainer who speaks your language reviews every question.",
  "contribute.newlang": "New languages start in the incubator. Read how to launch one:",
  "contribute.recent": "recently added",
  "contribute.recent.empty": "nothing merged yet; yours could be first.",

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
  "footer.license": "questions are public domain (CC0)",
  "footer.source": "source",
} as const;

export type StringKey = keyof typeof en;

const de: Record<StringKey, string> = {
  "nav.browse": "stöbern",
  "nav.contribute": "beitragen",
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

  "play.next": "nächste frage →",
  "play.category": "kategorie",
  "play.hint": "leertaste für nächste, c für kategorie",
  "play.save": "merken",
  "play.saved": "gemerkt",
  "play.share": "teilen",
  "play.copied": "kopiert",
  "play.empty": "in diesem deck gibt es noch keine fragen.",

  "browse.title": "stöbern",
  "browse.search": "fragen durchsuchen…",
  "browse.alltags": "alle schlagwörter",
  "browse.sort.newest": "neueste",
  "browse.sort.random": "zufällig",
  "browse.more": "mehr anzeigen",
  "browse.empty": "Keine Frage passt. Vielleicht fehlt deine?",
  "browse.empty.cta": "füge sie hinzu",

  "contribute.title": "beitragen",
  "contribute.pitch":
    "Gute Fragen schreiben Menschen. Füge deine der offenen Datenbank hinzu, sie erscheint auf jedem Gerät und dieser Seite.",
  "contribute.lang": "sprache",
  "contribute.lang.other": "eine andere sprache…",
  "contribute.decks": "decks",
  "contribute.decks.hint": "Wähle jede Kategorie, zu der die Frage passt.",
  "contribute.decks.required": "Wähle mindestens ein Deck.",
  "contribute.decks.wild": "Düstere und gewagte Fragen gehören nur in Wild.",
  "contribute.depth": "tiefe",
  "contribute.depth.1": "1 - wenig öffentliche Preisgabe",
  "contribute.depth.2": "2 - eine persönliche Einordnung",
  "contribute.depth.3": "3 - folgenreiche Preisgabe",
  "contribute.question": "deine frage",
  "contribute.question.hint": "10–140 zeichen, eine frage, endet mit ?",
  "contribute.rule.short": "etwas länger: mindestens 10 zeichen",
  "contribute.rule.long": "zu lang: höchstens 140 zeichen",
  "contribute.rule.mark": "muss mit ? enden",
  "contribute.rule.multiline": "eine frage pro eintrag, in einer zeile",
  "contribute.rule.duplicate": "diese frage steht schon in der datenbank",
  "contribute.rule.duplicate.link": "die vorhandene ansehen →",
  "contribute.rule.ok": "sieht gut aus",
  "contribute.tags": "schlagwörter (optional)",
  "contribute.name": "name für die nennung (optional)",
  "contribute.cc0":
    "Ich übergebe diese Frage der Gemeinfreiheit (CC0). Alle dürfen sie für immer und für jeden Zweck nutzen, ohne Namensnennung. Genau das lässt sie überallhin gelangen.",
  "contribute.cc0.link": "was CC0 bedeutet",
  "contribute.submit": "einreichung auf GitHub öffnen →",
  "contribute.fineprint":
    "Beim Absenden öffnet sich GitHub (kostenloses Konto nötig). Eine betreuende Person, die deine Sprache spricht, prüft jede Frage.",
  "contribute.newlang": "Neue Sprachen starten im Inkubator. So bringst du eine an den Start:",
  "contribute.recent": "zuletzt aufgenommen",
  "contribute.recent.empty": "noch nichts aufgenommen. Deine Frage könnte die erste sein.",

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
  "footer.license": "alle fragen sind gemeinfrei (CC0)",
  "footer.source": "quellcode",
};

export const strings: Record<Lang, Record<StringKey, string>> = { en, de };

export function t(lang: Lang, key: StringKey): string {
  return strings[lang]?.[key] ?? en[key];
}
