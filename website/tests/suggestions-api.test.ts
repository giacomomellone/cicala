import { beforeAll, describe, expect, it, vi } from "vitest";
import { generateKeyPairSync } from "node:crypto";
import { handleSuggestionPost, nativeIssueBody, onRequestGet } from "../functions/api/suggestions";
import { createGithubJwt } from "../src/lib/github-app";
import { CC0_CONSENT_VERSION, type ValidSuggestion } from "../src/lib/submission";

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
  question: "Which ordinary day would you happily live again?",
  language: "en",
  credit: "Ada",
  humanWritten: true,
  cc0: true,
  consentVersion: CC0_CONSENT_VERSION,
  submissionId: "4c527b9a-65a6-4c45-9a17-0b07620aebd0",
  source: "suggest",
  turnstileToken: "verified-token",
  website: "",
};

const request = (body: unknown = payload, origin = "https://cicala.dev") => {
  const raw = JSON.stringify(body);
  return {
    url: "https://cicala.dev/api/suggestions",
    headers: new Headers({ "Content-Type": "application/json", Origin: origin }),
    text: async () => raw,
  } as Request;
};

describe("suggestion endpoint", () => {
  it("reports unavailable without exposing missing secret names", async () => {
    const response = await onRequestGet({ env: {}, request: new Request("https://cicala.dev") });
    expect(response.status).toBe(200);
    expect(await response.json()).toEqual({
      available: false,
      turnstileRequired: false,
      turnstileSiteKey: "",
      turnstileAction: "suggest-question",
    });
  });

  it("fails closed when hostname verification is not configured", async () => {
    const incomplete = env();
    incomplete.SUBMISSION_HOSTNAME = "";
    const response = await onRequestGet({
      env: incomplete,
      request: new Request("https://cicala.dev"),
    });
    expect(await response.json()).toMatchObject({ available: false });
  });

  it("refuses an invalid origin before calling another service", async () => {
    const fetcher = vi.fn();
    const response = await handleSuggestionPost(request(payload, "https://example.com"), env(), {
      fetch: fetcher as typeof fetch,
      now: () => new Date("2026-08-31T12:00:00Z"),
    });
    expect(response.status).toBe(403);
    expect(fetcher).not.toHaveBeenCalled();
  });

  it("refuses missing Human Reserved confirmation before calling another service", async () => {
    const fetcher = vi.fn();
    const { humanWritten, ...unconfirmed } = payload;
    const response = await handleSuggestionPost(request(unconfirmed), env(), {
      fetch: fetcher as typeof fetch,
      now: () => new Date("2026-08-31T12:00:00Z"),
    });
    expect(response.status).toBe(400);
    expect(await response.json()).toEqual({ error: "Human Reserved confirmation is required" });
    expect(fetcher).not.toHaveBeenCalled();
  });

  it("verifies the person and creates a bot issue", async () => {
    const fetcher = vi
      .fn()
      .mockResolvedValueOnce(
        Response.json({ success: true, action: "suggest-question", hostname: "cicala.dev" }),
      )
      .mockResolvedValueOnce(Response.json({ token: "installation-token" }))
      .mockResolvedValueOnce(
        Response.json({
          number: 42,
          html_url: "https://github.com/giacomomellone/cicala/issues/42",
        }),
      );
    const response = await handleSuggestionPost(request(), env(), {
      fetch: fetcher as typeof fetch,
      now: () => new Date("2026-08-31T12:00:00Z"),
    });

    expect(response.status).toBe(201);
    expect(await response.json()).toEqual({ accepted: true, reference: 42 });
    expect(fetcher).toHaveBeenCalledTimes(3);

    const issueCall = fetcher.mock.calls[2]!;
    const options = issueCall[1] as RequestInit;
    const issue = JSON.parse(String(options.body));
    expect(issue.labels).toEqual(["question-submission"]);
    expect(issue.body).toContain("<!-- cicala-native:v1");
    expect(issue.body).toContain("- [x] I wrote this question myself, without generative AI.");
    expect(options.headers).toMatchObject({ Authorization: "Bearer installation-token" });
  });

  it("does not call GitHub after failed verification", async () => {
    const fetcher = vi.fn().mockResolvedValueOnce(Response.json({ success: false }));
    const response = await handleSuggestionPost(request(), env(), {
      fetch: fetcher as typeof fetch,
      now: () => new Date("2026-08-31T12:00:00Z"),
    });
    expect(response.status).toBe(400);
    expect(fetcher).toHaveBeenCalledTimes(1);
  });

  it("fails closed when the verification service is unavailable", async () => {
    const errorLog = vi.spyOn(console, "error").mockImplementation(() => undefined);
    const fetcher = vi.fn().mockRejectedValueOnce(new Error("offline"));
    try {
      const response = await handleSuggestionPost(request(), env(), {
        fetch: fetcher as typeof fetch,
        now: () => new Date("2026-08-31T12:00:00Z"),
      });
      expect(response.status).toBe(502);
      expect(await response.json()).toEqual({ error: "verification service unavailable" });
      expect(fetcher).toHaveBeenCalledTimes(1);
      expect(errorLog).toHaveBeenCalledWith("Turnstile verification service unavailable");
    } finally {
      errorLog.mockRestore();
    }
  });
});

describe("GitHub App JWT", () => {
  it("accepts the PKCS#1 PEM GitHub downloads", async () => {
    const { privateKey: key } = generateKeyPairSync("rsa", { modulusLength: 2048 });
    const pem = key.export({ type: "pkcs1", format: "pem" }).toString();
    const jwt = await createGithubJwt("1234", pem, new Date("2026-08-31T12:00:00Z"));
    expect(jwt.split(".")).toHaveLength(3);
  });
});

describe("nativeIssueBody", () => {
  it("escapes issue syntax without changing what promotion restores", () => {
    const suggestion: ValidSuggestion = {
      question: "Who made @home feel better than <away>?",
      language: "en",
      credit: "A & B",
      submissionId: payload.submissionId,
      source: "play",
      turnstileToken: "token",
      website: "",
    };
    const body = nativeIssueBody(suggestion, "English", new Date("2026-08-31T12:00:00Z"));
    expect(body).toContain("> Who made &#64;home feel better than &lt;away&gt;?");
    expect(body).toContain("> A &amp; B");
    expect(body).toContain(`consent:${CC0_CONSENT_VERSION}`);
  });
});
