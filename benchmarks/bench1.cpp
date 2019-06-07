#include <sys/time.h>
#include <map>

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

class XMLElement;

class XMLElementBody
{
    std::string m_content;
    std::vector<std::pair<XMLElement, std::string> > m_elements;
public:
    XMLElementBody()
    {
    }
    XMLElementBody(XMLElementBody&& old)
        : m_content(std::move(old.m_content)), m_elements(std::move(old.m_elements))
    {
    }
    XMLElementBody(const XMLElementBody& old)
        : m_content(old.m_content), m_elements(old.m_elements)
    {
    }
    XMLElementBody& operator=(XMLElementBody&& old)
    {
        m_content = std::move(old.m_content);
        m_elements = std::move(old.m_elements);
        return *this;
    }
    XMLElementBody& operator=(const XMLElementBody& old)
    {
        m_content = old.m_content;
        m_elements = old.m_elements;
        return *this;
    }
    void add_element(XMLElement&& elem);
    void add_content(const char *content, int len);
    const XMLElement *get_element(const char *name) const;
    const std::string& get_content() const
    {
        return m_content;
    }
    XMLElementBody(const SymbolContext<char>& ctx);
};

using XMLAttribute = std::pair<std::string, std::string>;
class XMLElement
{
    std::string m_name;
    std::map<std::string, std::string> m_attributes;
    XMLElementBody m_body;
public:
    XMLElement& operator=(XMLElement&& old)
    {
        m_name = std::move(old.m_name);
        m_attributes = std::move(old.m_attributes);
        m_body = std::move(old.m_body);
        return *this;
    }
    XMLElement& operator=(const XMLElement& old)
    {
        m_name = old.m_name;
        m_attributes = old.m_attributes;
        m_body = old.m_body;
        return *this;
    }
    XMLElement(const XMLElement& old)
        : m_name(old.m_name), m_attributes(old.m_attributes), m_body(old.m_body)
    {
    }
    XMLElement(const std::string& name)
        : m_name(name)
    {
    }
    XMLElement(const SymbolContext<char>& ctx)
    {
        std::string *name_str = ctx.value<std::string>(1);
        m_name = std::move(*name_str);
        delete name_str;
        for (int i = 2; i <= ctx.count() - 1; i++)
        {
            XMLAttribute *attr = ctx.value<XMLAttribute>(i);
            m_attributes.insert(*attr);
            delete attr;
        }
        XMLElementBody *body_ptr = ctx.value<XMLElementBody>(ctx.count());
        m_body = std::move(*body_ptr);
        delete body_ptr;
    }
    const std::string& get_name() const
    {
        return m_name;
    }
    const std::string& operator[](const std::string& key) const
    {
        return m_attributes.at(key);
    }
    void add_content(const char *content, int len)
    {
        m_body.add_content(content, len);
    }
    void add_element(XMLElement&& elem)
    {
        m_body.add_element(std::move(elem));
    }
    const XMLElement *get_element(const char *name) const
    {
        return m_body.get_element(name);
    }
    const std::string& get_content() const
    {
        return m_body.get_content();
    }
};

void XMLElementBody::add_element(XMLElement&& elem)
{
    m_elements.emplace_back(std::move(elem), "");
}
void XMLElementBody::add_content(const char *content, int len)
{
    if (m_content.empty())
    {
        m_content = std::string(content, len);
    }
    else
    {
        //m_elements.back().second = std::string(content, len);
    }
}
const XMLElement *XMLElementBody::get_element(const char *name) const
{
    for (const auto& p : m_elements)
    {
        if (p.first.get_name() == name)
        {
            return &p.first;
        }
    }
    return nullptr;
}
XMLElementBody::XMLElementBody(const SymbolContext<char>& ctx)
{
    if (ctx.count() >= 2)
    {
        std::string *content_str = ctx.value<std::string>(1);
        m_content = std::move(*content_str);
        delete content_str;
        for (int i = 2; i <= ctx.count() - 1; i += 2)
        {
            XMLElement *elem_ptr = ctx.value<XMLElement>(i);
            std::string *trail_ptr = ctx.value<std::string>(i + 1);
            m_elements.emplace_back(std::move(*elem_ptr), std::move(*trail_ptr));
            delete elem_ptr;
            delete trail_ptr;
        }
    }
}

void *parseDocument(const SymbolContext<char>& ctx)
{
    return ctx.value<XMLElement>(2);
}

void *parseName(const SymbolContext<char>& ctx)
{
    const char *s = ctx.start();
    int len = ctx.len();
    return new std::string(s, len);
}

void *parseValue(const SymbolContext<char>& ctx)
{
    const char *s = ctx.start();
    int len = ctx.len();
    return new std::string(s, len);
}

void *parseContent(const SymbolContext<char>& ctx)
{
    const char *s = ctx.start();
    int len = ctx.len();
    int i;
    for (i = 0; i < len; i++)
    {
        if (!isspace(i)) break;
    }
    if (i != len)
        return new std::string(s, len);
    return new std::string();
}

void *parseAttribute(const SymbolContext<char>& ctx)
{
    std::string *name = ctx.value<std::string>(1);
    std::string *value = ctx.value<std::string>(2);
    auto *p = new std::pair<std::string, std::string>(std::move(*name), std::move(*value));
    delete name;
    delete value;
    return p;
}

void *parseElementBody(const SymbolContext<char>& ctx)
{
    return new XMLElementBody(ctx);
    //std::cout << "ElementBody" << ctx.count() << std::endl;
}

void *parseElement(const SymbolContext<char>& ctx)
{
    XMLElement *elem = new XMLElement(ctx);

    std::string n = elem->get_name();
    if (n == "article")
    {
        const XMLElement *year = elem->get_element("year");
        if (year->get_content() == "1990")
        {
            //printf("\"%s\" article\n", year->get_content().c_str());
        }
        else
        {
            delete elem;
            return new XMLElement(n);
        }
    }
    else if (n == "inproceedings" || n == "proceedings" || n == "book" || n == "incollection" || n == "phdthesis" || n == "masterthesis" || n == "www")
    {
        delete elem;
        return new XMLElement(n);
    }
    return elem;
}

int main(int argc, const char *argv[])
{
    if (argc < 1) return 1;
    int worker_num = atoi(argv[1]);

    const char *input_path = "datasets/dblp.xml";
    const char *grammar_path = "grammar/xml.cgr";

    Context<char> context{grammar_path};

    /*context.attach(L"ElementBody", parseElementBody);
    context.attach(L"Name", parseName);
    context.attach(L"Value", parseValue);
    context.attach(L"Attribute", parseAttribute);
    context.attach(L"Content", parseContent);
    context.attach(L"Element", parseElement);*/

    uint64_t start_time = get_us_clock();

    context.parse(input_path, worker_num);

    uint64_t end_time = get_us_clock();

    //printf("Elapsed time: %ld[us]\n", end_time - start_time);
    printf("%d %ld\n", worker_num, end_time - start_time);

    return 0;
}
