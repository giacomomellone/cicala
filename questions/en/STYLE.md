# English style guide

How to write a Cicala question in English. The bar: it fits every selected
deck, cannot be answered with yes or no, and is specific enough to start a
story. See [theory](../../docs/theory.md) for the reasoning behind these rules.

## Voice

- Address one person, informally: **you**, never "one" or "people".
- Sentence case. No exclamation marks. Exactly one question per entry.
- Plain words over clever ones. If a 12-year-old needs a dictionary, rewrite it.
- **No idioms that don't travel.** English questions are read by non-native speakers at international tables. "What's your white whale?" fails; "What have you chased for years without catching it?" works.
- No cultural furniture that assumes a country: prom, Thanksgiving, fraternities, specific TV shows. (Universal furniture is fine: weddings, birthdays, school.)

## Shape

- Aim for 10–95 characters; 140 is the hard storage limit. End with `?`.
- Prefer *when/what/who/how* openers over *do/did/are/is*; the latter invite yes/no.
- Specific beats abstract: "When did you last change your mind about something important?" beats "Are you open-minded?"
- Two-part questions ("…, and what happened?") are fine when the second half pulls out the story. Never two unrelated questions.
- Write only a question the asker could answer too.
- Avoid superlatives such as "best", "worst", "most", and "least"; they turn recall into a ranking test.
- No trailing qualifiers that do the answerer's work: "…or not?", "…if any?".

## Deck eligibility

- `new_people`: assumes no shared history. Ask for a story, choice, observation,
  or present construction without demanding a biography.
- `close`: assumes familiarity. Prefer current change and interpretation over
  archive questions the table has probably heard.
- `family`: must be safe *and interesting* for a 10-year-old. Cross-generational: a grandparent and a kid can both answer.
- `work`: avoid forced intimacy, gossip, diagnosis, and answers that could
  change someone's status at work.
- `here`: uses the current room, table, event, or visible surroundings.
- `wild`: dark, spicy, macabre, or absurd tone. These questions appear nowhere
  else.

Store a question once and select every eligible deck. Eligibility is not
ownership.

## Depth

- `1`: little public exposure.
- `2`: asks for a personal construction.
- `3`: may involve vulnerability, conflict, fear, loss, or consequential
  disclosure.

Depth is separate from tone. Normal playback currently uses 1 and 2; 3 remains
part of the corpus and browse view.

## Tags

Tag sparingly. Most questions need zero or one.

- `icebreaker`: safe with total strangers in the first five minutes.
- `reflective`: asks the answerer to look inward; expect a pause.
- `spicy`: risqué or socially daring. Requires `decks: [wild]`.
- `dark`: morbid or macabre tone. Requires `decks: [wild]`.
- `hypothetical`: imagined situations ("if…", "what would…").
- `memory`: asks for a specific remembered moment.
- `wouldyourather`: a forced choice between exactly two options.
