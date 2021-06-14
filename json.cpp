#include "json.h"

#include <iterator>

namespace json {

    namespace {
        using namespace std::literals;

        Node LoadNode(std::istream& input);
        Node LoadString(std::istream& input);

        std::string LoadLiteral(std::istream& input) {

            std::string str;
            while (std::isalpha(input.peek())) {
                str.push_back(static_cast<char>(input.get()));
            }

            return str;
        }

        Node LoadArray(std::istream& input) {

            std::vector<Node> result;
            for (char current_char; input >> current_char && current_char != ']';) {
                if (current_char != ',') {
                    input.putback(current_char);
                }
                result.push_back(LoadNode(input));
            }

            if (!input) {
                throw ParsingError("Array parsing error"s);
            }

            return Node(std::move(result));
        }

        Node LoadDict(std::istream& input) {

            Dict dict;
            for (char current_char; input >> current_char && current_char != '}';) {
                if (current_char == '"') {
                    std::string key = LoadString(input).AsString();

                    if (input >> current_char && current_char == ':') {
                        if (dict.find(key) != dict.end()) {
                            throw ParsingError("Duplicate key '"s + key + "' have been found");
                        }

                        dict.emplace(std::move(key), LoadNode(input));
                    } else {
                        throw ParsingError(": is expected but '"s + current_char + "' has been found"s);
                    }
                } else if (current_char != ',') {
                    throw ParsingError(R"(',' is expected but ')"s + current_char + "' has been found"s);
                }
            }

            if (!input) {
                throw ParsingError("Dictionary parsing error"s);
            }

            return Node(std::move(dict));
        }

        Node LoadString(std::istream& input) {
            auto it = std::istreambuf_iterator<char>(input);
            auto end = std::istreambuf_iterator<char>();

            std::string str;
            while (true) {
                if (it == end) {
                    throw ParsingError("String parsing error");
                }
                const char current_char = *it;
                if (current_char == '"') {
                    ++it;
                    break;
                } else if (current_char == '\\') {
                    ++it;
                    if (it == end) {
                        throw ParsingError("String parsing error");
                    }
                    const char escaped_char = *(it);
                    switch (escaped_char) {
                    case 'n':
                        str.push_back('\n');
                        break;
                    case 't':
                        str.push_back('\t');
                        break;
                    case 'r':
                        str.push_back('\r');
                        break;
                    case '"':
                        str.push_back('"');
                        break;
                    case '\\':
                        str.push_back('\\');
                        break;
                    default:
                        throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                    }
                } else if (current_char == '\n' || current_char == '\r') {
                    throw ParsingError("Unexpected end of line"s);
                } else {
                    str.push_back(current_char);
                }
                ++it;
            }

            return Node(std::move(str));
        }

        Node LoadBool(std::istream& input) {
            const auto str = LoadLiteral(input);

            if (str == "true"sv) {
                return Node{ true };
            } else if (str == "false"sv) {
                return Node{ false };
            } else {
                throw ParsingError("Failed to parse '"s + str + "' as bool"s);
            }
        }

        Node LoadNull(std::istream& input) {
            if (auto literal = LoadLiteral(input); literal == "null"sv) {
                return Node{ nullptr };
            } else {
                throw ParsingError("Failed to parse '"s + literal + "' as null"s);
            }
        }

        Node LoadNumber(std::istream& input) {
            std::string parsed_num;

            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
            };

            auto read_digits = [&input, read_char] {
                if (!std::isdigit(input.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input.peek())) {
                    read_char();
                }
            };

            if (input.peek() == '-') {
                read_char();
            }

            if (input.peek() == '0') {
                read_char();
            } else {
                read_digits();
            }

            bool is_int = true;
            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_int = false;
            }

            if (int current_char = input.peek(); current_char == 'e' || current_char == 'E') {
                read_char();
                if (current_char = input.peek(); current_char == '+' || current_char == '-') {
                    read_char();
                }

                read_digits();
                is_int = false;
            }

            try {
                if (is_int) {
                    try {
                        return std::stoi(parsed_num);
                    } catch (...) {
                    }
                }

                return std::stod(parsed_num);
            } catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        Node LoadNode(std::istream& input) {

            char current_char;
            if (!(input >> current_char)) {
                throw ParsingError("Unexpected EOF"s);
            }
            switch (current_char) {
            case '[':
                return LoadArray(input);
            case '{':
                return LoadDict(input);
            case '"':
                return LoadString(input);
            case 't':
                input.putback(current_char);
                return LoadBool(input);
            case 'f':
                input.putback(current_char);
                return LoadBool(input);
            case 'n':
                input.putback(current_char);
                return LoadNull(input);
            default:
                input.putback(current_char);
                return LoadNumber(input);
            }
        }

        struct PrintContext {
            std::ostream& out;
            int indent_step = 4;
            int indent = 0;

            void PrintIndent() const {
                for (int i = 0; i < indent; ++i) {
                    out.put(' ');
                }
            }

            PrintContext Indented() const {
                return { out, indent_step, indent_step + indent };
            }
        };

        void PrintNode(const Node& value, const PrintContext& ctx);

        template <typename Value>
        void PrintValue(const Value& value, const PrintContext& ctx) {
            ctx.out << value;
        }

        void PrintString(const std::string& value, std::ostream& out) {
            out.put('"');
            for (const char current_char : value) {
                switch (current_char) {
                case '\r':
                    out << "\\r"sv;
                    break;
                case '\n':
                    out << "\\n"sv;
                    break;
                case '"':
                    out << "\\\""sv;
                    break;
                case '\\':
                    out << "\\\\"sv;
                    break;
                default:
                    out.put(current_char);
                    break;
                }
            }
            out.put('"');
        }

        template <>
        void PrintValue<std::string>(const std::string& value, const PrintContext& ctx) {
            PrintString(value, ctx.out);
        }

        template <>
        void PrintValue<std::nullptr_t>(const std::nullptr_t&, const PrintContext& ctx) {
            ctx.out << "null"sv;
        }

        template <>
        void PrintValue<bool>(const bool& value, const PrintContext& ctx) {
            ctx.out << (value ? "true"sv : "false"sv);
        }

        template <>
        void PrintValue<Array>(const Array& nodes, const PrintContext& ctx) {
            std::ostream& out = ctx.out;

            out << "[\n"sv;

            bool first = true;
            auto inner_ctx = ctx.Indented();
            for (const Node& node : nodes) {
                if (first) {
                    first = false;
                } else {
                    out << ",\n"sv;
                }

                inner_ctx.PrintIndent();
                PrintNode(node, inner_ctx);
            }

            out.put('\n');
            ctx.PrintIndent();
            out.put(']');
        }

        template <>
        void PrintValue<Dict>(const Dict& nodes, const PrintContext& ctx) {
            std::ostream& out = ctx.out;

            out << "{\n"sv;

            bool first = true;
            auto inner_ctx = ctx.Indented();
            for (const auto& [key, node] : nodes) {
                if (first) {
                    first = false;
                } else {
                    out << ",\n"sv;
                }

                inner_ctx.PrintIndent();
                PrintString(key, ctx.out);
                out << ": "sv;
                PrintNode(node, inner_ctx);
            }

            out.put('\n');
            ctx.PrintIndent();
            out.put('}');
        }

        void PrintNode(const Node& node, const PrintContext& ctx) {
            std::visit(
                [&ctx](const auto& value) {
                    PrintValue(value, ctx);
                },
                node.GetValue());
        }

    } // namespace

    Document Load(std::istream& input) {
        return Document{ LoadNode(input) };
    }

    void Print(const Document& doc, std::ostream& output) {
        PrintNode(doc.GetRoot(), PrintContext{ output });
    }

} // namespace json