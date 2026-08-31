# Hosting

The website and device artifacts use separate hosts:

| Host            | Serves                                    | Protocol   |
| --------------- | ----------------------------------------- | ---------- |
| Website         | question browser, submission API and docs | HTTPS      |
| Device endpoint | manifests, bundles, and firmware images   | plain HTTP |

The device has no trusted wall clock for certificate validation. It verifies
downloaded artifacts with Ed25519 signatures instead. Its HTTP client does not
follow redirects, so the device endpoint must answer HTTP directly without an
HTTPS upgrade.

## Domain names

Use one name for each host:

```text
cicala.example          website
device.cicala.example   device endpoint
```

The device hostname is compiled into firmware. Set it before flashing hardware
that cannot be reached later by cable.

## Website on Cloudflare Pages

1. Create a Cloudflare Pages project named `cicala` using **Upload assets**.
   CI deploys with Wrangler, so no Git connection is needed.
2. Create an API token with `Account · Cloudflare Pages · Edit` permission.
3. Add these GitHub repository secrets:

   ```text
   CLOUDFLARE_API_TOKEN
   CLOUDFLARE_ACCOUNT_ID
   ```

4. Add the website hostname under the Pages project's custom domains.
5. Configure the GitHub App and Turnstile runtime values from
   [native submission setup](native_submission_setup.md).

`.github/workflows/site.yml` deploys the site on pushes to `main` when both
deployment secrets exist. Astro still emits static pages. Wrangler also
deploys `website/functions/api/suggestions.ts` for `/api/suggestions`; the
checked-in `_routes.json` keeps every other request on Cloudflare's static
asset path.

The Pages project therefore has two credential boundaries:

| Location                          | Credentials                                     | Purpose                      |
| --------------------------------- | ----------------------------------------------- | ---------------------------- |
| GitHub Actions repository secrets | `CLOUDFLARE_API_TOKEN`, `CLOUDFLARE_ACCOUNT_ID` | Upload a deployment          |
| Cloudflare Pages runtime secrets  | `GITHUB_APP_PRIVATE_KEY`, `TURNSTILE_SECRET`    | Handle a question submission |

Never pass the runtime secrets through the Astro build or expose them through
the public `GET /api/suggestions` configuration response.

## Device endpoint on S3

An S3 static-website endpoint serves plain HTTP on port 80 without redirecting.

1. Create a bucket named exactly `device.cicala.example` in a suitable
   region.
2. Disable Block Public Access for the bucket.
3. Allow anonymous reads under `/device/`:

   ```json
   {
     "Version": "2012-10-17",
     "Statement": [
       {
         "Sid": "PublicReadForDevices",
         "Effect": "Allow",
         "Principal": "*",
         "Action": "s3:GetObject",
         "Resource": "arn:aws:s3:::device.cicala.example/device/*"
       }
     ]
   }
   ```

4. Enable static website hosting. The index document is unused.
5. Create a CNAME from `device` to the S3 website endpoint.

When Cloudflare manages DNS, set this CNAME to **DNS only**. The Cloudflare
proxy upgrades HTTP requests and breaks device downloads.

A different host is suitable if it provides public HTTP on port 80, stable
paths, and no redirects.

## CI credentials

Create an IAM user with programmatic access limited to the device bucket:

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": ["s3:PutObject", "s3:GetObject", "s3:ListBucket"],
      "Resource": [
        "arn:aws:s3:::device.cicala.example",
        "arn:aws:s3:::device.cicala.example/*"
      ]
    }
  ]
}
```

Add these repository secrets:

```text
DEVICE_BUCKET          device.cicala.example
DEVICE_BUCKET_KEY_ID   AKIA…
DEVICE_BUCKET_SECRET   …
DEVICE_BUCKET_REGION   eu-central-1
```

The site workflow uploads the newest database and firmware releases under
`/device/`. Manifests use a 60-second cache lifetime. Versioned artifacts are
immutable and use a long cache lifetime.

## Firmware configuration

Set these defaults in `firmware/Kconfig.policy`:

```text
CICALA_SYNC_HOST       "device.cicala.example"
CICALA_SYNC_BASE_URL   "http://device.cicala.example/device"
CICALA_OTA_BASE_URL    "http://device.cicala.example/device"
```

Keep `CICALA_SYNC_PORT=80` and `CICALA_SYNC_INSECURE=y`.

## Verification

Check that both manifests return `200` over HTTP:

```sh
curl -sI http://device.cicala.example/device/firmware.json | head -1
curl -sI http://device.cicala.example/device/manifest.json | head -1
```

A `301` or `308` response means that a proxy or host is upgrading the request.
On a device, use `just fw-monitor` during a cold boot. DNS failures appear as
`could not resolve`; connection timeouts usually indicate routing or client
isolation.

## Public bucket contents

The bucket is public and unencrypted in transit. Store only public question
bundles, firmware images, and their manifests there. Keep both private signing
keys in GitHub Actions.

The workflow does not publish MCUboot. The bootloader is paired with its firmware
signing key and belongs in the GitHub Release for wired flashing. See
[firmware updates](firmware_update.md).
