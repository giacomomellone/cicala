// Persistent client state uses the `cicala.` localStorage namespace.

function read(key: string): string | null {
  try {
    return localStorage.getItem(key);
  } catch {
    return null;
  }
}

function write(key: string, value: string): void {
  try {
    localStorage.setItem(key, value);
  } catch {
    /* Storage failures leave the UI usable without persistence. */
  }
}

function readJson<T>(key: string, fallback: T): T {
  const raw = read(key);
  if (raw === null) return fallback;
  try {
    return JSON.parse(raw) as T;
  } catch {
    return fallback;
  }
}

export function getStoredLang(): string | null {
  return read("cicala.lang");
}

export function setStoredLang(lang: string): void {
  write("cicala.lang", lang);
}

export function getFavs(): string[] {
  const favs = readJson<string[]>("cicala.favs", []);
  return Array.isArray(favs) ? favs.filter((x) => typeof x === "string") : [];
}

export function setFavs(ids: string[]): void {
  write("cicala.favs", JSON.stringify(ids));
}

export function isFav(id: string): boolean {
  return getFavs().includes(id);
}

/* Toggle; returns true if the id is now saved. */
export function toggleFav(id: string): boolean {
  const favs = getFavs();
  const i = favs.indexOf(id);
  if (i === -1) {
    favs.push(id);
    setFavs(favs);
    return true;
  }
  favs.splice(i, 1);
  setFavs(favs);
  return false;
}

/* How many tips the suggestion form has already shown this visitor. The count
   only ever moves forward, so a returning contributor opens on a tip they have
   not read; the arrows move within the list without touching it. */
export function getTipsSeen(): number {
  const n = Number.parseInt(read("cicala.tips") ?? "", 10);
  return Number.isInteger(n) && n >= 0 ? n : 0;
}

export function setTipsSeen(count: number): void {
  write("cicala.tips", String(count));
}
