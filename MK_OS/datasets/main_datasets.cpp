#include "loader.cpp"
#include "parser.cpp"
#include "db.cpp"
#include "compressor.cpp"

int main() {
    MKDatasetLoader loader;
    MKDatasetParser parser;
    MKDatasetDB db;
    MKCompressor compressor;

    // Load TXT dataset
    auto mathData = loader.loadTXT("datasets/math/formulas.txt");

    // Parse CSV dataset
    auto slangData = parser.parseCSV("datasets/talk/slang.csv");

    // Parse JSON dataset
    auto jsonData = parser.parseJSON("datasets/knowledge/facts.json");

    // Open SQLite DB
    db.openDB("datasets/code/codebase.db");

    // Compress dataset
    compressor.compress("datasets/math/formulas.txt");
    compressor.decompress("datasets/math/formulas.txt");

    std::cout << "[DATASETS] Integration complete." << std::endl;
    return 0;
}