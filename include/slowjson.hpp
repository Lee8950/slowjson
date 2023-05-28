#ifndef ECL_SLOWJSON_HPP
#define ECL_SLOWJSON_HPP

#include <exception>
#include <fstream>
#include <iostream>
#include <stack>
#include <string>
#include <variant>
#include <vector>

/**
 * @brief Tokenize -> Analysis -> Generate Json object
 */
namespace ecl {

enum valuetype {
    LBRACE = 0,    //   {
    RBRACE = 1,    // } Object
    LBRACKET = 2,  // [
    RBRACKET = 3,  // ] Array
    COMMA = 4,     // ,
    COLON = 5,     // :
    QUOTATION = 6, // "
    FLOAT = 7,     // double
    INTEGER = 8,   // int
    STRING = 9,    // std::string
    BOOLEAN = 10,
    OBJECT = 11,
    ARRAY = 12,
    JSONNULL = 13,
    TOP = 14, // father of all nodes
    PARSE_INIT = 15,
    PARSE_NAME_WAIT = 16,
    PARSE_VALUE_WAIT = 17,
    PARSE_COMMA_WAIT = 18,
    PARSE_COLON_WAIT = 19,
    PARSE_QUOTATION_WAIT = 20,
    PARSE_OBJECT_INIT_CONTENT_WAIT = 21,
    PARSE_ARRAY_INIT_CONTENT_WAIT = 22,
    PARSE_ARRAY_COMMA_WAIT = 23,
    PARSE_ARRAY_VALUE_WAIT = 24,
    TOKENIZE_IDLE = 25,
    TOKENIZE_NUMBER_WAIT = 26,
    TOKENIZE_STRING_WAIT = 27,
    TOKENIZE_FALSE_2_WAIT = 28,
    TOKENIZE_FALSE_3_WAIT = 29,
    TOKENIZE_FALSE_4_WAIT = 30,
    TOKENIZE_FALSE_5_WAIT = 31,
    TOKENIZE_TRUE_2_WAIT = 32,
    TOKENIZE_TRUE_3_WAIT = 33,
    TOKENIZE_TRUE_4_WAIT = 34,
};

struct jsonobj {
    jsonobj *next;  // Same-level json node
    jsonobj *child; // Next-level json node
    valuetype type;
    std::string name;
    std::variant<int64_t, bool, double, std::string> obj;
    jsonobj() : next(nullptr), child(nullptr), type(JSONNULL) {}
};

/**
 * @brief Token definition.
 * No need to generate jsonobj here.
 */
class Token {
public:
    valuetype token_type;
    std::string token_value;

public:
    Token(std::string v, valuetype t) {
        token_type = t;
        token_value = v;
    }
};

class json_storage {
private:
    jsonobj parsed_obj;

    std::string json_material;

    std::vector<Token> token_stream;

public:
    json_storage() {
        parsed_obj.child = nullptr;
        parsed_obj.next = nullptr;
    }

    void
    read(std::string material) {
        json_material.clear();
        json_material = material;
    }

    void
    read(std::fstream &material) {
        if (!material.is_open())
            throw std::runtime_error("bad fstream");
        material.seekg(std::ios::beg);
        json_material.clear();
        char tmp;
        while (1) {
            material.get(tmp);
            if (material.eof())
                break;
            json_material = json_material + tmp;
        }
    }

    void
    read_token_stream() {
        for (auto &v : token_stream) {
            switch (v.token_type) {
            case LBRACE:
                std::cout << "LBRACE";
                break;
            case RBRACE:
                std::cout << "RBRACE";
                break;
            case LBRACKET:
                std::cout << "LBRACKET";
                break;
            case RBRACKET:
                std::cout << "RBRACKET";
                break;
            case COMMA:
                std::cout << "COMMA";
                break;
            case COLON:
                std::cout << "COLON";
                break;
            case FLOAT:
                std::cout << "FLOAT";
                break;
            case INTEGER:
                std::cout << "INTEGER";
                break;
            case STRING:
                std::cout << "STRING";
                break;
            case BOOLEAN:
                std::cout << "BOOLEAN";
                break;
            default:
                throw std::runtime_error("Unexpected token tokenized.\n");
            }
            std::cout << ' ';
        }
    }

    // tokenize text. First step of processing raw json text.
    void
    tokenize() {
        // Tokenizing can be represented as status machine.
        // In this machine we mainly focus on these syntaxs:
        //  (a) LBRACE.
        //  (b) RBRACE.
        //  (c) LBRACKET.
        //  (d) RBRACKET.
        //  (e) COMMA.
        //  (f) COLON.
        //  (g) FLOAT.
        //  (h) INTEGER.
        //  (i) BOOLEAN.
        //  (j) STRING.
        //  (k) JSONNULL.
        valuetype machine_status = TOKENIZE_IDLE;
        std::string number_tmp;
        valuetype number_type_tmp;
        std::string string_tmp;
        for (auto &c : json_material) {
            switch (machine_status) {
            case TOKENIZE_IDLE: {
                switch (c) {
                case '\t':
                case '\n':
                case ' ': {
                    continue;
                } break;
                case '{': {
                    token_stream.push_back(Token("{", LBRACE));
                } break;
                case '}': {
                    token_stream.push_back(Token("}", RBRACE));
                } break;
                case '[': {
                    token_stream.push_back(Token("[", LBRACKET));
                } break;
                case ']': {
                    token_stream.push_back(Token("]", RBRACKET));
                } break;
                case ':': {
                    token_stream.push_back(Token(":", COLON));
                } break;
                case ',': {
                    token_stream.push_back(Token(",", COMMA));
                } break;
                case '"': {
                    string_tmp.clear();
                    machine_status = TOKENIZE_STRING_WAIT;
                } break;
                case 'f': {
                    machine_status = TOKENIZE_FALSE_2_WAIT;
                } break;
                case 't': {
                    machine_status = TOKENIZE_TRUE_2_WAIT;
                } break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9': {
                    number_tmp.clear();
                    number_tmp += c;
                    // Integer is the init type. When meet '.', transform.
                    number_type_tmp = INTEGER;
                    machine_status = TOKENIZE_NUMBER_WAIT;
                } break;
                }
            } break;
            case TOKENIZE_NUMBER_WAIT: {
                switch (c) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9': {
                    number_tmp += c;
                } break;
                case '.': {
                    number_tmp += c;
                    number_type_tmp = FLOAT;
                } break;
                case ' ':
                case '\t':
                case '\n': {
                    token_stream.push_back(
                        Token(number_tmp, number_type_tmp));
                    machine_status = TOKENIZE_IDLE;
                } break;
                case ',': {
                    token_stream.push_back(
                        Token(number_tmp, number_type_tmp));
                    token_stream.push_back(Token(",", COMMA));
                    machine_status = TOKENIZE_IDLE;
                } break;
                case '}': {
                    token_stream.push_back(
                        Token(number_tmp, number_type_tmp));
                    token_stream.push_back(Token("}", RBRACE));
                    machine_status = TOKENIZE_IDLE;
                } break;
                case ']': {
                    token_stream.push_back(
                        Token(number_tmp, number_type_tmp));
                    token_stream.push_back(Token("]", RBRACKET));
                    machine_status = TOKENIZE_IDLE;
                } break;
                default:
                    throw std::runtime_error("Bad Json: Bad numbers.\n");
                }
                break;
            } break;
            case TOKENIZE_STRING_WAIT: {
                if (c != '"') {
                    string_tmp += c;
                } else {
                    if (string_tmp.length() == 0)
                        throw std::runtime_error("Bad Json: Bad string.\n");
                    token_stream.push_back(Token(string_tmp, STRING));
                    machine_status = TOKENIZE_IDLE;
                }
            } break;
            case TOKENIZE_FALSE_2_WAIT: {
                if (c != 'a')
                    throw std::runtime_error("Bad Json: Bad syntax.\n");
                machine_status = TOKENIZE_FALSE_3_WAIT;
            } break;
            case TOKENIZE_FALSE_3_WAIT: {
                if (c != 'l')
                    throw std::runtime_error("Bad Json: Bad syntax.\n");
                machine_status = TOKENIZE_FALSE_4_WAIT;
            } break;
            case TOKENIZE_FALSE_4_WAIT: {
                if (c != 's')
                    throw std::runtime_error("Bad Json: Bad syntax.\n");
                machine_status = TOKENIZE_FALSE_5_WAIT;
            } break;
            case TOKENIZE_FALSE_5_WAIT: {
                if (c != 'e')
                    throw std::runtime_error("Bad Json: Bad syntax.\n");
                machine_status = TOKENIZE_IDLE;
                token_stream.push_back(Token("false", BOOLEAN));
            } break;
            case TOKENIZE_TRUE_2_WAIT: {
                if (c != 'r')
                    throw std::runtime_error("Bad Json: Bad syntax.\n");
                machine_status = TOKENIZE_TRUE_3_WAIT;
            } break;
            case TOKENIZE_TRUE_3_WAIT: {
                if (c != 'u')
                    throw std::runtime_error("Bad Json: Bad syntax.\n");
                machine_status = TOKENIZE_TRUE_4_WAIT;
            } break;
            case TOKENIZE_TRUE_4_WAIT: {
                if (c != 'e')
                    throw std::runtime_error("Bad Json: Bad syntax.\n");
                machine_status = TOKENIZE_IDLE;
                token_stream.push_back(Token("true", BOOLEAN));
            } break;
            default:
                throw std::runtime_error("Bad Json: Bad tokens.\n");
            }
        }
    }

private:
    bool
    validation_basic() {
        std::stack<valuetype> check;
        for (auto &i : token_stream) {
            if (i.token_type == LBRACE) {
                check.push(LBRACE);
            } else if (i.token_type == LBRACKET) {
                check.push(LBRACKET);
            } else if (i.token_type == RBRACE) {
                if (check.top() == LBRACE)
                    check.pop();
            } else if (i.token_type == RBRACKET) {
                if (check.top() == LBRACKET)
                    check.pop();
            }
        }
        return !check.empty();
    }

public:
    // parse stored token_stream, generating parsed_obj.
    void
    parse() {
        jsonobj *p = &parsed_obj;
        std::stack<jsonobj *> parse_stack;
        std::stack<valuetype> machine_status;

        std::string name_tmp;

        // parse_stack.push(nullptr);
        machine_status.push(PARSE_INIT);

        for (auto &i : token_stream) {
            if (machine_status.empty())
                return;
            switch (machine_status.top()) {
            // The init state needs to be dealed with seperately.
            // Since a lbrace is a forced requirement, we are expecting LBRACE.
            // After receiving LBRACE, we transform into
            // PARSE_OBJECT_INIT_CONTENT_WAIT, in case the package has no object.
            case PARSE_INIT: {
                if (i.token_type != LBRACE)
                    throw std::runtime_error("Bad Json: Not starting with '{'.\n");
                p->child = new jsonobj();
                p->type = TOP;

                machine_status.top() = PARSE_OBJECT_INIT_CONTENT_WAIT;

                parse_stack.push(p);
                p = p->child;
            } break;

            // When creating a new json object, there might be few cases
            // expected: (a). the object has content, therefore the machine
            // transfrom into PARSE_COLON_WAIT. (b). the object has no content,
            // in the case of getting syntax RBRACE. Pop machine_status Notice
            // that we will deal with array seperately. For (a) the procedure is
            // actually the same of PARSE_NAME_WAIT.
            case PARSE_OBJECT_INIT_CONTENT_WAIT: {
                switch (i.token_type) {
                case STRING: {
                    // Storing the name into name_tmp, waiting for the
                    // compelete unit to complete.
                    name_tmp = i.token_value;
                    // Transforming
                    machine_status.top() = PARSE_COLON_WAIT;
                } break;
                case RBRACE: {
                    machine_status.pop();
                    parse_stack.pop();
                    p = parse_stack.top();
                    if (parse_stack.empty())
                        return;
                    p->next = new jsonobj();
                    p = p->next;
                } break;
                default:
                    throw std::runtime_error("Bad Json: Bad expression.\n");
                }
            } break;

            // Since the array will be dealed with seperately, here we expect
            // these syntaxs: (a) STRING Then we will transform into
            // PARSE_COLON_WAIT.
            case PARSE_NAME_WAIT: {
                if (i.token_type != STRING)
                    throw std::runtime_error("Bad Json: Bad syntax.\n");
                // Storing the name into name_tmp, waiting for the compelete unit
                // to complete.
                name_tmp = i.token_value;
                // Transforming
                machine_status.top() = PARSE_COLON_WAIT;
            } break;

            // This is the most complicated part of the design. We are expecting:
            // (a) LBRACE. This means that a new object will be constructed.
            //     We need to transform into PARSE_COMMA_WAIT,
            //     and push a PARSE_OBJECT_INIT_CONTENT_WAIT.
            // (b) LBRACKET. This means that a new array will be constructed. We
            // need to transform
            //     into PARSE_COMMA_WAIT, and push a PARSING_ARRAY_INIT.
            // (c) FLOAT. This is part of the value. We need to make a new
            // jsonobj, put name_tmp and
            //     casted i.token_value(double) into std::variant, transform into
            //     PARSE_COMMA_WAIT.
            // (c) INTEGER. This is part of the value. We need to make a new
            // jsonobj, put name_tmp and
            //     casted i.token_value(int64_t) into std::variant, transform
            //     into PARSE_COMMA_WAIT.
            // (c) STRING. This is part of the value. We need to make a new
            // jsonobj, put name_tmp and
            //     i.token_value into std::variant, transform into
            //     PARSE_COMMA_WAIT.
            // (c) BOOLEAN. This is part of the value. We need to make a new
            // jsonobj, put name_tmp and
            //     casted i.token_value(bool) into std::variant, transform into
            //     PARSE_COMMA_WAIT.
            case PARSE_VALUE_WAIT: {
                switch (i.token_type) {
                case LBRACE: {
                    machine_status.top() = PARSE_COMMA_WAIT;
                    machine_status.push(PARSE_OBJECT_INIT_CONTENT_WAIT);
                    parse_stack.push(p);
                    p->name = name_tmp;
                    p->type = OBJECT;
                    p->child = new jsonobj();
                    p = p->child;
                } break;
                case LBRACKET: {
                    machine_status.top() = PARSE_COMMA_WAIT;
                    machine_status.push(PARSE_ARRAY_INIT_CONTENT_WAIT);
                    p->name = name_tmp;
                    p->type = ARRAY;
                    p->child = new jsonobj();
                    parse_stack.push(p);
                    p = p->child;
                } break;
                case FLOAT: {
                    machine_status.top() = PARSE_COMMA_WAIT;
                    p->name = name_tmp;
                    p->type = FLOAT;
                    p->obj = atof(i.token_value.c_str());
                    p->next = new jsonobj();
                    p = p->next;
                } break;
                case INTEGER: {
                    machine_status.top() = PARSE_COMMA_WAIT;
                    p->name = name_tmp;
                    p->type = INTEGER;
                    p->obj = atoll(i.token_value.c_str());
                    p->next = new jsonobj();
                    p = p->next;
                } break;
                case STRING: {
                    machine_status.top() = PARSE_COMMA_WAIT;
                    p->name = name_tmp;
                    p->type = STRING;
                    p->obj = i.token_value;
                    p->next = new jsonobj();
                    p = p->next;
                } break;
                case BOOLEAN: {
                    machine_status.top() = PARSE_COMMA_WAIT;
                    p->name = name_tmp;
                    p->type = BOOLEAN;
                    if (i.token_value == "true")
                        p->obj = true;
                    else if (i.token_value == "false")
                        p->obj = false;
                    p->next = new jsonobj();
                    p = p->next;
                } break;
                default:
                    throw std::runtime_error("Bad Json: Illegal value.\n");
                }
            } break;

            // Since the array will be dealed with seperately, we are simply
            // expecting these syntaxs: (a) COMMA. This means that the package is
            // not done, transform into PARSE_NAME_WAIT for next item. (b)
            // RBRACE. This means that the package is done, Pop machine_status.
            case PARSE_COMMA_WAIT: {
                switch (i.token_type) {
                case COMMA: {
                    machine_status.top() = PARSE_NAME_WAIT;
                } break;
                case RBRACE: {
                    machine_status.pop();
                    p = parse_stack.top();
                    parse_stack.pop();
                    if (parse_stack.empty())
                        return;
                    p->next = new jsonobj();
                    p = p->next;
                } break;
                default:
                    throw std::runtime_error("Bad Json: Bad syntax.\n");
                }
            } break;

            // Since the array will be dealed with seperately, we are simply
            // expecting COLON.
            case PARSE_COLON_WAIT: {
                if (i.token_type != COLON)
                    throw std::runtime_error("Bad Json: Bad syntax.\n");
                // Transforming
                machine_status.top() = PARSE_VALUE_WAIT;
            } break;

            // Abolished. Dont generate this token.
            case PARSE_QUOTATION_WAIT: {
            } break;

            // The init state needs to be dealed with seperately.
            // There are four targets actually, the syntaxs are as follows:
            // (a) VALUE. including FLOAT, INTEGER, STRING, BOOLEAN.
            //      1. FLOAT. We need to transform into PARSING_ARRAY_COMMA_WAIT.
            //      2. INTEGER. We need to transform into
            //      PARSING_ARRAY_COMMA_WAIT.
            //      3. STRING. We need to transform into
            //      PARSING_ARRAY_COMMA_WAIT.
            //      4. BOOLEAN. We need to transform into
            //      PARSING_ARRAY_COMMA_WAIT.
            // (b) LBRACE. We need to transform into PARSING_ARRAY_COMMA_WAIT,
            //     and push a PARSE_OBJECT_INIT_CONTENT_WAIT into machine_status.
            // (c) LBRACKET. We need to transform into PARSING_ARRAY_COMMA_WAIT,
            //     and push a PARSE_ARRAY_INIT into machine_status.
            // (d) RBRACKET. Pop machine_status.
            case PARSE_ARRAY_INIT_CONTENT_WAIT: {
                switch (i.token_type) {
                case LBRACE: {
                    machine_status.top() = PARSE_ARRAY_COMMA_WAIT;
                    machine_status.push(PARSE_OBJECT_INIT_CONTENT_WAIT);
                    p->type = OBJECT;
                    p->child = new jsonobj();
                    parse_stack.push(p);
                    p = p->child;
                } break;
                case LBRACKET: {
                    machine_status.top() = PARSE_ARRAY_COMMA_WAIT;
                    machine_status.push(PARSE_ARRAY_INIT_CONTENT_WAIT);
                    p->type = ARRAY;
                    p->child = new jsonobj();
                    parse_stack.push(p);
                    p = p->child;
                } break;
                case FLOAT: {
                    machine_status.top() = PARSE_ARRAY_COMMA_WAIT;
                    p->type = FLOAT;
                    p->obj = atof(i.token_value.c_str());
                    p->next = new jsonobj();
                    p = p->next;
                } break;
                case INTEGER: {
                    machine_status.top() = PARSE_ARRAY_COMMA_WAIT;
                    p->type = INTEGER;
                    p->obj = atoll(i.token_value.c_str());
                    p->next = new jsonobj();
                    p = p->next;
                } break;
                case STRING: {
                    machine_status.top() = PARSE_ARRAY_COMMA_WAIT;
                    p->type = STRING;
                    p->obj = i.token_value;
                    p->next = new jsonobj();
                    p = p->next;
                } break;
                case BOOLEAN: {
                    machine_status.top() = PARSE_ARRAY_COMMA_WAIT;
                    p->type = BOOLEAN;
                    if (i.token_value == "true")
                        p->obj = true;
                    else if (i.token_value == "false")
                        p->obj = false;
                    p->next = new jsonobj();
                    p = p->next;
                } break;
                case RBRACKET: {
                    machine_status.pop();
                    p = parse_stack.top();
                    parse_stack.pop();
                    p->next = new jsonobj();
                    p = p->next;
                } break;
                default:
                    throw std::runtime_error("Bad Json: Bad token stream.\n");
                    break;
                }
            } break;

            // For array value parsing, we are expecting these syntaxs:
            // (a) VALUE. including LBRACE, LBRACKET, FLOAT, INTEGER, STRING,
            // BOOLEAN.
            //      1. LBRACE. We need to transform into
            //      PARSING_ARRAY_COMMA_WAIT,
            //         and push a PARSE_OBJECT_INIT_CONTENT_WAIT into
            //         machine_status.
            //      2. LBRACKET. We need to transform into
            //      PARSING_ARRAY_COMMA_WAIT,
            //         and push a PARSE_ARRAY_INIT into machine_status.
            //      3. FLOAT. We need to transform into PARSING_ARRAY_COMMA_WAIT.
            //      4. INTEGER. We need to transform into
            //      PARSING_ARRAY_COMMA_WAIT.
            //      5. STRING. We need to transform into
            //      PARSING_ARRAY_COMMA_WAIT.
            //      6. BOOLEAN. We need to transform into
            //      PARSING_ARRAY_COMMA_WAIT.
            case PARSE_ARRAY_VALUE_WAIT: {
                switch (i.token_type) {
                case LBRACE: {
                    machine_status.top() = PARSE_ARRAY_COMMA_WAIT;
                    machine_status.push(PARSE_OBJECT_INIT_CONTENT_WAIT);
                    p->type = OBJECT;
                    p->child = new jsonobj();
                    parse_stack.push(p);
                    p = p->child;
                } break;
                case LBRACKET: {
                    machine_status.top() = PARSE_ARRAY_COMMA_WAIT;
                    machine_status.push(PARSE_ARRAY_INIT_CONTENT_WAIT);
                    p->type = ARRAY;
                    p->child = new jsonobj();
                    parse_stack.push(p);
                    p = p->child;
                } break;
                case FLOAT: {
                    machine_status.top() = PARSE_ARRAY_COMMA_WAIT;
                    p->type = FLOAT;
                    p->obj = atof(i.token_value.c_str());
                    p->next = new jsonobj();
                    p = p->next;
                } break;
                case INTEGER: {
                    machine_status.top() = PARSE_ARRAY_COMMA_WAIT;
                    p->type = INTEGER;
                    p->obj = atoll(i.token_value.c_str());
                    p->next = new jsonobj();
                    p = p->next;
                } break;
                case STRING: {
                    machine_status.top() = PARSE_ARRAY_COMMA_WAIT;
                    p->type = STRING;
                    p->obj = i.token_value;
                    p->next = new jsonobj();
                    p = p->next;
                } break;
                case BOOLEAN: {
                    machine_status.top() = PARSE_ARRAY_COMMA_WAIT;
                    p->type = BOOLEAN;
                    if (i.token_value == "true")
                        p->obj = true;
                    else if (i.token_value == "false")
                        p->obj = false;
                    p->next = new jsonobj();
                    p = p->next;
                } break;
                default:
                    throw std::runtime_error("Bad Json: Bad value.\n");
                }
            } break;

            // For array comma parsing, we are expecting these syntaxs:
            // (a) COMMA. It shows that there will be more values. Transform into
            // PARSING_ARRAY_VALUE_WAIT. (b) RBRACKET. It indicates the end of
            // array. Pop machine_status.
            case PARSE_ARRAY_COMMA_WAIT: {
                switch (i.token_type) {
                case COMMA: {
                    machine_status.top() = PARSE_ARRAY_VALUE_WAIT;
                } break;
                case RBRACKET: {
                    machine_status.pop();
                    p = parse_stack.top();
                    parse_stack.pop();
                    p->next = new jsonobj();
                    p = p->next;
                } break;
                default:
                    throw std::runtime_error("Bad Json: Bad syntax.\n");
                }
            } break;

            default:
                throw std::runtime_error("Bad Json: Bad status.\n");
                break;
            }
        }
    }
};

} // namespace parse

#endif