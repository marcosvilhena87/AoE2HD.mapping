"""Testes sem dependências externas da sincronização modular."""
import csv
import tempfile
import unittest
from pathlib import Path

from scripts.validar_catalogo import HEADER, validate


def save(path, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as file:
        writer = csv.writer(file)
        writer.writerow(HEADER)
        writer.writerows(rows)


A = ("World", "0x174", "Índice", "uint32", "validado", "evidencia.md")
B = ("Object", "0x078", "Ponteiro", "ptr32", "hipotese", "origem.md")


class CatalogValidationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.master = self.root / "dados" / "enderecos.csv"
        self.a = self.root / "dados" / "modulos" / "nucleo.csv"
        self.b = self.root / "dados" / "modulos" / "unidades.csv"
        save(self.master, [A, B])
        save(self.a, [A])
        save(self.b, [B])

    def test_integrity_and_order_independence(self):
        self.assertEqual([], validate(self.root))
        save(self.master, [B, A])
        self.assertEqual([], validate(self.root))

    def test_missing_module_record(self):
        save(self.a, [])
        self.assertTrue(any("Ausente nos módulos" in x for x in validate(self.root)))

    def test_extra_module_record(self):
        save(self.master, [A])
        self.assertTrue(any("Ausente no consolidado" in x for x in validate(self.root)))

    def test_duplicate_in_modules(self):
        save(self.a, [A, A])
        self.assertTrue(any("Registro duplicado nos módulos" in x for x in validate(self.root)))

    def test_duplicate_in_consolidated(self):
        save(self.master, [A, B, A])
        self.assertTrue(any("Registro duplicado no consolidado" in x for x in validate(self.root)))

    def test_same_address_different_metadata(self):
        save(self.a, [A, ("World", "0x174", "Outro sentido", "uint32", "hipotese", "evidencia.md")])
        self.assertTrue(any("Conflito de identidade" in x for x in validate(self.root)))

    def test_mismatched_status(self):
        save(self.b, [("Object", "0x078", "Ponteiro", "ptr32", "validado", "origem.md")])
        self.assertTrue(validate(self.root))

    def test_bad_header(self):
        self.a.write_text("bad,header\\n", encoding="utf-8")
        self.assertTrue(any("Cabeçalho inválido" in x for x in validate(self.root)))

    def test_missing_module_directory(self):
        for path in (self.a, self.b):
            path.unlink()
        (self.root / "dados" / "modulos").rmdir()
        self.assertTrue(validate(self.root))


if __name__ == "__main__":
    unittest.main()
