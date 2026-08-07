# Hosting

Two hosts, one job each, and they cannot be the same host.

| | Serves | To | Over |
|---|---|---|---|
| Website | the question browser, the docs | people, browsers | HTTPS |
| Device endpoint | `/device/manifest.json`, `/device/firmware.json` and the artifacts they name | devices | **plain HTTP** |

The split is forced by one property of the device: it has no clock, so it can
never validate a certificate, and its security comes from Ed25519 signatures
instead — see the decision log. A device therefore asks for `http://`, and
Zephyr's HTTP client does not follow redirects, so **any host that upgrades the
request is unusable**. Cloudflare Pages and GitHub Pages both upgrade. That is
the whole reason for the second host.

Everything below is done once. `site.yml` already knows how to publish to both
and no-ops on whichever secrets are missing, so you can do these in any order
and watch each half start working.

## 1. The domain

Buy it anywhere. Cloudflare Registrar sells at cost and saves a step if the
website lives on Cloudflare; any registrar works if you point its nameservers
at Cloudflare afterwards.

Two names get used, and it is worth deciding both now:

```text
tischkarte.example          the website
device.tischkarte.example   what devices fetch from
```

The second one goes into the firmware image, so pick it before you flash
anything you cannot reach with a cable. See step 5.

## 2. The website, on Cloudflare Pages

1. Cloudflare dashboard → **Workers & Pages** → **Create** → **Pages** →
   **Upload assets**. Name the project **`tischkarte`** — `site.yml` passes
   `--project-name=tischkarte`, so a different name means editing that line.
   No Git connection: CI deploys with `wrangler`.
2. **My Profile → API Tokens → Create Token → Custom token.** One permission is
   enough: *Account · Cloudflare Pages · Edit*. Scope it to your account.
3. Copy the **Account ID** from the sidebar of any dashboard page.
4. Add both as GitHub repository secrets:

   ```text
   CLOUDFLARE_API_TOKEN
   CLOUDFLARE_ACCOUNT_ID
   ```

5. Pages project → **Custom domains** → add `tischkarte.example` (and `www` if
   you want it). Cloudflare writes the DNS records itself.

Push to `main` and the `site` workflow deploys. Until the secrets exist the
deploy step is skipped and everything else still runs, which is why the site
has never deployed so far.

## 3. The device endpoint, on a bucket

What it has to do: answer plain HTTP on port 80, without redirecting, with
public read, at stable paths. An **S3 static-website endpoint** does exactly
this — website endpoints are HTTP-only by design, which is a limitation
everywhere else and the feature here.

1. **Create the bucket, named exactly `device.tischkarte.example`.** The name
   must equal the hostname, because S3 website hosting matches the `Host`
   header when you point a CNAME at it. Pick a region near your users.
2. **Permissions → Block public access → uncheck all.** It is a public
   endpoint; see the warning at the end of this page about what may live there.
3. **Permissions → Bucket policy**, allowing anonymous reads and nothing else:

   ```json
   {
     "Version": "2012-10-17",
     "Statement": [{
       "Sid": "PublicReadForDevices",
       "Effect": "Allow",
       "Principal": "*",
       "Action": "s3:GetObject",
       "Resource": "arn:aws:s3:::device.tischkarte.example/device/*"
     }]
   }
   ```

   Note the `/device/*` — reads are allowed under that prefix and nowhere else.
4. **Properties → Static website hosting → Enable.** Set any index document;
   nothing requests it. The page then shows the endpoint:

   ```text
   http://device.tischkarte.example.s3-website-eu-central-1.amazonaws.com
   ```

5. **DNS: a CNAME from `device` to that endpoint.**

   > If your DNS is on Cloudflare, this record **must be DNS-only — the grey
   > cloud, not the orange one.** A proxied record puts Cloudflare in front of
   > the bucket, which reintroduces the HTTPS upgrade this whole arrangement
   > exists to avoid, and devices go back to receiving a redirect they cannot
   > follow. This is the single easiest way to break the update path.

Anything else that serves HTTP works too — a small VPS with nginx is three
lines of config. Avoid putting a CDN in front unless you have checked it will
not upgrade the request.

## 4. Let CI write to the bucket

Create an IAM user with programmatic access and a policy limited to this one
bucket:

```json
{
  "Version": "2012-10-17",
  "Statement": [{
    "Effect": "Allow",
    "Action": ["s3:PutObject", "s3:GetObject", "s3:ListBucket"],
    "Resource": [
      "arn:aws:s3:::device.tischkarte.example",
      "arn:aws:s3:::device.tischkarte.example/*"
    ]
  }]
}
```

Then four more repository secrets:

```text
DEVICE_BUCKET          device.tischkarte.example
DEVICE_BUCKET_KEY_ID   AKIA…
DEVICE_BUCKET_SECRET   …
DEVICE_BUCKET_REGION   eu-central-1
```

`site.yml` picks these up on the next push to `main`. It uploads the newest
`db-*` and `fw-*` release into `/device/`, with two different cache lifetimes:
manifests for 60 seconds, artifacts forever. That is deliberate — a manifest
cached for an hour is an hour in which a release reaches nobody, while an image
is named after its version and never changes.

## 5. Point the firmware at it

Three defaults in `firmware/Kconfig.policy`:

```text
TK_SYNC_HOST       "device.tischkarte.example"
TK_SYNC_BASE_URL   "http://device.tischkarte.example/device"
TK_OTA_BASE_URL    "http://device.tischkarte.example/device"
```

`TK_SYNC_PORT` stays 80 and `TK_SYNC_INSECURE` stays `y`.

> **Do this before you flash anything you cannot reach with a cable.** The
> hostname is compiled into the image. A device carrying `tischkarte.invalid`
> asks a host that does not exist, forever, and the only fix is a wired
> reflash — an update cannot deliver the address of the place updates come
> from.

So the order is: pick the hostname, set it here, cut a release with it, flash
devices from *that* release, and only then does the device path work by itself.

## 6. Check it

From any machine — the point is that no client authentication is involved:

```sh
curl -sI http://device.tischkarte.example/device/firmware.json | head -1
```

`HTTP/1.1 200 OK` is what devices need. **`301` or `308` means something is
upgrading the request** — the Cloudflare proxy is the usual culprit — and the
device path is broken even though a browser sees a working URL.

```sh
curl -s http://device.tischkarte.example/device/firmware.json
curl -sI http://device.tischkarte.example/device/manifest.json | head -1
```

Then on a device, `just fw-monitor` through a cold boot:

```text
tk_sync: checking http://device.tischkarte.example/device for a newer en corpus
tk_ota:  checking http://device.tischkarte.example/device for firmware newer than 0.2.0
```

`could not resolve` is DNS. `could not connect … 116` is a timeout — usually a
device on a guest network that isolates its clients. A 301 shows up as a failed
fetch rather than as anything about redirects, because the client simply has no
handling for one.

## What may live on that bucket

It is world-readable and unencrypted in transit, so it may hold only things
that are already public: question bundles, firmware images, and the two
manifests. All of those are published artifacts of a public repository, which
is why plain HTTP costs nothing here.

Two things that must **not** go there. Private keys, obviously — neither
signing key ever leaves GitHub Actions. And the **bootloader**: `site.yml`
publishes `tischkarte-<version>.bin` but never `mcuboot-<version>.bin`, because
a bootloader is not something a device fetches, and serving it beside the image
invites somebody installing one without the other — which is how a board ends
up trusting a key its images are not signed with. It stays on the GitHub
Release, where the wired-flash procedure in
[firmware_update.md](firmware_update.md) expects it.
