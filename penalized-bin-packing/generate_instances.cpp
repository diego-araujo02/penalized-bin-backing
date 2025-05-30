#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <string>
#include <filesystem>
#include <algorithm>
#include <iomanip>

namespace fs = std::filesystem;

int main() {
    // Parâmetros de geração: 50 bases x 5 gamma = 250 instâncias
    std::vector<int> n_values = {50, 100, 200, 500, 1000};    // tamanhos
    std::vector<double> load_factors = {0.2, 0.35, 0.5, 0.65, 0.8};
    std::vector<int> K_values = {3, 7};                       // categorias
    const std::vector<double> gamma_values = {0.0, 0.25, 0.5, 0.75, 1.0};

    // Diretório de saída
    const std::string output_dir = "instances_balanced";
    fs::create_directories(output_dir);

    // RNG
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> size_dist(1, 100);

    // Geração
    int base_count = 0;
    for (int n : n_values) {
        for (double lf : load_factors) {
            for (int K : K_values) {
                // Gera tamanhos e soma total para capacidade
                std::vector<int> sizes(n);
                long long total_size = 0;
                for (int i = 0; i < n; ++i) {
                    sizes[i] = size_dist(gen);
                    total_size += sizes[i];
                }
                int Q = static_cast<int>(lf * total_size);
                Q = std::max(Q, *std::max_element(sizes.begin(), sizes.end()));

                // Gera categorias uniformes
                std::uniform_int_distribution<> cat_dist(0, K - 1);
                std::vector<int> cats(n);
                for (int i = 0; i < n; ++i) cats[i] = cat_dist(gen);

                // Base instance index
                ++base_count;
                for (double gamma : gamma_values) {
                    std::ostringstream fname;
                    fname << output_dir << "/inst_" << std::setw(2) << std::setfill('0')
                          << base_count << "_gamma" << std::fixed << std::setprecision(2)
                          << gamma << ".txt";
                    std::ofstream out(fname.str());
                    out << "n=" << n << "\n";
                    out << "Q=" << Q << "\n";
                    out << "K=" << K << "\n";
                    out << "gamma=" << std::fixed << std::setprecision(2) << gamma << "\n\n";
                    out << "# size category\n";
                    for (int i = 0; i < n; ++i)
                        out << sizes[i] << " " << cats[i] << "\n";
                    out.close();
                }
            }
        }
    }

    std::cout << "Total de bases geradas: " << base_count
              << " (cada uma com 5 gamma, total = "
              << base_count * gamma_values.size() << ")\n";
    std::cout << "Arquivos em '" << output_dir << "'" << std::endl;
    return 0;
}
