#include "CSV.h"


void CSV::fill_map(std::map<std::string, unsigned long long>& words, std::istream& file_in) {
    std::string line;
    while (std::getline(file_in, line)) {
        std::string word = "";
        for (int i = 0; i < line.size(); i++) {
            if (isalnum(line[i])) {
                word += line[i];
            } else {
                if (word != ""){
                    words[word]++;
                    word = "";
                }
            }
        }
        if (!word.empty()) {
            ++words[word];
            word.clear();
        }
    }
}

void CSV::fill_vector(std::vector<word_degree>& vector_words, std::istream& file_in) {
    using namespace std;
    map<string, unsigned long long> words;
    fill_map(words, file_in);
    for (auto& [key, value] : words) {
        word_degree a;
        a.word = key;
        a.count = value;
        vector_words.push_back(a);
    }
}

bool CSV::comp(const word_degree& a, const word_degree& b) {
    if (a.count > b.count) return true;
    else return false;
}

unsigned long long CSV::counting_words(std::vector<word_degree>& vector_words) {
    unsigned long long sum = 0;
    for (int i = 0; i < vector_words.size(); i++) {
        sum += vector_words[i].count;
    }
    return sum;
}


void CSV::fill_csv(std::istream& file_in, std::ostream& file_out) {
    using namespace std;
    vector<word_degree> vector_words;
    fill_vector(vector_words, file_in);
    sort(vector_words.begin(), vector_words.end(), comp);
    unsigned long long quantity_words = counting_words(vector_words);
    double freq;
    for (int i = 0; i < vector_words.size(); i++) {
        freq = double(vector_words[i].count) / quantity_words;
        file_out << vector_words[i].word << ";" << vector_words[i].count << ";" << freq << endl;
    }
}
