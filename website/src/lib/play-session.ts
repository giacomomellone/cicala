import { DEFAULT_PERMISSIONS, RECENT_WINDOW, type Permissions } from "../config";
import type { Bag } from "./bag";

export interface PlaySession {
  version: string;
  permissions: Permissions;
  bag: Bag;
  shown: string;
  menu: boolean;
  draft: Permissions;
  cursor: number;
}
const freshPermissions = (): Permissions => ({ ...DEFAULT_PERMISSIONS });
function permissions(value: unknown): Permissions {
  const p = value as Partial<Permissions> | null;
  return { dark: p?.dark === true, sexual: p?.sexual === true, heavy: p?.heavy === true };
}
export function getPlaySession(lang: string, version: string): PlaySession {
  const state: PlaySession = {
    version,
    permissions: freshPermissions(),
    bag: { seen: [], r: [], last: "" },
    shown: "",
    menu: false,
    draft: freshPermissions(),
    cursor: 0,
  };
  try {
    state.permissions = permissions(
      JSON.parse(sessionStorage.getItem("cicala.permissions.v1") ?? "null"),
    );
    state.draft = { ...state.permissions };
    const saved = JSON.parse(
      sessionStorage.getItem(`cicala.play.v1.${lang}`) ?? "null",
    ) as PlaySession | null;
    if (!saved || saved.version !== version) return state;
    const ids = (value: unknown): string[] =>
      Array.isArray(value) ? value.filter((id): id is string => typeof id === "string") : [];
    state.bag = {
      seen: ids(saved.bag?.seen),
      r: ids(saved.bag?.r).slice(-RECENT_WINDOW),
      last: typeof saved.bag?.last === "string" ? saved.bag.last : "",
    };
    state.shown = typeof saved.shown === "string" ? saved.shown : "";
    state.menu = saved.menu === true;
    state.cursor =
      Number.isInteger(saved.cursor) && saved.cursor >= 0 && saved.cursor <= 3 ? saved.cursor : 0;
    state.draft = state.menu ? permissions(saved.draft) : { ...state.permissions };
  } catch {
    /* A fresh state remains usable when storage is unavailable or malformed. */
  }
  return state;
}
export function setPlaySession(lang: string, state: PlaySession): void {
  try {
    sessionStorage.setItem("cicala.permissions.v1", JSON.stringify(state.permissions));
    sessionStorage.setItem(`cicala.play.v1.${lang}`, JSON.stringify(state));
  } catch {
    /* Persistence is optional. */
  }
}
