# Módulos dos catálogos de mapeamento

| Catálogo | Arquivos modulares | Critério |
|---|---|---|
| tipos_genie | dados/catalogos/modulos/tipos_genie/*.csv | Função do objeto |
| classes_objetos | dados/catalogos/modulos/classes_objetos/*.csv | Classes conhecidas e desconhecidas |
| identidades_individuais | dados/catalogos/modulos/identidades_individuais/*.csv | Sessão experimental |
| estruturas_memoria | dados/modulos/*.csv | Sete domínios históricos |

Os registros canônicos e os espelhos históricos permanecem intactos.
A organização modular não é evidência de validação de uma hipótese.
Nunca apagar registros candidatos, nem atribuir estabilidade a endereços
absolutos de sessão. Para verificar as correspondências, execute:

```bash
python scripts/validar_catalogos.py
python scripts/validar_modulos_catalogos.py
python -m unittest discover -s tests -v
```
