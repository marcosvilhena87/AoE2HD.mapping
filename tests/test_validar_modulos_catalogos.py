import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from validar_modulos_catalogos import validate

class TestModulosTematicos(unittest.TestCase):
    def test_integridade(self):
        self.assertEqual(validate(Path(__file__).resolve().parents[1]), [])

if __name__ == "__main__":
    unittest.main()
