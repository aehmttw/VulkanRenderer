#include <map>
#include <string>
#include <vector>

const int type_string = 0;
const int type_number = 1;
const int type_object = 2;
const int type_array = 3;
const int type_bool = 4;
const int type_null = -1;

const char* type_to_string(int type)
{
    if (type == type_string)
        return "string";
    else if (type == type_number)
        return "number";
    else if (type == type_object)
        return "object";
    else if (type == type_array)
        return "array";
    else if (type == type_bool)
        return "bool";
    else if (type == type_null)
        return "null";

    return "bad type";
}

struct jvalue
{
    int type;
};

void typecheck(jvalue* value, int expected)
{
    if (value == nullptr)
        return;

    if (value->type != expected)
    {
        printf("Type mismatch: expected %s but got %s\n", type_to_string(expected), type_to_string(value->type));
        abort();
    }
}

struct jarray : jvalue
{
    std::vector<jvalue*> elements;

    jarray()
    {
        this->type = type_array;
    }

    ~jarray()
    {
        for (auto e: elements)
        {
            delete e;
        }
    }

    jvalue* operator[] (int i)
    {
        return (elements[i]);
    }

    size_t size()
    {
        return elements.size();
    }

    static jarray* cast(jvalue* v)
    {
        typecheck(v, type_array);
        return static_cast<jarray*>(v);
    }
};

struct jstring : jvalue
{
    std::string value;

    jstring(std::string s)
    {
        this->value = s;
        this->type = type_string;
    }

    static jstring* cast(jvalue* v)
    {
        typecheck(v, type_string);
        return static_cast<jstring*>(v);
    }
};

struct jnumber : jvalue
{
    double value;

    jnumber(double d)
    {
        this->value = d;
        this->type = type_number;
    }

    static jnumber* cast(jvalue* v)
    {
        typecheck(v, type_number);
        return static_cast<jnumber*>(v);
    }
};

struct jbool : jvalue
{
    bool value;

    jbool(bool b)
    {
        this->value = b;
        this->type = type_bool;
    }

    static jbool* cast(jvalue* v)
    {
        typecheck(v, type_bool);
        return static_cast<jbool*>(v);
    }
};

struct jnull : jvalue
{
    jnull()
    {
        this->type = type_null;
    }
};

struct jobject : jvalue
{
    jobject()
    {
        this->type = type_object;
    }

    ~jobject()
    {
        for (const auto& e: elements)
        {
            delete e.second;
        }
    }

    std::map<std::string, jvalue*> elements;

    jvalue* operator[] (std::string &s)
    {
        if (elements.contains(s))
            return (elements.at(s));
        else
            return nullptr;
    }

    jvalue* operator[] (const char* s)
    {
        if (elements.contains(s))
            return (elements.at(s));
        else
            return nullptr;
    }

    static jobject* cast(jvalue* v)
    {
        typecheck(v, type_object);
        return static_cast<jobject*>(v);
    }
};

static jvalue* parse_value(const char* chars, int* index);
static jobject* parse_object(const char* chars, int* index);
static jarray* parse_array(const char* chars, int* index);
static jstring* parse_string(const char* chars, int* index);
static jnumber* parse_number(const char* chars, int* index);
static jbool* parse_bool(const char* chars, int* index);
static jnull* parse_null(const char* chars, int* index);

static jvalue* parse_value(const char* chars, int* index)
{
    while (true)
    {
        int i = *index;
        (*index)++;

        if (chars[i] == '{')
            return parse_object(chars, index);
        else if (chars[i] == '[')
            return parse_array(chars, index);
        else if (chars[i] == '"')
            return parse_string(chars, index);
        else if ((chars[i] >= '0' && chars[i] <= '9') || chars[i] == '-')
            return parse_number(chars, index);
        else if (chars[i] == 't' || chars[i] == 'f')
            return parse_bool(chars, index);
        else if (chars[i] == 'n')
            return parse_null(chars, index);
        else if (chars[i] != ' ' && chars[i] != '\n' && chars[i] != '\t' && chars[i] != '\r')
        {
            printf("unknown character in value at index %d: '%c'\n", i, chars[i]);
            throw std::runtime_error("parse failed");
        }
    }
}

static jobject* parse_object(const char* chars, int* index)
{
    jobject* o = new jobject();
    int phase = 0;
    std::string key;
    while (true)
    {
        int i = *index;
        (*index)++;

        if (chars[i] == '}')
            return o;
        else if (chars[i] == '"' && phase == 0)
        {
            jstring* s = parse_string(chars, index);
            key = s->value;
            delete s;
            phase = 1;
        }
        else if (chars[i] == ':' && phase == 1)
        {
            o->elements[key] = parse_value(chars, index);
            phase = 2;
        }
        else if (phase == 2 && chars[i] == ',')
            phase = 0;
        else if (chars[i] != ' ' && chars[i] != '\n' && chars[i] != '\t' && chars[i] != '\r')
        {
            printf("unknown character in object at index %d: '%c'\n", i, chars[i]);
            throw std::runtime_error("parse failed");
        }
    }
}

static jarray* parse_array(const char* chars, int* index)
{
    auto* a = new jarray();
    bool expectComma = false;
    while (true)
    {
        int i = *index;
        (*index)++;

        if (chars[i] == ']')
            return a;
        else if (chars[i] == ' ' || chars[i] == '\n' || chars[i] == '\t' || chars[i] == '\r')
            {}
        else if (!expectComma)
        {
            (*index)--;
            a->elements.emplace_back(parse_value(chars, index));
            expectComma = true;
        }
        else if (chars[i] == ',')
            expectComma = false;
        else
        {
            printf("expected comma at index %d but got: '%c'\n", i, chars[i]);
            throw std::runtime_error("parse failed");
        }
    }
}

static jnumber* parse_number(const char* chars, int* index)
{
    (*index)--;

    bool first = true;
    int startIndex = *index;

    while (true)
    {
        int i = *index;
        (*index)++;

        if ((chars[i] < '0' || chars[i] > '9') && chars[i] != '-' && chars[i] != '+' && chars[i] != '-' && chars[i] != '.' && chars[i] != 'e' && chars[i] != 'E')
            break;
    }

    char num[*index - startIndex + 1];
    num[*index - startIndex] = '\0';
    std::memcpy(num, &chars[startIndex], *index - startIndex);
    (*index)--;

    return new jnumber {std::stod(num)};
}

static jstring* parse_string(const char* chars, int* index)
{
    std::vector<char> str;

    bool escape = false;
    while (true)
    {
        int i = *index;
        (*index)++;

        if (escape)
        {
            if (chars[i] == '\"')
                str.emplace_back('\"');
            else if (chars[i] == '\\')
                str.emplace_back('\\');
            else if (chars[i] == 'n')
                str.emplace_back('\n');
            else
            {
                printf("unknown escape sequence at index %d: '%c'\n", i, chars[i]);
                throw std::runtime_error("parse failed");
            }

            escape = false;
        }
        else if (chars[i] == '\\')
            escape = true;
        else if (chars[i] == '\"')
        {
            std::string s (str.begin(), str.end());
            return new jstring {s};
        }
        else
            str.emplace_back(chars[i]);
    }
}

static jbool* parse_bool(const char* chars, int* index)
{
    (*index)--;
    if (memcmp(&chars[*index], "true", 4) == 0)
    {
        *index += 4;
        return new jbool {true};
    }
    else if (memcmp(&chars[*index], "false", 5) == 0)
    {
        *index += 5;
        return new jbool {false};
    }
    else
    {
        printf("unknown value at index %d: '%c'\n", *index, chars[*index]);
        throw std::runtime_error("parse failed");
    }
}


static jnull* parse_null(const char* chars, int* index)
{
    (*index)--;
    if (memcmp(&chars[*index], "null", 4) == 0)
    {
        *index += 4;
        return nullptr;
    }
    else
    {
        printf("unknown value at index %d: '%c'\n", *index, chars[*index]);
        throw std::runtime_error("parse failed");
    }
}

static jobject* jparse(std::string str)
{
    int i = 0;
    const char* chars = str.c_str();

    while (chars[i] != '{')
        i++;

    i++;

    return parse_object(chars, &i);
}

static jarray* jparse_array(std::string str)
{
    int i = 0;
    const char* chars = str.c_str();

    while (chars[i] != '[')
        i++;

    i++;

    return parse_array(chars, &i);
}