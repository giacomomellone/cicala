// Persistent client state uses the `kveld.` localStorage namespace.

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
  return read("kveld.lang");
}

export function setStoredLang(lang: string): void {
  write("kveld.lang", lang);
}

export function getDeck(): string {
  return read("kveld.deck") ?? "new_people";
}

export function setDeck(deck: string): void {
  write("kveld.deck", deck);
}

export function getFavs(): string[] {
  const favs = readJson<string[]>("kveld.favs", []);
  return Array.isArray(favs) ? favs.filter((x) => typeof x === "string") : [];
}

export function setFavs(ids: string[]): void {
  write("kveld.favs", JSON.stringify(ids));
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

export interface Bag {
  b: string[]; // remaining ids, popped from the end
  r: string[]; // last 5 shown, excluded from the next reshuffle
}

export function getBag(lang: string, deck: string): Bag {
  const bag = readJson<Bag>(`kveld.bag.${lang}.${deck}`, { b: [], r: [] });
  return {
    b: Array.isArray(bag.b) ? bag.b : [],
    r: Array.isArray(bag.r) ? bag.r : [],
  };
}

export function setBag(lang: string, deck: string, bag: Bag): void {
  write(`kveld.bag.${lang}.${deck}`, JSON.stringify(bag));
}
