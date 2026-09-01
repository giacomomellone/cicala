/* GitHub App authentication for the Pages Functions that open issues on a
   contributor's behalf. Credentials arrive from the Function environment on
   every call; nothing is cached across requests, because an installation
   token lives one hour and a Worker isolate can outlive it. */

export interface GithubAppEnv {
  GITHUB_APP_ID?: string;
  GITHUB_INSTALLATION_ID?: string;
  GITHUB_APP_PRIVATE_KEY?: string;
  GITHUB_OWNER?: string;
  GITHUB_REPO?: string;
}

export interface Dependencies {
  fetch: typeof fetch;
  now: () => Date;
}

export interface GithubIssue {
  number: number;
  html_url: string;
}

export interface IssueRequest {
  title: string;
  body: string;
  labels: string[];
}

const API_VERSION = "2026-03-10";

export const DEFAULT_DEPS: Dependencies = {
  fetch: globalThis.fetch.bind(globalThis),
  now: () => new Date(),
};

export function githubAppConfigured(env: GithubAppEnv): boolean {
  return Boolean(
    env.GITHUB_APP_ID &&
    env.GITHUB_INSTALLATION_ID &&
    env.GITHUB_APP_PRIVATE_KEY &&
    env.GITHUB_OWNER &&
    env.GITHUB_REPO,
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

/* WebCrypto imports pkcs8 only, and GitHub hands out either encoding. */
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

/* Backdated 60 s because GitHub rejects a JWT issued in its future. */
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

async function installationToken(env: GithubAppEnv, deps: Dependencies): Promise<string> {
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

/* Contributor text reaches a Markdown issue body and a notification email.
   Escaping `@` stops a stray handle from paging an unrelated person. */
export function githubText(value: string): string {
  return value
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/@/g, "&#64;");
}

export async function createIssue(
  env: GithubAppEnv,
  issue: IssueRequest,
  deps: Dependencies,
): Promise<GithubIssue> {
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
      body: JSON.stringify(issue),
    },
  );
  if (!response.ok) throw new Error(`GitHub issue creation failed (${response.status})`);
  const created = (await response.json()) as Partial<GithubIssue>;
  if (typeof created.number !== "number" || typeof created.html_url !== "string")
    throw new Error("GitHub returned an invalid issue");
  return created as GithubIssue;
}
