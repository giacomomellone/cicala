// All persistent client state lives in localStorage under the `tk.` namespace
// (spec §7.9): tk.lang, tk.cat, tk.favs, tk.bag.<lang>.<cat>. No cookies, no
// accounts, nothing leaves the browser.

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
    /* private mode / quota — the site still works, state just won't persist */
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

// ---------------------------------------------------------------- language

export function getStoredLang(): string | null {
  return read("tk.lang");
}

export function setStoredLang(lang: string): void {
  write("tk.lang", lang);
}

// ---------------------------------------------------------------- category

export function getCat(): string {
  return read("tk.cat") ?? "all";
}

export function setCat(cat: string): void {
  write("tk.cat", cat);
}

// --------------------------------------------------------------- favorites

export function getFavs(): string[] {
  const favs = readJson<string[]>("tk.favs", []);
  return Array.isArray(favs) ? favs.filter((x) => typeof x === "string") : [];
}

export function setFavs(ids: string[]): void {
  write("tk.favs", JSON.stringify(ids));
}

export function isFav(id: string): boolean {
  return getFavs().includes(id);
}

/** Toggle; returns true if the id is now saved. */
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

// ------------------------------------------------------------- shuffle bag

export interface Bag {
  b: string[]; // remaining ids, popped from the end
  r: string[]; // last 5 shown, excluded from the next reshuffle
}

export function getBag(lang: string, cat: string): Bag {
  const bag = readJson<Bag>(`tk.bag.${lang}.${cat}`, { b: [], r: [] });
  return {
    b: Array.isArray(bag.b) ? bag.b : [],
    r: Array.isArray(bag.r) ? bag.r : [],
  };
}

export function setBag(lang: string, cat: string, bag: Bag): void {
  write(`tk.bag.${lang}.${cat}`, JSON.stringify(bag));
}
