# Optional Nix dev shell. Nothing in this repo requires it: `just setup` and
# every recipe in the justfile work against system-installed tools, which is
# what CI and the macOS workflow use. This exists so that on a machine with Nix
# a single `nix develop` (or `direnv allow`, see .envrc) produces the exact
# toolchain versions CI runs, with no system packages installed.
#
# Scope: host tools only. Three things are deliberately NOT provided here, and
# should be installed the way their upstream documents:
#
#   * The Zephyr SDK. The justfile is built around the upstream tarball at
#     $ZEPHYR_SDK_INSTALL_DIR (default ~/Projects/zephyr-sdk-1.0.1): the cmake
#     registration written by `setup.sh -c`, the per-toolchain `setup.sh -t`,
#     and the qemu in its hosttools/. nixpkgs' zephyr-sdk has a different
#     layout and `just fw-doctor` cannot see it.
#   * Playwright's browsers. `just test-e2e` runs `npx playwright install
#     --with-deps chromium`, which needs to match the npm @playwright/test
#     version exactly; nixpkgs' playwright-driver pins its own and drifts.
#   * The Python packages in requirements.txt. `just setup` builds .venv with
#     pip, as documented. This shell only supplies the interpreter.
#
# The last one has one consequence worth knowing: .venv symlinks the
# interpreter in the Nix store, so a `nix flake update` that moves Python
# invalidates it. `rm -rf .venv && just setup` after such an update.
{
  description = "cicala host toolchain (optional; see README)";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { nixpkgs, ... }:
    let
      # Extend when someone works on another platform. The shell is
      # host-tool-only, so nothing in it is arch-specific.
      systems = [ "x86_64-linux" "aarch64-linux" "aarch64-darwin" "x86_64-darwin" ];
      forEach = f: nixpkgs.lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});
    in
    {
      devShells = forEach (pkgs: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            # Without this, `git` in the shell resolves to /usr/bin/git, which
            # on macOS is Apple's xcrun shim: it searches DEVELOPER_DIR, mkShell
            # points that at the Nix SDK, and there is no git there -- every git
            # command fails with `error: tool 'git' not found`. Shipping git
            # here puts a real one on PATH ahead of the shim.
            git

            just

            # `just setup` prefers python3.13 and falls back to python3; pinning
            # 3.13 here is what requirements.txt documents. CI validates on
            # 3.12, so tools/ must keep working on both -- do not reach for a
            # 3.13-only feature because this shell offers it.
            python313

            # setup-node@v4 with node-version 22 in .github/workflows/site.yml.
            #
            # One wrinkle: node 22 bundles npm 10, and website/package-lock.json
            # was written by npm >= 11, which records per-platform `libc` fields
            # npm 10 does not know. `just setup` runs `npm install`, so it
            # silently strips them -- a lockfile-format downgrade, not a
            # dependency change. CI installs with `npm ci`, which never rewrites
            # the file, so nothing breaks either way; `git restore
            # website/package-lock.json` after setup keeps the diff clean.
            nodejs_22

            # clang-format for `just fmt-fw` and `just fmt-check`.
            clang-tools

            # Zephyr host tools. dtc also ships in the SDK's hosttools, but
            # `just fw-doctor` looks for it on PATH.
            cmake
            ninja
            dtc
            gperf
          ];

          # openscad and kicad (for `just hw-check`) are not in the list: they
          # pull a large GUI closure that a website or database change has no
          # use for. Get them per-invocation instead:
          #   nix shell nixpkgs#openscad nixpkgs#kicad -c just hw-check

          # A plain mkShell, not mkShellNoCC: native_sim firmware builds and
          # Zephyr's host-side build tools compile with the host compiler, so
          # the shell needs a cc.

          shellHook = ''
            # nixpkgs ships CMake 4.x, which drops compatibility with
            # cmake_minimum_required(< 3.5) -- the failure `just fw-doctor`
            # warns about, hit by some Zephyr modules. Its own advice, applied
            # here so a firmware build does not need it discovered twice.
            export CMAKE_POLICY_VERSION_MINIMUM=''${CMAKE_POLICY_VERSION_MINIMUM:-3.5}
          '';

          # Serial-port detection is deliberately absent. A dev shell's
          # environment is computed once and then cached by nix-direnv, while
          # which /dev/tty* exists changes every time a cable moves, so
          # anything this hook works out about the board is stale by the time
          # `just fw-flash` runs. Worse, both stale answers are actively
          # harmful: Zephyr's esp32 runner reads ESPTOOL_PORT and forwards it
          # verbatim, so an empty value becomes `esptool --port ''` and a value
          # naming a port that has since disappeared becomes a hard failure --
          # in both cases instead of the probe that would have found the board.
          # Leaving ESPTOOL_PORT unset is what lets esptool auto-detect; name a
          # port explicitly when auto-detection picks the wrong one:
          #   just port=/dev/ttyUSB0 fw-flash
          #   ESPTOOL_PORT=/dev/ttyUSB0 just fw-flash
        };
      });
    };
}
