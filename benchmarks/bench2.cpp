#include <list>
#include <unordered_map>
#include <atomic>
#include <assert.h>
#include <sys/time.h>

#include "Context.hpp"

using namespace Centaurus;

static uint64_t get_us_clock()
{
#if defined(CENTAURUS_BUILD_LINUX)
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
#elif defined(CENTAURUS_BUILD_WINDOWS)
	LARGE_INTEGER qpc;
	QueryPerformanceCounter(&qpc);
	LARGE_INTEGER qpf;
	QueryPerformanceFrequency(&qpf);
	return (uint64_t)qpc.QuadPart * 1000000 / (uint64_t)qpf.QuadPart;
#endif
}

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

void *parseNone(const SymbolContext<char>& ctx)
{
    return nullptr;
}

void *parseBoolean(const SymbolContext<char>& ctx)
{
    if (ctx.read() == "true")
        return new JSONValue(true);
    else
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
    JSONValue *o = (ctx.count() == 2) ? ctx.value<JSONValue>(2) : new JSONValue;
    void *p = new std::pair<JSONString, JSONValue>(s->str(), std::move(*o));
    delete s;
    delete o;
    return p;
}

std::atomic<int> count;

void *parseObject(const SymbolContext<char>& ctx)
{
    if (ctx.count() == 0) return nullptr;

    JSONValue *v = ctx.value<JSONValue>(1);

    /*if (v->is_object())
    {
        const JSONObject& obj = v->object();

        for (const auto& p : obj)
        {
            if (p.second.is_str())
            {
                std::cout << p.first << p.second.str() << std::endl;
            }
        }
    }*/

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
                        //std::cout << street.str() << std::endl;
                        if (street.str() != "JEFFERSON")
                        {
                            delete v;
                            return nullptr;
                        }
                        else
                        {
                            count++;
                        }
                    }
                    else
                    {
                        //std::cout << street << std::endl;
                        count++;
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
    int worker_num = atoi(argv[1]);

    const char *input_path = "datasets/citylots.json";
    const char *grammar_path = "grammar/json2.cgr";

    Context<char> context{grammar_path};

    /*context.attach(L"ElementBody", parseElementBody);
    context.attach(L"Name", parseName);
    context.attach(L"Value", parseValue);
    context.attach(L"Attribute", parseAttribute);
    context.attach(L"Content", parseContent);
    context.attach(L"Element", parseElement);*/

    /*context.attach(L"None", parseNone);
    context.attach(L"String", parseString);
    context.attach(L"Dictionary", parseDictionary);
    context.attach(L"List", parseList);
    context.attach(L"DictionaryEntry", parseDictionaryEntry);
    context.attach(L"Number", parseNumber);
    context.attach(L"Boolean", parseBoolean);
    context.attach(L"Object", parseObject);*/

    count.store(0);

    uint64_t start_time = get_us_clock();

    context.parse(input_path, worker_num);

    uint64_t end_time = get_us_clock();

    //printf("%d elements found\n", count.load());
    //printf("Elapsed time: %ld[us]\n", end_time - start_time);
    printf("%d %ld\n", worker_num, end_time - start_time);

    return 0;
}
