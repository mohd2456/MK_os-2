#ifndef MK_ANSWER_EXTRACTOR_CPP
#define MK_ANSWER_EXTRACTOR_CPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cmath>

// ============================================================================
// MKAnswerExtractor - Finds answers to questions from search results
// Uses keyword overlap scoring and sentence extraction with confidence
// ============================================================================

struct ExtractedAnswer {
    std::string answer_text;
    std::string source_snippet;
    std::string source_url;
    float confidence;
    int sentence_index;
    std::vector<std::string> matching_keywords;
};

struct ScoredSnippet {
    std::string text;
    std::string url;
    float score;
    std::vector<std::string> sentences;
};

struct AnswerSearchResult {
    std::string title;
    std::string url;
    std::string snippet;
};

class MKAnswerExtractor {
private:
    std::set<std::string> stopwords_;
    float min_confidence_threshold_;

    void initializeStopwords() {
        stopwords_ = {
            "the", "a", "an", "is", "are", "was", "were", "be", "been", "being",
            "have", "has", "had", "do", "does", "did", "will", "would", "could",
            "should", "may", "might", "shall", "can", "need", "dare", "to", "of",
            "in", "for", "on", "with", "at", "by", "from", "as", "into", "through",
            "during", "before", "after", "above", "below", "between", "out", "off",
            "over", "under", "again", "further", "then", "once", "here", "there",
            "when", "where", "why", "how", "all", "each", "every", "both", "few",
            "more", "most", "other", "some", "such", "no", "nor", "not", "only",
            "own", "same", "so", "than", "too", "very", "just", "because", "but",
            "and", "or", "if", "while", "about", "up", "it", "its", "this", "that",
            "these", "those", "i", "me", "my", "we", "our", "you", "your", "he",
            "him", "his", "she", "her", "they", "them", "their", "what", "which"
        };
    }

    std::string toLower(const std::string& str) const {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower;
    }

    std::vector<std::string> tokenize(const std::string& text) const {
        std::vector<std::string> tokens;
        std::istringstream stream(toLower(text));
        std::string word;
        while (stream >> word) {
            word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
            if (!word.empty()) tokens.push_back(word);
        }
        return tokens;
    }

    std::vector<std::string> getKeywords(const std::string& text) const {
        std::vector<std::string> tokens = tokenize(text);
        std::vector<std::string> keywords;
        for (const auto& token : tokens) {
            if (stopwords_.find(token) == stopwords_.end() && token.length() > 2) {
                keywords.push_back(token);
            }
        }
        return keywords;
    }

    std::vector<std::string> splitSentences(const std::string& text) const {
        std::vector<std::string> sentences;
        std::string current;

        for (size_t i = 0; i < text.size(); i++) {
            current += text[i];
            if (text[i] == '.' || text[i] == '!' || text[i] == '?') {
                // Check it's end of sentence (not abbreviation)
                bool is_end = (i + 1 >= text.size()) || 
                              (i + 1 < text.size() && text[i + 1] == ' ');
                if (is_end && current.length() > 10) {
                    // Trim whitespace
                    size_t start = current.find_first_not_of(" \t\n");
                    if (start != std::string::npos) {
                        sentences.push_back(current.substr(start));
                    }
                    current.clear();
                }
            }
        }
        if (!current.empty()) {
            size_t start = current.find_first_not_of(" \t\n");
            if (start != std::string::npos && current.length() > 10) {
                sentences.push_back(current.substr(start));
            }
        }
        return sentences;
    }

    float computeKeywordOverlap(const std::vector<std::string>& question_keywords,
                                const std::string& text) const {
        std::vector<std::string> text_tokens = tokenize(text);
        std::set<std::string> text_set(text_tokens.begin(), text_tokens.end());

        int matches = 0;
        for (const auto& kw : question_keywords) {
            if (text_set.count(kw)) matches++;
        }

        if (question_keywords.empty()) return 0.0f;
        return static_cast<float>(matches) / static_cast<float>(question_keywords.size());
    }

    float scoreSentencePosition(int index, int total) const {
        // First few sentences tend to contain the answer
        if (index == 0) return 1.0f;
        if (index == 1) return 0.9f;
        if (index < 3) return 0.8f;
        return 0.6f - (0.1f * std::min(index - 3, 4));
    }

    float scoreSentenceLength(const std::string& sentence) const {
        int word_count = 0;
        std::istringstream stream(sentence);
        std::string word;
        while (stream >> word) word_count++;

        // Prefer medium-length sentences (10-30 words)
        if (word_count >= 10 && word_count <= 30) return 1.0f;
        if (word_count >= 5 && word_count < 10) return 0.7f;
        if (word_count > 30 && word_count <= 50) return 0.8f;
        return 0.5f;
    }

    std::vector<std::string> findMatchingKeywords(const std::vector<std::string>& question_kw,
                                                   const std::string& text) const {
        std::vector<std::string> matches;
        std::string lower_text = toLower(text);
        for (const auto& kw : question_kw) {
            if (lower_text.find(kw) != std::string::npos) {
                matches.push_back(kw);
            }
        }
        return matches;
    }

public:
    MKAnswerExtractor() : min_confidence_threshold_(0.2f) {
        initializeStopwords();
    }

    ExtractedAnswer extractAnswer(const std::string& question,
                                  const std::vector<AnswerSearchResult>& search_results) {
        ExtractedAnswer best_answer;
        best_answer.confidence = 0.0f;

        std::vector<std::string> question_keywords = getKeywords(question);

        for (const auto& result : search_results) {
            // Score the entire snippet first
            float snippet_score = scoreRelevance(question, result.snippet);
            
            // Split into sentences and score each
            std::vector<std::string> sentences = splitSentences(result.snippet);
            
            for (int i = 0; i < static_cast<int>(sentences.size()); i++) {
                float keyword_score = computeKeywordOverlap(question_keywords, sentences[i]);
                float position_score = scoreSentencePosition(i, static_cast<int>(sentences.size()));
                float length_score = scoreSentenceLength(sentences[i]);
                
                // Combined scoring
                float total_score = (keyword_score * 0.5f + position_score * 0.2f + 
                                    length_score * 0.1f + snippet_score * 0.2f);

                if (total_score > best_answer.confidence) {
                    best_answer.answer_text = sentences[i];
                    best_answer.source_snippet = result.snippet;
                    best_answer.source_url = result.url;
                    best_answer.confidence = total_score;
                    best_answer.sentence_index = i;
                    best_answer.matching_keywords = findMatchingKeywords(question_keywords, sentences[i]);
                }
            }
        }

        // If no good sentence found, use best snippet directly
        if (best_answer.confidence < min_confidence_threshold_ && !search_results.empty()) {
            float best_snippet_score = 0.0f;
            for (const auto& result : search_results) {
                float score = scoreRelevance(question, result.snippet);
                if (score > best_snippet_score) {
                    best_snippet_score = score;
                    best_answer.answer_text = result.snippet;
                    best_answer.source_url = result.url;
                    best_answer.source_snippet = result.snippet;
                    best_answer.confidence = score * 0.7f; // Lower confidence for full snippet
                }
            }
        }

        return best_answer;
    }

    float scoreRelevance(const std::string& question, const std::string& snippet) const {
        std::vector<std::string> q_keywords = getKeywords(question);
        if (q_keywords.empty()) return 0.0f;

        float overlap = computeKeywordOverlap(q_keywords, snippet);
        
        // Bonus for exact phrase matches
        std::string lower_q = toLower(question);
        std::string lower_s = toLower(snippet);
        float phrase_bonus = 0.0f;

        // Check for 2-word and 3-word phrase matches
        std::vector<std::string> q_words = tokenize(question);
        for (size_t i = 0; i + 1 < q_words.size(); i++) {
            std::string bigram = q_words[i] + " " + q_words[i + 1];
            if (lower_s.find(bigram) != std::string::npos) {
                phrase_bonus += 0.15f;
            }
        }
        for (size_t i = 0; i + 2 < q_words.size(); i++) {
            std::string trigram = q_words[i] + " " + q_words[i + 1] + " " + q_words[i + 2];
            if (lower_s.find(trigram) != std::string::npos) {
                phrase_bonus += 0.25f;
            }
        }

        return std::min(1.0f, overlap + phrase_bonus);
    }

    void setConfidenceThreshold(float threshold) {
        min_confidence_threshold_ = std::max(0.0f, std::min(1.0f, threshold));
    }

    float getConfidenceThreshold() const { return min_confidence_threshold_; }

    std::string getConfidenceLevel(float confidence) const {
        if (confidence >= 0.8f) return "high";
        if (confidence >= 0.5f) return "medium";
        if (confidence >= 0.3f) return "low";
        return "very low";
    }

    std::vector<ScoredSnippet> rankSnippets(const std::string& question,
                                             const std::vector<AnswerSearchResult>& results) const {
        std::vector<ScoredSnippet> scored;
        for (const auto& r : results) {
            ScoredSnippet ss;
            ss.text = r.snippet;
            ss.url = r.url;
            ss.score = scoreRelevance(question, r.snippet);
            ss.sentences = splitSentences(r.snippet);
            scored.push_back(ss);
        }
        std::sort(scored.begin(), scored.end(),
                  [](const ScoredSnippet& a, const ScoredSnippet& b) {
                      return a.score > b.score;
                  });
        return scored;
    }
};

#endif // MK_ANSWER_EXTRACTOR_CPP
