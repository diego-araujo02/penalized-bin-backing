#include <ilcplex/ilocplex.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;
ILOSTLBEGIN

// Função para remover espaços de uma string
std::string remove_whitespace(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (!std::isspace(c)) {
            result += c;
        }
    }
    return result;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <dir_instancias>\n";
        return 1;
    }
    fs::path inst_dir = argv[1];
    fs::path out_dir = "results";
    fs::create_directories(out_dir);

    for (auto& p : fs::directory_iterator(inst_dir)) {
        if (!p.is_regular_file() || p.path().extension() != ".txt")
            continue;

        std::ifstream fin(p.path());
        if (!fin.is_open()) {
            std::cerr << "Erro ao abrir arquivo: " << p.path() << "\n";
            continue;
        }

        // ----- 1) Leitura da instância -----
        int n = 0, Q = 0, K = 0;
        double gamma = 0.0;
        std::vector<int> tamanho, categoria;
        std::string line;

        // Ler parâmetros
        while (std::getline(fin, line)) {
            // Remover espaços e verificar se é linha vazia/comentário
            std::string clean_line = remove_whitespace(line);
            if (clean_line.empty() || clean_line[0] == '#') continue;

            // Processar parâmetros
            if (clean_line.find("n=") == 0) {
                n = std::stoi(clean_line.substr(2));
            } 
            else if (clean_line.find("Q=") == 0) {
                Q = std::stoi(clean_line.substr(2));
            } 
            else if (clean_line.find("K=") == 0) {
                K = std::stoi(clean_line.substr(2));
            } 
            else if (clean_line.find("gamma=") == 0) {
                gamma = std::stod(clean_line.substr(6));
            }
            else {
                // Se não é parâmetro, deve ser dado de item
                break;
            }
        }

        // Processar itens (a linha atual já contém o primeiro item)
        do {
            if (line.empty() || line[0] == '#') continue;
            
            std::istringstream iss(line);
            int s, c;
            if (iss >> s >> c) {
                tamanho.push_back(s);
                categoria.push_back(c);
            }
        } while (std::getline(fin, line));

        fin.close();

        // Verificar consistência
        if (n <= 0 || Q <= 0 || K <= 0 || tamanho.size() != static_cast<size_t>(n)) {
            std::cerr << "Dados inválidos ou incompletos em: " << p.path() << "\n";
            std::cerr << "n=" << n << " Q=" << Q << " K=" << K 
                      << " itens=" << tamanho.size() << "\n";
            continue;
        }

        // ----- 2) Configurar e resolver o modelo -----
        IloEnv env;
        try {
            int maxBins = n;
            IloModel model(env);

            // Variáveis
            IloArray<IloBoolVarArray> x(env, n);
            for (int i = 0; i < n; ++i) {
                x[i] = IloBoolVarArray(env, maxBins);
                for (int b = 0; b < maxBins; ++b) {
                    x[i][b] = IloBoolVar(env);
                }
            }
            
            IloBoolVarArray y(env, maxBins);
            for (int b = 0; b < maxBins; ++b) {
                y[b] = IloBoolVar(env);
            }
            
            IloArray<IloBoolVarArray> w(env, maxBins);
            for (int b = 0; b < maxBins; ++b) {
                w[b] = IloBoolVarArray(env, K);
                for (int c = 0; c < K; ++c) {
                    w[b][c] = IloBoolVar(env);
                }
            }
            
            IloBoolVarArray delta(env, maxBins);
            for (int b = 0; b < maxBins; ++b) {
                delta[b] = IloBoolVar(env);
            }

            // Restrições
            // (C1) Cada item em exatamente um bin
            for (int i = 0; i < n; ++i) {
                IloExpr sum(env);
                for (int b = 0; b < maxBins; ++b) {
                    sum += x[i][b];
                }
                model.add(sum == 1);
                sum.end();
            }

            // (C2) Respeitar capacidade do bin
            for (int b = 0; b < maxBins; ++b) {
                IloExpr cap(env);
                for (int i = 0; i < n; ++i) {
                    cap += tamanho[i] * x[i][b];
                }
                model.add(cap <= Q * y[b]);
                cap.end();
            }

            // (C3) Se item está no bin, ativar categoria correspondente
            for (int i = 0; i < n; ++i) {
                int c = categoria[i];
                for (int b = 0; b < maxBins; ++b) {
                    model.add(x[i][b] <= w[b][c]);
                }
            }

            // (C4) Se categoria está ativa, deve ter pelo menos um item
            for (int b = 0; b < maxBins; ++b) {
                for (int c = 0; c < K; ++c) {
                    IloExpr sum(env);
                    for (int i = 0; i < n; ++i) {
                        if (categoria[i] == c) {
                            sum += x[i][b];
                        }
                    }
                    model.add(w[b][c] <= sum);
                    model.add(w[b][c] >= 0.1 * sum); // Evitar falsos positivos
                    sum.end();
                }
            }

            // (C5) Ordem dos bins (simetria)
            for (int b = 1; b < maxBins; ++b) {
                model.add(y[b] <= y[b - 1]);
            }

            // (C6) Identificar bins impuros (com mais de uma categoria)
            for (int b = 0; b < maxBins; ++b) {
                IloExpr sum_cats(env);
                for (int c = 0; c < K; ++c) {
                    sum_cats += w[b][c];
                }
                
                // Bin é impuro se tiver mais de uma categoria
                model.add(delta[b] >= sum_cats - 1);
                model.add(delta[b] <= y[b]); // Só pode ser impuro se bin estiver ativo
                sum_cats.end();
            }

            // Função objetivo
            IloExpr exprBins(env);
            IloExpr exprImp(env);
            
            for (int b = 0; b < maxBins; ++b) {
                exprBins += y[b];
                exprImp += delta[b];
            }
            
            model.add(IloMinimize(env, exprBins + gamma * exprImp));
            exprBins.end();
            exprImp.end();

            // Resolver
            IloCplex cplex(model);
            cplex.setParam(IloCplex::Param::TimeLimit, 60);
            cplex.setParam(IloCplex::Param::Threads, 0);
            cplex.setOut(env.getNullStream());
            
            auto t0 = std::chrono::high_resolution_clock::now();
            bool solved = cplex.solve();
            auto t1 = std::chrono::high_resolution_clock::now();
            double elapsed = std::chrono::duration<double>(t1 - t0).count();

            // Processar resultados
            double totalBins = 0.0;
            double totalImp = 0.0;
            double obj = 0.0;
            IloAlgorithm::Status st = IloAlgorithm::Unknown;
            
            if (solved) {
                st = cplex.getStatus();
                obj = cplex.getObjValue();
                
                for (int b = 0; b < maxBins; ++b) {
                    if (cplex.getValue(y[b]) > 0.5) {
                        totalBins += 1.0;
                        totalImp += cplex.getValue(delta[b]);
                    }
                }
            }

            env.end();

            // ----- 3) Gravar resultados -----
            std::string base = p.path().stem().string();
            std::ofstream fout(out_dir / (base + "_res.txt"));
            fout << "instancia: " << p.path().filename() << "\n";
            fout << "n: " << n << "\n";
            fout << "Q: " << Q << "\n";
            fout << "K: " << K << "\n";
            fout << "gamma: " << gamma << "\n";
            fout << "tempo_s: " << std::fixed << std::setprecision(4) << elapsed << "\n";
            fout << "status: " << st << "\n";
            fout << "objetivo: " << obj << "\n";
            fout << "bins_usados: " << totalBins << "\n";
            fout << "bins_impuro: " << totalImp << "\n";
            fout.close();

        } catch (IloException& e) {
            env.end();
            std::cerr << "CPLEX error on " << p.path() << ": " << e << "\n";
        } catch (...) {
            env.end();
            std::cerr << "Unknown error on " << p.path() << "\n";
        }
    }
    std::cout << "Processamento concluído. Veja resultados em 'results/'\n";
    return 0;
}