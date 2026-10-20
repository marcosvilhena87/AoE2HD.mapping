"""Valida os quatro catálogos do AoE2HD.mapping e os espelhos históricos."""
from __future__ import annotations
import csv
from collections import Counter
from pathlib import Path
from .validar_catalogo import validate as validate_memory

BASE = Path(__file__).resolve().parents[1]

def read(path, fields):
    with path.open(encoding="utf-8-sig", newline="") as f:
        reader = csv.DictReader(f)
        if tuple(reader.fieldnames or ()) != fields:
            raise ValueError(f"Schema incorreto: {path}")
        rows = list(reader)
    if not rows or any(any(not (row.get(k) or "").strip() for k in fields) for row in rows):
        raise ValueError(f"Catalogo vazio ou incompleto: {path}")
    return rows

def validate(root=BASE):
    errors = validate_memory(root)
    folder = root / "dados" / "catalogos"
    pairs = [
        (root/"dados/genie_type_ids.csv",folder/"tipos_genie.csv",
         ("genie_type_id","nome_pt","sexo","status_evidencia","origem")),
        (root/"dados/enderecos.csv",folder/"estruturas_memoria.csv",
         ("raiz","offset_ou_endereco","descricao","tipo","status","origem")),
    ]
    try:
        for old, new, schema in pairs:
            a,b=read(old,schema),read(new,schema)
            key=lambda r: tuple(r[f] for f in schema)
            if Counter(map(key,a)) != Counter(map(key,b)):
                errors.append(f"Espelho divergente: {new} vs {old}")
            if len(a)!=len({key(r) for r in a}):
                errors.append(f"Registros duplicados: {old}")
        genie=read(folder/"tipos_genie.csv",pairs[0][2])
        known={int(r["genie_type_id"]) for r in genie}
        if len(known)!=len(genie) or any(not 0 <= x <= 65535 for x in known):
            errors.append("IDs Genie duplicados ou invalidos")
        classes=read(folder/"classes_objetos.csv",
                     ("class_id","nome_pt","status_evidencia","origem","observacao"))
        if len({int(r["class_id"]) for r in classes})!=len(classes):
            errors.append("Classes duplicadas")
        identities=read(folder/"identidades_individuais.csv",
                        ("sessao","object_id","genie_type_id","proprietario_escopo","status_evidencia","origem","observacao"))
        keys={(r["sessao"],r["object_id"]) for r in identities}
        if len(keys)!=len(identities):
            errors.append("Object ID duplicado na mesma sessao")
        for r in identities:
            if int(r["object_id"])<0:
                errors.append("Object ID invalido")
            if r["genie_type_id"] and int(r["genie_type_id"]) not in known:
                errors.append(f"Tipo sem catalogo: {r['genie_type_id']}")
    except (ValueError, OSError, csv.Error) as exc:
        errors.append(str(exc))
    return errors

if __name__=="__main__":
    errors=validate()
    for error in errors:
        print("ERRO:",error)
    if not errors:
        print("OK: quatro catalogos integrados e espelhos preservados")
    raise SystemExit(bool(errors))
