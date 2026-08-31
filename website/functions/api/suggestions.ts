import { LANGUAGES } from "../../src/config";
import {
  CC0_CONSENT_VERSION,
  type ValidSuggestion,
  validateSuggestion,
} from "../../src/lib/submission";

interface SuggestionEnv {
  GITHUB_APP_ID?: string;
  GITHUB_INSTALLATION_ID?: string;
  GITHUB_APP_PRIVATE_KEY?: string;
  GITHUB_OWNER?: string;
  GITHUB_REPO?: string;
  SUBMISSION_HOSTNAME?: string;
  TURNSTILE_SITE_KEY?: string;
  TURNSTILE_SECRET?: string;
}

interface FunctionContext {
  request: Request;
  env: SuggestionEnv;
}

interface TurnstileResult {
  success?: boolean;
  action?: string;
  hostname?: string;
}

interface GithubIssue {
  number: number;
  html_url: string;
}

interface Dependencies {
  fetch: typeof fetch;
  now: () => Date;
}

const API_VERSION = "2026-03-10";
const TURNSTILE_ACTION = "suggest-question";
const MAX_BODY_BYTES = 4096;
const DEFAULT_DEPS: Dependencies = {
  fetch: globalThis.fetch.bind(globalThis),
  now: () => new Date(),
};

function json(body: unknown, status = 200): Response {
  return Response.json(body, {
    status,
    headers: {
      "Cache-Control": "no-store",
      "Content-Type": "application/json; charset=utf-8",
    },
  });
}

function configured(env: SuggestionEnv): boolean {
  return Boolean(
    env.GITHUB_APP_ID &&
    env.GITHUB_INSTALLATION_ID &&
    env.GITHUB_APP_PRIVATE_KEY &&
    env.GITHUB_OWNER &&
    env.GITHUB_REPO &&
    env.SUBMISSION_HOSTNAME &&
    env.TURNSTILE_SITE_KEY &&
    env.TURNSTILE_SECRET,
  );
}

function bytesToBase64Url(bytes: Uint8Array): string {
  let binary = "";
  for (let i = 0; i < bytes.length; i += 0x8000)
    binary += String.fromCharCode(...bytes.subarray(i, i + 0x8000));
  return btoa(binary).replace(/=/g, "").replace(/\+/g, "-").replace(/\//g, "_");
}

function textToBase64Url(value: string): string {
  return bytesToBase64Url(new TextEncoder().encode(value));
}

function derLength(length: number): number[] {
  if (length < 0x80) return [length];
  const bytes = [];
  for (let value = length; value > 0; value >>= 8) bytes.unshift(value & 0xff);
  return [0x80 | bytes.length, ...bytes];
}

function der(tag: number, body: Uint8Array | number[]): Uint8Array {
  const bytes = body instanceof Uint8Array ? body : Uint8Array.from(body);
  return Uint8Array.from([tag, ...derLength(bytes.length), ...bytes]);
}

function pkcs1ToPkcs8(pkcs1: Uint8Array): Uint8Array {
  const version = [0x02, 0x01, 0x00];
  const rsaAlgorithm = [
    0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00,
  ];
  const privateKey = der(0x04, pkcs1);
  return der(0x30, [...version, ...rsaAlgorithm, ...privateKey]);
}

function privateKeyBytes(pem: string): Uint8Array {
  const normalized = pem.replace(/\\n/g, "\n").trim();
  const pkcs1 = normalized.includes("BEGIN RSA PRIVATE KEY");
  const base64 = normalized
    .replace(/-----BEGIN (RSA )?PRIVATE KEY-----/, "")
    .replace(/-----END (RSA )?PRIVATE KEY-----/, "")
    .replace(/\s/g, "");
  if (!base64) throw new Error("the GitHub App private key is empty or malformed");
  const bytes = Uint8Array.from(atob(base64), (char) => char.charCodeAt(0));
  return pkcs1 ? pkcs1ToPkcs8(bytes) : bytes;
}

function asArrayBuffer(bytes: Uint8Array): ArrayBuffer {
  return bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength) as ArrayBuffer;
}

export async function createGithubJwt(
  appId: string,
  privateKey: string,
  now: Date,
): Promise<string> {
  const issued = Math.floor(now.getTime() / 1000) - 60;
  const header = textToBase64Url(JSON.stringify({ alg: "RS256", typ: "JWT" }));
  const claims = textToBase64Url(JSON.stringify({ iat: issued, exp: issued + 600, iss: appId }));
  const input = `${header}.${claims}`;
  const key = await crypto.subtle.importKey(
    "pkcs8",
    asArrayBuffer(privateKeyBytes(privateKey)),
    { name: "RSASSA-PKCS1-v1_5", hash: "SHA-256" },
    false,
    ["sign"],
  );
  const signature = await crypto.subtle.sign(
    "RSASSA-PKCS1-v1_5",
    key,
    new TextEncoder().encode(input),
  );
  return `${input}.${bytesToBase64Url(new Uint8Array(signature))}`;
}

async function installationToken(env: SuggestionEnv, deps: Dependencies): Promise<string> {
  const jwt = await createGithubJwt(env.GITHUB_APP_ID!, env.GITHUB_APP_PRIVATE_KEY!, deps.now());
  const response = await deps.fetch(
    `https://api.github.com/app/installations/${env.GITHUB_INSTALLATION_ID}/access_tokens`,
    {
      method: "POST",
      headers: {
        Accept: "application/vnd.github+json",
        Authorization: `Bearer ${jwt}`,
        "User-Agent": "cicala-bot",
        "X-GitHub-Api-Version": API_VERSION,
      },
    },
  );
  if (!response.ok)
    throw new Error(`GitHub installation authentication failed (${response.status})`);
  const payload = (await response.json()) as { token?: string };
  if (!payload.token) throw new Error("GitHub returned no installation token");
  return payload.token;
}

function githubText(value: string): string {
  return value
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/@/g, "&#64;");
}

export function nativeIssueBody(
  suggestion: ValidSuggestion,
  languageName: string,
  submittedAt: Date,
): string {
  const credit = suggestion.credit ? `> ${githubText(suggestion.credit)}` : "_No response_";
  return [
    "### Language",
    "",
    `${languageName} (${suggestion.language})`,
    "",
    "### Question",
    "",
    `> ${githubText(suggestion.question)}`,
    "",
    "### Name for credit (optional)",
    "",
    credit,
    "",
    "### Public domain dedication",
    "",
    "- [x] I dedicate this question to the public domain (CC0-1.0). Anyone may use it for any purpose, without attribution.",
    "",
    "### Submission channel",
    "",
    `Submitted through cicala.dev at ${submittedAt.toISOString()}.`,
    "",
    `<!-- cicala-native:v1 submission:${suggestion.submissionId} source:${suggestion.source} consent:${CC0_CONSENT_VERSION} -->`,
  ].join("\n");
}

async function createGithubIssue(
  env: SuggestionEnv,
  suggestion: ValidSuggestion,
  deps: Dependencies,
): Promise<GithubIssue> {
  const language = LANGUAGES.find(({ code }) => code === suggestion.language)!;
  const token = await installationToken(env, deps);
  const response = await deps.fetch(
    `https://api.github.com/repos/${env.GITHUB_OWNER}/${env.GITHUB_REPO}/issues`,
    {
      method: "POST",
      headers: {
        Accept: "application/vnd.github+json",
        Authorization: `Bearer ${token}`,
        "Content-Type": "application/json",
        "User-Agent": "cicala-bot",
        "X-GitHub-Api-Version": API_VERSION,
      },
      body: JSON.stringify({
        title: `question(${suggestion.language}): ${suggestion.question.replace(/@/g, "＠")}`.slice(
          0,
          180,
        ),
        body: nativeIssueBody(suggestion, language.name, deps.now()),
        labels: ["question-submission"],
      }),
    },
  );
  if (!response.ok) throw new Error(`GitHub issue creation failed (${response.status})`);
  const issue = (await response.json()) as Partial<GithubIssue>;
  if (typeof issue.number !== "number" || typeof issue.html_url !== "string")
    throw new Error("GitHub returned an invalid issue");
  return issue as GithubIssue;
}

async function verifyTurnstile(
  env: SuggestionEnv,
  suggestion: ValidSuggestion,
  deps: Dependencies,
): Promise<boolean> {
  const form = new FormData();
  form.set("secret", env.TURNSTILE_SECRET!);
  form.set("response", suggestion.turnstileToken);
  form.set("idempotency_key", suggestion.submissionId);
  const response = await deps.fetch("https://challenges.cloudflare.com/turnstile/v0/siteverify", {
    method: "POST",
    body: form,
  });
  if (!response.ok) return false;
  const result = (await response.json()) as TurnstileResult;
  if (!result.success || result.action !== TURNSTILE_ACTION) return false;
  return result.hostname === env.SUBMISSION_HOSTNAME;
}

export async function handleSuggestionPost(
  request: Request,
  env: SuggestionEnv,
  deps: Dependencies = DEFAULT_DEPS,
): Promise<Response> {
  if (!configured(env)) return json({ error: "suggestions are not configured" }, 503);
  if (request.headers.get("Origin") !== new URL(request.url).origin)
    return json({ error: "invalid origin" }, 403);
  if (!request.headers.get("Content-Type")?.toLowerCase().startsWith("application/json"))
    return json({ error: "expected JSON" }, 415);
  const declared = Number(request.headers.get("Content-Length") ?? 0);
  if (declared > MAX_BODY_BYTES) return json({ error: "request is too large" }, 413);

  let raw: string;
  try {
    raw = await request.text();
  } catch {
    return json({ error: "invalid request" }, 400);
  }
  if (new TextEncoder().encode(raw).length > MAX_BODY_BYTES)
    return json({ error: "request is too large" }, 413);

  let payload: unknown;
  try {
    payload = JSON.parse(raw);
  } catch {
    return json({ error: "invalid JSON" }, 400);
  }
  const checked = validateSuggestion(
    payload,
    LANGUAGES.map(({ code }) => code),
  );
  if (!checked.ok) return json({ error: checked.error }, 400);
  if (checked.suggestion.website) return json({ accepted: true }, 201);

  try {
    if (!(await verifyTurnstile(env, checked.suggestion, deps)))
      return json({ error: "verification failed" }, 400);
  } catch {
    console.error("Turnstile verification service unavailable");
    return json({ error: "verification service unavailable" }, 502);
  }

  try {
    const issue = await createGithubIssue(env, checked.suggestion, deps);
    return json({ accepted: true, reference: issue.number }, 201);
  } catch (error) {
    console.error(error instanceof Error ? error.message : "suggestion submission failed");
    return json({ error: "submission service unavailable" }, 502);
  }
}

export async function onRequestGet({ env }: FunctionContext): Promise<Response> {
  const available = configured(env);
  return json({
    available,
    turnstileRequired: available,
    turnstileSiteKey: available ? env.TURNSTILE_SITE_KEY : "",
    turnstileAction: TURNSTILE_ACTION,
  });
}

export async function onRequestPost({ request, env }: FunctionContext): Promise<Response> {
  return handleSuggestionPost(request, env);
}
