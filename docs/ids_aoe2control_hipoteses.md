# IDs do AoE2Control como hipoteses de pesquisa no AoE2 HD

Fonte externa: https://aoe2control.github.io/enums/ (Age of Empires II: Definitive Edition).
Estes IDs sao valores de **enumeracoes da API**, nao enderecos de memoria e nao offsets.
A tabela externa nao demonstra equivalencia entre layouts internos de DE (x64) e HD 5.8.INT (x86).

| API | Identificador | Valor | Interpretacao para testes HD |
| --- | --- | ---: | --- |
| UnitClass | VILLAGER | 904 | Classe de aldeao; **hipotese de correspondencia** a 4 interno, nao offset provado |
| UnitClass | BUILDING | 903 | Controle de edificio; hipotese de campo 903 ou 3 |
| UnitClass | INFANTRY | 906 | Controle de infantaria; hipotese de campo 906 ou 6 |
| UnitClass | SPEARMAN | 950 | Classe especifica disponibilizada pela API; controle 950 ou 50 |
| UnitObjectType | VILLAGER_MALE | 83 | Tipo de unidade, diferente de classe e diferente do Object_ID |

IDs 74 (milicia) e 93 (lanceiro) ja foram observados experimentalmente no projeto HD em campos relacionados a treinamento. Nesta tabela sao **controles empiricos do HD**, nao uma prova de definicao de unit object na memoria nem uma afirmacao de que esta pagina externa documenta esses dois valores.

## Hipoteses concorrentes

- H1: algum campo da definicao de unidade contem a classe interna (`4` para aldeao) em `uint8`, `uint16` ou `uint32`.
- H2: algum campo expoe uma representacao da API (`904`) ou valor equivalente.
- H3: o objeto faz referencia a uma definicao compartilhada onde outro campo contem classe ou tipo, sem representar os enums em bytes identicos.
- H4: valores `4` observados nos offsets `Object+0x78 -> +0x0C/+0x94` sao coincidencias ou outro atributo semantico.

Nao presumir `904 == 4` em bytes de memoria: a transformacao entre valores e apenas uma hipotese, nao uma regra validada para o HD.

## Protocolo de teste

Comparar tres aldeoes (inclusive com atividades diferentes), milicia, lanceiro e construcao; capturar valores brutos completos e a largura efetiva; repetir apos reinicio. Provar especificidade com controles negativos antes de classificar objetos automaticamente. Buscar tanto offsets diretos como cadeias por definicao de unidade e registrar resultados positivos e negativos.

**Status:** referencia de enum externa, sem descoberta de endereco novo e sem alteracao da classificacao de offsets existentes.
