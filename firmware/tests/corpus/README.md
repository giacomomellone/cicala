# Fixture corpus

The bundles the firmware suites draw from are built from here, not from
`questions/`. The real database is editorial: it was reset to empty in #23 and
will grow question by question, and neither state should decide whether the bag,
the integration seams or the soak run are green.

| Fixture                              | Built from          | Used by                                                             |
| ------------------------------------ | ------------------- | ------------------------------------------------------------------- |
| `firmware/tests/fixtures/{lang}.qdb` | this directory      | qdb bag cases, integration, soak                                    |
| `dist/corpus/{lang}.qdb`             | `questions/{lang}/` | layout, panel, the qdb shipped-corpus guards, the application image |

Both sets are generated — `just fw-test` and the firmware workflow build them
before twister runs. The suites that guard shipped content skip themselves while
a shipped corpus is empty; the suites that exercise behaviour never do.

## Shapes the suites depend on

Editing the entries is fine. Breaking one of these is not:

- **English `new_people` and `close` hold at least twelve questions each, and no
  question sits in both.** `test_the_ring_is_shared_across_decks` alternates ten
  draws from each against a 20-deep shared ring; a deck that runs dry makes the
  bag relax the ring and hand a question back, which is what the case forbids.
- **Every English deck yields at least one question inside playback depth.**
  `test_a_cycle_never_repeats` walks all six.
- **English carries depth-3 questions.** `test_depth_three_is_out_of_normal_playback`
  checks they never draw automatically, which means nothing while there are none.
- **German `here` stays smaller than the 20-deep ring but is not empty.**
  `test_the_smallest_deck_still_draws_despite_the_ring` takes 50 draws from it
  and requires the ring to relax rather than starve the deck.
- **Tone tags (`spicy`, `dark`) appear only on `wild` questions**, the same rule
  the shipped corpora hold to.
- **Text stays inside `CONFIG_CICALA_MAX_QUESTION_BYTES`** and renders on the
  panel: the integration suite drives the real display path and asserts the
  render succeeded.

Ids follow the shipped scheme (`tools/validate.py` derives them from the text),
but nothing outside this directory refers to them.
