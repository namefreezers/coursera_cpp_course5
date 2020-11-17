#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>

using namespace std;

namespace Parse {

    bool operator==(const Token &lhs, const Token &rhs) {
        using namespace TokenType;

        if (lhs.index() != rhs.index()) {
            return false;
        }
        if (lhs.Is<Char>()) {
            return lhs.As<Char>().value == rhs.As<Char>().value;
        } else if (lhs.Is<Number>()) {
            return lhs.As<Number>().value == rhs.As<Number>().value;
        } else if (lhs.Is<String>()) {
            return lhs.As<String>().value == rhs.As<String>().value;
        } else if (lhs.Is<Id>()) {
            return lhs.As<Id>().value == rhs.As<Id>().value;
        } else {
            return true;
        }
    }

    std::ostream &operator<<(std::ostream &os, const Token &rhs) {
        using namespace TokenType;

#define VALUED_OUTPUT(type) \
  if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

        VALUED_OUTPUT(Number);
        VALUED_OUTPUT(Id);
        VALUED_OUTPUT(String);
        VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

        UNVALUED_OUTPUT(Class);
        UNVALUED_OUTPUT(Return);
        UNVALUED_OUTPUT(If);
        UNVALUED_OUTPUT(Else);
        UNVALUED_OUTPUT(Def);
        UNVALUED_OUTPUT(Newline);
        UNVALUED_OUTPUT(Print);
        UNVALUED_OUTPUT(Indent);
        UNVALUED_OUTPUT(Dedent);
        UNVALUED_OUTPUT(And);
        UNVALUED_OUTPUT(Or);
        UNVALUED_OUTPUT(Not);
        UNVALUED_OUTPUT(Eq);
        UNVALUED_OUTPUT(NotEq);
        UNVALUED_OUTPUT(LessOrEq);
        UNVALUED_OUTPUT(GreaterOrEq);
        UNVALUED_OUTPUT(None);
        UNVALUED_OUTPUT(True);
        UNVALUED_OUTPUT(False);
        UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

        return os << "Unknown token :(";
    }

    Lexer::Lexer(std::istream &input) : is(input) {
        while (getline(is, cur_line) && cur_line.find_first_not_of(' ') == string::npos);
        if (!is) {
            cur_token = TokenType::Eof{};
            return;
        }
        cur_line_view = cur_line;
        cur_token = NextToken();
    }

    const Token &Lexer::CurrentToken() const {
        return cur_token;
    }


    Token Lexer::NextToken() {
        Token res;

        if ((cur_token.Is<TokenType::Newline>() || cur_token.Is<TokenType::Dedent>() ) && !indents.empty()) {
            if (!is) {
                indents.pop();
                res = TokenType::Dedent();
                cur_token = res;
                return res;
            }
            if (int cur_ind = cur_line_view.find_first_not_of(' '); cur_ind < indents.top()) {
                indents.pop();
                res = TokenType::Dedent();
                cur_token = res;
                return res;
            } else {
                cur_line_view.remove_prefix(indents.top());
            }
        }

        if (!is) {
            res = TokenType::Eof();
            cur_token = res;
            return res;
        }

        if (cur_line_view.empty()) {
            res = TokenType::Newline{};
            while (getline(is, cur_line) && cur_line.find_first_not_of(' ') == string::npos);
            if (!is) {
                cur_line_view = string_view();
            } else {
                cur_line_view = cur_line;
            }

            cur_token = res;
            return res;
        }

        char c = cur_line_view.at(0);

        if (c == ' ') {
            size_t lex_end_idx = cur_line_view.find_first_not_of(' ');

            indents.push(indents.empty() ? lex_end_idx : indents.top() + lex_end_idx);

            cur_line_view.remove_prefix(lex_end_idx);
            res = TokenType::Indent{};
            cur_token = res;
            return res;


        }

        if (isalpha(c) || c == '_') {
            size_t lex_end_idx = cur_line_view.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
            string_view lex_to_ret = cur_line_view.substr(0, lex_end_idx);
            if (auto it = keywords.find(lex_to_ret); it != keywords.end()) {
                res = it->second;
            } else {
                res = TokenType::Id{string(lex_to_ret)};
            }

            cur_line_view.remove_prefix(lex_end_idx != string_view::npos ? lex_end_idx : cur_line_view.size());
            size_t space_end_idx = cur_line_view.find_first_not_of(' ');
            cur_line_view.remove_prefix(space_end_idx != string_view::npos ? space_end_idx : cur_line_view.size());
        } else if (c >= '0' && c <= '9') {
            size_t lex_end_idx = cur_line_view.find_first_not_of("0123456789");
            string_view lex_to_ret = cur_line_view.substr(0, lex_end_idx);
            res = TokenType::Number{stoi(string(lex_to_ret))};

            cur_line_view.remove_prefix(lex_end_idx != string_view::npos ? lex_end_idx : cur_line_view.size());
            size_t space_end_idx = cur_line_view.find_first_not_of(' ');
            cur_line_view.remove_prefix(space_end_idx != string_view::npos ? space_end_idx : cur_line_view.size());
        } else if (c == '"' || c == '\'') {
            size_t lex_end_idx = cur_line_view.find(c, 1) + 1;
            string_view lex_to_ret = cur_line_view.substr(0, lex_end_idx);
            res = TokenType::String{string(lex_to_ret.substr(1, lex_end_idx - 2))};

            cur_line_view.remove_prefix(lex_end_idx != string_view::npos ? lex_end_idx : cur_line_view.size());
            size_t space_end_idx = cur_line_view.find_first_not_of(' ');
            cur_line_view.remove_prefix(space_end_idx != string_view::npos ? space_end_idx : cur_line_view.size());
        } else {


            if (c == '=' && cur_line_view.size() > 1 && cur_line_view[1] == '=') {
                res = TokenType::Eq{};
                cur_line_view.remove_prefix(2);
            } else if (c == '!' && cur_line_view.size() > 1 && cur_line_view[1] == '=') {
                res = TokenType::NotEq{};
                cur_line_view.remove_prefix(2);
            } else if (c == '<' && cur_line_view.size() > 1 && cur_line_view[1] == '=') {
                res = TokenType::LessOrEq{};
                cur_line_view.remove_prefix(2);
            } else if (c == '>' && cur_line_view.size() > 1 && cur_line_view[1] == '=') {
                res = TokenType::GreaterOrEq{};
                cur_line_view.remove_prefix(2);
            } else {
                res = TokenType::Char{c};
                cur_line_view.remove_prefix(1);
            }

            size_t space_end_idx = cur_line_view.find_first_not_of(' ');
            cur_line_view.remove_prefix(space_end_idx != string_view::npos ? space_end_idx : cur_line_view.size());
        }

        cur_token = res;
        return res;

    }

    std::unordered_map<std::string_view, Token> Lexer::keywords = {
            {"class",  TokenType::Class{}},
            {"return", TokenType::Return{}},
            {"if",     TokenType::If{}},
            {"else",   TokenType::Else{}},
            {"def",    TokenType::Def{}},
            {"print",  TokenType::Print{}},
            {"or",     TokenType::Or{}},
            {"and",    TokenType::And{}},
            {"not",    TokenType::Not{}},
            {"None",   TokenType::None{}},
            {"True",   TokenType::True{}},
            {"False",  TokenType::False{}},
    };


} /* namespace Parse */
