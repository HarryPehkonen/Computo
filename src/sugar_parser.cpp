#include "sugar_parser.hpp"
#include <cctype>
#include <limits>
#include <set>
#include <vector>

namespace computo {

// ============================================================================
// Token types
// ============================================================================

enum class TokenType {
    // Literals
    Number, String, True, False, Null,
    // Identifiers & keywords
    Identifier, DollarInput, DollarInputs,
    Let, In, If, Then, Else, And, Or, Not,
    // Operators
    Plus, Minus, Star, Percent,
    Greater, Less, GreaterEq, LessEq, EqualEq, BangEq,
    // Delimiters
    LParen, RParen, LBracket, RBracket, LBrace, RBrace,
    Comma, Colon, Equals, Arrow,
    // Slash (tracked for whitespace sensitivity)
    Slash,
    // End
    Eof,
};

struct Token {
    TokenType type;
    std::string text;
    int line;
    int col;
    bool space_before; // was there whitespace before this token?
    bool space_after;  // was there whitespace after this token? (only meaningful for Slash)
};

// ============================================================================
// Tokenizer
// ============================================================================

class Tokenizer {
public:
    Tokenizer(const std::string& source) : src_(source) {
        skip_shebang();
    }

    struct State {
        size_t pos;
        int line;
        int col;
        bool had_whitespace;
    };

    auto save_state() const -> State {
        return {pos_, line_, col_, had_whitespace_};
    }

    void restore_state(const State& state) {
        pos_ = state.pos;
        line_ = state.line;
        col_ = state.col;
        had_whitespace_ = state.had_whitespace;
    }

    auto next() -> Token {
        skip_whitespace_and_comments();
        bool had_space = had_whitespace_;

        if (pos_ >= src_.size()) {
            return {TokenType::Eof, "", line_, col_, had_space, false};
        }

        int tok_line = line_;
        int tok_col = col_;
        char ch = src_[pos_];

        // String literal
        if (ch == '"') return lex_string(tok_line, tok_col, had_space);

        // Number literal
        if (std::isdigit(static_cast<unsigned char>(ch)) ||
            (ch == '.' && pos_ + 1 < src_.size() &&
             std::isdigit(static_cast<unsigned char>(src_[pos_ + 1])))) {
            return lex_number(tok_line, tok_col, had_space);
        }

        // $input / $inputs
        if (ch == '$') {
            return lex_dollar(tok_line, tok_col, had_space);
        }

        // Identifier or keyword
        if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
            return lex_identifier(tok_line, tok_col, had_space);
        }

        // Two-char operators
        if (pos_ + 1 < src_.size()) {
            char next = src_[pos_ + 1];
            if (ch == '=' && next == '>') { advance(); advance(); return {TokenType::Arrow, "=>", tok_line, tok_col, had_space, false}; }
            if (ch == '=' && next == '=') { advance(); advance(); return {TokenType::EqualEq, "==", tok_line, tok_col, had_space, false}; }
            if (ch == '!' && next == '=') { advance(); advance(); return {TokenType::BangEq, "!=", tok_line, tok_col, had_space, false}; }
            if (ch == '>' && next == '=') { advance(); advance(); return {TokenType::GreaterEq, ">=", tok_line, tok_col, had_space, false}; }
            if (ch == '<' && next == '=') { advance(); advance(); return {TokenType::LessEq, "<=", tok_line, tok_col, had_space, false}; }
        }

        // Single-char tokens
        advance();
        switch (ch) {
        case '+': return {TokenType::Plus, "+", tok_line, tok_col, had_space, false};
        case '-': return {TokenType::Minus, "-", tok_line, tok_col, had_space, false};
        case '*': return {TokenType::Star, "*", tok_line, tok_col, had_space, false};
        case '/': {
            // For slash, check if there's whitespace after (for ` / ` vs `/` distinction)
            bool has_space_after = pos_ < src_.size() &&
                (src_[pos_] == ' ' || src_[pos_] == '\t' || src_[pos_] == '\n' || src_[pos_] == '\r');
            return {TokenType::Slash, "/", tok_line, tok_col, had_space, has_space_after};
        }
        case '%': return {TokenType::Percent, "%", tok_line, tok_col, had_space, false};
        case '>': return {TokenType::Greater, ">", tok_line, tok_col, had_space, false};
        case '<': return {TokenType::Less, "<", tok_line, tok_col, had_space, false};
        case '(': return {TokenType::LParen, "(", tok_line, tok_col, had_space, false};
        case ')': return {TokenType::RParen, ")", tok_line, tok_col, had_space, false};
        case '[': return {TokenType::LBracket, "[", tok_line, tok_col, had_space, false};
        case ']': return {TokenType::RBracket, "]", tok_line, tok_col, had_space, false};
        case '{': return {TokenType::LBrace, "{", tok_line, tok_col, had_space, false};
        case '}': return {TokenType::RBrace, "}", tok_line, tok_col, had_space, false};
        case ',': return {TokenType::Comma, ",", tok_line, tok_col, had_space, false};
        case ':': return {TokenType::Colon, ":", tok_line, tok_col, had_space, false};
        case '=': return {TokenType::Equals, "=", tok_line, tok_col, had_space, false};
        default:
            throw SugarParseError(std::string("Unexpected character '") + ch + "'",
                                  tok_line, tok_col);
        }
    }

private:
    const std::string& src_;
    size_t pos_ = 0;
    int line_ = 1;
    int col_ = 1;
    bool had_whitespace_ = true; // start-of-input counts as "had whitespace"

    void advance() {
        if (pos_ < src_.size()) {
            if (src_[pos_] == '\n') {
                ++line_;
                col_ = 1;
            } else {
                ++col_;
            }
            ++pos_;
        }
    }

    char peek() const {
        return pos_ < src_.size() ? src_[pos_] : '\0';
    }

    void skip_shebang() {
        if (pos_ + 1 < src_.size() && src_[pos_] == '#' && src_[pos_ + 1] == '!') {
            while (pos_ < src_.size() && src_[pos_] != '\n') {
                advance();
            }
            if (pos_ < src_.size()) advance(); // consume newline
        }
    }

    void skip_whitespace_and_comments() {
        had_whitespace_ = false;
        while (pos_ < src_.size()) {
            char ch = src_[pos_];
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                had_whitespace_ = true;
                advance();
            } else if (ch == '-' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '-') {
                // Line comment: -- ...
                had_whitespace_ = true;
                while (pos_ < src_.size() && src_[pos_] != '\n') {
                    advance();
                }
            } else {
                break;
            }
        }
    }

    auto lex_string(int tok_line, int tok_col, bool had_space) -> Token {
        advance(); // skip opening "
        std::string value;
        while (pos_ < src_.size() && src_[pos_] != '"') {
            if (src_[pos_] == '\\') {
                advance();
                if (pos_ >= src_.size()) break;
                switch (src_[pos_]) {
                case '"': value += '"'; break;
                case '\\': value += '\\'; break;
                case '/': value += '/'; break;
                case 'b': value += '\b'; break;
                case 'f': value += '\f'; break;
                case 'n': value += '\n'; break;
                case 'r': value += '\r'; break;
                case 't': value += '\t'; break;
                case 'u': {
                    // Parse 4 hex digits
                    advance();
                    std::string hex;
                    for (int i = 0; i < 4 && pos_ < src_.size(); ++i) {
                        hex += src_[pos_];
                        advance();
                    }
                    unsigned int codepoint = 0;
                    for (char hc : hex) {
                        codepoint <<= 4;
                        if (hc >= '0' && hc <= '9') codepoint |= (hc - '0');
                        else if (hc >= 'a' && hc <= 'f') codepoint |= (hc - 'a' + 10);
                        else if (hc >= 'A' && hc <= 'F') codepoint |= (hc - 'A' + 10);
                    }
                    // UTF-8 encode
                    if (codepoint < 0x80) {
                        value += static_cast<char>(codepoint);
                    } else if (codepoint < 0x800) {
                        value += static_cast<char>(0xC0 | (codepoint >> 6));
                        value += static_cast<char>(0x80 | (codepoint & 0x3F));
                    } else {
                        value += static_cast<char>(0xE0 | (codepoint >> 12));
                        value += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        value += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    continue; // already advanced past the hex digits
                }
                default: value += src_[pos_]; break;
                }
            } else {
                value += src_[pos_];
            }
            advance();
        }
        if (pos_ < src_.size()) advance(); // skip closing "
        return {TokenType::String, value, tok_line, tok_col, had_space, false};
    }

    auto lex_number(int tok_line, int tok_col, bool had_space) -> Token {
        std::string num;
        while (pos_ < src_.size() && (std::isdigit(static_cast<unsigned char>(src_[pos_])) || src_[pos_] == '.')) {
            num += src_[pos_];
            advance();
        }
        // Handle scientific notation
        if (pos_ < src_.size() && (src_[pos_] == 'e' || src_[pos_] == 'E')) {
            num += src_[pos_];
            advance();
            if (pos_ < src_.size() && (src_[pos_] == '+' || src_[pos_] == '-')) {
                num += src_[pos_];
                advance();
            }
            while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
                num += src_[pos_];
                advance();
            }
        }
        return {TokenType::Number, num, tok_line, tok_col, had_space, false};
    }

    auto lex_dollar(int tok_line, int tok_col, bool had_space) -> Token {
        advance(); // skip $
        std::string word;
        while (pos_ < src_.size() &&
               (std::isalnum(static_cast<unsigned char>(src_[pos_])) || src_[pos_] == '_')) {
            word += src_[pos_];
            advance();
        }
        if (word == "input") return {TokenType::DollarInput, "$input", tok_line, tok_col, had_space, false};
        if (word == "inputs") return {TokenType::DollarInputs, "$inputs", tok_line, tok_col, had_space, false};
        // Unknown $ identifier - treat as an identifier named "$" + word
        throw SugarParseError("Unknown $ variable: $" + word, tok_line, tok_col);
    }

    auto lex_identifier(int tok_line, int tok_col, bool had_space) -> Token {
        std::string word;
        while (pos_ < src_.size() &&
               (std::isalnum(static_cast<unsigned char>(src_[pos_])) || src_[pos_] == '_')) {
            word += src_[pos_];
            advance();
        }

        // Keywords
        if (word == "let") return {TokenType::Let, word, tok_line, tok_col, had_space, false};
        if (word == "in") return {TokenType::In, word, tok_line, tok_col, had_space, false};
        if (word == "if") return {TokenType::If, word, tok_line, tok_col, had_space, false};
        if (word == "then") return {TokenType::Then, word, tok_line, tok_col, had_space, false};
        if (word == "else") return {TokenType::Else, word, tok_line, tok_col, had_space, false};
        if (word == "and") return {TokenType::And, word, tok_line, tok_col, had_space, false};
        if (word == "or") return {TokenType::Or, word, tok_line, tok_col, had_space, false};
        if (word == "not") return {TokenType::Not, word, tok_line, tok_col, had_space, false};
        if (word == "true") return {TokenType::True, word, tok_line, tok_col, had_space, false};
        if (word == "false") return {TokenType::False, word, tok_line, tok_col, had_space, false};
        if (word == "null") return {TokenType::Null, word, tok_line, tok_col, had_space, false};

        return {TokenType::Identifier, word, tok_line, tok_col, had_space, false};
    }
};

// ============================================================================
// Parser (Pratt / precedence climbing)
// ============================================================================

// Precedence levels for infix operators
static auto infix_precedence(TokenType type) -> int {
    switch (type) {
    case TokenType::Or: return 2;
    case TokenType::And: return 3;
    case TokenType::Greater: case TokenType::Less:
    case TokenType::GreaterEq: case TokenType::LessEq:
    case TokenType::EqualEq: case TokenType::BangEq:
        return 5;
    case TokenType::Plus: case TokenType::Minus:
        return 6;
    case TokenType::Star: case TokenType::Percent:
        return 7;
    case TokenType::Slash:
        return 7; // division (only when space_before)
    default: return 0;
    }
}

static auto token_to_op_name(TokenType type) -> std::string {
    switch (type) {
    case TokenType::Plus: return "+";
    case TokenType::Minus: return "-";
    case TokenType::Star: return "*";
    case TokenType::Slash: return "/";
    case TokenType::Percent: return "%";
    case TokenType::Greater: return ">";
    case TokenType::Less: return "<";
    case TokenType::GreaterEq: return ">=";
    case TokenType::LessEq: return "<=";
    case TokenType::EqualEq: return "==";
    case TokenType::BangEq: return "!=";
    case TokenType::And: return "and";
    case TokenType::Or: return "or";
    default: return "";
    }
}

static auto is_comparison_op(TokenType type) -> bool {
    return type == TokenType::Greater || type == TokenType::Less ||
           type == TokenType::GreaterEq || type == TokenType::LessEq ||
           type == TokenType::EqualEq || type == TokenType::BangEq;
}

// Check if slash has valid spacing: either both sides or neither
static void validate_slash_spacing(const Token& tok) {
    if (tok.type != TokenType::Slash) return;
    bool left = tok.space_before;
    bool right = tok.space_after;
    if (left != right) {
        throw SugarParseError(
            "Slash must have spaces on both sides (division: ` / `) or neither (path: `/`)",
            tok.line, tok.col);
    }
}

static auto is_infix_token(const Token& tok) -> bool {
    switch (tok.type) {
    case TokenType::Plus: case TokenType::Minus:
    case TokenType::Star: case TokenType::Percent:
    case TokenType::Greater: case TokenType::Less:
    case TokenType::GreaterEq: case TokenType::LessEq:
    case TokenType::EqualEq: case TokenType::BangEq:
    case TokenType::And: case TokenType::Or:
        return true;
    case TokenType::Slash:
        // Division requires spaces on both sides
        return tok.space_before && tok.space_after;
    default:
        return false;
    }
}

class Parser {
public:
    Parser(const std::string& source, const SugarParseOptions& opts)
        : tokenizer_(source), opts_(opts) {
        advance(); // prime the first token
    }

    auto parse_program() -> jsom::JsonDocument {
        auto result = parse_expression(0);
        if (current_.type != TokenType::Eof) {
            throw SugarParseError("Unexpected token '" + current_.text + "'",
                                  current_.line, current_.col);
        }
        return result;
    }

private:
    Tokenizer tokenizer_;
    SugarParseOptions opts_;
    Token current_{};

    void advance() {
        current_ = tokenizer_.next();
    }

    void expect(TokenType type, const std::string& what) {
        if (current_.type != type) {
            throw SugarParseError("Expected " + what + ", got '" + current_.text + "'",
                                  current_.line, current_.col);
        }
        advance();
    }

    // Lookahead: check if tokens from current position form lambda params
    // Pattern: Ident (, Ident)* ) =>
    // Returns true if it's a lambda, false otherwise
    // Does NOT consume tokens - uses save/restore
    auto is_lambda_ahead() -> bool {
        auto saved_tok = current_;
        auto saved_state = tokenizer_.save_state();

        // We're positioned after '(' - check for: [ident (, ident)*] ) =>
        // First, allow empty params: ) =>
        if (current_.type == TokenType::RParen) {
            advance();
            bool result = (current_.type == TokenType::Arrow);
            current_ = saved_tok;
            tokenizer_.restore_state(saved_state);
            return result;
        }

        // Must start with identifier
        if (current_.type != TokenType::Identifier) {
            current_ = saved_tok;
            tokenizer_.restore_state(saved_state);
            return false;
        }

        advance(); // consume first identifier

        // Continue with (, identifier)*
        while (current_.type == TokenType::Comma) {
            advance(); // consume ','
            if (current_.type != TokenType::Identifier) {
                current_ = saved_tok;
                tokenizer_.restore_state(saved_state);
                return false;
            }
            advance(); // consume identifier
        }

        // Must end with ) =>
        if (current_.type != TokenType::RParen) {
            current_ = saved_tok;
            tokenizer_.restore_state(saved_state);
            return false;
        }
        advance(); // consume ')'

        bool result = (current_.type == TokenType::Arrow);
        current_ = saved_tok;
        tokenizer_.restore_state(saved_state);
        return result;
    }

    // ---------------------------------------------------------------
    // Expression parsing (Pratt)
    // ---------------------------------------------------------------

    auto parse_expression(int min_prec) -> jsom::JsonDocument {
        auto left = parse_prefix();
        return parse_infix(std::move(left), min_prec);
    }

    auto parse_infix(jsom::JsonDocument left, int min_prec) -> jsom::JsonDocument {
        while (true) {
            // Validate slash spacing: must be symmetric
            if (current_.type == TokenType::Slash) {
                validate_slash_spacing(current_);
            }

            // Check for path access: slash with no spaces on either side
            if (current_.type == TokenType::Slash && !current_.space_before) {
                // This is path access on the left expression
                // Only valid if left is a variable access
                left = extend_path(std::move(left));
                continue;
            }

            // Check for function call: ( with no space before
            if (current_.type == TokenType::LParen && !current_.space_before) {
                left = parse_call(std::move(left));
                continue;
            }

            if (!is_infix_token(current_)) break;

            int prec = infix_precedence(current_.type);
            if (prec < min_prec) break;

            TokenType op_type = current_.type;
            std::string op_name = token_to_op_name(op_type);
            advance(); // consume operator

            // Right operand: bind tighter (left-associative)
            auto right = parse_expression(prec + 1);

            // Variadic flattening: if same operator appears consecutively,
            // extend the array instead of nesting
            if (left.is_array() && !left.empty() && left[0].is_string() &&
                left[0].as<std::string>() == op_name) {
                left.push_back(std::move(right));
            }
            // For comparison chaining: a > b > c -> [">", a, b, c]
            else if (is_comparison_op(op_type) && left.is_array() && !left.empty() &&
                     left[0].is_string() && is_comparison_op_name(left[0].as<std::string>()) &&
                     left[0].as<std::string>() == op_name) {
                left.push_back(std::move(right));
            }
            else {
                auto node = jsom::JsonDocument::make_array();
                node.push_back(jsom::JsonDocument(op_name));
                node.push_back(std::move(left));
                node.push_back(std::move(right));
                left = std::move(node);
            }
        }

        return left;
    }

    static auto is_comparison_op_name(const std::string& name) -> bool {
        return name == ">" || name == "<" || name == ">=" || name == "<=" ||
               name == "==" || name == "!=";
    }

    // ---------------------------------------------------------------
    // Prefix / atom parsing
    // ---------------------------------------------------------------

    auto parse_prefix() -> jsom::JsonDocument {
        switch (current_.type) {
        case TokenType::Number: return parse_number();
        case TokenType::String: return parse_string();
        case TokenType::True: { advance(); return jsom::JsonDocument(true); }
        case TokenType::False: { advance(); return jsom::JsonDocument(false); }
        case TokenType::Null: { advance(); return jsom::JsonDocument(nullptr); }
        case TokenType::Not: return parse_not();
        case TokenType::Minus: return parse_unary_minus();
        case TokenType::Let: return parse_let();
        case TokenType::If: return parse_if();
        case TokenType::LParen: return parse_paren_or_lambda();
        case TokenType::LBracket: return parse_array_literal();
        case TokenType::LBrace: return parse_object_literal();
        case TokenType::DollarInput: return parse_dollar_input();
        case TokenType::DollarInputs: return parse_dollar_inputs();
        case TokenType::Identifier: return parse_identifier();
        default:
            throw SugarParseError("Unexpected token '" + current_.text + "'",
                                  current_.line, current_.col);
        }
    }

    auto parse_number() -> jsom::JsonDocument {
        std::string text = current_.text;
        advance();
        // Integer or float?
        if (text.find('.') != std::string::npos || text.find('e') != std::string::npos ||
            text.find('E') != std::string::npos) {
            return jsom::JsonDocument(std::stod(text));
        }
        // Try integer
        long long val = std::stoll(text);
        if (val >= std::numeric_limits<int>::min() && val <= std::numeric_limits<int>::max()) {
            return jsom::JsonDocument(static_cast<int>(val));
        }
        return jsom::JsonDocument::from_lazy_number(text);
    }

    auto parse_string() -> jsom::JsonDocument {
        auto val = jsom::JsonDocument(current_.text);
        advance();
        return val;
    }

    auto parse_not() -> jsom::JsonDocument {
        advance(); // consume 'not'
        auto operand = parse_expression(5); // bind tighter than comparison
        auto node = jsom::JsonDocument::make_array();
        node.push_back(jsom::JsonDocument("not"));
        node.push_back(std::move(operand));
        return node;
    }

    auto parse_unary_minus() -> jsom::JsonDocument {
        advance(); // consume '-'
        auto operand = parse_expression(8); // unary neg binds very tight
        auto node = jsom::JsonDocument::make_array();
        node.push_back(jsom::JsonDocument("-"));
        node.push_back(std::move(operand));
        return node;
    }

    auto parse_let() -> jsom::JsonDocument {
        advance(); // consume 'let'
        auto bindings = jsom::JsonDocument::make_array();

        // Parse bindings: name = expr [, name = expr ...]
        while (current_.type == TokenType::Identifier) {
            std::string name = current_.text;
            advance();
            expect(TokenType::Equals, "'='");
            auto value = parse_expression(0);

            auto binding = jsom::JsonDocument::make_array();
            binding.push_back(jsom::JsonDocument(name));
            binding.push_back(std::move(value));
            bindings.push_back(std::move(binding));

            // Optional comma between bindings
            if (current_.type == TokenType::Comma) {
                advance();
            }
        }

        expect(TokenType::In, "'in'");
        auto body = parse_expression(0);

        auto node = jsom::JsonDocument::make_array();
        node.push_back(jsom::JsonDocument("let"));
        node.push_back(std::move(bindings));
        node.push_back(std::move(body));
        return node;
    }

    auto parse_if() -> jsom::JsonDocument {
        advance(); // consume 'if'
        auto condition = parse_expression(0);
        expect(TokenType::Then, "'then'");
        auto then_branch = parse_expression(0);
        expect(TokenType::Else, "'else'");
        auto else_branch = parse_expression(0);

        auto node = jsom::JsonDocument::make_array();
        node.push_back(jsom::JsonDocument("if"));
        node.push_back(std::move(condition));
        node.push_back(std::move(then_branch));
        node.push_back(std::move(else_branch));
        return node;
    }

    auto parse_paren_or_lambda() -> jsom::JsonDocument {
        advance(); // consume '('

        // Use lookahead to determine if this is a lambda
        if (is_lambda_ahead()) {
            return parse_lambda_params_and_body();
        }

        // It's a grouped expression
        if (current_.type == TokenType::RParen) {
            throw SugarParseError("Empty parentheses are only valid for zero-arg lambdas: () =>",
                                  current_.line, current_.col);
        }

        auto expr = parse_expression(0);
        expect(TokenType::RParen, "')'");
        return parse_infix(std::move(expr), 0);
    }

    // Called when is_lambda_ahead() returned true - parse lambda params and body
    auto parse_lambda_params_and_body() -> jsom::JsonDocument {
        std::vector<std::string> params;

        // Collect params (could be empty for () =>)
        if (current_.type == TokenType::Identifier) {
            params.push_back(current_.text);
            advance();
            while (current_.type == TokenType::Comma) {
                advance(); // consume ','
                if (current_.type != TokenType::Identifier) {
                    throw SugarParseError("Expected parameter name",
                                          current_.line, current_.col);
                }
                params.push_back(current_.text);
                advance();
            }
        }

        expect(TokenType::RParen, "')'");
        expect(TokenType::Arrow, "'=>'");

        auto body = parse_expression(0);

        auto params_arr = jsom::JsonDocument::make_array();
        for (const auto& p : params) {
            params_arr.push_back(jsom::JsonDocument(p));
        }

        auto node = jsom::JsonDocument::make_array();
        node.push_back(jsom::JsonDocument("lambda"));
        node.push_back(std::move(params_arr));
        node.push_back(std::move(body));
        return node;
    }

    auto parse_array_literal() -> jsom::JsonDocument {
        advance(); // consume '['
        auto arr = jsom::JsonDocument::make_array();
        if (current_.type != TokenType::RBracket) {
            arr.push_back(parse_expression(0));
            while (current_.type == TokenType::Comma) {
                advance();
                if (current_.type == TokenType::RBracket) break; // trailing comma
                arr.push_back(parse_expression(0));
            }
        }
        expect(TokenType::RBracket, "']'");

        // Wrap in array key: {"array": [...]}
        return jsom::JsonDocument({{opts_.array_key, std::move(arr)}});
    }

    auto parse_object_literal() -> jsom::JsonDocument {
        advance(); // consume '{'
        auto obj = jsom::JsonDocument::make_object();
        if (current_.type != TokenType::RBrace) {
            parse_object_entry(obj);
            while (current_.type == TokenType::Comma) {
                advance();
                if (current_.type == TokenType::RBrace) break; // trailing comma
                parse_object_entry(obj);
            }
        }
        expect(TokenType::RBrace, "'}'");
        return obj;
    }

    void parse_object_entry(jsom::JsonDocument& obj) {
        std::string key;
        if (current_.type == TokenType::Identifier) {
            key = current_.text;
            advance();
        } else if (current_.type == TokenType::String) {
            key = current_.text;
            advance();
        } else {
            throw SugarParseError("Expected object key", current_.line, current_.col);
        }
        expect(TokenType::Colon, "':'");
        auto value = parse_expression(0);
        obj.set(key, std::move(value));
    }

    auto parse_dollar_input() -> jsom::JsonDocument {
        advance(); // consume '$input'
        auto node = jsom::JsonDocument::make_array();
        node.push_back(jsom::JsonDocument("$input"));

        // Check for path access
        if (current_.type == TokenType::Slash && !current_.space_before) {
            validate_slash_spacing(current_);
            advance(); // consume '/'
            std::string path = "/" + parse_path_segments();
            node.push_back(jsom::JsonDocument(path));
        }
        return node;
    }

    auto parse_dollar_inputs() -> jsom::JsonDocument {
        advance(); // consume '$inputs'
        auto node = jsom::JsonDocument::make_array();
        node.push_back(jsom::JsonDocument("$inputs"));

        // Check for path access
        if (current_.type == TokenType::Slash && !current_.space_before) {
            validate_slash_spacing(current_);
            advance(); // consume '/'
            std::string path = "/" + parse_path_segments();
            node.push_back(jsom::JsonDocument(path));
        }
        return node;
    }

    auto parse_identifier() -> jsom::JsonDocument {
        std::string name = current_.text;
        advance();

        // Check for function call: name(args...)
        if (current_.type == TokenType::LParen && !current_.space_before) {
            return parse_function_call(name);
        }

        // Check for path access: name/path
        if (current_.type == TokenType::Slash && !current_.space_before) {
            validate_slash_spacing(current_); // Ensure symmetric: no spaces on either side
            advance(); // consume '/'
            std::string path = "/" + name + "/" + parse_path_segments();
            auto node = jsom::JsonDocument::make_array();
            node.push_back(jsom::JsonDocument("$"));
            node.push_back(jsom::JsonDocument(path));
            return node;
        }

        // Bare identifier -> variable access
        return make_variable(name);
    }

    auto parse_function_call(const std::string& name) -> jsom::JsonDocument {
        advance(); // consume '('
        auto node = jsom::JsonDocument::make_array();
        node.push_back(jsom::JsonDocument(name));
        if (current_.type != TokenType::RParen) {
            node.push_back(parse_expression(0));
            while (current_.type == TokenType::Comma) {
                advance();
                if (current_.type == TokenType::RParen) break; // trailing comma
                node.push_back(parse_expression(0));
            }
        }
        expect(TokenType::RParen, "')'");
        return node;
    }

    auto make_variable(const std::string& name) -> jsom::JsonDocument {
        auto node = jsom::JsonDocument::make_array();
        node.push_back(jsom::JsonDocument("$"));
        node.push_back(jsom::JsonDocument("/" + name));
        return node;
    }

    // Parse path segments after the first /
    // e.g., in "x/name/age" after consuming x and /, parse "name/age" -> "name/age"
    auto parse_path_segments() -> std::string {
        std::string path;
        // First segment must be identifier or number
        if (current_.type == TokenType::Identifier || current_.type == TokenType::Number) {
            path += current_.text;
            advance();
        } else {
            throw SugarParseError("Expected path segment", current_.line, current_.col);
        }

        // Continue with /segment
        while (current_.type == TokenType::Slash && !current_.space_before) {
            advance(); // consume '/'
            if (current_.type == TokenType::Identifier || current_.type == TokenType::Number) {
                path += '/';
                path += current_.text;
                advance();
            } else {
                throw SugarParseError("Expected path segment after '/'",
                                      current_.line, current_.col);
            }
        }
        return path;
    }

    // Extend an existing variable path: if left is ["$", "/x"], and we see /name,
    // extend to ["$", "/x/name"]
    auto extend_path(jsom::JsonDocument left) -> jsom::JsonDocument {
        advance(); // consume '/'
        std::string extra = parse_path_segments();

        if (left.is_array() && left.size() == 2 && left[0].is_string()) {
            const auto& op = left[0].as<std::string>();
            if (op == "$" && left[1].is_string()) {
                std::string existing = left[1].as<std::string>();
                auto node = jsom::JsonDocument::make_array();
                node.push_back(jsom::JsonDocument("$"));
                node.push_back(jsom::JsonDocument(existing + "/" + extra));
                return node;
            }
            if ((op == "$input" || op == "$inputs") && left.size() == 2 && left[1].is_string()) {
                std::string existing = left[1].as<std::string>();
                auto node = jsom::JsonDocument::make_array();
                node.push_back(jsom::JsonDocument(op));
                node.push_back(jsom::JsonDocument(existing + "/" + extra));
                return node;
            }
            if ((op == "$input" || op == "$inputs") && left.size() == 1) {
                auto node = jsom::JsonDocument::make_array();
                node.push_back(jsom::JsonDocument(op));
                node.push_back(jsom::JsonDocument("/" + extra));
                return node;
            }
        }

        // Can't extend path on non-variable expression — treat as division
        // Actually, by this point we already consumed the /, so let's make it an error
        throw SugarParseError("Path access on non-variable expression",
                              current_.line, current_.col);
    }

    auto parse_call(jsom::JsonDocument left) -> jsom::JsonDocument {
        // left is the function expression (should be a variable reference to function name)
        // Extract the function name from the variable access
        std::string func_name;
        if (left.is_array() && left.size() == 2 && left[0].is_string() &&
            left[0].as<std::string>() == "$" && left[1].is_string()) {
            const auto& path = left[1].as<std::string>();
            // "/funcname" -> "funcname"
            if (!path.empty() && path[0] == '/' && path.find('/', 1) == std::string::npos) {
                func_name = path.substr(1);
            }
        }

        if (func_name.empty()) {
            throw SugarParseError("Function call on non-identifier expression",
                                  current_.line, current_.col);
        }

        return parse_function_call(func_name);
    }
};

// ============================================================================
// Public API
// ============================================================================

auto SugarParser::parse(const std::string& source,
                        const SugarParseOptions& options) -> jsom::JsonDocument {
    Parser parser(source, options);
    return parser.parse_program();
}

} // namespace computo
