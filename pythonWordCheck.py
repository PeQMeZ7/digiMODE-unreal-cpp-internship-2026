"""
Kelime/Ifade Sayim Botu - Tek Dosya (TLS/SSL Dayanikli + Gorsellestirme) Surum
==============================================================================
Ozellikler:
    - Site ici tarama (alt sayfalar / baglantilar - basit crawler).
    - Bulunan terimler renklendirilir; hucreye tiklayinca o kelimenin hangi
      URL'lerde gectigini gosteren detay penceresi acilir.
    - TLS/SSL sorunlarina dayanikli baglanti (ozel TLS adapter + gerekirse
      sertifika dogrulamasini atlayarak yeniden deneme).
    - Anlasilir, kategorili hata mesajlari (SSL, baglanti, zaman asimi, HTTP,
      JavaScript ile yuklenen bos sayfa vb.).

NOT: EKAP gibi icerigi JavaScript ile olusturan siteler (SPA) requests ile
GORUNTULENEMEZ; bu tur sayfalar "JavaScript ile yukleniyor" olarak isaretlenir.
Bunlar icin Selenium/Playwright gibi tarayici otomasyonu gerekir.

Calistirmak icin:
    pip install requests beautifulsoup4
    python kelime_sayim_botu_tek_dosya.py

.exe icin:
    pip install pyinstaller
    pyinstaller --onefile --windowed --name KelimeSayimBotu kelime_sayim_botu_tek_dosya.py
"""

from __future__ import annotations

import re
import ssl
import time
import json
import csv
import logging
import threading
import webbrowser
from collections import deque
from dataclasses import dataclass, field
from urllib.parse import urlparse, urljoin

import requests
from requests.adapters import HTTPAdapter
from bs4 import BeautifulSoup

try:
    import urllib3
    from urllib3.util.retry import Retry
    urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)
except Exception:  # pragma: no cover
    Retry = None

import tkinter as tk
from tkinter import ttk, messagebox, filedialog


# =============================================================================
# BOLUM 1: CEKIRDEK MANTIK
# =============================================================================

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
    "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
    "Accept-Language": "tr-TR,tr;q=0.9,en-US;q=0.8,en;q=0.7",
}

DEFAULT_TIMEOUT = 20  # saniye
DEFAULT_RETRIES = 2
RETRY_DELAY = 1.5  # saniye
CRAWL_DELAY = 0.3

DEFAULT_TERMS = [
    "3 boyutlu yazıcı", "3b yazıcı", "3d yazıcı", "3d printer", "3b printer",
    "Eklemeli imalat", "Katmanlı imalat", "SLS", "SLA", "DLP", "FDM",
    "Ekstrüzyon", "Filament", "Metal tozu", "SLM", "DED", "WAAM",
    "Lazer sinter", "Lazer ergitme", "AR", "VR", "Sanal Gerçeklik",
    "Simülasyon", "DFAM",
]

SKIP_EXTENSIONS = (
    ".jpg", ".jpeg", ".png", ".gif", ".svg", ".webp", ".bmp", ".ico",
    ".pdf", ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx", ".zip",
    ".rar", ".7z", ".mp3", ".mp4", ".avi", ".mov", ".wmv", ".css", ".js",
)

# Arayuz renkleri
CELL_BG = "#ffffff"        # normal hucre arka plani
CELL_FG = "#222222"        # normal hucre yazi rengi
MATCH_BG = "#c8e6c9"       # terim BULUNAN hucre arka plani (yesil)
MATCH_FG = "#1b5e20"       # terim bulunan hucre yazi rengi (koyu yesil)
ERROR_FG = "#c62828"       # hatali satirin URL yazi rengi (kirmizi)
HEADER_BG = "#eceff1"      # baslik satiri arka plani


# ---------------------------------------------------------------------------
# TLS/SSL dayanikliligi
# ---------------------------------------------------------------------------

class TLSAdapter(HTTPAdapter):
    """Eski/kati TLS ayarli sunuculara (orn. bazi kamu siteleri) baglanabilmek
    icin OpenSSL guvenlik seviyesini dusuren ve eski (legacy) baglantiya izin
    veren ozel bir requests adapter'i.

    verify=False verilirse sertifika dogrulamasi da devre disi birakilir
    (guvenli degildir; sadece baglanti kurulamayan siteler icin son care).
    """

    def __init__(self, verify: bool = True, **kwargs):
        self._verify_ssl = verify
        super().__init__(**kwargs)

    def _build_context(self):
        ctx = ssl.create_default_context()
        try:
            ctx.set_ciphers("DEFAULT@SECLEVEL=1")  # eski cipher'lara izin ver
        except ssl.SSLError:
            pass
        for opt_name in ("OP_LEGACY_SERVER_CONNECT", "OP_NO_TICKET"):
            opt = getattr(ssl, opt_name, 0)
            if opt:
                ctx.options |= opt
        if not self._verify_ssl:
            ctx.check_hostname = False
            ctx.verify_mode = ssl.CERT_NONE
        return ctx

    def init_poolmanager(self, *args, **kwargs):
        kwargs["ssl_context"] = self._build_context()
        return super().init_poolmanager(*args, **kwargs)

    def proxy_manager_for(self, *args, **kwargs):
        kwargs["ssl_context"] = self._build_context()
        return super().proxy_manager_for(*args, **kwargs)


def make_session(verify: bool = True) -> requests.Session:
    """TLS adapter + otomatik yeniden deneme ile bir oturum olusturur."""
    s = requests.Session()
    retry = None
    if Retry is not None:
        retry = Retry(
            total=DEFAULT_RETRIES,
            backoff_factor=0.5,
            status_forcelist=(429, 500, 502, 503, 504),
            allowed_methods=frozenset(["GET", "HEAD"]),
        )
    adapter = TLSAdapter(verify=verify, max_retries=retry) if retry else TLSAdapter(verify=verify)
    s.mount("https://", adapter)
    s.mount("http://", adapter)
    s.verify = verify
    s.headers.update(DEFAULT_HEADERS)
    return s


# Sertifika dogrulamasi acikken SSL hatasi olursa devreye girecek yedek oturum
_INSECURE_SESSION: requests.Session | None = None


def _get_insecure_session() -> requests.Session:
    global _INSECURE_SESSION
    if _INSECURE_SESSION is None:
        _INSECURE_SESSION = make_session(verify=False)
    return _INSECURE_SESSION


# ---------------------------------------------------------------------------
# Turkce'ye duyarli normalize
# ---------------------------------------------------------------------------

def tr_lower(text: str) -> str:
    text = text.replace("İ", "i").replace("I", "ı")
    return text.lower()


# ---------------------------------------------------------------------------
# URL yardimcilari
# ---------------------------------------------------------------------------

def normalize_url(url: str) -> str:
    url = url.strip()
    if not url:
        return url
    parsed = urlparse(url)
    if not parsed.scheme:
        url = "https://" + url
    return url


def get_domain(url: str) -> str:
    netloc = urlparse(url).netloc.lower()
    return netloc[4:] if netloc.startswith("www.") else netloc


def _canonical(url: str) -> str:
    url = url.split("#", 1)[0]
    return url.rstrip("/")


class FetchError(Exception):
    """Sayfa indirme sirasinda olusan (anlasilir mesajli) hata."""


def _describe_error(e: Exception) -> str:
    """requests istisnasini Turkce, kategorili bir mesaja cevirir."""
    if isinstance(e, requests.exceptions.SSLError):
        return "SSL/TLS el sikisma hatasi (sunucunun sertifikasi/TLS ayari uyumsuz)"
    if isinstance(e, requests.exceptions.ConnectTimeout):
        return "Baglanti zaman asimina ugradi"
    if isinstance(e, requests.exceptions.ReadTimeout):
        return "Sunucu yanit vermedi (okuma zaman asimi)"
    if isinstance(e, requests.exceptions.ConnectionError):
        return "Baglanti kurulamadi (reddedildi/sifirlandi veya DNS hatasi)"
    if isinstance(e, requests.exceptions.HTTPError):
        code = getattr(getattr(e, "response", None), "status_code", "?")
        return f"HTTP {code} hatasi"
    if isinstance(e, requests.exceptions.TooManyRedirects):
        return "Cok fazla yonlendirme"
    return f"Baglanti hatasi: {type(e).__name__}"


# ---------------------------------------------------------------------------
# Sayfa indirme + metin/link cikarma
# ---------------------------------------------------------------------------

def _parse_response(response: requests.Response) -> tuple[str, set[str], str]:
    final_url = response.url
    ctype = response.headers.get("Content-Type", "").lower()
    if ctype and ("html" not in ctype and "xml" not in ctype and "text" not in ctype):
        raise FetchError(f"HTML olmayan icerik ({ctype.split(';')[0]}) - atlandi")

    soup = BeautifulSoup(response.content, "html.parser")

    links: set[str] = set()
    for a_tag in soup.find_all("a", href=True):
        href = a_tag["href"].strip()
        if not href or href.startswith(("#", "mailto:", "tel:", "javascript:")):
            continue
        absolute = urljoin(final_url, href)
        if absolute.lower().endswith(SKIP_EXTENSIONS):
            continue
        links.add(_canonical(absolute))

    for tag in soup(["script", "style", "noscript", "template", "svg"]):
        tag.decompose()

    text = soup.get_text(separator=" ")
    text = re.sub(r"\s+", " ", text).strip()

    if not text and not links:
        raise FetchError(
            "Sayfa bos geldi - icerik JavaScript ile yukleniyor (SPA). "
            "Bu arac JS calistiramaz; Selenium/Playwright gerekir."
        )
    return text, links, final_url


def fetch_page(
    url: str,
    session: requests.Session | None = None,
    timeout: int = DEFAULT_TIMEOUT,
    retries: int = DEFAULT_RETRIES,
    allow_insecure_fallback: bool = True,
) -> tuple[str, set[str], str]:
    """Sayfayi indirir; (metin, linkler, gercek_url) doner.

    SSL hatasinda (ve allow_insecure_fallback=True ise) sertifika
    dogrulamasi kapali yedek oturumla bir kez daha dener.
    """
    sess = session or make_session()
    last_error: Exception | None = None

    for attempt in range(1, retries + 2):
        try:
            response = sess.get(url, timeout=timeout, allow_redirects=True)
            response.raise_for_status()
            return _parse_response(response)

        except requests.exceptions.SSLError as e:
            last_error = e
            logger.warning("SSL hatasi (%s): %s", url, e)
            if allow_insecure_fallback:
                try:
                    logger.info("Sertifika dogrulamasi atlanarak tekrar deneniyor: %s", url)
                    resp2 = _get_insecure_session().get(url, timeout=timeout, allow_redirects=True)
                    resp2.raise_for_status()
                    return _parse_response(resp2)
                except FetchError:
                    raise
                except requests.exceptions.RequestException as e2:
                    last_error = e2
            break

        except FetchError:
            raise

        except requests.exceptions.RequestException as e:
            last_error = e
            logger.warning("Deneme %d/%d basarisiz (%s): %s", attempt, retries + 1, url, e)
            if attempt <= retries:
                time.sleep(RETRY_DELAY)

    raise FetchError(_describe_error(last_error) if last_error else "Bilinmeyen hata")


# ---------------------------------------------------------------------------
# Sayim mantigi
# ---------------------------------------------------------------------------

def _build_pattern(term: str) -> re.Pattern:
    normalized = tr_lower(term.strip())
    escaped = re.escape(normalized)
    if " " in normalized:
        pattern = re.sub(r"\\\ ", r"\\s+", escaped)
    else:
        pattern = r"\b" + escaped + r"\b"
    return re.compile(pattern, flags=re.UNICODE)


def count_occurrences(text: str, terms: list[str]) -> dict[str, int]:
    normalized_text = tr_lower(text)
    results: dict[str, int] = {}
    for term in terms:
        term = term.strip()
        if not term:
            continue
        pattern = _build_pattern(term)
        results[term] = len(pattern.findall(normalized_text))
    return results


@dataclass
class ScanResult:
    url: str
    counts: dict[str, int] = field(default_factory=dict)
    error: str | None = None
    depth: int = 0


def scan_url(url: str, terms: list[str], session: requests.Session | None = None) -> ScanResult:
    try:
        text, _links, final_url = fetch_page(url, session=session)
        return ScanResult(url=final_url, counts=count_occurrences(text, terms))
    except FetchError as e:
        logger.error("Sayfa alinamadi: %s -> %s", url, e)
        return ScanResult(url=url, error=str(e))


# ---------------------------------------------------------------------------
# Site ici tarama (crawl)
# ---------------------------------------------------------------------------

def crawl_site(
    start_url: str,
    terms: list[str],
    max_pages: int = 20,
    max_depth: int = 2,
    same_domain_only: bool = True,
    session: requests.Session | None = None,
    on_page_done=None,
    should_stop=None,
):
    sess = session or make_session()
    start_domain = get_domain(start_url)
    visited: set[str] = set()
    queue = deque([(start_url, 0)])
    results = []

    while queue and len(visited) < max_pages:
        if should_stop and should_stop():
            break

        url, depth = queue.popleft()
        key = _canonical(url)
        if key in visited:
            continue
        visited.add(key)

        try:
            text, links, final_url = fetch_page(url, session=sess)
            result = ScanResult(url=final_url, counts=count_occurrences(text, terms), depth=depth)
        except FetchError as e:
            logger.error("Sayfa alinamadi: %s -> %s", url, e)
            result = ScanResult(url=url, error=str(e), depth=depth)
            links = set()

        results.append(result)
        if on_page_done:
            on_page_done(result)

        if depth < max_depth:
            for link in links:
                if len(visited) >= max_pages:
                    break
                if same_domain_only and get_domain(link) != start_domain:
                    continue
                if link not in visited:
                    queue.append((link, depth + 1))

        time.sleep(CRAWL_DELAY)

    return results


# =============================================================================
# BOLUM 2: GUI
# =============================================================================

class WordCounterApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Kelime/Ifade Sayim Botu")
        self.root.geometry("920x760")
        self.root.minsize(760, 580)

        self.last_results: dict[str, dict] = {}
        self.detailed_results: dict[str, list[ScanResult]] = {}
        self.current_terms: list[str] = []

        self.session = make_session(verify=True)
        self._stop_requested = False
        self._scan_thread: threading.Thread | None = None

        self._build_widgets()

    # ---------------- Arayuz ----------------
    def _build_widgets(self):
        main = ttk.Frame(self.root, padding=12)
        main.pack(fill="both", expand=True)

        ttk.Label(main, text="Site URL'leri (her satira bir tane):").pack(anchor="w")
        self.url_text = tk.Text(main, height=5, wrap="none")
        self.url_text.pack(fill="x", pady=(2, 10))

        terms_header = ttk.Frame(main)
        terms_header.pack(fill="x")
        ttk.Label(terms_header, text="Aranacak kelime/ifadeler (her satira bir tane):").pack(side="left")
        ttk.Button(terms_header, text="Varsayilan Terimleri Yukle", command=self._load_default_terms).pack(side="right")

        self.terms_text = tk.Text(main, height=8, wrap="none")
        self.terms_text.pack(fill="x", pady=(2, 10))

        crawl_frame = ttk.LabelFrame(main, text="Site Ici Tarama (Alt Sayfalar)", padding=8)
        crawl_frame.pack(fill="x", pady=(0, 10))

        self.crawl_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            crawl_frame,
            text="Girdigim URL'lerin alt sayfalarini / baglantilarini da tara",
            variable=self.crawl_var, command=self._toggle_crawl_options,
        ).grid(row=0, column=0, columnspan=4, sticky="w")

        ttk.Label(crawl_frame, text="Maks. sayfa sayisi (site basina):").grid(row=1, column=0, sticky="w", pady=(6, 0))
        self.max_pages_var = tk.IntVar(value=20)
        self.max_pages_spin = ttk.Spinbox(crawl_frame, from_=1, to=500, width=6, textvariable=self.max_pages_var)
        self.max_pages_spin.grid(row=1, column=1, sticky="w", padx=(6, 20), pady=(6, 0))

        ttk.Label(crawl_frame, text="Maks. derinlik (tiklama mesafesi):").grid(row=1, column=2, sticky="w", pady=(6, 0))
        self.max_depth_var = tk.IntVar(value=2)
        self.max_depth_spin = ttk.Spinbox(crawl_frame, from_=0, to=10, width=6, textvariable=self.max_depth_var)
        self.max_depth_spin.grid(row=1, column=3, sticky="w", padx=(6, 0), pady=(6, 0))

        self.same_domain_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            crawl_frame, text="Sadece ayni site (alan adi) icindeki baglantilari takip et",
            variable=self.same_domain_var,
        ).grid(row=2, column=0, columnspan=4, sticky="w", pady=(6, 0))

        self.skip_ssl_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(
            crawl_frame,
            text="SSL sertifikasini bastan dogrulama (kati/eski TLS'li kamu sitelerinde ise yarar; guvenli degildir)",
            variable=self.skip_ssl_var,
        ).grid(row=3, column=0, columnspan=4, sticky="w", pady=(6, 0))

        options_frame = ttk.Frame(main)
        options_frame.pack(fill="x", pady=(0, 10))
        self.run_button = ttk.Button(options_frame, text="Taramayi Baslat", command=self.start_scan)
        self.run_button.pack(side="right")
        self.stop_button = ttk.Button(options_frame, text="Durdur", command=self.stop_scan, state="disabled")
        self.stop_button.pack(side="right", padx=(0, 8))
        self.export_button = ttk.Button(
            options_frame, text="Sonuclari Disa Aktar (CSV/JSON)", command=self.export_results, state="disabled",
        )
        self.export_button.pack(side="right", padx=(0, 8))

        self.status_var = tk.StringVar(value="Hazir.")
        ttk.Label(main, textvariable=self.status_var, foreground="#555").pack(anchor="w", pady=(0, 6))
        self.progress = ttk.Progressbar(main, mode="determinate")
        self.progress.pack(fill="x", pady=(0, 10))

        legend = ttk.Frame(main)
        legend.pack(fill="x")
        ttk.Label(legend, text="Sonuclar (her satir bir site):").pack(side="left")
        tk.Label(legend, text=" bulundu ", bg=MATCH_BG, fg=MATCH_FG,
                 font=("Segoe UI", 8, "bold")).pack(side="left", padx=(4, 2))
        ttk.Label(legend, text="yesil = terim bulundu (tiklanabilir)", foreground="#555").pack(side="left")
        ttk.Label(legend, text="   |  Kirmizi URL = hatali sayfa", foreground="#c62828").pack(side="left")

        # --- Sonuc izgarasi (her hucre ayri renklenebilsin diye Label grid) ---
        table_frame = ttk.Frame(main)
        table_frame.pack(fill="both", expand=True, pady=(4, 0))

        self.grid_canvas = tk.Canvas(table_frame, highlightthickness=0, background="#ffffff")
        vsb = ttk.Scrollbar(table_frame, orient="vertical", command=self.grid_canvas.yview)
        hsb = ttk.Scrollbar(table_frame, orient="horizontal", command=self.grid_canvas.xview)
        self.grid_canvas.configure(yscrollcommand=vsb.set, xscrollcommand=hsb.set)

        self.grid_inner = tk.Frame(self.grid_canvas, background="#ffffff")
        self._grid_window = self.grid_canvas.create_window((0, 0), window=self.grid_inner, anchor="nw")

        self.grid_inner.bind(
            "<Configure>",
            lambda e: self.grid_canvas.configure(scrollregion=self.grid_canvas.bbox("all")),
        )
        # Fare tekerlegi ile dikey kaydirma
        self.grid_canvas.bind_all(
            "<MouseWheel>",
            lambda e: self.grid_canvas.yview_scroll(int(-1 * (e.delta / 120)), "units"),
        )

        self.grid_canvas.grid(row=0, column=0, sticky="nsew")
        vsb.grid(row=0, column=1, sticky="ns")
        hsb.grid(row=1, column=0, sticky="ew")
        table_frame.rowconfigure(0, weight=1)
        table_frame.columnconfigure(0, weight=1)

        self._grid_row = 0  # bir sonraki eklenecek veri satirinin izgara indeksi

    def _toggle_crawl_options(self):
        state = "normal" if self.crawl_var.get() else "disabled"
        self.max_pages_spin.config(state=state)
        self.max_depth_spin.config(state=state)

    def _load_default_terms(self):
        self.terms_text.delete("1.0", "end")
        self.terms_text.insert("1.0", "\n".join(DEFAULT_TERMS))

    # ---------------- Islemler ----------------
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
            messagebox.showwarning("Eksik bilgi", "Gecerli URL veya terim bulunamadi.")
            return

        self.session = make_session(verify=not self.skip_ssl_var.get())

        crawl_enabled = self.crawl_var.get()
        max_pages = max(1, self.max_pages_var.get())
        max_depth = max(0, self.max_depth_var.get())
        same_domain_only = self.same_domain_var.get()

        self.run_button.config(state="disabled")
        self.stop_button.config(state="normal")
        self.export_button.config(state="disabled")

        self.progress.config(mode="determinate", value=0,
                             maximum=len(urls) * max_pages if crawl_enabled else len(urls))

        self._setup_table(terms)
        self.last_results = {}
        self.detailed_results = {}
        self._stop_requested = False
        self._pages_done = 0

        self._scan_thread = threading.Thread(
            target=self._scan_worker,
            args=(urls, terms, crawl_enabled, max_pages, max_depth, same_domain_only),
            daemon=True,
        )
        self._scan_thread.start()

    def stop_scan(self):
        self._stop_requested = True
        self.status_var.set("Durduruluyor... (mevcut sayfa tamamlanana kadar bekleyin)")
        self.stop_button.config(state="disabled")

    def _setup_table(self, terms):
        self.current_terms = list(terms)
        # Onceki izgara icerigini temizle
        for w in self.grid_inner.winfo_children():
            w.destroy()
        self._grid_row = 0

        headers = ["Site (URL)", "Taranan", "Hatali"] + terms
        for col, title in enumerate(headers):
            width = 44 if col == 0 else (9 if col in (1, 2) else 13)
            anchor = "w" if col == 0 else "center"
            tk.Label(
                self.grid_inner, text=title, width=width, anchor=anchor,
                bg=HEADER_BG, fg="#111", font=("Segoe UI", 9, "bold"),
                relief="flat", padx=4, pady=3, borderwidth=1,
            ).grid(row=0, column=col, sticky="nsew", padx=1, pady=(0, 1))
        self._grid_row = 1

    def _scan_worker(self, urls, terms, crawl_enabled, max_pages, max_depth, same_domain_only):
        for i, start_url in enumerate(urls, start=1):
            if self._stop_requested:
                break

            site_totals = {term: 0 for term in terms}
            counters = {"pages": 0, "errors": 0}
            page_results: list[ScanResult] = []
            last_error_msg = {"text": None}

            def handle_result(result: ScanResult):
                page_results.append(result)
                if result.error:
                    counters["errors"] += 1
                    last_error_msg["text"] = result.error
                else:
                    counters["pages"] += 1
                    for term in terms:
                        site_totals[term] += result.counts.get(term, 0)

                self._pages_done += 1
                total_done = counters["pages"] + counters["errors"]
                self.root.after(0, self._advance_progress, self._pages_done)
                self._set_status(f"({i}/{len(urls)}) {start_url} -> {total_done} sayfa tarandi...")

            if crawl_enabled:
                self._set_status(f"({i}/{len(urls)}) Site taraniyor: {start_url}")
                crawl_site(
                    start_url, terms, max_pages=max_pages, max_depth=max_depth,
                    same_domain_only=same_domain_only, session=self.session,
                    on_page_done=handle_result, should_stop=lambda: self._stop_requested,
                )
            else:
                self._set_status(f"({i}/{len(urls)}) Isleniyor: {start_url}")
                handle_result(scan_url(start_url, terms, session=self.session))

            self.detailed_results[start_url] = page_results

            if counters["pages"] == 0 and counters["errors"] > 0:
                msg = last_error_msg["text"] or "Sayfa alinamadi"
                self.last_results[start_url] = {"hata": msg}
                row_data = {
                    "url": start_url, "is_error": True, "error_msg": msg,
                    "pages": 0, "errors": counters["errors"], "totals": {},
                }
            else:
                self.last_results[start_url] = dict(site_totals)
                row_data = {
                    "url": start_url, "is_error": False,
                    "pages": counters["pages"], "errors": counters["errors"],
                    "totals": dict(site_totals),
                }

            self.root.after(0, self._add_row, row_data)

        self.root.after(0, self._scan_finished)

    def _add_row(self, row_data):
        """Sonuc izgarasina bir site satiri ekler. Sadece terimin BULUNDUGU
        hucre (count > 0) yesil ile isaretlenir; diger hucreler notrdur."""
        r = self._grid_row
        site_url = row_data["url"]

        # 1. sutun: Site URL
        url_fg = ERROR_FG if row_data["is_error"] else CELL_FG
        tk.Label(
            self.grid_inner, text=site_url, width=44, anchor="w",
            bg=CELL_BG, fg=url_fg, padx=4, pady=2, borderwidth=1, relief="flat",
        ).grid(row=r, column=0, sticky="nsew", padx=1, pady=1)

        # 2. sutun: Taranan sayfa (hata varsa kisa hata mesaji)
        if row_data["is_error"]:
            mid_text = f"HATA: {row_data['error_msg'][:30]}"
        else:
            mid_text = str(row_data["pages"])
        tk.Label(
            self.grid_inner, text=mid_text, width=9 if not row_data["is_error"] else 30,
            anchor="center", bg=CELL_BG, fg=CELL_FG, padx=4, pady=2, borderwidth=1,
        ).grid(row=r, column=1, sticky="nsew", padx=1, pady=1)

        # 3. sutun: Hatali sayfa sayisi
        tk.Label(
            self.grid_inner, text=str(row_data["errors"]), width=9, anchor="center",
            bg=CELL_BG, fg=CELL_FG, padx=4, pady=2, borderwidth=1,
        ).grid(row=r, column=2, sticky="nsew", padx=1, pady=1)

        # Terim sutunlari: yalnizca bulunanlar (count > 0) yesil
        totals = row_data["totals"]
        for idx, term in enumerate(self.current_terms):
            count = totals.get(term, 0)
            found = count > 0
            if row_data["is_error"]:
                cell_text, bg, fg, bold = "-", CELL_BG, CELL_FG, False
            elif found:
                cell_text, bg, fg, bold = str(count), MATCH_BG, MATCH_FG, True
            else:
                cell_text, bg, fg, bold = "0", CELL_BG, "#999999", False

            lbl = tk.Label(
                self.grid_inner, text=cell_text, width=13, anchor="center",
                bg=bg, fg=fg, padx=4, pady=2, borderwidth=1,
                font=("Segoe UI", 9, "bold" if bold else "normal"),
            )
            lbl.grid(row=r, column=3 + idx, sticky="nsew", padx=1, pady=1)

            # Sadece bulunan hucre tiklanabilir -> detay penceresi
            if found:
                lbl.config(cursor="hand2")
                lbl.bind(
                    "<Button-1>",
                    lambda e, u=site_url, t=term: self._show_term_detail(u, t),
                )
                # Uzerine gelince hafif vurgu
                lbl.bind("<Enter>", lambda e, w=lbl: w.config(bg="#a5d6a7"))
                lbl.bind("<Leave>", lambda e, w=lbl: w.config(bg=MATCH_BG))

        self._grid_row += 1

    def _advance_progress(self, value):
        if value > self.progress["maximum"]:
            self.progress.config(maximum=value)
        self.progress.config(value=value)

    def _set_status(self, text):
        self.root.after(0, lambda: self.status_var.set(text))

    def _scan_finished(self):
        total = len(self.last_results)
        prefix = "Kullanici tarafindan durduruldu." if self._stop_requested else "Tarama tamamlandi."
        self.status_var.set(f"{prefix} Toplam {total} site islendi.")
        self.run_button.config(state="normal")
        self.stop_button.config(state="disabled")
        self.export_button.config(state="normal" if self.last_results else "disabled")

    # ---------------- Detay penceresi ----------------
    def _show_term_detail(self, site_url, term):
        pages = self.detailed_results.get(site_url, [])
        hits = [(r.url, r.counts.get(term, 0)) for r in pages if not r.error and r.counts.get(term, 0) > 0]
        hits.sort(key=lambda x: x[1], reverse=True)
        total = sum(c for _, c in hits)

        win = tk.Toplevel(self.root)
        win.title(f"'{term}' - Bulunan Sayfalar")
        win.geometry("720x460")
        win.minsize(520, 320)
        pad = ttk.Frame(win, padding=12)
        pad.pack(fill="both", expand=True)

        ttk.Label(pad, text=f"\"{term}\"  |  Site: {site_url}", font=("Segoe UI", 10, "bold")).pack(anchor="w")

        if not hits:
            ttk.Label(pad, text="Bu terim, taranan hicbir alt sayfada bulunamadi.",
                      foreground="#c62828").pack(anchor="w", pady=(10, 0))
            ttk.Button(pad, text="Kapat", command=win.destroy).pack(anchor="e", pady=(12, 0))
            return

        ttk.Label(
            pad,
            text=f"{len(hits)} sayfada toplam {total} kez gecti. (Satira cift tiklayarak tarayicida acabilirsiniz)",
            foreground="#1565c0",
        ).pack(anchor="w", pady=(4, 8))

        tbl = ttk.Frame(pad)
        tbl.pack(fill="both", expand=True)
        detail_tree = ttk.Treeview(tbl, columns=("url", "count"), show="headings")
        detail_tree.heading("url", text="URL")
        detail_tree.heading("count", text="Adet")
        detail_tree.column("url", width=560, anchor="w")
        detail_tree.column("count", width=70, anchor="center")
        dvsb = ttk.Scrollbar(tbl, orient="vertical", command=detail_tree.yview)
        dhsb = ttk.Scrollbar(tbl, orient="horizontal", command=detail_tree.xview)
        detail_tree.configure(yscrollcommand=dvsb.set, xscrollcommand=dhsb.set)
        detail_tree.grid(row=0, column=0, sticky="nsew")
        dvsb.grid(row=0, column=1, sticky="ns")
        dhsb.grid(row=1, column=0, sticky="ew")
        tbl.rowconfigure(0, weight=1)
        tbl.columnconfigure(0, weight=1)

        for url, c in hits:
            detail_tree.insert("", "end", values=(url, c))

        def open_in_browser(_event=None):
            sel = detail_tree.selection()
            if not sel:
                return
            url = detail_tree.item(sel[0], "values")[0]
            try:
                webbrowser.open(url)
            except Exception as e:
                messagebox.showerror("Hata", f"URL acilamadi:\n{e}")

        detail_tree.bind("<Double-1>", open_in_browser)
        btns = ttk.Frame(pad)
        btns.pack(fill="x", pady=(8, 0))
        ttk.Button(btns, text="Secili URL'yi Tarayicida Ac", command=open_in_browser).pack(side="left")
        ttk.Button(btns, text="Kapat", command=win.destroy).pack(side="right")

    # ---------------- Disa aktarma ----------------
    def export_results(self):
        if not self.last_results:
            messagebox.showinfo("Bilgi", "Disa aktarilacak sonuc yok.")
            return
        filetypes = [("JSON dosyasi", "*.json"), ("CSV dosyasi", "*.csv")]
        path = filedialog.asksaveasfilename(defaultextension=".json", filetypes=filetypes)
        if not path:
            return
        try:
            if path.lower().endswith(".csv"):
                self._export_csv(path)
            else:
                self._export_json(path)
            messagebox.showinfo("Basarili", f"Sonuclar kaydedildi:\n{path}")
        except Exception as e:
            messagebox.showerror("Hata", f"Kaydetme sirasinda hata olustu:\n{e}")

    def _export_json(self, path):
        detailed = {
            site: [{"url": r.url, "hata": r.error, "counts": r.counts} for r in pages]
            for site, pages in self.detailed_results.items()
        }
        with open(path, "w", encoding="utf-8") as f:
            json.dump({"ozet": self.last_results, "ayrintili": detailed}, f, ensure_ascii=False, indent=2)

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
    WordCounterApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()