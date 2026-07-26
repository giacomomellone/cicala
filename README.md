# tischkarte

An open-source system for conversation questions, in three parts sharing one database:

1. **[Question database](questions/)**: community-maintained YAML files in this repo. The product's core asset. Licensed [CC0](LICENSE-QUESTIONS): every question is dedicated to the public domain.
2. **[Website](website/)**: a free, static front-end to the database. No accounts. Anyone can play, save favorites, and contribute questions.
3. **[Device](hardware/)**: a pocket-sized offline object with one e-paper display, a six-position absolute deck selector, and a separate Next button. [Firmware](firmware/) remains a structure-only stub.

> **Design principle:** minimize time-to-question, maximize time-in-conversation. The product succeeds when people stop looking at it.

## Parts

| Part | Directory | Status | License |
|---|---|---|---|
| Question database | [`questions/`](questions/) | shipped: English (240), German (99) | [CC0-1.0](LICENSE-QUESTIONS) |
| Website | [`website/`](website/) | v1 | [MIT](LICENSE-CODE) |
| Tools & CI | [`tools/`](tools/) | v1 | [MIT](LICENSE-CODE) |
| Firmware | [`firmware/`](firmware/) | structure only | [MIT](LICENSE-CODE) |
| Hardware | [`hardware/`](hardware/) | placeholders | [CERN-OHL-S-2.0](LICENSE-HARDWARE) |

## Contributing a question

You don't need to be a developer. Open a [new-question issue](../../issues/new?template=new-question.yml) or use the form on the website's contribute page. A maintainer who speaks your language reviews it, and once merged it ships to the site and every device. All questions are dedicated to the public domain (CC0); see [CONTRIBUTING.md](CONTRIBUTING.md).

Want to add a whole language? Read [docs/languages.md](docs/languages.md). New languages start in the incubator.

## Development

[`just`](https://github.com/casey/just) is the entry point. Type `just` to see every command:

```sh
just setup     # one-time: python venv + npm install
just validate  # check the question database
just website   # run the site locally
just test      # database validation + tools tests + website tests
just docs      # live-preview the docs (mkdocs)
```

Requires `just`, Python 3.11+ and Node 20+. `just setup` creates the venv (`.venv/`) from [requirements.txt](requirements.txt).

## Documentation

- [docs/design.md](docs/design.md): product and UX rationale
- [docs/device_prototype.md](docs/device_prototype.md): device feasibility review, concept renders, and prototype plan
- [docs/prototype_bom.md](docs/prototype_bom.md): bench-prototype shopping list with order links (DE/EU)
- [docs/decisions.md](docs/decisions.md): decision log
- [docs/languages.md](docs/languages.md): multilingual policy and maintainers
- [docs/sync_protocol.md](docs/sync_protocol.md): device sync and bundle format

<!-- screenshots: add website + device photos here once available -->
