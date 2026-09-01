/* Server-side Turnstile verification. Rendering the widget proves nothing on
   its own: the token has to be redeemed with Siteverify, which also reports
   the action and hostname it was minted for. Both are checked, so a token
   from one form cannot be replayed against another. */

export interface TurnstileEnv {
  SUBMISSION_HOSTNAME?: string;
  TURNSTILE_SITE_KEY?: string;
  TURNSTILE_SECRET?: string;
}

interface TurnstileResult {
  success?: boolean;
  action?: string;
  hostname?: string;
}

const SITEVERIFY = "https://challenges.cloudflare.com/turnstile/v0/siteverify";

export function turnstileConfigured(env: TurnstileEnv): boolean {
  return Boolean(env.SUBMISSION_HOSTNAME && env.TURNSTILE_SITE_KEY && env.TURNSTILE_SECRET);
}

export async function verifyTurnstile(
  env: TurnstileEnv,
  token: string,
  action: string,
  deps: { fetch: typeof fetch },
): Promise<boolean> {
  const form = new FormData();
  form.set("secret", env.TURNSTILE_SECRET!);
  form.set("response", token);

  const response = await deps.fetch(SITEVERIFY, { method: "POST", body: form });
  if (!response.ok) throw new Error(`Turnstile verification failed (${response.status})`);
  const result = (await response.json()) as TurnstileResult;
  if (!result.success || result.action !== action) return false;
  return result.hostname === env.SUBMISSION_HOSTNAME;
}
