#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    fs::path currentDir = fs::current_path();
    fs::path programPath = fs::absolute(argv[0]);  // this program itself

    std::ofstream out("summary.txt");
    if (!out.is_open()) {
        std::cerr << "Could not create summary.txt\n";
        return 1;
    }

    for (auto& entry : fs::recursive_directory_iterator(currentDir)) {
        if (fs::is_regular_file(entry.path())) {
            // Skip this program and the summary file itself
            if (fs::equivalent(entry.path(), programPath) ||
                entry.path().filename() == "summary.txt") {
                continue;
            }

            std::ifstream file(entry.path());
            if (!file.is_open()) {
                std::cerr << "Could not open file: " << entry.path() << "\n";
                continue;
            }

            out << entry.path().string() << "\n";  // file name
            out << file.rdbuf();                   // file contents
            out << "\n\n";                         // separator
        }
    }

    std::cout << "Summary written to summary.txt\n";
    return 0;
}

