#pragma once

#include <string>
#include <unordered_map>

namespace freerouting::designforms::specctra {

class Keyword
{
public:
    const std::string& get_name() const { return name_; }

    bool operator==(const Keyword& other) const { return this == &other; }
    bool operator!=(const Keyword& other) const { return !(*this == other); }

    explicit Keyword(const std::string& p_name);

private:
    std::string name_;
    friend class KeywordRegistry;
};

class ScopeKeyword : public Keyword
{
public:
    using Keyword::get_name;

    virtual ~ScopeKeyword() = default;

protected:
    using Keyword::Keyword;
};

class KeywordRegistry
{
public:
    static KeywordRegistry& instance();

    const Keyword* find(const std::string& p_name) const;
    void register_keyword(const std::string& p_name, const Keyword* p_keyword);

private:
    KeywordRegistry() = default;
    std::unordered_map<std::string, const Keyword*> keywords_;
};

} // namespace freerouting::designforms::specctra