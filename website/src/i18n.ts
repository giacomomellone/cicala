// All UI strings — one object per shipped language (spec §7.3).
// Sentence case everywhere. Question text itself never passes through here;
// it lives in the per-language payloads.

export type Lang = "en" | "de";

const en = {
  // header / nav
  "nav.browse": "browse",
  "nav.contribute": "contribute",
  "nav.device": "device",
  "nav.skip": "skip to content",

  // categories (keys are language-invariant; labels translate)
  "cat.all": "all",
  "cat.party": "party",
  "cat.family": "family",
  "cat.love": "love",
  "cat.work": "work",
  "cat.deep": "deep",

  // tags
  "tag.icebreaker": "icebreaker",
  "tag.reflective": "reflective",
  "tag.spicy": "spicy",
  "tag.hypothetical": "hypothetical",
  "tag.memory": "memory",
  "tag.wouldyourather": "would you rather",

  // play
  "play.next": "next question →",
  "play.hint": "or press space",
  "play.save": "save",
  "play.saved": "saved",
  "play.share": "share",
  "play.copied": "copied",
  "play.empty": "no questions in this category yet.",

  // browse
  "browse.title": "browse",
  "browse.search": "search questions…",
  "browse.alltags": "all tags",
  "browse.sort.newest": "newest",
  "browse.sort.random": "random",
  "browse.more": "show more",
  "browse.empty": "No question matches — maybe yours is missing?",
  "browse.empty.cta": "add it",

  // contribute
  "contribute.title": "contribute",
  "contribute.pitch":
    "Good questions are written by people. Add yours to the open database — it ships to every device and this site.",
  "contribute.lang": "language",
  "contribute.lang.other": "another language…",
  "contribute.category": "category",
  "contribute.question": "your question",
  "contribute.question.hint": "10–140 characters, one question, ends with ?",
  "contribute.rule.short": "a bit longer — at least 10 characters",
  "contribute.rule.long": "too long — 140 characters max",
  "contribute.rule.mark": "must end with ?",
  "contribute.rule.ok": "looks good",
  "contribute.tags": "tags (optional)",
  "contribute.name": "name for credit (optional)",
  "contribute.cc0":
    "I dedicate this question to the public domain (CC0). Anyone may use it for any purpose, forever, without attribution — that's what lets it ship everywhere.",
  "contribute.cc0.link": "what CC0 means",
  "contribute.submit": "open the submission on GitHub →",
  "contribute.fineprint":
    "Submitting opens GitHub (free account required). A maintainer who speaks your language reviews every question.",
  "contribute.newlang":
    "New languages start in the incubator — read how to launch one:",
  "contribute.recent": "recently added",
  "contribute.recent.empty": "nothing merged yet — yours could be first.",

  // deck
  "deck.title": "your deck",
  "deck.shared.title": "a shared deck",
  "deck.empty": "Your deck is empty. Heart questions while you play.",
  "deck.share": "share deck",
  "deck.share.copied": "link copied",
  "deck.share.truncated": "deck too large to share — link carries the first 150",
  "deck.export": "export",
  "deck.import": "import",
  "deck.saveall": "save all to my deck",
  "deck.saved": "saved to your deck",
  "deck.count": "questions",

  // device page
  "device.title": "the device",
  "device.p1":
    "A pocket-sized gadget for the middle of the table: ESP32-S3, a 2.13″ e-paper display, a small OLED, and one rotary knob. It shows one conversation question at a time — and nothing else, ever.",
  "device.p2":
    "Unlike a phone, it's a shared object. The question stays on the e-paper at zero power, so the device is the table card: placing it on the table is the invitation, like producing a deck of cards. It's finite and offline — no feed, no notifications, nowhere else to be.",
  "device.p3":
    "It works fully out of the box with the preloaded database. Wi-Fi is optional forever; when you do connect it, it syncs new questions from this same open database while it charges.",
  "device.how": "how it works",
  "device.t.input": "input",
  "device.t.action": "action",
  "device.t.feedback": "feedback",
  "device.t.r1a": "turn knob",
  "device.t.r1b": "change category",
  "device.t.r1c": "OLED wakes and scrolls category names; e-paper untouched",
  "device.t.r2a": "press",
  "device.t.r2b": "next question in category",
  "device.t.r2c": "one e-paper partial refresh (~0.3 s); OLED shows #274 · deep for 3 s",
  "device.t.r3a": "long-press 1.5 s",
  "device.t.r3b": "utility menu on OLED only",
  "device.t.r3c": "sync now / Wi-Fi setup / language / battery / about; 10 s timeout",
  "device.t.r4a": "idle 30 s",
  "device.t.r4b": "deep sleep",
  "device.t.r4c": "OLED off; question remains on e-paper; µA draw",
  "device.photo": "device photo coming with rev A",
  "device.build": "build one",
  "device.build.text":
    "Hardware (CERN-OHL-S) and firmware (MIT) live in this repository. It's early: the PCB is at the block-diagram stage.",
  "device.link.firmware": "firmware sources",
  "device.link.hardware": "hardware / PCB",
  "device.link.guide": "build guide (soon)",
  "device.link.releases": "releases",
  "device.sync": "sync",
  "device.sync.text":
    "The device pulls signed per-language question bundles built from this same repository — the site and the device consume the same release. It only ever downloads the languages you keep on it.",
  "device.sync.link": "how sync works",
  "device.deckcodes": "deck codes",
  "device.deckcodes.text":
    "Move a deck of favorites from this site onto a device with a short code.",

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

  "cat.all": "alle",
  "cat.party": "feier",
  "cat.family": "familie",
  "cat.love": "liebe",
  "cat.work": "arbeit",
  "cat.deep": "tiefgang",

  "tag.icebreaker": "eisbrecher",
  "tag.reflective": "nachdenklich",
  "tag.spicy": "gewagt",
  "tag.hypothetical": "was wäre wenn",
  "tag.memory": "erinnerung",
  "tag.wouldyourather": "entweder oder",

  "play.next": "nächste frage →",
  "play.hint": "oder leertaste drücken",
  "play.save": "merken",
  "play.saved": "gemerkt",
  "play.share": "teilen",
  "play.copied": "kopiert",
  "play.empty": "in dieser kategorie gibt es noch keine fragen.",

  "browse.title": "stöbern",
  "browse.search": "fragen durchsuchen…",
  "browse.alltags": "alle schlagwörter",
  "browse.sort.newest": "neueste",
  "browse.sort.random": "zufällig",
  "browse.more": "mehr anzeigen",
  "browse.empty": "Keine Frage passt — vielleicht fehlt deine?",
  "browse.empty.cta": "füge sie hinzu",

  "contribute.title": "beitragen",
  "contribute.pitch":
    "Gute Fragen schreiben Menschen. Füge deine der offenen Datenbank hinzu — sie erscheint auf jedem Gerät und dieser Seite.",
  "contribute.lang": "sprache",
  "contribute.lang.other": "eine andere sprache…",
  "contribute.category": "kategorie",
  "contribute.question": "deine frage",
  "contribute.question.hint": "10–140 zeichen, eine frage, endet mit ?",
  "contribute.rule.short": "etwas länger — mindestens 10 zeichen",
  "contribute.rule.long": "zu lang — höchstens 140 zeichen",
  "contribute.rule.mark": "muss mit ? enden",
  "contribute.rule.ok": "sieht gut aus",
  "contribute.tags": "schlagwörter (optional)",
  "contribute.name": "name für die nennung (optional)",
  "contribute.cc0":
    "Ich übergebe diese Frage der Gemeinfreiheit (CC0). Alle dürfen sie für immer und für jeden Zweck nutzen, ohne Namensnennung — genau das lässt sie überallhin gelangen.",
  "contribute.cc0.link": "was CC0 bedeutet",
  "contribute.submit": "einreichung auf GitHub öffnen →",
  "contribute.fineprint":
    "Beim Absenden öffnet sich GitHub (kostenloses Konto nötig). Eine betreuende Person, die deine Sprache spricht, prüft jede Frage.",
  "contribute.newlang":
    "Neue Sprachen starten im Inkubator — so bringst du eine an den Start:",
  "contribute.recent": "zuletzt aufgenommen",
  "contribute.recent.empty": "noch nichts aufgenommen — deine Frage könnte die erste sein.",

  "deck.title": "dein deck",
  "deck.shared.title": "ein geteiltes deck",
  "deck.empty": "Dein Deck ist leer. Merke dir Fragen mit dem Herz, während du spielst.",
  "deck.share": "deck teilen",
  "deck.share.copied": "link kopiert",
  "deck.share.truncated": "deck zu groß zum Teilen — der Link enthält die ersten 150",
  "deck.export": "exportieren",
  "deck.import": "importieren",
  "deck.saveall": "alle in mein deck übernehmen",
  "deck.saved": "in dein deck übernommen",
  "deck.count": "fragen",

  "device.title": "das gerät",
  "device.p1":
    "Ein Gerät für die Mitte des Tisches, klein genug für die Hosentasche: ESP32-S3, ein 2,13″-E-Papier-Bildschirm, ein kleines OLED und ein Drehknopf. Es zeigt eine Gesprächsfrage auf einmal — und sonst nie etwas.",
  "device.p2":
    "Anders als ein Telefon ist es ein geteilter Gegenstand. Die Frage bleibt ohne Strom auf dem E-Papier stehen, das Gerät ist also die Tischkarte: Es auf den Tisch zu legen ist die Einladung, wie ein Kartenspiel hervorzuholen. Es ist endlich und offline — kein Strom an Neuigkeiten, keine Benachrichtigungen, kein Woanders.",
  "device.p3":
    "Es funktioniert vollständig ab Werk, mit vorinstallierter Datenbank. WLAN bleibt für immer optional; wenn du es doch verbindest, lädt es beim Aufladen neue Fragen aus genau dieser offenen Datenbank.",
  "device.how": "so funktioniert es",
  "device.t.input": "eingabe",
  "device.t.action": "aktion",
  "device.t.feedback": "rückmeldung",
  "device.t.r1a": "knopf drehen",
  "device.t.r1b": "kategorie wechseln",
  "device.t.r1c": "OLED wacht auf und blättert durch die Kategorien; E-Papier bleibt unberührt",
  "device.t.r2a": "drücken",
  "device.t.r2b": "nächste frage der kategorie",
  "device.t.r2c": "eine partielle E-Papier-Aktualisierung (~0,3 s); OLED zeigt 3 s lang #274 · tiefgang",
  "device.t.r3a": "1,5 s gedrückt halten",
  "device.t.r3b": "menü nur auf dem OLED",
  "device.t.r3c": "jetzt synchronisieren / WLAN einrichten / sprache / akku / info; 10 s zeitlimit",
  "device.t.r4a": "30 s untätig",
  "device.t.r4b": "tiefschlaf",
  "device.t.r4c": "OLED aus; die Frage bleibt auf dem E-Papier; µA-Verbrauch",
  "device.photo": "gerätefoto folgt mit rev A",
  "device.build": "selbst bauen",
  "device.build.text":
    "Hardware (CERN-OHL-S) und Firmware (MIT) liegen in diesem Repository. Es ist früh: die Platine ist im Blockdiagramm-Stadium.",
  "device.link.firmware": "firmware-quellen",
  "device.link.hardware": "hardware / platine",
  "device.link.guide": "bauanleitung (bald)",
  "device.link.releases": "veröffentlichungen",
  "device.sync": "synchronisierung",
  "device.sync.text":
    "Das Gerät lädt signierte Fragenpakete je Sprache, gebaut aus genau diesem Repository — Seite und Gerät nutzen dieselbe Veröffentlichung. Es lädt nur die Sprachen herunter, die du darauf behältst.",
  "device.sync.link": "wie die synchronisierung funktioniert",
  "device.deckcodes": "deck-codes",
  "device.deckcodes.text":
    "Bring ein Deck gemerkter Fragen mit einem kurzen Code von dieser Seite auf ein Gerät.",

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
