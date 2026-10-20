#!/usr/bin/env python3
"""Verifica a integridade entre o catálogo consolidado e os módulos AoE2HD."""
from __future__ import annotations

import argparse
import csv
import sys
from collections import Counter
from pathlib import Path

HEADER = ("raiz", "offset_ou_endereco", "descricao", "tipo", "status", "origem")


def read_catalog(path: Path) -> list[tuple[str, ...]]:
    if not path.is_file():
        raise ValueError(f"Arquivo ausente: {path}")
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        reader = csv.reader(stream, strict=True)
        rows = [tuple(cell.strip() for cell in row) for row in reader if any(cell.strip() for cell in row)]
    if not rows or rows[0] != HEADER:
        raise ValueError(f"Cabeçalho inválido em {path}. Esperado: {HEADER}")
    data = rows[1:]
    for line, row in enumerate(data, start=2):
        if len(row) != len(HEADER) or any(not field for field in row):
            raise ValueError(f"{path}: registro incompleto na linha lógica {line}: {row!r}")
    return data


def format_record(row: tuple[str, ...]) -> str:
    return " | ".join(row)


def validate(root: Path) -> list[str]:
    errors: list[str] = []
    master = root / "dados" / "enderecos.csv"
    folder = root / "dados" / "modulos"
    if not folder.is_dir():
        return [f"Diretório de módulos ausente: {folder}"]
    modules = sorted(folder.glob("*.csv"))
    if not modules:
        return [f"Nenhum módulo CSV encontrado em {folder}"]
    try:
        consolidated = read_catalog(master)
    except (ValueError, OSError, csv.Error) as exc:
        return [str(exc)]

    module_rows: list[tuple[str, ...]] = []
    for path in modules:
        try:
            records = read_catalog(path)
            if not records:
                errors.append(f"Módulo vazio: {path}")
            module_rows.extend(records)
        except (ValueError, OSError, csv.Error) as exc:
            errors.append(str(exc))
    if errors:
        # Prosseguir com os módulos legíveis para também apontar registros ausentes.
        pass

    for label, records in (("consolidado", consolidated), ("módulos", module_rows)):
        duplicates = Counter(records)
        for row, count in sorted(duplicates.items()):
            if count > 1:
                errors.append(f"Registro duplicado {'nos' if label == 'módulos' else 'no'} {label} ({count} ocorrências): {format_record(row)}")

        identities: dict[tuple[str, str], set[tuple[str, ...]]] = {}
        for row in records:
            identities.setdefault((row[0], row[1]), set()).add(row)
        for (source, address), variants in sorted(identities.items()):
            if len(variants) > 1:
                errors.append(
                    f"Conflito de identidade no {label}: {source}+{address} possui "
                    f"{len(variants)} registros distintos"
                )

    consolidated_count = Counter(consolidated)
    modular_count = Counter(module_rows)
    for row, count in sorted((consolidated_count - modular_count).items()):
        errors.append(f"Ausente nos módulos ({count}): {format_record(row)}")
    for row, count in sorted((modular_count - consolidated_count).items()):
        errors.append(f"Ausente no consolidado ({count}): {format_record(row)}")

    if not errors:
        print(f"OK: {len(consolidated)} registros idênticos distribuídos em {len(modules)} módulos; sem perdas, duplicações ou conflitos.")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    errors = validate(args.root.resolve())
    for error in errors:
        print(f"ERRO: {error}", file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
