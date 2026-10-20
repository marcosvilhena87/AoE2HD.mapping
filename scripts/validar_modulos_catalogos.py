"""Confere preservação integral dos catálogos divididos em módulos."""
import csv
from collections import Counter
from pathlib import Path

SCHEMAS = {
    "tipos_genie": ("genie_type_id", "nome_pt", "sexo", "status_evidencia", "origem"),
    "classes_objetos": ("class_id", "nome_pt", "status_evidencia", "origem", "observacao"),
    "identidades_individuais": ("sessao", "object_id", "genie_type_id", "proprietario_escopo", "status_evidencia", "origem", "observacao"),
}

def read(path, schema):
    with path.open(encoding="utf-8-sig", newline="") as f:
        reader = csv.reader(f, strict=True)
        rows = list(reader)
    if not rows or tuple(rows[0]) != schema:
        raise ValueError(f"Cabeçalho inválido: {path}")
    data = [tuple(row) for row in rows[1:]]
    if not data:
        raise ValueError(f"Arquivo vazio: {path}")
    for i, row in enumerate(data, 2):
        if len(row) != len(schema) or any(not field.strip() for j, field in enumerate(row)
              if not (schema[j] == "genie_type_id" and "object_id" in schema)):
            raise ValueError(f"Linha incompleta: {path}:{i}")
    return data

def validate(root):
    errors = []
    for name, schema in SCHEMAS.items():
        folder = root / "dados" / "catalogos" / "modulos" / name
        paths = sorted(folder.glob("*.csv"))
        if not paths:
            errors.append(f"Módulos ausentes: {folder}")
            continue
        try:
            master = read(root / "dados" / "catalogos" / f"{name}.csv", schema)
            parts = [row for path in paths for row in read(path, schema)]
            for label, rows in (("consolidado", master), ("módulos", parts)):
                if len(rows) != len(set(rows)):
                    errors.append(f"{name}: duplicações no {label}")
            missing = Counter(master) - Counter(parts)
            extra = Counter(parts) - Counter(master)
            if missing or extra:
                errors.append(f"{name}: {sum(missing.values())} registros ausentes e {sum(extra.values())} extras")
        except (ValueError, OSError, csv.Error) as exc:
            errors.append(str(exc))
    return errors

if __name__ == "__main__":
    errors = validate(Path(__file__).resolve().parents[1])
    for error in errors:
        print("ERRO:", error)
    if not errors:
        print("OK: todos os catálogos temáticos sincronizados")
    raise SystemExit(bool(errors))
