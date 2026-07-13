"""
Kelime/Ifade Sayim Botu - Tek Dosya (Birlestirilmis) Surum
==============================================================
Bu dosya, onceki kelime_sayim_core.py + kelime_sayim_botu_gui.py
dosyalarinin TEK bir .py dosyasinda birlestirilmis halidir. Artik
baska bir dosyaya ihtiyac duymaz; PyInstaller ile tek basina
.exe'ye cevrilebilir:

    pip install requests beautifulsoup4 pyinstaller
    pyinstaller --onefile --windowed --name KelimeSayimBotu kelime_sayim_botu_tek_dosya.py

Calistirmak icin:
    python kelime_sayim_botu_tek_dosya.py

Gerekli kutuphaneler: requests, beautifulsoup4
"""

from __future__ import annotations


# =============================================================================
# BOLUM 1: CEKIRDEK MANTIK (fetch, Turkce normalize, sayim)
# =============================================================================

"""
kelime_sayim_core.py
---------------------
Kelime/İfade Sayım Botu'nun ortak (paylaşılan) mantığını içerir.
Hem konsol sürümü hem de GUI sürümü bu modülü kullanır.

Bu sürümde önceki versiyonda kelimelerin (özellikle Türkçe karakter
içerenlerin) bulunamamasına yol açan iki temel sorun giderildi:

1) ENCODING SORUNU
   Bazı siteler HTTP yanıtında doğru karakter kodlamasını (charset)
   bildirmiyor. `requests`, charset belirtilmemişse varsayılan olarak
   Latin-1 (ISO-8859-1) kullanır ve bu da Türkçe karakterleri (ı, ş, ğ,
   ü, ö, ç) bozar ("yazıcı" -> "yazÄ±cÄ±" gibi). Bu sürümde sayfa,
   ham bayt (bytes) olarak alınıp BeautifulSoup'un kendi otomatik
   encoding tespitine (UnicodeDammit) bırakılıyor; bu, gerçek dünya
   sitelerinde çok daha güvenilir sonuç veriyor.

2) TÜRKÇE BÜYÜK/KÜÇÜK HARF SORUNU ("Turkish I problem")
   Python'un standart `str.lower()` / `re.IGNORECASE` mekanizması
   Türkçe'ye özgü İ/I/ı/i ayrımını bilmez. Örneğin "İmalat" kelimesinin
   standart (Türkçe olmayan) küçük harfe çevrimi "i̇malat" olur
   (noktalı "i" + görünmez birleşen nokta karakteri), "imalat" değil.
   Bu yüzden "Eklemeli İmalat" arattığınızda "eklemeli imalat" ile
   eşleşmeyebiliyordu. Bu modülde Türkçe'ye özgü bir normalize
   fonksiyonu (`tr_lower`) kullanılarak bu sorun çözülüyor.
"""

import re
import time
import logging
from dataclasses import dataclass, field
from urllib.parse import urlparse

import requests
from bs4 import BeautifulSoup

logger = logging.getLogger("kelime_sayim_botu")
logger.setLevel(logging.INFO)
if not logger.handlers:
    handler = logging.StreamHandler()
    handler.setFormatter(logging.Formatter("%(asctime)s [%(levelname)s] %(message)s", "%H:%M:%S"))
    logger.addHandler(handler)


DEFAULT_HEADERS = {
    "User-Agent": (
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
        "AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/124.0.0.0 Safari/537.36"
    ),
    "Accept-Language": "tr-TR,tr;q=0.9,en-US;q=0.8,en;q=0.7",
}

DEFAULT_TIMEOUT = 15  # saniye
DEFAULT_RETRIES = 2
RETRY_DELAY = 1.5  # saniye

# Sıkça kullanılan varsayılan terim listesi (GUI'de "varsayılanları yükle" için)
DEFAULT_TERMS = [
    "3 boyutlu yazıcı", "3b yazıcı", "3d yazıcı", "3d printer", "3b printer",
    "Eklemeli imalat", "Katmanlı imalat", "SLS", "SLA", "DLP", "FDM",
    "Ekstrüzyon", "Filament", "Metal tozu", "SLM", "DED", "WAAM",
    "Lazer sinter", "Lazer ergitme", "AR", "VR", "Sanal Gerçeklik",
    "Simülasyon", "DFAM",
]


# ---------------------------------------------------------------------------
# Türkçe'ye duyarlı normalize fonksiyonu
# ---------------------------------------------------------------------------

def tr_lower(text: str) -> str:
    """Türkçe İ/I/ı/i ayrımını doğru şekilde küçük harfe çevirir.

    Standart str.lower(), 'İ' (U+0130, noktalı büyük I) karakterini
    'i' + birleşen nokta (U+0307) olarak çevirir; bu da arama
    eşleşmelerini bozar. Burada önce İ->i ve I->ı dönüşümü elle
    yapılıp, ardından geri kalan karakterler için standart .lower()
    (ç, ş, ğ, ü, ö için doğru çalışır) uygulanır.
    """
    text = text.replace("İ", "i").replace("I", "ı")
    return text.lower()


# ---------------------------------------------------------------------------
# URL / ağ işlemleri
# ---------------------------------------------------------------------------

def normalize_url(url: str) -> str:
    """URL'nin başında http/https yoksa ekler, gereksiz boşlukları temizler."""
    url = url.strip()
    if not url:
        return url
    parsed = urlparse(url)
    if not parsed.scheme:
        url = "https://" + url
    return url


class FetchError(Exception):
    """Sayfa indirme sırasında oluşan hataları taşımak için özel istisna."""


def fetch_page_text(
    url: str,
    session: requests.Session | None = None,
    timeout: int = DEFAULT_TIMEOUT,
    retries: int = DEFAULT_RETRIES,
) -> str:
    """Sayfayı indirir ve görünür (script/style hariç) metnini döndürür.

    Encoding sorunlarını önlemek için sayfa ham bayt olarak alınır ve
    kodlama tespiti BeautifulSoup'a (UnicodeDammit) bırakılır.
    """
    sess = session or requests
    last_error: Exception | None = None

    for attempt in range(1, retries + 2):  # ilk deneme + retries
        try:
            response = sess.get(
                url, headers=DEFAULT_HEADERS, timeout=timeout, allow_redirects=True
            )
            response.raise_for_status()

            # ÖNEMLİ: response.text yerine response.content (ham bayt)
            # kullanıyoruz; BeautifulSoup kendi encoding tespitini yapıyor.
            soup = BeautifulSoup(response.content, "html.parser")

            for tag in soup(["script", "style", "noscript", "template", "svg"]):
                tag.decompose()

            text = soup.get_text(separator=" ")
            text = re.sub(r"\s+", " ", text).strip()
            return text

        except requests.exceptions.RequestException as e:
            last_error = e
            logger.warning("Deneme %d/%d başarısız (%s): %s", attempt, retries + 1, url, e)
            if attempt <= retries:
                time.sleep(RETRY_DELAY)

    raise FetchError(str(last_error))


# ---------------------------------------------------------------------------
# Sayım mantığı
# ---------------------------------------------------------------------------

def _build_pattern(term: str) -> re.Pattern:
    """Bir terim için, kelime sınırlarına duyarlı regex deseni oluşturur.

    - Tek kelimelik/kısaltma terimler (örn. 'SLA', 'AR', 'VR') için \\b
      kelime sınırı kullanılır; böylece 'AR' terimi 'ARaç' gibi
      kelimelerin bir parçası olarak yanlış eşleşmez.
    - Çok kelimeli ifadeler (örn. '3 boyutlu yazıcı') için kelimeler
      arasındaki boşluk(lar) esnek şekilde (\\s+) eşleştirilir; böylece
      satır sonu, birden fazla boşluk veya HTML etiketleri arasında
      kalan ifadeler de yakalanır.
    """
    normalized = tr_lower(term.strip())
    escaped = re.escape(normalized)
    if " " in normalized:
        pattern = re.sub(r"\\\ ", r"\\s+", escaped)
    else:
        pattern = r"\b" + escaped + r"\b"
    # Metin zaten tr_lower ile normalize edilip küçük harfe çevrildiği
    # için burada re.IGNORECASE kullanmıyoruz (Türkçe I sorunu nedeniyle
    # re.IGNORECASE güvenilir değil).
    return re.compile(pattern, flags=re.UNICODE)


def count_occurrences(text: str, terms: list[str]) -> dict[str, int]:
    """Her bir terimin metin içinde kaç kez geçtiğini sayar.

    Not: Artık ayrı bir 'case_sensitive' parametresi yoktur; Türkçe
    karakter sorunlarını doğru çözebilmek için arama her zaman
    Türkçe'ye duyarlı şekilde büyük/küçük harften bağımsız yapılır
    (tr_lower ile normalize edilerek).
    """
    normalized_text = tr_lower(text)
    results: dict[str, int] = {}

    for term in terms:
        term = term.strip()
        if not term:
            continue
        pattern = _build_pattern(term)
        matches = pattern.findall(normalized_text)
        results[term] = len(matches)

    return results


@dataclass
class ScanResult:
    url: str
    counts: dict[str, int] = field(default_factory=dict)
    error: str | None = None


def scan_url(url: str, terms: list[str], session: requests.Session | None = None) -> ScanResult:
    """Tek bir URL'yi indirir ve terim sayımlarını döndürür (hata güvenli)."""
    try:
        text = fetch_page_text(url, session=session)
        counts = count_occurrences(text, terms)
        return ScanResult(url=url, counts=counts)
    except FetchError as e:
        logger.error("Sayfa alınamadı: %s -> %s", url, e)
        return ScanResult(url=url, error=str(e))


# =============================================================================
# BOLUM 2: GUI (tkinter arayuzu)
# =============================================================================

"""
Kelime/İfade Sayım Botu - GUI Sürümü (tkinter)
--------------------------------------------------
Verilen site URL'lerine gider, sayfa içeriğini indirir ve belirtilen
kelime/ifadelerin kaç kere geçtiğini gösteren masaüstü uygulaması.

Çalıştırmak için:
    python kelime_sayim_botu_gui.py

Gerekli kütüphaneler: requests, beautifulsoup4
    pip install requests beautifulsoup4

Not: Bu dosya, ortak mantığı içeren kelime_sayim_core.py dosyasına
ihtiyaç duyar; ikisini aynı klasörde tutun (.exe'ye çevirirken de
PyInstaller ikisini otomatik birlikte paketler, ayrıca bir şey
yapmanıza gerek yok).
"""

import json
import csv
import threading

import tkinter as tk
from tkinter import ttk, messagebox, filedialog

import requests


class WordCounterApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Kelime/İfade Sayım Botu")
        self.root.geometry("880x640")
        self.root.minsize(720, 520)

        self.last_results = {}  # {url: {term: count}} veya {url: {"hata": ...}}
        self.session = requests.Session()

        self._build_widgets()

    # ---------------- Arayüz kurulumu ----------------
    def _build_widgets(self):
        main = ttk.Frame(self.root, padding=12)
        main.pack(fill="both", expand=True)

        ttk.Label(main, text="Site URL'leri (her satıra bir tane):").pack(anchor="w")
        self.url_text = tk.Text(main, height=6, wrap="none")
        self.url_text.pack(fill="x", pady=(2, 10))

        terms_header = ttk.Frame(main)
        terms_header.pack(fill="x")
        ttk.Label(terms_header, text="Aranacak kelime/ifadeler (her satıra bir tane):").pack(side="left")
        ttk.Button(
            terms_header, text="Varsayılan Terimleri Yükle", command=self._load_default_terms
        ).pack(side="right")

        self.terms_text = tk.Text(main, height=10, wrap="none")
        self.terms_text.pack(fill="x", pady=(2, 10))

        options_frame = ttk.Frame(main)
        options_frame.pack(fill="x", pady=(0, 10))

        self.run_button = ttk.Button(options_frame, text="Taramayı Başlat", command=self.start_scan)
        self.run_button.pack(side="right")

        self.export_button = ttk.Button(
            options_frame, text="Sonuçları Dışa Aktar (CSV/JSON)", command=self.export_results,
            state="disabled",
        )
        self.export_button.pack(side="right", padx=(0, 8))

        self.status_var = tk.StringVar(value="Hazır.")
        ttk.Label(main, textvariable=self.status_var, foreground="#555").pack(anchor="w", pady=(0, 6))
        self.progress = ttk.Progressbar(main, mode="determinate")
        self.progress.pack(fill="x", pady=(0, 10))

        ttk.Label(main, text="Sonuçlar:").pack(anchor="w")
        table_frame = ttk.Frame(main)
        table_frame.pack(fill="both", expand=True)

        self.tree = ttk.Treeview(table_frame, show="headings")
        vsb = ttk.Scrollbar(table_frame, orient="vertical", command=self.tree.yview)
        hsb = ttk.Scrollbar(table_frame, orient="horizontal", command=self.tree.xview)
        self.tree.configure(yscrollcommand=vsb.set, xscrollcommand=hsb.set)

        self.tree.grid(row=0, column=0, sticky="nsew")
        vsb.grid(row=0, column=1, sticky="ns")
        hsb.grid(row=1, column=0, sticky="ew")
        table_frame.rowconfigure(0, weight=1)
        table_frame.columnconfigure(0, weight=1)

    def _load_default_terms(self):
        self.terms_text.delete("1.0", "end")
        self.terms_text.insert("1.0", "\n".join(DEFAULT_TERMS))

    # ---------------- İşlemler ----------------
    def start_scan(self):
        raw_urls = self.url_text.get("1.0", "end").strip()
        raw_terms = self.terms_text.get("1.0", "end").strip()

        if not raw_urls:
            messagebox.showwarning("Eksik bilgi", "En az bir site URL'si girmelisiniz.")
            return
        if not raw_terms:
            messagebox.showwarning("Eksik bilgi", "En az bir arama terimi girmelisiniz.")
            return

        urls = [normalize_url(u) for u in raw_urls.splitlines() if u.strip()]
        terms = [t.strip() for t in raw_terms.splitlines() if t.strip()]

        if not urls or not terms:
            messagebox.showwarning("Eksik bilgi", "Geçerli URL veya terim bulunamadı.")
            return

        self.run_button.config(state="disabled")
        self.export_button.config(state="disabled")
        self.progress.config(value=0, maximum=len(urls))
        self._setup_table(terms)
        self.last_results = {}

        thread = threading.Thread(
            target=self._scan_worker, args=(urls, terms), daemon=True,
        )
        thread.start()

    def _setup_table(self, terms):
        for row in self.tree.get_children():
            self.tree.delete(row)

        columns = ["URL"] + terms
        self.tree["columns"] = columns
        for col in columns:
            self.tree.heading(col, text=col)
            width = 320 if col == "URL" else 110
            self.tree.column(col, width=width, anchor="w" if col == "URL" else "center")

    def _scan_worker(self, urls, terms):
        for i, url in enumerate(urls, start=1):
            self._set_status(f"({i}/{len(urls)}) İşleniyor: {url}")
            result = scan_url(url, terms, session=self.session)

            if result.error:
                self.last_results[url] = {"hata": result.error}
                row_values = [url] + ["HATA" for _ in terms]
            else:
                self.last_results[url] = result.counts
                row_values = [url] + [str(result.counts.get(term, 0)) for term in terms]

            self.root.after(0, self._add_row, row_values)
            self.root.after(0, self._advance_progress, i)

        self.root.after(0, self._scan_finished)

    def _add_row(self, values):
        self.tree.insert("", "end", values=values)

    def _advance_progress(self, value):
        self.progress.config(value=value)

    def _set_status(self, text):
        self.root.after(0, lambda: self.status_var.set(text))

    def _scan_finished(self):
        self.status_var.set("Tarama tamamlandı.")
        self.run_button.config(state="normal")
        self.export_button.config(state="normal")

    def export_results(self):
        if not self.last_results:
            messagebox.showinfo("Bilgi", "Dışa aktarılacak sonuç yok.")
            return

        filetypes = [("JSON dosyası", "*.json"), ("CSV dosyası", "*.csv")]
        path = filedialog.asksaveasfilename(defaultextension=".json", filetypes=filetypes)
        if not path:
            return

        try:
            if path.lower().endswith(".csv"):
                self._export_csv(path)
            else:
                with open(path, "w", encoding="utf-8") as f:
                    json.dump(self.last_results, f, ensure_ascii=False, indent=2)
            messagebox.showinfo("Başarılı", f"Sonuçlar kaydedildi:\n{path}")
        except Exception as e:
            messagebox.showerror("Hata", f"Kaydetme sırasında hata oluştu:\n{e}")

    def _export_csv(self, path):
        all_terms = set()
        for counts in self.last_results.values():
            all_terms.update(k for k in counts.keys() if k != "hata")
        all_terms = sorted(all_terms)

        with open(path, "w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(["URL"] + all_terms)
            for url, counts in self.last_results.items():
                if "hata" in counts:
                    writer.writerow([url] + ["HATA"] * len(all_terms))
                else:
                    writer.writerow([url] + [counts.get(t, 0) for t in all_terms])


def main():
    root = tk.Tk()
    try:
        style = ttk.Style()
        if "clam" in style.theme_names():
            style.theme_use("clam")
    except Exception:
        pass
    app = WordCounterApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()