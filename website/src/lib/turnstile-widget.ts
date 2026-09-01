/* Loads the Turnstile browser API on demand. The script tag is shared: two
   forms never coexist on one page today, but a second call must reuse the
   pending script rather than append another. */

export interface TurnstileApi {
  render(
    target: HTMLElement,
    options: {
      sitekey: string;
      action: string;
      appearance: "interaction-only";
      size: "flexible";
      callback: (token: string) => void;
      "expired-callback": () => void;
      "error-callback": () => void;
    },
  ): string;
  reset(widget: string): void;
}

declare global {
  interface Window {
    turnstile?: TurnstileApi;
  }
}

export function loadTurnstile(): Promise<TurnstileApi> {
  if (window.turnstile) return Promise.resolve(window.turnstile);
  return new Promise((resolve, reject) => {
    const existing = document.querySelector<HTMLScriptElement>("script[data-cicala-turnstile]");
    const script = existing ?? document.createElement("script");
    const ready = () =>
      window.turnstile ? resolve(window.turnstile) : reject(new Error("missing API"));
    script.addEventListener("load", ready, { once: true });
    script.addEventListener("error", () => reject(new Error("failed to load")), { once: true });
    if (!existing) {
      script.src = "https://challenges.cloudflare.com/turnstile/v0/api.js?render=explicit";
      script.async = true;
      script.defer = true;
      script.dataset.cicalaTurnstile = "";
      document.head.append(script);
    }
  });
}
