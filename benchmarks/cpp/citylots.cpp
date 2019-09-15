#include <list>
#include <unordered_map>
#include <atomic>
#include <cassert>
#include <chrono>

#include "Context.hpp"

using namespace Centaurus;

enum class JSONType
{
    None,
    Number,
    String,
    Boolean,
    List,
    Object
};

class JSONValue;

using JSONString = std::string;
using JSONList = std::list<JSONValue>;
using JSONObject = std::unordered_map<std::string, JSONValue>;

class JSONValue
{
    friend std::ostream& operator<<(std::ostream& os, const JSONValue& v);

    JSONType type;
    union
    {
        bool bvalue;
        double dvalue;
        JSONString *svalue;
        JSONList *lvalue;
        JSONObject *ovalue;
    };
public:
    JSONValue(JSONValue&& old)
        : type(old.type)
    {
        switch (type)
        {
        case JSONType::Boolean: bvalue = old.bvalue; break;
        case JSONType::Number: dvalue = old.dvalue; break;
        case JSONType::String: svalue = old.svalue; old.svalue = nullptr; break;
        case JSONType::List: lvalue = old.lvalue; old.lvalue = nullptr; break;
        case JSONType::Object: ovalue = old.ovalue; old.ovalue = nullptr; break;
        }
    }
    JSONValue(const JSONValue& old)
        : type(old.type)
    {
        switch (type)
        {
        case JSONType::Boolean: bvalue = old.bvalue; break;
        case JSONType::Number: dvalue = old.dvalue; break;
        case JSONType::String: svalue = new JSONString(*old.svalue); break;
        case JSONType::List: lvalue = new JSONList(*old.lvalue); break;
        case JSONType::Object: ovalue = new JSONObject(*old.ovalue); break;
        }
    }
    JSONValue& operator=(JSONValue&& old)
    {
        type = old.type;
        switch (type)
        {
        case JSONType::Boolean: bvalue = old.bvalue; break;
        case JSONType::Number: dvalue = old.dvalue; break;
        case JSONType::String: svalue = old.svalue; old.svalue = nullptr; break;
        case JSONType::List: lvalue = old.lvalue; old.lvalue = nullptr; break;
        case JSONType::Object: ovalue = old.ovalue; old.ovalue = nullptr;break;
        }
    }
    JSONValue& operator=(const JSONValue& old)
    {
        type = old.type;
        switch (type)
        {
        case JSONType::Boolean: bvalue = old.bvalue; break;
        case JSONType::Number: dvalue = old.dvalue; break;
        case JSONType::String: svalue = new JSONString(*old.svalue); break;
        case JSONType::List: lvalue = new JSONList(*old.lvalue); break;
        case JSONType::Object: ovalue = new JSONObject(*old.ovalue); break;
        }
    }
    JSONValue(bool b) : type(JSONType::Boolean), bvalue(b) {}
    JSONValue(double d) : type(JSONType::Number), dvalue(d) {}
    JSONValue() : type(JSONType::None) {}
    JSONValue(JSONString&& s) : type(JSONType::String), svalue(new JSONString(s)) {}
    JSONValue(JSONString *s) : type(JSONType::String), svalue(s) {}
    JSONValue(JSONList *l) : type(JSONType::List), lvalue(l) {}
    JSONValue(JSONObject *o) : type(JSONType::Object), ovalue(o) {}
    ~JSONValue()
    {
        switch (type)
        {
        case JSONType::String:
            if (svalue != nullptr) delete svalue; break;
        case JSONType::List:
            if (lvalue != nullptr) delete lvalue; break;
        case JSONType::Object:
            if (ovalue != nullptr) delete ovalue; break;
        }
    }
    const JSONString& str() const
    {
        assert(type == JSONType::String);
        return *svalue;
    }
    double num() const
    {
        assert(type == JSONType::Number);
        return dvalue;
    }
    operator bool() const
    {
        assert(type == JSONType::Boolean);
        return bvalue;
    }
    const JSONObject& object() const
    {
        return *ovalue;
    }
    bool is_object() const
    {
        return type == JSONType::Object;
    }
    bool is_str() const
    {
        return type == JSONType::String;
    }
    const JSONValue& operator[](const std::string& key) const
    {
        assert(is_object());
        return ovalue->at(key);
    }
    bool has_item(const std::string& key) const
    {
        assert(is_object());
        return ovalue->count(key) > 0;
    }
};

std::ostream& operator<<(std::ostream& os, const JSONValue& v)
{
    switch (v.type)
    {
    case JSONType::None:
        os << "None";
        break;
    case JSONType::Number:
        os << "Number " << v.dvalue;
        break;
    case JSONType::String:
        os << "String " << v.svalue;
        break;
    case JSONType::Boolean:
        os << "Boolean " << v.bvalue;
        break;
    case JSONType::List:
        os << "List" << std::endl;
        break;
    case JSONType::Object:
        os << "Object" << std::endl;
        break;
    }
    return os;
}

void *parseNumber(const SymbolContext<char>& ctx)
{
    return new JSONValue(std::stod(ctx.read()));
}

void *parseString(const SymbolContext<char>& ctx)
{
    const char *start = ctx.start();
    const char *end = ctx.end();
    for (; start != end && *start != '\"'; start++)
        ;
    start++;
    for (; end != start && *end != '\"'; end--)
        ;
    int len = end - start;
    return new JSONValue(new std::string(start, len));
}

void *parseNull(const SymbolContext<char>& ctx)
{
    return nullptr;
}

void *parseTrue(const SymbolContext<char>& ctx)
{
  return new JSONValue(true);
}

void *parseFalse(const SymbolContext<char>& ctx)
{
  return new JSONValue(false);
}

void *parseList(const SymbolContext<char>& ctx)
{
    JSONList *l = new JSONList();
    for (int i = 1; i <= ctx.count(); i++)
    {
        JSONValue *v = ctx.value<JSONValue>(i);
        l->emplace_back(std::move(*v));
        delete v;
    }
    return new JSONValue(l); 
}

void *parseDictionary(const SymbolContext<char>& ctx)
{
    JSONObject *o = new JSONObject();
    for (int i = 1; i <= ctx.count(); i++)
    {
        std::pair<JSONString, JSONValue> *p = ctx.value<std::pair<JSONString, JSONValue> >(i);
        o->emplace(p->first, std::move(p->second));
        delete p;
    }
    return new JSONValue(o);
}

void *parseDictionaryEntry(const SymbolContext<char>& ctx)
{
    JSONValue *s = ctx.value<JSONValue>(1);
    void *p;
    if (ctx.count() == 2) {
      JSONValue *o = ctx.value<JSONValue>(2);
      p = new std::pair<JSONString, JSONValue>(s->str(), std::move(*o));
      delete o;
    } else {
      p = new std::pair<JSONString, JSONValue>(s->str(), JSONValue());
    }
    delete s;
    return p;
}

std::atomic<int> count;

void *parseObject(const SymbolContext<char>& ctx)
{
    if (ctx.count() == 0) return nullptr;

    JSONValue *v = ctx.value<JSONValue>(1);

    if (v->is_object() && v->has_item("type"))
    {
        if (v->operator[]("type").str() == "Feature")
        {
            if (v->has_item("properties"))
            {
                const JSONValue& properties = v->operator[]("properties");

                if (properties.has_item("STREET"))
                {
                    const JSONValue& street = properties.operator[]("STREET");
                    if (street.is_str())
                    {
                        if (street.str() != "JEFFERSON")
                        {
                            delete v;
                            return nullptr;
                        }
                        else
                        {
                            // count++;
                        }
                    }
                    else
                    {
                        // count++;
                    }
                }
            }
        }
    }
    return v;
}

int main(int argc, const char *argv[])
{
    if (argc < 1) return 1;
    int worker_num = std::atoi(argv[1]);
    bool no_action = argc >= 3 && argv[2] == std::string("dry");

    const char *input_path = "datasets/citylots.json";
    const char *grammar_path = "grammar/json2.cgr";

    Context<char> context{grammar_path};

    if (!no_action) {
      context.attach(L"Null", parseNull);
      context.attach(L"String", parseString);
      context.attach(L"Dictionary", parseDictionary);
      context.attach(L"List", parseList);
      context.attach(L"DictionaryEntry", parseDictionaryEntry);
      context.attach(L"Number", parseNumber);
      context.attach(L"True", parseTrue);
      context.attach(L"False", parseFalse);
      context.attach(L"Object", parseObject);
    }

    using namespace std::chrono;

    auto start = high_resolution_clock::now();;

    context.parse(input_path, worker_num);

    auto end = high_resolution_clock::now();;

    std::cout << worker_num << " " << duration_cast<milliseconds>(end - start).count() << std::endl;

    return 0;
}
