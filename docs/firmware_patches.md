# Patching Zephyr

`deps/` is gitignored: Zephyr and its modules are cloned by `just fw-init` and
never committed. So a change to a Zephyr source file cannot simply be edited in
place — `west update` checks the pinned revision back out, and a fresh
workspace on another machine would never have it at all. Local changes live in
`firmware/patches/` as diffs, listed in `firmware/patches.yml`, and are applied
by west's own `patch` command.

Every entry is a divergence from the pinned Zephyr that somebody has to
re-check at the next version bump. Keep the list short.

## What is patched today

| Patch | Symbol it adds | Why |
|---|---|---|
| `zephyr/ssd16xx-preserve-image-on-init.patch` | `CONFIG_SSD16XX_PRESERVE_IMAGE_ON_INIT` | Keeps the panel image across a deep-sleep wake |

`ssd16xx` clears both controller RAM buffers, pulses the hardware reset and
drives a full update from its init function, unconditionally. On this device a
deep-sleep wake *is* a reset, so all three ran on every press: the panel spent
2314 ms going white before the application's own refresh began, and the
retained refresh counter described a panel state that no longer existed.

The symbol skips all three. The controller keeps power through deep sleep, so
its RAM still holds what is on the glass, and the refresh after a wake can be a
622 ms partial instead of a 2315 ms full one.

It is safe after a power cycle too, where the RAM contents *are* undefined: the
application forces a full refresh whenever the retained block did not survive,
and the driver's full-refresh path writes both RAM buffers, so a known state is
re-established before the first partial. The controller has its own power-on
reset, which is what makes skipping the pin acceptable.

Enabled in `firmware/app/sleep.conf` only — the awake image never reboots, so
it has nothing to preserve. `app/src/sleep.c` carries an `#error` if the symbol
is missing, because Kconfig only *warns* when a `.conf` assigns a symbol that
does not exist and that warning scrolls past in the middle of a build.

## Applying and dropping

```sh
just fw-patch      # apply everything in patches.yml
just fw-unpatch    # restore the pinned tree
```

`just fw-init` runs `fw-patch` for you, after `west update`. Run it by hand
after any later `west update`, which reverts the tree.

`fw-unpatch` is `git checkout .` plus `git clean -d -f -x` inside
`deps/zephyr`, so it discards *any* local edit there, not only ours. Nothing in
`deps/` is committed to this repo, so nothing unique is lost.

## Adding a patch

1. Edit the file in `deps/zephyr/` and get it working on hardware.
2. Write the diff out:

    ```sh
    git -C deps/zephyr diff -- <paths> > firmware/patches/zephyr/<name>.patch
    shasum -a 256 firmware/patches/zephyr/<name>.patch
    ```

3. Add an entry to `firmware/patches.yml` with that checksum. `module` is the
   project's **path in the workspace** — `deps/zephyr`, not `zephyr`. West
   resolves it as a path, and a patch whose module does not resolve is skipped
   in silence rather than reported, which looks exactly like a patch that
   applied cleanly.
4. Verify the mechanism rather than the working tree you already have:

    ```sh
    just fw-unpatch
    just fw-patch     # must report the patch applied
    ```

`west patch` takes its flags *before* the subcommand: `west patch -b patches
-l patches.yml apply`. Both paths are relative to the manifest repo, which is
`firmware/`.

## Upstreaming

A patch here is a liability until it is upstream or deliberately rejected.
`upstreamable: true` in `patches.yml` marks the ones that should go.

The order that works for Zephyr:

1. **Open an enhancement issue first**, before writing the pull request. Not
   required for a one-line bug fix, but this kind of change is small in code
   and large in assumption — it gives a shared driver a mode it did not have,
   and only the display maintainers can say whether it should be a Kconfig
   symbol or a devicetree property on the panel node. Finding that out costs
   one issue and saves rewriting the PR.

    Describe the case, not the patch: an e-paper device whose wake is a reset,
    a controller that keeps power across it, and a driver that blanks the
    panel every time. Bring the measurements — 2314 ms and 622 ms are the
    argument, and a maintainer will ask for them.

2. Open the pull request against `main`, linking the issue.

3. Zephyr requires a DCO sign-off on every commit (`git commit -s`), and the
   sign-off name and email must match the commit author.

4. When it merges, fill in `merge-pr`, `merge-status`, `merge-commit` and
   `merge-date` in `patches.yml`, and drop the patch at the version bump that
   first contains the commit.

## The measurement the patch invalidated

`ssd16xx_write_cmd()` waits for the BUSY pin *before* each command rather than
after, so `ssd16xx_update_display()` starts a refresh and returns. The wait
lands on whatever command comes next.

A partial refresh therefore times honestly — the driver issues a second RAM
write straight afterwards, which blocks. A full refresh does not: nothing
follows its final update, so the application measures 19 ms for a refresh the
panel spends roughly two seconds performing.

The 2315 ms recorded for a full refresh in
[hardware wiring](hardware_wiring.md) was measured before this patch, and it
was the application waiting out the driver's own init clear rather than its own
update. `CONFIG_TK_REFRESH_TIMEOUT_MS` is unaffected — 15 s clears any real
refresh by a wide margin — but the number's provenance is wrong and the true
full-refresh duration has not been measured off the BUSY pin.
