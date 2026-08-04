# Design

Product and interaction rationale. [Theory](theory.md) governs the question
corpus. When choices conflict, the principle below wins.

## The principle

**Minimize time-to-question, maximize time-in-conversation.** A feature must
shorten the path to a useful question or help the conversation after it starts.
Time spent operating the product is a cost.

## Non-goals

- No accounts, login, or profiles.
- No mobile app or Bluetooth.
- No analytics in v1 and no third-party scripts.
- No visible vote counts on the play page.
- No dark mode in website v1.
- No CMS or database server. The Git repository is the database.
- No master-language translation pipeline; see [languages.md](languages.md).
- No device menus, modes, favorites, or session progression.

## Why a device

The device is a shared object rather than personal territory. Its question
remains visible on e-paper without power. Putting it in the middle of a table
is the invitation, as with putting down a deck of cards. It has no feed,
notifications, or reason to be checked between questions.

## The complete device interaction

The top face has a small Category button, a larger primary Next button, and one
e-paper display. The active category is printed on the e-paper rather than
around a physical selector.

| Input | Action | Feedback |
|---|---|---|
| Press Category | Advance through `new people`, `close`, `family`, `work`, `here`, and `wild` | Replace the old question with the selected category name; Wild wraps to New People |
| Press Next | Draw another eligible question | One e-paper refresh |
| Hold Next | Same as a short press | One e-paper refresh; press duration has no second meaning |
| Leave it alone | Sleep | The question and active category remain readable on e-paper |
| Connect USB while holding Next | Enter service setup | Wi-Fi and language setup open on a phone; the tabletop face stays a question display |

Hard rules:

- E-paper shows the active category and either its question or a category-only
  selection screen: no logo, number, status, menu, progress, or follow-up
  nudge.
- Category advances one step in a fixed cycle. Next always draws from the
  category printed on the panel.
- The active category is retained with the other RTC state and defaults to New
  People after total state loss.
- A press discarded during an e-paper refresh changes neither the display nor
  hidden category state.
- Long press is not Favorite. The device has no way to confirm a save without
  adding status UI, and a hidden saved collection would introduce a mode.
- Wi-Fi is optional. Sync runs while charging and never interrupts use.

## Six decks

Decks are eligibility lenses, not mutually exclusive folders. One stored
question may be eligible for several category choices.

| Deck | Assumption |
|---|---|
| `new_people` | The table may have no shared history. Ask for a story, choice, observation, or present construction without testing how well people know each other |
| `close` | People already know one another. Prefer change, interpretation, and the present over basic biography they have probably heard |
| `family` | The relationship is family. Questions remain safe and answerable for a 10-year-old |
| `work` | The shared place is work. Avoid forced intimacy, gossip, diagnosis, and material that can change someone's standing |
| `here` | The room, table, event, or visible surroundings provide a third object for attention |
| `wild` | The table explicitly chose dark, spicy, macabre, or absurd tone |

`wild` is not Random and not a depth setting. Random describes a selection
algorithm and gives no warning about tone. Dark and spicy questions are
exclusive to Wild so they cannot leak into Work or Family through overlapping
membership.

## Depth without a control

Depth remains editorial metadata:

1. little public exposure;
2. a personal construction;
3. vulnerability, conflict, fear, loss, or consequential disclosure.

Tone and depth are independent. A macabre cartoon-villain question can be
depth 1; a calm question about forgiveness can be depth 3.

The first player and physical prototype draw only depths 1 and 2. Depth 3 stays
in the corpus and browse view while consent is unresolved. There is no ramp,
session counter, idle heuristic, or inferred readiness. The product cannot
observe a conversation well enough to know when to escalate.

A physical depth slider remains a testable hypothesis, not part of this
prototype. Its proposed benefit is public, low-cost boundary setting. Its
largest risk is the same public signal: moving a date or work table to “light”
can read as a judgment about the people present. The non-functional study must
show people changing such a control in front of others without prompting
before it earns a component.

## Website jobs

Navigation is play / browse / contribute / device.

1. The player: one question in dominant type, a six-choice deck control,
   Next, save, and share. It uses depths 1 and 2 and defaults to New People.
2. Browse: all questions, including depth 3, with deck and editorial metadata.
3. Contribution: one question stored once, with one or more eligible decks and
   an editorial depth.
4. Device explanation and build documentation.

Favorites remain on the website because a browser can show confirmation and
ownership without changing the physical object's interaction.

The visual language is warm paper, quiet typography, and one accent. Motion is
limited to the 120 ms question fade and selector state transition.

## Deferred work

- Anonymous submissions through a small backend.
- Anonymous “sparked a good conversation” voting; see [votes.md](votes.md).
- A token-swap dark theme.
- A depth-boundary physical study. A slider is reconsidered only if people use
  it publicly and unprompted.
