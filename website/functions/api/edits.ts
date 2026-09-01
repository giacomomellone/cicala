import { LANGUAGES } from "../../src/config";
import {
  createIssue,
  DEFAULT_DEPS,
  type Dependencies,
  type GithubAppEnv,
  githubAppConfigured,
  githubText,
} from "../../src/lib/github-app";
import { EDIT_ASPECT_LABELS, EDIT_ASPECTS, type ValidEdit, validateEdit } from "../../src/lib/edit";
import { CC0_CONSENT_VERSION } from "../../src/lib/submission";
import { type TurnstileEnv, turnstileConfigured, verifyTurnstile } from "../../src/lib/turnstile";

type EditEnv = GithubAppEnv & TurnstileEnv;

interface FunctionContext {
  request: Request;
  env: EditEnv;
}

/* Distinct from the suggestion form's action, so a token minted on one form
   is refused by the other endpoint. */
const TURNSTILE_ACTION = "suggest-edit";
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

function configured(env: EditEnv): boolean {
  return githubAppConfigured(env) && turnstileConfigured(env);
}

function quote(value: string): string {
  return value
    .split("\n")
    .map((line) => (line ? `> ${githubText(line)}` : ">"))
    .join("\n");
}

/* Section headings match .github/ISSUE_TEMPLATE/edit-question.yml so a
   maintainer, and anything that parses these issues later, reads one shape
   whichever route the request came through. */
export function editIssueBody(edit: ValidEdit, languageName: string, submittedAt: Date): string {
  const wording = edit.proposedText ? `> ${githubText(edit.proposedText)}` : "_No response_";
  const metadata = EDIT_ASPECTS.map(
    (aspect) => `- [${edit.aspects.includes(aspect) ? "x" : " "}] ${EDIT_ASPECT_LABELS[aspect]}`,
  ).join("\n");
  const consent = edit.proposedText
    ? [
        "### Human Reserved confirmation",
        "",
        "- [x] I wrote any proposed wording myself and did not use generative AI to create or rewrite the question.",
        "",
        "### Public domain dedication",
        "",
        "- [x] I dedicate any wording I contribute in this edit to the public domain (CC0-1.0).",
      ]
    : [
        "### Human Reserved confirmation",
        "",
        "_No wording contributed; this request changes editorial metadata only._",
      ];

  return [
    "### Language",
    "",
    `${languageName} (${edit.language})`,
    "",
    "### Question ID",
    "",
    edit.questionId,
    "",
    "### Proposed wording (optional)",
    "",
    wording,
    "",
    "### Metadata to reconsider (optional)",
    "",
    metadata,
    "",
    "### Why should it change?",
    "",
    quote(edit.reason),
    "",
    ...consent,
    "",
    "### Submission channel",
    "",
    `Submitted through cicala.dev at ${submittedAt.toISOString()}.`,
    "",
    `<!-- cicala-native:v1 edit:${edit.submissionId} question:${edit.questionId} consent:${
      edit.proposedText ? CC0_CONSENT_VERSION : "none"
    } -->`,
  ].join("\n");
}

export async function handleEditPost(
  request: Request,
  env: EditEnv,
  deps: Dependencies = DEFAULT_DEPS,
): Promise<Response> {
  if (!configured(env)) return json({ error: "edits are not configured" }, 503);
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

  const checked = validateEdit(
    payload,
    LANGUAGES.map(({ code }) => code),
  );
  if (!checked.ok) return json({ error: checked.error }, 400);
  /* A filled honeypot is answered as though it succeeded, so a bot learns
     nothing from the difference. */
  if (checked.edit.website) return json({ accepted: true }, 201);

  try {
    if (!(await verifyTurnstile(env, checked.edit.turnstileToken, TURNSTILE_ACTION, deps)))
      return json({ error: "verification failed" }, 400);
  } catch {
    console.error("Turnstile verification service unavailable");
    return json({ error: "verification service unavailable" }, 502);
  }

  try {
    const language = LANGUAGES.find(({ code }) => code === checked.edit.language)!;
    const issue = await createIssue(
      env,
      {
        title: `question edit: ${checked.edit.questionId}`,
        body: editIssueBody(checked.edit, language.name, deps.now()),
        labels: ["question-edit"],
      },
      deps,
    );
    return json({ accepted: true, reference: issue.number }, 201);
  } catch (error) {
    console.error(error instanceof Error ? error.message : "edit submission failed");
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
  return handleEditPost(request, env);
}
