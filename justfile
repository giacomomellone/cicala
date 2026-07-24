# tischkarte — type `just` to see what you can do.
# One-time machine setup: `just setup`

set shell := ["bash", "-cu"]

python := ".venv/bin/python"
mkdocs := ".venv/bin/mkdocs"

[private]
default:
    @just --list --unsorted

# one-time setup: python venv (tools + docs) and website npm dependencies
setup:
    @if [ ! -d .venv ]; then (command -v python3.13 >/dev/null && python3.13 -m venv .venv) || python3 -m venv .venv; fi
    .venv/bin/pip install -q -r requirements.txt
    cd website && npm install
    @echo "ready — try: just validate · just website · just docs"

# ---------------------------------------------------------------- database

# validate the question database (schema, dedup, denylist, origin refs)
validate:
    {{python}} tools/validate.py

# assign ids/dates to new questions and normalize file formatting
fix:
    {{python}} tools/validate.py --fix

# regenerate website/src/data/*.json from the database
data:
    {{python}} tools/build_site_data.py

# build per-language device bundles + manifest (unsigned dev build)
bundle:
    {{python}} tools/build_bundle.py

# generate the ed25519 bundle-signing keypair (see docs/sync_protocol.md)
keygen:
    {{python}} tools/keygen.py

# ----------------------------------------------------------------- website

# run the website dev server (rebuilds data payloads first)
website: data
    cd website && npm run dev

# production build of the website into website/dist
website-build: data
    cd website && npm run build

# ------------------------------------------------------------------- tests

# run everything: database validation, tools tests, website tests
test: validate test-tools test-website

# tools test suite (python unittest)
test-tools:
    {{python}} -m unittest discover tools/tests

# website test suite (vitest)
test-website:
    cd website && npm test

# -------------------------------------------------------------------- docs

# live-preview the docs at http://127.0.0.1:8000
docs:
    {{mkdocs}} serve

# build the docs site into site/
docs-build:
    {{mkdocs}} build

# ---------------------------------------------------------------- firmware

# build the firmware (activates once the phase-C skeleton lands)
fw-build:
    @if [ -f firmware/CMakeLists.txt ]; then cd firmware && idf.py build; \
    else echo "firmware/ is structure-only (phase C not started) — nothing to build yet"; fi

# flash the firmware to a connected ESP32-S3 devkit
fw-flash:
    @if [ -f firmware/CMakeLists.txt ]; then cd firmware && idf.py flash monitor; \
    else echo "firmware/ is structure-only (phase C not started) — nothing to flash yet"; fi
