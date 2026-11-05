"""Check the portable ZIP structure and native Windows dependency closure."""
import argparse
from pathlib import Path, PurePosixPath
import struct
import zipfile


def pe_info(data):
    """Read the PE32+ import table without external tools or executing the binary."""
    assert data[:2] == b"MZ", "Not a Windows executable"
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    assert data[pe:pe + 4] == b"PE\0\0"
    machine, count = struct.unpack_from("<HH", data, pe + 4)
    assert machine == 0x8664, "Expected Windows x64"
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    optional = pe + 24
    assert struct.unpack_from("<H", data, optional)[0] == 0x20B, "Expected PE32+"
    sections = []
    for i in range(count):
        offset = optional + optional_size + i * 40
        virtual_size, rva, size, raw = struct.unpack_from("<IIII", data, offset + 8)
        sections.append((rva, max(virtual_size, size), raw))

    def at(rva):
        for base, size, raw in sections:
            if base <= rva < base + size:
                return raw + rva - base
        raise ValueError(f"Invalid PE RVA {rva:x}")

    imports = []
    rva, size = struct.unpack_from("<II", data, optional + 112 + 8)
    if rva:
        offset = at(rva)
        while any(data[offset:offset + 20]):
            name = at(struct.unpack_from("<I", data, offset + 12)[0])
            imports.append(data[name:data.index(0, name)].decode("ascii").lower())
            offset += 20
    return imports


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("zip", type=Path)
    args = parser.parse_args()
    with zipfile.ZipFile(args.zip) as archive:
        assert archive.testzip() is None, "ZIP CRC failure"
        entries = archive.namelist()
        for entry in entries:
            path = PurePosixPath(entry)
            assert path.parts[0] == "sPDF" and ".." not in path.parts and not path.is_absolute(), entry
        required = ["sPDF/sPDF.exe", "sPDF/pdfium.dll", "sPDF/README.txt",
                    "sPDF/licenses/pdfium/LICENSE", "sPDF/licenses/pdfium/PDFium.md",
                    "sPDF/licenses/KDE-Modern-Classic/COPYING", "sPDF/licenses/KDE-Modern-Classic/COPYING.LESSER",
                    "sPDF/licenses/KDE-Modern-Classic/AUTHORS", "sPDF/licenses/KDE-Modern-Classic/README.md"]
        for path in required:
            assert path in entries, f"Missing {path}"
        # Known OS-provided DLLs on the target; any other import must be packaged.
        system = {"kernel32.dll", "user32.dll", "gdi32.dll", "advapi32.dll", "comctl32.dll",
                  "comdlg32.dll", "shell32.dll", "shlwapi.dll", "uxtheme.dll", "msvcrt.dll",
                  "ole32.dll", "oleaut32.dll", "version.dll", "winspool.drv", "ws2_32.dll"}
        bundled = {PurePosixPath(x).name.lower() for x in entries if PurePosixPath(x).parent == PurePosixPath("sPDF")}
        for path in entries:
            if path.lower().endswith((".exe", ".dll")):
                imports = pe_info(archive.read(path))
                for dependency in imports:
                    assert dependency in system or dependency in bundled, f"Unbundled DLL: {path}: {dependency}"
                print(f"{path}: x64; imports {', '.join(imports)}")
        assert "sPDF/spdf_pdf_tests.exe" not in entries
        print(f"PASS: {len(entries)} ZIP entries, one sPDF folder, x64 binaries, dependency and license checks")


if __name__ == "__main__":
    main()
