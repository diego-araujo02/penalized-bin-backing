#include <ilcplex/ilocplex.h>
#include <iostream>
#include <vector>

ILOSTLBEGIN

int main() {
    const int n = 6;
    std::vector<int> tamanho = {2, 3, 1, 3, 2, 3};
    const int Q = 5;
    std::vector<int> categoria = {0, 0, 0, 1, 1, 1};
    const int numCategorias = 2;
    const double gamma = 0.7;  
    const int maxRecipientes = n;
   
    try {
        IloEnv env;
        IloModel model(env);

        // verificação de consistência
        if ((int)categoria.size() != n) {
            std::cerr << "ERRO: vetor de categoria tamanho incorreto\n";
            return 1;
        }
        for (int i = 0; i < n; ++i) {
            if (categoria[i] < 0 || categoria[i] >= numCategorias) {
                std::cerr << "ERRO: categoria inválida no item " << i << '\n';
                return 1;
            }
        }
        if (gamma < 0.0 || gamma > 1.0) {
            std::cerr << "ERRO: gamma deve estar em [0,1]\n";
            return 1;
        }

        // VARIÁVEIS DE DECISÃO
        IloArray<IloBoolVarArray> x(env, n);
        for (int i = 0; i < n; ++i) {
            x[i] = IloBoolVarArray(env, maxRecipientes);
        }
        IloBoolVarArray y(env, maxRecipientes);            // bin usado
        IloArray<IloBoolVarArray> w(env, maxRecipientes);  // categoria presente
        for (int b = 0; b < maxRecipientes; ++b) {
            w[b] = IloBoolVarArray(env, numCategorias);
        }
        IloBoolVarArray delta(env, maxRecipientes);        // impureza[b]

        
        // RESTRIÇÕES
        
        // (C1) cada item em exato um bin
        for (int i = 0; i < n; ++i) {
            IloExpr sum(env);
            for (int b = 0; b < maxRecipientes; ++b)
                sum += x[i][b];
            model.add(sum == 1);
            sum.end();
        }
        // (C2) capacidade
        for (int b = 0; b < maxRecipientes; ++b) {
            IloExpr cap(env);
            for (int i = 0; i < n; ++i)
                cap += tamanho[i] * x[i][b];
            model.add(cap <= Q * y[b]);
            cap.end();
        }
        // (C3) link item→categoria
        for (int i = 0; i < n; ++i) {
            int c = categoria[i];
            for (int b = 0; b < maxRecipientes; ++b) {
                model.add(x[i][b] <= w[b][c]);
            }
        }
        // (C4) w só ativa se itens
        for (int b = 0; b < maxRecipientes; ++b) {
            for (int c = 0; c < numCategorias; ++c) {
                IloExpr sum(env);
                for (int i = 0; i < n; ++i)
                    if (categoria[i] == c)
                        sum += x[i][b];
                model.add(w[b][c] <= sum);
                sum.end();
            }
        }
        // (C5) simetria de bins
        for (int b = 1; b < maxRecipientes; ++b)
            model.add(y[b] <= y[b - 1]);

        // (C6) limite de categorias por bin
        for (int b = 0; b < maxRecipientes; ++b) {
            for (int c1 = 0; c1 < numCategorias; ++c1) {
                for (int c2 = c1 + 1; c2 < numCategorias; ++c2) {
                    // delta[b] >= w[b][c1] + w[b][c2] - 1
                    model.add(delta[b] >= w[b][c1] + w[b][c2] - 1);
                }
            }
            // delta[b] só se usar bin
            model.add(delta[b] <= y[b]);
        }

        
        // FUNÇÃO OBJETIVO
        
        IloExpr exprBins(env), exprImp(env);
        for (int b = 0; b < maxRecipientes; ++b) {
            exprBins += y[b];
            exprImp  += delta[b];
        }
        model.add(IloMinimize(env, exprBins + gamma * exprImp));
        exprBins.end(); exprImp.end();

        
        // RESOLUÇÃO
        
        IloCplex cplex(model);
        cplex.setParam(IloCplex::Param::TimeLimit, 60);
        cplex.setParam(IloCplex::Param::Threads, 0);
        if (!cplex.solve()) {
            std::cerr << "Falha na otimização" << std::endl;
            return 1;
        }

        // RESULTADOS
        
        double totalBins = 0, totalImp = 0;
        std::cout << "Status: " << cplex.getStatus() << "\n";
        std::cout << "Custo objetivo: " << cplex.getObjValue() << "\n";
        for (int b = 0; b < maxRecipientes; ++b) {
            if (cplex.getValue(y[b]) > 0.5) {
                totalBins++;
                totalImp += cplex.getValue(delta[b]);
                std::cout << "\nRecipiente " << b << ": Itens [";
                int soma = 0;
                for (int i = 0; i < n; ++i) {
                    if (cplex.getValue(x[i][b]) > 0.5) {
                        std::cout << i << " ";
                        soma += tamanho[i];
                    }
                }
                std::cout << "] | Tamanho: " << soma << "/" << Q;
                std::cout << " | Impuro=" << (int)cplex.getValue(delta[b]);
            }
        }
        std::cout << "\n\nResumo: Bins usados=" << totalBins
                  << ", Impurezas=" << totalImp << std::endl;

        env.end();
    } catch (IloException& e) {
        std::cerr << "Erro CPLEX: " << e << std::endl;
    } catch (...) {
        std::cerr << "Erro desconhecido" << std::endl;
    }
    return 0;
}