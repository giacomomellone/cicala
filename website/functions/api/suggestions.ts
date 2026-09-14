import { LANGUAGES } from "../../src/config";
import {
  createIssue,
  DEFAULT_DEPS,
  type Dependencies,
  type GithubAppEnv,
  githubAppConfigured,
  githubText,
} from "../../src/lib/github-app";
import {
  CC0_CONSENT_VERSION,
  type ValidSuggestion,
  validateSuggestion,
} from "../../src/lib/submission";
import { type TurnstileEnv, turnstileConfigured, verifyTurnstile } from "../../src/lib/turnstile";

type SuggestionEnv = GithubAppEnv & TurnstileEnv;

interface FunctionContext {
  request: Request;
  env: SuggestionEnv;
}

const TURNSTILE_ACTION = "suggest-question";
const MAX_BODY_BYTES = 4096;

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
  return githubAppConfigured(env) && turnstileConfigured(env);
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
    "### Human Reserved",
    "",
    "- [x] I wrote this question myself, without generative AI.",
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
    if (!(await verifyTurnstile(env, checked.suggestion.turnstileToken, TURNSTILE_ACTION, deps)))
      return json({ error: "verification failed" }, 400);
  } catch {
    console.error("Turnstile verification service unavailable");
    return json({ error: "verification service unavailable" }, 502);
  }

  try {
    const language = LANGUAGES.find(({ code }) => code === checked.suggestion.language)!;
    const issue = await createIssue(
      env,
      {
        title: `question(${checked.suggestion.language}): ${checked.suggestion.question.replace(
          /@/g,
          "＠",
        )}`.slice(0, 180),
        body: nativeIssueBody(checked.suggestion, language.name, deps.now()),
        labels: ["question-submission"],
      },
      deps,
    );
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
