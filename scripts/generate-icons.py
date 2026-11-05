"""Regenerate only the used icons from the repository's original archive.

Development-only requirements: CairoSVG 2.8.2 and Pillow. No runtime dependency.
The original SVGs and license/author documents are distributed alongside ICOs.
"""
from pathlib import Path
from io import BytesIO
import tarfile
import cairosvg
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
NAMES = {
    "document-open": "actions", "go-previous": "actions", "go-next": "actions",
    "zoom-in": "actions", "zoom-out": "actions", "zoom-fit-best": "actions",
    "zoom-fit-width": "actions", "object-rotate-right": "actions",
    "application-pdf": "mimetypes",
    "document-print": "devices", "edit-find": "actions",
}
with tarfile.open(ROOT / "KDE-Modern-Classic-20261027.tar.gz") as archive:
    for name, section in NAMES.items():
        svg = archive.extractfile(f"KDE-Modern-Classic/scalable/{section}/{name}.svg").read()
        target = ROOT / "resources/icons"
        target.mkdir(parents=True, exist_ok=True)
        (target / f"{name}.svg").write_bytes(svg)
        png = cairosvg.svg2png(bytestring=svg, output_width=256, output_height=256)
        Image.open(BytesIO(png)).save(target / f"{name}.ico", sizes=[(n, n) for n in (16, 24, 32, 48, 64, 128, 256)])
    for name in ("COPYING", "COPYING.LESSER", "LICENSE-KDE-Classic.txt", "AUTHORS", "README.md"):
        target = ROOT / "LICENSES/KDE-Modern-Classic"
        target.mkdir(parents=True, exist_ok=True)
        (target / name).write_bytes(archive.extractfile(f"KDE-Modern-Classic/{name}").read())
print(f"Generated {len(NAMES)} KDE Modern Classic icons and copied license documents.")
