import { beforeAll, describe, expect, it, vi } from "vitest";
import { editIssueBody, handleEditPost, onRequestGet } from "../functions/api/edits";
import { CC0_CONSENT_VERSION } from "../src/lib/submission";
import type { ValidEdit } from "../src/lib/edit";

let privateKey = "";

beforeAll(async () => {
  const pair = (await crypto.subtle.generateKey(
    {
      name: "RSASSA-PKCS1-v1_5",
      modulusLength: 2048,
      publicExponent: new Uint8Array([1, 0, 1]),
      hash: "SHA-256",
    },
    true,
    ["sign", "verify"],
  )) as CryptoKeyPair;
  const bytes = new Uint8Array(await crypto.subtle.exportKey("pkcs8", pair.privateKey));
  let binary = "";
  for (const byte of bytes) binary += String.fromCharCode(byte);
  privateKey = `-----BEGIN PRIVATE KEY-----\n${btoa(binary)}\n-----END PRIVATE KEY-----`;
});

const env = () => ({
  GITHUB_APP_ID: "1234",
  GITHUB_INSTALLATION_ID: "5678",
  GITHUB_APP_PRIVATE_KEY: privateKey,
  GITHUB_OWNER: "giacomomellone",
  GITHUB_REPO: "cicala",
  SUBMISSION_HOSTNAME: "cicala.dev",
  TURNSTILE_SITE_KEY: "public-key",
  TURNSTILE_SECRET: "private-key",
});

const payload = {
  questionId: "q-5a3fb181",
  language: "en",
  proposedText: "Which ordinary day would you happily live again?",
  aspects: ["depth"],
  reason: "The current wording asks for a ranking rather than a memory.",
  humanWritten: true,
  cc0: true,
  consentVersion: CC0_CONSENT_VERSION,
  submissionId: "4c527b9a-65a6-4c45-9a17-0b07620aebd0",
  turnstileToken: "verified-token",
  website: "",
};

const request = (body: unknown = payload, origin = "https://cicala.dev") => {
  const raw = JSON.stringify(body);
  return {
    url: "https://cicala.dev/api/edits",
    headers: new Headers({ "Content-Type": "application/json", Origin: origin }),
    text: async () => raw,
  } as Request;
};

const now = () => new Date("2026-09-01T12:00:00Z");

const verified = () =>
  Response.json({ success: true, action: "suggest-edit", hostname: "cicala.dev" });

describe("edit endpoint", () => {
  it("advertises its own Turnstile action", async () => {
    const response = await onRequestGet({ env: {}, request: new Request("https://cicala.dev") });
    expect(await response.json()).toEqual({
      available: false,
      turnstileRequired: false,
      turnstileSiteKey: "",
      turnstileAction: "suggest-edit",
    });
  });

  it("refuses a cross-origin post before calling another service", async () => {
    const fetcher = vi.fn();
    const response = await handleEditPost(request(payload, "https://example.com"), env(), {
      fetch: fetcher as typeof fetch,
      now,
    });
    expect(response.status).toBe(403);
    expect(fetcher).not.toHaveBeenCalled();
  });

  it("opens a question-edit issue as the bot", async () => {
    const fetcher = vi
      .fn()
      .mockResolvedValueOnce(verified())
      .mockResolvedValueOnce(Response.json({ token: "installation-token" }))
      .mockResolvedValueOnce(
        Response.json({
          number: 77,
          html_url: "https://github.com/giacomomellone/cicala/issues/77",
        }),
      );
    const response = await handleEditPost(request(), env(), {
      fetch: fetcher as typeof fetch,
      now,
    });

    expect(response.status).toBe(201);
    expect(await response.json()).toEqual({ accepted: true, reference: 77 });

    const issue = JSON.parse(String((fetcher.mock.calls[2]![1] as RequestInit).body));
    expect(issue.title).toBe("question edit: q-5a3fb181");
    expect(issue.labels).toEqual(["question-edit"]);
    expect(issue.body).toContain("### Question ID");
    expect(issue.body).toContain("<!-- cicala-native:v1 edit:");
  });

  /* A token minted on the suggestion form must not open an edit issue. */
  it("rejects a token verified for another action", async () => {
    const fetcher = vi
      .fn()
      .mockResolvedValueOnce(
        Response.json({ success: true, action: "suggest-question", hostname: "cicala.dev" }),
      );
    const response = await handleEditPost(request(), env(), {
      fetch: fetcher as typeof fetch,
      now,
    });
    expect(response.status).toBe(400);
    expect(fetcher).toHaveBeenCalledTimes(1);
  });

  it("rejects a token verified for another hostname", async () => {
    const fetcher = vi
      .fn()
      .mockResolvedValueOnce(
        Response.json({ success: true, action: "suggest-edit", hostname: "evil.example" }),
      );
    const response = await handleEditPost(request(), env(), {
      fetch: fetcher as typeof fetch,
      now,
    });
    expect(response.status).toBe(400);
  });

  it("answers a filled honeypot as success without reaching GitHub", async () => {
    const fetcher = vi.fn();
    const response = await handleEditPost(
      request({ ...payload, website: "https://spam.example" }),
      env(),
      { fetch: fetcher as typeof fetch, now },
    );
    expect(response.status).toBe(201);
    expect(await response.json()).toEqual({ accepted: true });
    expect(fetcher).not.toHaveBeenCalled();
  });

  it("fails closed when the GitHub App is not configured", async () => {
    const incomplete = env();
    incomplete.GITHUB_APP_PRIVATE_KEY = "";
    const response = await handleEditPost(request(), incomplete, {
      fetch: vi.fn() as unknown as typeof fetch,
      now,
    });
    expect(response.status).toBe(503);
  });
});

describe("editIssueBody", () => {
  const base: ValidEdit = {
    questionId: "q-5a3fb181",
    language: "en",
    proposedText: "Who made @home feel better than <away>?",
    aspects: ["decks", "tags"],
    reason: "It reads as a ranking.\n\nAnd it assumes a home.",
    submissionId: "4c527b9a-65a6-4c45-9a17-0b07620aebd0",
    turnstileToken: "token",
    website: "",
  };

  it("uses the same headings as the GitHub issue template", () => {
    const body = editIssueBody(base, "English", now());
    for (const heading of [
      "### Language",
      "### Question ID",
      "### Proposed wording (optional)",
      "### Metadata to reconsider (optional)",
      "### Why should it change?",
      "### Human Reserved confirmation",
      "### Public domain dedication",
    ])
      expect(body).toContain(heading);
  });

  it("escapes issue syntax in contributor text", () => {
    const body = editIssueBody(base, "English", now());
    expect(body).toContain("> Who made &#64;home feel better than &lt;away&gt;?");
  });

  it("ticks only the aspects that were asked for", () => {
    const body = editIssueBody(base, "English", now());
    expect(body).toContain("- [x] Deck eligibility");
    expect(body).toContain("- [ ] Depth");
    expect(body).toContain("- [x] Tags");
    expect(body).toContain("- [ ] Translation provenance or origin");
  });

  it("quotes every line of a multi-paragraph reason", () => {
    const body = editIssueBody(base, "English", now());
    expect(body).toContain("> It reads as a ranking.\n>\n> And it assumes a home.");
  });

  it("records no dedication for a metadata-only request", () => {
    const body = editIssueBody({ ...base, proposedText: "" }, "English", now());
    expect(body).toContain("_No response_");
    expect(body).toContain("changes editorial metadata only");
    expect(body).toContain("consent:none");
    expect(body).not.toContain(CC0_CONSENT_VERSION);
  });
});
