# Penalized Bin Packing (PBP)

Este repositório contém a implementação de um modelo de Programação Linear Inteira Mista (MILP) para o problema de Bin Packing Penalizado, utilizando o solver IBM CPLEX Optimization Studio.

## Visão Geral

O problema de Bin Packing Penalizado estende o problema clássico de Bin Packing ao introduzir um termo de penalidade para a "impureza" (ou heterogeneidade) dos itens alocados em cada compartimento (bin). O objetivo é minimizar o número total de bins utilizados, considerando um custo adicional associado à impureza, que é ponderado por um parâmetro $\gamma$.

## Estrutura do Repositório

* `pen_binpacking.cpp`: Implementação padrão do modelo MILP para uma única instância do problema.
* `batch_run.cpp`: Versão do modelo adaptada para processar múltiplas instâncias de uma pasta específica.
* `/instances_balanced`: Diretório contendo os arquivos de instâncias de entrada para o `batch_run`.
* `/results`: (Sugestão: Crie esta pasta para armazenar os resultados gerados)

## Requisitos

Para compilar e executar este código, você precisará ter o **IBM CPLEX Optimization Studio** instalado e configurado em seu ambiente. As instruções de compilação abaixo assumem que as variáveis de ambiente `$CPLEX_PATH` e `$CONCERT_PATH` estão configuradas corretamente para apontar para os diretórios de instalação do CPLEX e Concert Technology, respectivamente.

Este projeto foi desenvolvido e testado no seguinte ambiente:

* **Máquina:** Intel Core i5-11300 / 16GB RAM
* **Sistema Operacional:** Windows 11 com WSL (Windows Subsystem for Linux)
* **Solver:** IBM CPLEX Optimization Studio V22.1.1
* **Linguagem de Programação:** C++
* **API:** Concert Technology

## Compilação e Execução

### `pen_binpacking.cpp` (Modo Padrão)

Este arquivo é compilado para gerar um executável que pode resolver uma instância específica (geralmente hardcoded ou lida de uma forma específica dentro do próprio `.cpp`).

**Compilação:**

```bash
g++ -std=c++17 -o pen_binpacking pen_binpacking.cpp -I$CPLEX_PATH/include -I$CONCERT_PATH/include -L$CPLEX_PATH/lib/x86-64_linux/static_pic -L$CONCERT_PATH/lib/x86-64_linux/static_pic -lconcert -lilocplex -lcplex -lm -lpthread -ldl -DIL_STD
```
```bash
./pen_binpacking.cpp
```
**batch_run.cpp (Modo em Lote):**

Este arquivo é projetado para automatizar a execução do modelo para múltiplas instâncias localizadas na pasta /instances_balanced.

**Compilação:**

```bash
g++ -std=c++17 -o batch_run batch_run.cpp -I$CPLEX_PATH/include -I$CONCERT_PATH/include -L$CPLEX_PATH/lib/x86-64_linux/static_pic -L$CONCERT_PATH/lib/x86-64_linux/static_pic -lconcert -lilocplex -lcplex -lm -lpthread -ldl -DIL_STD
```
```bash
./batch_run instances_balanced
```