// All UI strings, one object per shipped language (spec §7.3).

export type Lang = "en" | "de" | "it";

const en = {
  // header / nav
  "nav.browse": "browse",
  "nav.suggest": "suggest",
  "nav.device": "device",
  "nav.skip": "skip to content",

  // editorial metadata; the player does not expose a depth control
  "depth.1": "depth 1",
  "depth.2": "depth 2",
  "depth.3": "depth 3",

  // tags
  "tag.icebreaker": "icebreaker",
  "tag.reflective": "reflective",
  "tag.sexual": "sexual",
  "tag.dark": "dark",
  "tag.hypothetical": "hypothetical",
  "tag.memory": "memory",
  "tag.wouldyourather": "would you rather",

  // question provenance
  "question.translated": "automatically translated · human-reviewed",
  "question.edit": "suggest an edit →",

  // play
  "play.next": "next",
  "filter.dark": "dark",
  "filter.sexual": "sexual",
  "filter.heavy": "heavy",
  "filter.done": "done",
  "filter.allow": "allowed",
  "filter.exclude": "excluded",
  "filter.hint": "Tap a row, or use Filters: move · Next: change / done",
  "filter.scope": "For automatic play. All questions remain in Browse.",
  "filter.normal": "dark, sexual & heavy excluded",
  "play.filters": "filters",
  "browse.alldepths": "all depths",
  "play.skip": "not every question is for every moment. feel free to skip.",
  "play.disclaimer":
    "Original questions are written by people. Machine translations are marked and reviewed by a fluent editor before publication.",
  "play.save": "save",
  "play.saved": "saved",
  "play.share": "share",
  "play.copied": "copied",
  "play.empty": "no questions match these filters yet.",
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
  "suggest.review": "Questions are reviewed before they are added. Editors assign depth and tags.",
  "suggest.name": "name for public credit (optional)",
  "suggest.human": "I wrote this question myself, without generative AI.",
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
  "device.size": "core dimensions",
  "device.screen": "centered e-paper",
  "device.cell": "protected rechargeable battery",
  "device.carrier.title": "take it with you",
  "device.carrier.text":
    "An optional removable carrier adds a magnetic ring behind the device. Its portrait mounting position is sized around larger phone backs. Fit, camera clearance and retention remain to be tested; remove it for wireless charging.",
  "device.carrier.caption": "Removable carrier · 3.2 mm added thickness",
  "device.carrier.alt":
    "CAD model of the removable phone carrier and its recessed magnetic ring cover.",
  "device.title": "the device",
  "device.wip": "work in progress",
  "device.updated": "updated 15 September 2026",
  "device.progress.link": "where we are ↓",
  "device.progress.title": "where we are",
  "device.progress.text":
    "Rev B is a flatter landscape prototype, with a centered screen and two large, flat buttons beside the screen. The breadboard works; this integrated hardware still needs its first physical build.",
  "device.progress.firmware": "firmware",
  "device.progress.firmware.text":
    "Running on the breadboard. Display, charging and radio operation on the integrated Rev B hardware remain to be tested.",
  "device.progress.pcb": "PCB · Rev B",
  "device.progress.pcb.text":
    "An L-shaped, four-layer board puts the battery beside the electronics. The ESP32-S3 and power circuit sit on one face.",
  "device.progress.case": "enclosure",
  "device.progress.case.text":
    "Parametric CAD includes the shell, display supports, button fit coupons and an optional magnetic phone carrier. Printed fit and button feel need physical checks.",
  "device.progress.next":
    "Next: build the first Rev B prototypes and measure charging, display operation, sleep current, radio performance and enclosure fit.",
  "device.progress.source": "Rev B hardware notes →",
  "device.renders.title": "inside Rev B",
  "device.renders.text":
    "Views generated from the public engineering CAD. These are digital prototype models; no integrated Rev B unit has been built yet.",
  "device.controls.title": "bigger, flat buttons",
  "device.controls.text":
    "Flat 8 × 14 mm and 8 × 18 mm faces press straight down. They sit slightly above a recessed ledge; the bevel stays on the enclosure edge. Removable keepers retain the caps. Click feel and off-center presses still need prototype testing.",
  "device.controls.caption": "Button assembly · cap, captured silicone strip, keeper and screws",
  "device.controls.alt":
    "Exploded CAD view of the strengthened cap, silicone strip, removable keeper and two screws.",
  "device.pcb.title": "the routed PCB",
  "device.pcb.text":
    "Straight top and bottom views from the native KiCad board. All 89 fitted components sit on top. The silkscreen names the circuit groups, probe signals and connectors; the back includes a recovery pinout and power guide. Some component models show only their outer envelope. Open either image for the full-resolution view.",
  "device.pcb.silkscreen": "Open silkscreen drawing (SVG)",
  "device.render.pcbTop": "PCB top · ESP32-S3, power, display connector and switches",
  "device.render.pcbTop.alt":
    "Straight overhead KiCad view of the populated L-shaped Rev B PCB with Cicala branding, circuit boundaries, labelled test points and two switches on the right.",
  "device.render.pcbBottom": "PCB underside · power guide, recovery pinout and test-point key",
  "device.render.pcbBottom.alt":
    "Straight underside KiCad view of the Rev B PCB with the Cicala logo, power-flow guide, recovery pinout and labelled test contacts.",
  "device.render.assembly": "Rev B · centered screen, large flat buttons",
  "device.render.assembly.alt":
    "Landscape CAD view with a centered screen and two flat rounded buttons on the right-hand ledge.",
  "device.render.exploded": "Exploded view · shell, lens, display supports, electronics and base",
  "device.render.exploded.alt":
    "Exploded Rev B CAD showing the thin battery beside the L-shaped PCB beneath the display.",
  "device.render.inside": "Inside · battery beside the L-shaped board",
  "device.render.inside.alt":
    "CAD view of the battery bay and components on the L-shaped Rev B board.",
  "device.render.section": "Section · the vertical assembly",
  "device.render.section.alt":
    "Cross section through the Rev B enclosure, battery, circuit board and display.",
  "device.p1":
    "A pocket-sized object for the middle of the table: one e-paper display, a small Filters button, and a larger Next button. One mixed stream of questions.",
  "device.p2":
    "Dark, Sexual, and Heavy start excluded. Filters opens four rows: Dark, Sexual, Heavy, Done. Filters moves the selection; Next changes it. Done applies your choices.",
  "device.p3":
    "The question and permissions survive sleep. A fresh start after total power loss excludes all three again. During play, Next draws another permitted question.",
  "device.how": "how it works",
  "device.t.input": "input",
  "device.t.action": "action",
  "device.t.feedback": "feedback",
  "device.t.r1a": "press Filters",
  "device.t.r1b": "open Filters or move to the next row",
  "device.t.r1c": "the display highlights Dark, Sexual, Heavy, or Done",
  "device.t.r2a": "press Next",
  "device.t.r2b": "draw a question, or change the selected filter",
  "device.t.r2c": "Done applies the draft and resumes the question when allowed",
  "device.t.r3a": "hold Filters and Next during startup",
  "device.t.r3b": "open service setup",
  "device.t.r3c": "Wi-Fi and language setup open on a phone",
  "device.t.r4a": "hands off",
  "device.t.r4b": "sleep",
  "device.t.r4c": "the question or menu remains readable at zero display power",
  "device.build": "build one",
  "device.build.text":
    "Hardware (CERN-OHL-S) and firmware (MIT) are open source. Start with the breadboard guide, or explore the Rev B PCB, enclosure CAD and engineering exports. Rev B needs physical testing on its first assembled prototypes.",
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

  "depth.1": "tiefe 1",
  "depth.2": "tiefe 2",
  "depth.3": "tiefe 3",

  "tag.icebreaker": "eisbrecher",
  "tag.reflective": "nachdenklich",
  "tag.sexual": "sexuell",
  "tag.dark": "düster",
  "tag.hypothetical": "was wäre wenn",
  "tag.memory": "erinnerung",
  "tag.wouldyourather": "entweder oder",

  "question.translated": "automatisch übersetzt · von Menschen geprüft",
  "question.edit": "änderung vorschlagen →",

  "play.next": "weiter",
  "filter.dark": "düster",
  "filter.sexual": "sexuell",
  "filter.heavy": "schwer",
  "filter.done": "fertig",
  "filter.allow": "erlaubt",
  "filter.exclude": "ausgeschlossen",
  "filter.hint": "Zeile antippen, oder Filter: weiter · Weiter: ändern / fertig",
  "filter.scope": "Für das automatische Spiel. Alle Fragen bleiben beim Stöbern sichtbar.",
  "filter.normal": "düster, sexuell & schwer ausgeschlossen",
  "play.filters": "filter",
  "browse.alldepths": "alle Tiefen",
  "play.skip": "nicht jede frage passt zu jedem moment. überspring sie ruhig.",
  "play.disclaimer":
    "Originalfragen werden von Menschen geschrieben. Maschinelle Übersetzungen werden gekennzeichnet und vor der Veröffentlichung von einer sprachkundigen Person geprüft.",
  "play.save": "merken",
  "play.saved": "gemerkt",
  "play.share": "teilen",
  "play.copied": "kopiert",
  "play.empty": "noch keine Fragen mit diesen Filtern.",
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
    "Fragen werden vor der Aufnahme geprüft. Tiefe und Schlagwörter ordnet die Redaktion zu.",
  "suggest.name": "name für die öffentliche nennung (optional)",
  "suggest.human": "Ich habe diese Frage selbst geschrieben, ohne generative KI.",
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

  "device.size": "Gehäusemaße",
  "device.screen": "mittiges E-Paper",
  "device.cell": "geschützter wiederaufladbarer Akku",
  "device.carrier.title": "zum Mitnehmen",
  "device.carrier.text":
    "Eine abnehmbare Halterung ergänzt einen Magnetring auf der Rückseite. Die Position im Hochformat ist auf größere Handyrückseiten ausgelegt. Passform, Kameraabstand und Halt müssen noch geprüft werden; zum kabellosen Laden abnehmen.",
  "device.carrier.caption": "Abnehmbare Halterung · 3,2 mm zusätzliche Dicke",
  "device.carrier.alt":
    "CAD-Modell der abnehmbaren Handyhalterung mit vertiefter Magnetringabdeckung.",
  "device.title": "das gerät",
  "device.wip": "in Entwicklung",
  "device.updated": "Stand: 15. September 2026",
  "device.progress.link": "aktueller Stand ↓",
  "device.progress.title": "aktueller Stand",
  "device.progress.text":
    "Rev B ist ein flacherer Prototyp im Querformat mit mittigem Display und zwei großen, flachen Tasten neben dem Display. Der Aufbau auf dem Steckbrett funktioniert; die integrierte Hardware muss noch gebaut und erprobt werden.",
  "device.progress.firmware": "Firmware",
  "device.progress.firmware.text":
    "Läuft auf dem Steckbrett. Display, Ladefunktion und Funk müssen auf der integrierten Rev-B-Hardware noch getestet werden.",
  "device.progress.pcb": "Platine · Rev B",
  "device.progress.pcb.text":
    "Eine L-förmige Vierlagenplatine lässt neben der Elektronik Platz für den Akku. ESP32-S3 und Stromversorgung sitzen auf einer Seite.",
  "device.progress.case": "Gehäuse",
  "device.progress.case.text":
    "Das parametrische CAD enthält Gehäuse, Displayhalterungen, Tastenproben und eine optionale magnetische Handyhalterung. Passform und Tastengefühl müssen am Druck geprüft werden.",
  "device.progress.next":
    "Als Nächstes: erste Rev-B-Prototypen bauen und Laden, Display, Ruhestrom, Funk und Gehäusepassform prüfen.",
  "device.progress.source": "Rev-B-Hardwaredokumentation →",
  "device.renders.title": "im Inneren von Rev B",
  "device.renders.text":
    "Ansichten aus dem öffentlichen Konstruktions-CAD. Es sind digitale Prototypmodelle; ein integriertes Rev-B-Gerät wurde noch nicht gebaut.",
  "device.controls.title": "größere, flache Tasten",
  "device.controls.text":
    "Flache Tastenflächen von 8 × 14 und 8 × 18 mm werden senkrecht gedrückt. Sie stehen leicht über einer vertieften Auflage; die Schräge bleibt am Gehäuserand. Abnehmbare Haltebügel sichern die Kappen. Tastengefühl und außermittiges Drücken müssen am Prototyp geprüft werden.",
  "device.controls.caption":
    "Tastenbaugruppe · Kappe, eingefasster Silikonstreifen, Haltebügel und Schrauben",
  "device.controls.alt":
    "CAD-Explosionsansicht der verstärkten Tastenkappe mit Silikonstreifen, abnehmbarem Haltebügel und zwei Schrauben.",
  "device.pcb.title": "die geroutete Platine",
  "device.pcb.text":
    "Gerade Draufsichten auf beide Seiten der KiCad-Platine. Alle 89 bestückten Bauteile sitzen oben. Der Bestückungsdruck erklärt Schaltungsgruppen, Messsignale und Anschlüsse; die Rückseite zeigt die Recovery-Pinbelegung und Stromversorgung. Einige Bauteilmodelle zeigen nur die Außenkontur. Beide Bilder lassen sich in voller Auflösung öffnen.",
  "device.pcb.silkscreen": "Bestückungsdruck öffnen (SVG)",
  "device.render.pcbTop":
    "Platinenoberseite · ESP32-S3, Stromversorgung, Displayanschluss und Taster",
  "device.render.pcbTop.alt":
    "Gerade KiCad-Draufsicht der bestückten L-förmigen Rev-B-Platine mit Cicala-Logo, Schaltungsgruppen, beschrifteten Testpunkten und zwei Tastern rechts.",
  "device.render.pcbBottom": "Platinenunterseite · Stromversorgung, Recovery-Pinbelegung und Testpunktlegende",
  "device.render.pcbBottom.alt":
    "Gerade KiCad-Ansicht der Rev-B-Platinenunterseite mit Cicala-Logo, Stromversorgungsübersicht, Recovery-Pinbelegung und beschrifteten Testkontakten.",
  "device.render.assembly": "Rev B · mittiges Display, große flache Tasten",
  "device.render.assembly.alt":
    "CAD-Ansicht im Querformat mit mittigem Display und zwei flachen, abgerundeten Tasten auf der rechten Auflage.",
  "device.render.exploded":
    "Explosionsansicht · Gehäuse, Scheibe, Displayhalterungen, Elektronik und Boden",
  "device.render.exploded.alt":
    "Explosionsansicht von Rev B: flacher Akku neben der L-förmigen Platine unter dem Display.",
  "device.render.inside": "Innenansicht · Akku neben der L-förmigen Platine",
  "device.render.inside.alt":
    "CAD-Ansicht des Akkufachs und der Bauteile auf der L-förmigen Rev-B-Platine.",
  "device.render.section": "Schnitt · Aufbau in der Höhe",
  "device.render.section.alt": "Schnitt durch Rev-B-Gehäuse, Akku, Platine und Display.",
  "device.p1":
    "Ein kleines Objekt für die Tischmitte: ein E-Papier-Bildschirm, eine kleine Filter-Taste und eine größere Weiter-Taste. Ein gemischter Fragenstrom.",
  "device.p2":
    "Düster, Sexuell und Schwer sind zunächst ausgeschlossen. Filter öffnet vier Zeilen: Düster, Sexuell, Schwer, Fertig. Filter bewegt die Auswahl; Weiter ändert sie. Fertig übernimmt die Auswahl.",
  "device.p3":
    "Frage und Freigaben überstehen den Schlaf. Nach vollständigem Stromverlust sind alle drei wieder ausgeschlossen. Beim Spielen zieht Weiter eine weitere erlaubte Frage.",
  "device.how": "so funktioniert es",
  "device.t.input": "eingabe",
  "device.t.action": "aktion",
  "device.t.feedback": "rückmeldung",
  "device.t.r1a": "Filter drücken",
  "device.t.r1b": "Filter öffnen oder zur nächsten Zeile wechseln",
  "device.t.r1c": "der Bildschirm markiert die ausgewählte Zeile",
  "device.t.r2a": "Weiter drücken",
  "device.t.r2b": "Frage ziehen oder ausgewählten Filter ändern",
  "device.t.r2c": "Fertig übernimmt den Entwurf und zeigt die bisherige Frage, falls erlaubt",
  "device.t.r3a": "Filter und Weiter beim Start gedrückt halten",
  "device.t.r3b": "Service-Einrichtung öffnen",
  "device.t.r3c": "WLAN und Sprache werden am Telefon eingerichtet",
  "device.t.r4a": "Hände weg",
  "device.t.r4b": "schlafen",
  "device.t.r4c": "Frage oder Menü bleiben ohne Displaystrom lesbar",
  "device.build": "selbst bauen",
  "device.build.text":
    "Hardware (CERN-OHL-S) und Firmware (MIT) sind Open Source. Beginne mit der Breadboard-Anleitung oder sieh dir die Rev-B-Platine, das Gehäuse-CAD und die Entwicklungsdateien an. Rev B braucht noch Tests an den ersten aufgebauten Prototypen.",
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

  "depth.1": "profondità 1",
  "depth.2": "profondità 2",
  "depth.3": "profondità 3",

  "tag.icebreaker": "rompighiaccio",
  "tag.reflective": "riflessiva",
  "tag.sexual": "sessuale",
  "tag.dark": "cupa",
  "tag.hypothetical": "e se",
  "tag.memory": "ricordo",
  "tag.wouldyourather": "preferiresti",

  "question.translated": "tradotta automaticamente · verificata da una persona",
  "question.edit": "proponi una modifica →",

  "play.next": "avanti",
  "filter.dark": "cupo",
  "filter.sexual": "sessuale",
  "filter.heavy": "pesante",
  "filter.done": "fine",
  "filter.allow": "incluso",
  "filter.exclude": "escluso",
  "filter.hint": "Tocca una riga, oppure Filtri: sposta · Avanti: cambia / fine",
  "filter.scope": "Per il gioco automatico. Tutte le domande restano in Sfoglia.",
  "filter.normal": "cupo, sessuale e pesante esclusi",
  "play.filters": "filtri",
  "browse.alldepths": "tutte le profondità",
  "play.skip": "non ogni domanda va bene in ogni momento. saltala pure.",
  "play.disclaimer":
    "Le domande originali le scrivono le persone. Le traduzioni automatiche sono segnalate e verificate da chi conosce la lingua prima della pubblicazione.",
  "play.save": "salva",
  "play.saved": "salvata",
  "play.share": "condividi",
  "play.copied": "copiato",
  "play.empty": "nessuna domanda con questi filtri per ora.",
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
    "Le domande sono esaminate prima di essere aggiunte. Chi cura la lingua assegna profondità ed etichette.",
  "suggest.name": "nome per il credito pubblico (facoltativo)",
  "suggest.human": "Ho scritto io questa domanda, senza usare IA generativa.",
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

  "device.size": "dimensioni della scocca",
  "device.screen": "e-paper centrato",
  "device.cell": "batteria ricaricabile protetta",
  "device.carrier.title": "portalo con te",
  "device.carrier.text":
    "Un supporto rimovibile opzionale aggiunge un anello magnetico sul retro. La posizione verticale è dimensionata per telefoni più grandi. Compatibilità, spazio per la fotocamera e tenuta restano da verificare; rimuoverlo per la ricarica wireless.",
  "device.carrier.caption": "Supporto rimovibile · 3,2 mm di spessore aggiuntivo",
  "device.carrier.alt":
    "Modello CAD del supporto rimovibile per telefono con copertura dell’anello magnetico incassata.",
  "device.title": "il dispositivo",
  "device.wip": "lavori in corso",
  "device.updated": "aggiornato il 15 settembre 2026",
  "device.progress.link": "a che punto siamo ↓",
  "device.progress.title": "a che punto siamo",
  "device.progress.text":
    "Rev B è un prototipo più piatto, orizzontale, con schermo centrato e due grandi pulsanti piatti accanto allo schermo. Il circuito sulla breadboard funziona; questa versione integrata deve ancora essere costruita e provata.",
  "device.progress.firmware": "firmware",
  "device.progress.firmware.text":
    "Funziona sulla breadboard. Display, ricarica e radio restano da verificare sulla scheda Rev B integrata.",
  "device.progress.pcb": "PCB · Rev B",
  "device.progress.pcb.text":
    "Una scheda a L a quattro strati lascia la batteria accanto all’elettronica. ESP32-S3 e circuito di alimentazione sono su un solo lato.",
  "device.progress.case": "scocca",
  "device.progress.case.text":
    "Il CAD parametrico comprende scocca, supporti del display, provini per i pulsanti e un supporto magnetico opzionale per il telefono. Accoppiamenti e risposta dei pulsanti vanno verificati sulle stampe.",
  "device.progress.next":
    "Prossimo passo: costruire i primi prototipi Rev B e misurare ricarica, display, consumo in standby, radio e accoppiamento della scocca.",
  "device.progress.source": "Documentazione hardware Rev B →",
  "device.renders.title": "dentro Rev B",
  "device.renders.text":
    "Viste generate dal CAD tecnico pubblico. Sono modelli digitali del prototipo: nessuna unità Rev B integrata è stata ancora costruita.",
  "device.controls.title": "pulsanti più grandi e piatti",
  "device.controls.text":
    "Le superfici piatte da 8 × 14 e 8 × 18 mm si premono verticalmente. Sporgono leggermente da un piano ribassato; lo smusso resta sul bordo della scocca. I fermi rimovibili trattengono i pulsanti. Scatto e pressioni decentrate vanno provati sul prototipo.",
  "device.controls.caption":
    "Pulsante esploso · copritasto, striscia di silicone trattenuta, fermo e viti",
  "device.controls.alt":
    "Vista CAD esplosa del copritasto rinforzato con striscia di silicone, fermo rimovibile e due viti.",
  "device.pcb.title": "il PCB con le piste",
  "device.pcb.text":
    "Viste dritte dei due lati della scheda KiCad nativa. Tutti gli 89 componenti montati sono sul lato superiore. La serigrafia identifica gruppi funzionali, segnali di test e connettori; il retro include la piedinatura di ripristino e una guida all’alimentazione. Alcuni modelli mostrano solo l’ingombro esterno. Apri un’immagine per vederla a piena risoluzione.",
  "device.pcb.silkscreen": "Apri il disegno della serigrafia (SVG)",
  "device.render.pcbTop":
    "PCB, lato superiore · ESP32-S3, alimentazione, connettore display e pulsanti",
  "device.render.pcbTop.alt":
    "Vista KiCad dall’alto del PCB Rev B a L popolato, con logo Cicala, gruppi funzionali, punti di test etichettati e due pulsanti a destra.",
  "device.render.pcbBottom": "PCB, lato inferiore · alimentazione, ripristino e legenda dei test point",
  "device.render.pcbBottom.alt":
    "Vista KiCad diritta del lato inferiore del PCB Rev B con logo Cicala, guida all’alimentazione, piedinatura di ripristino e contatti di test etichettati.",
  "device.render.assembly": "Rev B · schermo centrato, grandi pulsanti piatti",
  "device.render.assembly.alt":
    "Vista CAD orizzontale con schermo centrato e due pulsanti piatti e arrotondati sul piano destro.",
  "device.render.exploded":
    "Vista esplosa · scocca, lente, supporti del display, elettronica e base",
  "device.render.exploded.alt":
    "Esploso CAD Rev B con la batteria sottile accanto al PCB a L sotto il display.",
  "device.render.inside": "Interno · batteria accanto alla scheda a L",
  "device.render.inside.alt":
    "Vista CAD del vano batteria e dei componenti sulla scheda Rev B a L.",
  "device.render.section": "Sezione · disposizione in altezza",
  "device.render.section.alt": "Sezione della scocca Rev B con batteria, scheda e display.",
  "device.p1":
    "Un oggetto tascabile per il centro del tavolo: un display e-paper, un piccolo tasto Filtri e un tasto Avanti più grande. Un unico flusso di domande mescolate.",
  "device.p2":
    "Cupo, Sessuale e Pesante partono esclusi. Filtri apre quattro righe: Cupo, Sessuale, Pesante, Fine. Filtri sposta la selezione; Avanti la modifica. Fine applica le scelte.",
  "device.p3":
    "La domanda e i permessi restano durante il sonno. Dopo una perdita totale di alimentazione, tutti e tre tornano esclusi. Durante il gioco, Avanti estrae un’altra domanda consentita.",
  "device.how": "come funziona",
  "device.t.input": "comando",
  "device.t.action": "effetto",
  "device.t.feedback": "riscontro",
  "device.t.r1a": "premi Filtri",
  "device.t.r1b": "apri Filtri o passa alla riga successiva",
  "device.t.r1c": "il display evidenzia la riga selezionata",
  "device.t.r2a": "premi Next",
  "device.t.r2b": "estrai una domanda o cambia il filtro selezionato",
  "device.t.r2c": "Fine applica la bozza e riprende la domanda, se consentita",
  "device.t.r3a": "tieni premuti Filtri e Avanti all’accensione",
  "device.t.r3b": "apri la configurazione di servizio",
  "device.t.r3c": "Wi-Fi e lingua si impostano dal telefono",
  "device.t.r4a": "mani lontane",
  "device.t.r4b": "sonno",
  "device.t.r4c": "domanda o menu restano leggibili senza alimentare il display",
  "device.build": "costruiscine uno",
  "device.build.text":
    "Hardware (CERN-OHL-S) e firmware (MIT) sono open source. Parti dalla guida per la breadboard, oppure esplora il PCB Rev B, il CAD della scocca e i file di progetto. Rev B richiede ancora prove fisiche sui primi prototipi assemblati.",
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
