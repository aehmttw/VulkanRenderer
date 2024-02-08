#include <map>
#include <string>
#include <vector>

struct jvalue
{

};

struct jarray : jvalue
{
    std::vector<jvalue*> elements;

    jvalue* operator[] (int i)
    {
        return (elements[i]);
    }

    size_t size()
    {
        return elements.size();
    }
};

struct jstring : jvalue
{
    std::string s;

    jstring(std::string s)
    {
        this->s = s;
    }
};

struct jnumber : jvalue
{
    double v;

    jnumber(double d)
    {
        this->v = d;
    }
};

struct jbool : jvalue
{
    bool b;

    jbool(bool b)
    {
        this->b = b;
    }
};

struct jnull : jvalue
{

};

struct jobject : jvalue
{
    std::map<std::string, jvalue*> elements;

    jvalue* operator[] (std::string s)
    {
        return (elements[s]);
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
    printf("value %d\n", *index);

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
    printf("obj %d\n", *index);

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
            key = parse_string(chars, index)->s;
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
    printf("arr %d %c\n", *index, chars[*index]);

    auto* a = new jarray();
    bool expectComma = false;
    while (true)
    {
        int i = *index;
        (*index)++;

        if (chars[i] == ']')
        {
            printf("x arr %d\n", *index);
            return a;
        }
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
    printf("num %d\n", *index);

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
    printf("str %d\n", *index);

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
    printf("bool %d\n", *index);

    (*index)--;
    if (memcmp(&chars[*index], "true", 4) == 0)
    {
        *index += 4;
        printf("x bool %d\n", *index);
        return new jbool {true};
    }
    else if (memcmp(&chars[*index], "false", 5) == 0)
    {
        *index += 5;
        printf("x bool %d\n", *index);
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
    printf("null %d\n", *index);

    (*index)--;
    if (memcmp(&chars[*index], "null", 4) == 0)
    {
        *index += 4;
        printf("x null %d\n", *index);
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