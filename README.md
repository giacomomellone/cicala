# tischkarte

An open-source system for conversation questions, in three parts sharing one database:

1. **[Question database](questions/)** — community-maintained YAML files in this repo. The product's core asset. Licensed [CC0](LICENSE-QUESTIONS): every question is dedicated to the public domain.
2. **[Website](website/)** — a free, static, no-account front-end to the database. Anyone can play, save favorites, and contribute questions.
3. **[Device](hardware/)** — a pocket-sized offline gadget (ESP32-S3, 2.13" e-paper, rotary knob) that shows one question at a time and syncs the database over Wi-Fi. [Firmware](firmware/) and hardware live here too.

> **Design principle:** minimize time-to-question, maximize time-in-conversation. The product succeeds when people stop looking at it.

## Parts

| Part | Directory | Status | License |
|---|---|---|---|
| Question database | [`questions/`](questions/) | ✅ English (200) + German (75) shipped | [CC0-1.0](LICENSE-QUESTIONS) |
| Website | [`website/`](website/) | ✅ v1 | [MIT](LICENSE-CODE) |
| Tools & CI | [`tools/`](tools/) | ✅ v1 | [MIT](LICENSE-CODE) |
| Firmware | [`firmware/`](firmware/) | 🚧 structure only | [MIT](LICENSE-CODE) |
| Hardware | [`hardware/`](hardware/) | 🚧 placeholders | [CERN-OHL-S-2.0](LICENSE-HARDWARE) |

## Contributing a question

You don't need to be a developer. Open a [new-question issue](../../issues/new?template=new-question.yml) (or use the form on the website's contribute page), a maintainer who speaks your language reviews it, and it ships to the site and every device. Questions are dedicated to the public domain (CC0) — see [CONTRIBUTING.md](CONTRIBUTING.md).

Want to add a whole language? Read [docs/LANGUAGES.md](docs/LANGUAGES.md) — new languages start in the incubator.

## Development

```sh
# validate the question database
python3 tools/validate.py

# build website data payloads
python3 tools/build_site_data.py

# run the website locally
cd website && npm install && npm run dev
```

Requires Python 3.11+ (`pyyaml`, `jsonschema`) and Node 20+.

## Documentation

- [docs/DESIGN.md](docs/DESIGN.md) — product and UX rationale
- [docs/DECISIONS.md](docs/DECISIONS.md) — decision log
- [docs/LANGUAGES.md](docs/LANGUAGES.md) — multilingual policy and maintainers
- [docs/SYNC-PROTOCOL.md](docs/SYNC-PROTOCOL.md) — device sync and bundle format

<!-- screenshots: add website + device photos here once available -->
