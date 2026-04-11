// csv_parser.h
#pragma once

#include <tuple>
#include <ostream>
#include <istream>
#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <type_traits>

// =====================================================
// Subtask #1: печать std::tuple (рекурсивные шаблоны)
// =====================================================
namespace tuple_print_detail {

    template <std::size_t I, class Ch, class Tr, class... Args>
    struct Printer {
        static void print(std::basic_ostream<Ch, Tr>& os, const std::tuple<Args...>& t) {
            Printer<I - 1, Ch, Tr, Args...>::print(os, t);
            os << ", " << std::get<I>(t);
        }
    };

    template <class Ch, class Tr, class... Args>
    struct Printer<0, Ch, Tr, Args...> {
        static void print(std::basic_ostream<Ch, Tr>& os, const std::tuple<Args...>& t) {
            os << std::get<0>(t);
        }
    };

} // namespace tuple_print_detail

template <class Ch, class Tr, class... Args>
std::basic_ostream<Ch, Tr>& operator<<(std::basic_ostream<Ch, Tr>& os,
    const std::tuple<Args...>& t)
{
    os << "(";
    if constexpr (sizeof...(Args) > 0) {
        tuple_print_detail::Printer<sizeof...(Args) - 1, Ch, Tr, Args...>::print(os, t);
    }
    os << ")";
    return os;
}

// =====================================================
// Subtask #2: CSVParser + InputIterator + lazy чтение
// Improved: quoting + config + exceptions(row,col)
// =====================================================

struct CSVConfig {
    char col_delim = ';';  // просили ; (можешь поменять на ',' если нужно по умолчанию)
    char row_delim = '\n';
    char quote = '"';
};

struct CSVParseError : std::runtime_error {
    std::size_t row; // 1-based
    std::size_t col; // 1-based (номер CSV-колонки)
    CSVParseError(std::size_t r, std::size_t c, const std::string& msg)
        : std::runtime_error(msg), row(r), col(c) {}
};

namespace csv_detail {

    inline std::string make_msg(std::size_t row, std::size_t col, const std::string& what) {
        return "CSV parse error at row " + std::to_string(row) +
            ", col " + std::to_string(col) + ": " + what;
    }

    // Разбор одной строки на поля с поддержкой кавычек:
    // - поле может быть "...."
    // - внутри кавычек разделитель колонок не работает
    // - "" внутри кавычек означает одну кавычку
    inline std::vector<std::string> split_line(const std::string& line,
        const CSVConfig& cfg,
        std::size_t row_1based)
    {
        std::vector<std::string> fields;
        std::string cur;
        bool in_quotes = false;

        for (std::size_t i = 0; i < line.size(); ++i) {
            char ch = line[i];

            if (!in_quotes) {
                if (ch == cfg.col_delim) {
                    fields.push_back(cur);
                    cur.clear();
                    continue;
                }
                if (ch == cfg.quote) {
                    // кавычка разрешена только в начале поля
                    if (!cur.empty()) {
                        throw CSVParseError(row_1based, fields.size() + 1,
                            make_msg(row_1based, fields.size() + 1,
                                "unexpected quote inside unquoted field"));
                    }
                    in_quotes = true;
                    continue;
                }
                cur.push_back(ch);
            }
            else {
                if (ch == cfg.quote) {
                    // "" -> "
                    if (i + 1 < line.size() && line[i + 1] == cfg.quote) {
                        cur.push_back(cfg.quote);
                        ++i;
                        continue;
                    }
                    // закрывающая кавычка
                    in_quotes = false;

                    // после закрывающей кавычки: либо разделитель, либо конец строки
                    if (i + 1 < line.size() && line[i + 1] != cfg.col_delim) {
                        throw CSVParseError(row_1based, fields.size() + 1,
                            make_msg(row_1based, fields.size() + 1,
                                "characters after closing quote"));
                    }
                    continue;
                }
                cur.push_back(ch);
            }
        }

        if (in_quotes) {
            throw CSVParseError(row_1based, fields.size() + 1,
                make_msg(row_1based, fields.size() + 1, "missing closing quote"));
        }

        fields.push_back(cur);
        return fields;
    }

    // Конвертация ячейки в тип T.
    // Требование лабы: парсить в заданные типы шаблонов.
    // Делается просто: для string — как есть, иначе через stringstream.
    template <class T>
    T parse_cell(const std::string& s, std::size_t row, std::size_t col)
    {
        if constexpr (std::is_same_v<T, std::string>) {
            return s;
        }
        else {
            std::istringstream iss(s);
            T value{};
            iss >> value;
            // ошибка, если не прочиталось или остался мусор
            if (iss.fail()) {
                throw CSVParseError(row, col, make_msg(row, col, "cannot parse value"));
            }
            char extra;
            if (iss >> extra) {
                throw CSVParseError(row, col, make_msg(row, col, "extra characters in value"));
            }
            return value;
        }
    }

    template <std::size_t I, class... Ts>
    struct TupleBuilder {
        static void fill(std::tuple<Ts...>& out,
            const std::vector<std::string>& fields,
            std::size_t row_1based)
        {
            TupleBuilder<I - 1, Ts...>::fill(out, fields, row_1based);
            using T = std::tuple_element_t<I, std::tuple<Ts...>>;
            std::get<I>(out) = parse_cell<T>(fields[I], row_1based, I + 1);
        }
    };

    template <class... Ts>
    struct TupleBuilder<0, Ts...> {
        static void fill(std::tuple<Ts...>& out,
            const std::vector<std::string>& fields,
            std::size_t row_1based)
        {
            using T0 = std::tuple_element_t<0, std::tuple<Ts...>>;
            std::get<0>(out) = parse_cell<T0>(fields[0], row_1based, 1);
        }
    };

} // namespace csv_detail

template <class... Ts>
class CSVParser {
public:
    using value_type = std::tuple<Ts...>;

    class Iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::tuple<Ts...>;
        using pointer = const value_type*;
        using reference = const value_type&;

        // end-итератор
        Iterator() : in_(nullptr), is_end_(true), row_(0) {}

        Iterator(std::istream* in, std::size_t skip, CSVConfig cfg)
            : in_(in), cfg_(cfg), is_end_(false), row_(0)
        {
            std::string dummy;
            for (std::size_t i = 0; i < skip; ++i) {
                if (!std::getline(*in_, dummy, cfg_.row_delim)) {
                    make_end();
                    return;
                }
                ++row_;
            }
            ++(*this); // читаем первую строку данных
        }

        reference operator*() const { return current_; }
        pointer operator->() const { return &current_; }

        Iterator& operator++() {
            if (!in_ || is_end_) return *this;

            std::string line;
            if (!std::getline(*in_, line, cfg_.row_delim)) {
                make_end();
                return *this;
            }
            ++row_; // 1-based относительно начала файла (с учётом пропуска)

            auto fields = csv_detail::split_line(line, cfg_, row_);

            constexpr std::size_t N = sizeof...(Ts);
            if (fields.size() != N) {
                throw CSVParseError(row_, 1,
                    csv_detail::make_msg(row_, 1,
                        "wrong number of columns: expected " + std::to_string(N) +
                        ", got " + std::to_string(fields.size())));
            }

            if constexpr (N > 0) {
                std::tuple<Ts...> tmp{};
                csv_detail::TupleBuilder<N - 1, Ts...>::fill(tmp, fields, row_);
                current_ = std::move(tmp);
            }
            else {
                current_ = value_type{};
            }

            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const Iterator& a, const Iterator& b) {
            return a.is_end_ == b.is_end_;
        }
        friend bool operator!=(const Iterator& a, const Iterator& b) {
            return !(a == b);
        }

    private:
        void make_end() {
            is_end_ = true;
            in_ = nullptr;
        }

        std::istream* in_ = nullptr;
        CSVConfig cfg_{};
        value_type current_{};
        bool is_end_ = true;
        std::size_t row_ = 0; // 1-based строки файла (после чтения)
    };

    CSVParser(std::istream& in, std::size_t skip_lines = 0, CSVConfig cfg = {})
        : in_(in), skip_(skip_lines), cfg_(cfg) {}

    Iterator begin() { return Iterator(&in_, skip_, cfg_); }
    Iterator end() { return Iterator(); }

private:
    std::istream& in_;
    std::size_t skip_;
    CSVConfig cfg_;
};
