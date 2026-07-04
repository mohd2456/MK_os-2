#ifndef MK_TOKENIZER_CPP
#define MK_TOKENIZER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <utility>
#include "safe_parse.h"

// ===================================================================================
// MK BYTE-PAIR ENCODING (BPE) TOKENIZER
// Custom scratch-built tokenizer for MK's neural engine. Designed to:
//   - Learn merge rules from training text (vocabulary building)
//   - Encode arbitrary text into integer token sequences
//   - Decode token sequences back to human-readable text
//   - Operate with minimal memory footprint for low-end hardware
//   - Support special tokens: <PAD>, <UNK>, <BOS>, <EOS>, <SEP>
// ===================================================================================

struct MKMergeRule {
    std::string left;
    std::string right;
    std::string merged;
    int priority;  // Lower = applied first
};

class MKTokenizer {
private:
    // Core vocabulary: token_id <-> string mapping
    std::unordered_map<std::string, int> tokenToId;
    std::unordered_map<int, std::string> idToToken;
    
    // BPE merge rules learned from training data
    std::vector<MKMergeRule> mergeRules;
    
    int vocabSize;
    int nextId;

    // Special token IDs
    int PAD_ID;
    int UNK_ID;
    int BOS_ID;
    int EOS_ID;
    int SEP_ID;

    // Split a word into individual UTF-8 characters
    std::vector<std::string> splitToChars(const std::string& word) {
        std::vector<std::string> chars;
        for (size_t i = 0; i < word.size(); i++) {
            unsigned char c = word[i];
            int charLen = 1;
            if ((c & 0x80) == 0) charLen = 1;
            else if ((c & 0xE0) == 0xC0) charLen = 2;
            else if ((c & 0xF0) == 0xE0) charLen = 3;
            else if ((c & 0xF8) == 0xF0) charLen = 4;
            
            chars.push_back(word.substr(i, charLen));
            i += charLen - 1;
        }
        return chars;
    }

    // Split text into words (whitespace-based with punctuation separation)
    std::vector<std::string> splitToWords(const std::string& text) {
        std::vector<std::string> words;
        std::string current;
        
        for (char c : text) {
            if (c == ' ' || c == '\n' || c == '\t') {
                if (!current.empty()) {
                    words.push_back(current);
                    current.clear();
                }
                // Preserve space as a special character for reconstruction
                words.push_back(" ");
            } else if (c == '.' || c == ',' || c == '!' || c == '?' || 
                       c == ';' || c == ':' || c == '(' || c == ')') {
                if (!current.empty()) {
                    words.push_back(current);
                    current.clear();
                }
                words.push_back(std::string(1, c));
            } else {
                current += c;
            }
        }
        if (!current.empty()) words.push_back(current);
        return words;
    }

    // Find the most frequent adjacent pair in a token sequence
    std::pair<std::string, std::string> findMostFrequentPair(
        const std::vector<std::vector<std::string>>& corpus) {
        
        std::map<std::pair<std::string, std::string>, int> pairCounts;
        
        for (const auto& word : corpus) {
            for (size_t i = 0; i + 1 < word.size(); i++) {
                pairCounts[{word[i], word[i+1]}]++;
            }
        }
        
        std::pair<std::string, std::string> bestPair = {"", ""};
        int bestCount = 0;
        
        for (const auto& kv : pairCounts) {
            if (kv.second > bestCount) {
                bestCount = kv.second;
                bestPair = kv.first;
            }
        }
        return bestPair;
    }

    // Apply a single merge rule to all words in corpus
    void applyMerge(std::vector<std::vector<std::string>>& corpus,
                    const std::string& left, const std::string& right) {
        std::string merged = left + right;
        
        for (auto& word : corpus) {
            std::vector<std::string> newWord;
            size_t i = 0;
            while (i < word.size()) {
                if (i + 1 < word.size() && word[i] == left && word[i+1] == right) {
                    newWord.push_back(merged);
                    i += 2;
                } else {
                    newWord.push_back(word[i]);
                    i++;
                }
            }
            word = newWord;
        }
    }

    // Register a token in both lookup tables
    void addToken(const std::string& token) {
        if (tokenToId.find(token) == tokenToId.end()) {
            tokenToId[token] = nextId;
            idToToken[nextId] = token;
            nextId++;
        }
    }

public:
    MKTokenizer(int maxVocab = 8192) : vocabSize(maxVocab), nextId(0) {
        // Register special tokens first (guaranteed lowest IDs)
        addToken("<PAD>"); PAD_ID = 0;
        addToken("<UNK>"); UNK_ID = 1;
        addToken("<BOS>"); BOS_ID = 2;
        addToken("<EOS>"); EOS_ID = 3;
        addToken("<SEP>"); SEP_ID = 4;
        
        // Register all printable ASCII as base vocabulary
        for (int c = 32; c < 127; c++) {
            addToken(std::string(1, (char)c));
        }
        
        std::cout << "[TOKENIZER] BPE engine initialized. Base vocab=" << nextId 
                  << " | Max vocab=" << vocabSize << "\n";
    }

    // Train BPE merge rules from raw text corpus
    void train(const std::string& text, int numMerges = 1000) {
        std::cout << "[TOKENIZER] Training BPE on " << text.size() << " bytes with " 
                  << numMerges << " merges...\n";
        
        // Split all words into character sequences
        std::vector<std::string> words = splitToWords(text);
        std::vector<std::vector<std::string>> corpus;
        
        for (const auto& word : words) {
            corpus.push_back(splitToChars(word));
        }
        
        // Iteratively find and merge most frequent pairs
        int mergesPerformed = 0;
        for (int i = 0; i < numMerges && nextId < vocabSize; i++) {
            auto bestPair = findMostFrequentPair(corpus);
            if (bestPair.first.empty()) break;  // No more pairs to merge
            
            std::string merged = bestPair.first + bestPair.second;
            
            // Add merge rule
            MKMergeRule rule;
            rule.left = bestPair.first;
            rule.right = bestPair.second;
            rule.merged = merged;
            rule.priority = mergesPerformed;
            mergeRules.push_back(rule);
            
            // Register new token
            addToken(merged);
            
            // Apply merge to entire corpus
            applyMerge(corpus, bestPair.first, bestPair.second);
            mergesPerformed++;
        }
        
        std::cout << "[TOKENIZER] Training complete. Merges=" << mergesPerformed 
                  << " | Final vocab=" << nextId << "\n";
    }

    // Encode text into token ID sequence
    std::vector<int> encode(const std::string& text, bool addSpecialTokens = true) {
        std::vector<int> tokenIds;
        
        if (addSpecialTokens) tokenIds.push_back(BOS_ID);
        
        // Split into words, then apply BPE merges to each
        std::vector<std::string> words = splitToWords(text);
        
        for (const auto& word : words) {
            std::vector<std::string> tokens = splitToChars(word);
            
            // Apply all merge rules in priority order
            for (const auto& rule : mergeRules) {
                std::vector<std::string> newTokens;
                size_t i = 0;
                while (i < tokens.size()) {
                    if (i + 1 < tokens.size() && 
                        tokens[i] == rule.left && tokens[i+1] == rule.right) {
                        newTokens.push_back(rule.merged);
                        i += 2;
                    } else {
                        newTokens.push_back(tokens[i]);
                        i++;
                    }
                }
                tokens = newTokens;
            }
            
            // Convert tokens to IDs
            for (const auto& tok : tokens) {
                auto it = tokenToId.find(tok);
                if (it != tokenToId.end()) {
                    tokenIds.push_back(it->second);
                } else {
                    tokenIds.push_back(UNK_ID);
                }
            }
        }
        
        if (addSpecialTokens) tokenIds.push_back(EOS_ID);
        return tokenIds;
    }

    // Decode token ID sequence back to text
    std::string decode(const std::vector<int>& tokenIds) {
        std::string result;
        for (int id : tokenIds) {
            // Skip special tokens in output
            if (id == PAD_ID || id == BOS_ID || id == EOS_ID || id == SEP_ID) continue;
            if (id == UNK_ID) {
                result += "?";
                continue;
            }
            
            auto it = idToToken.find(id);
            if (it != idToToken.end()) {
                result += it->second;
            }
        }
        return result;
    }

    // Save vocabulary and merge rules to file
    void saveVocab(const std::string& filename) {
        std::ofstream out(filename);
        if (!out.is_open()) return;
        
        // Write vocab size
        out << nextId << "\n";
        
        // Write all tokens
        for (int i = 0; i < nextId; i++) {
            out << i << "\t" << idToToken[i] << "\n";
        }
        
        // Write merge rules
        out << "---MERGES---\n";
        for (const auto& rule : mergeRules) {
            out << rule.left << "\t" << rule.right << "\t" << rule.merged << "\n";
        }
        out.close();
        std::cout << "[TOKENIZER] Vocabulary saved: " << filename << "\n";
    }

    // Load vocabulary from file
    void loadVocab(const std::string& filename) {
        std::ifstream in(filename);
        if (!in.is_open()) {
            std::cerr << "[TOKENIZER ERROR] Cannot load vocab from: " << filename << "\n";
            return;
        }
        
        tokenToId.clear();
        idToToken.clear();
        mergeRules.clear();
        
        std::string line;
        std::getline(in, line);
        int totalTokens = mk::safeStoi(line, 0);
        
        for (int i = 0; i < totalTokens; i++) {
            std::getline(in, line);
            size_t tab = line.find('\t');
            if (tab != std::string::npos) {
                int id = mk::safeStoi(line.substr(0, tab), -1);
                if (id < 0) continue;
                std::string token = line.substr(tab + 1);
                tokenToId[token] = id;
                idToToken[id] = token;
            }
        }
        nextId = totalTokens;
        
        // Read merge rules
        std::getline(in, line); // "---MERGES---"
        while (std::getline(in, line)) {
            std::stringstream ss(line);
            MKMergeRule rule;
            std::getline(ss, rule.left, '\t');
            std::getline(ss, rule.right, '\t');
            std::getline(ss, rule.merged, '\t');
            rule.priority = mergeRules.size();
            mergeRules.push_back(rule);
        }
        
        in.close();
        std::cout << "[TOKENIZER] Loaded vocab: " << nextId << " tokens, " 
                  << mergeRules.size() << " merge rules.\n";
    }

    // Getters
    int getVocabSize() const { return nextId; }
    int getMaxVocab() const { return vocabSize; }
    int getPadId() const { return PAD_ID; }
    int getUnkId() const { return UNK_ID; }
    int getBosId() const { return BOS_ID; }
    int getEosId() const { return EOS_ID; }
};

#endif // MK_TOKENIZER_CPP
