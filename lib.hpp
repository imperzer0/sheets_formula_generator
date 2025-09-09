//
// Created by dima on 08 Sept 2025.
//

#ifndef SHEETS_FORMULA_GENERATOR_LIB_H
#define SHEETS_FORMULA_GENERATOR_LIB_H
#include <functional>
#include <map>
#include <string>
#include <vector>

union FormulaError;

class Formula
{
public:
    Formula(std::string&& template_);
    ~Formula();

    const std::string& status();
    const std::string& result();

    Formula&& expect(std::function<void (int)>& fn);

    Formula&& parse();

    Formula&& define(std::string&& var, std::string&& value);

    Formula&& define(std::map<std::string, std::string>&& vars);

    Formula&& replace();

    enum ErrorCode : int
    {
        ENONE = -1,    // '${' Not found
        EINVALID = -2, // Invalid variable name
        EIDXOVF = -3,  // Index value is too big, does not fit into int type
    };

    struct SearchResult
    {
        ErrorCode error_pos;
        int len;
    };

    template <typename Names, typename Indices_>
    struct Indices
    {
        Names names;
        Indices_ indices;
    };

private:
    /// Helper Functions ///

    SearchResult next_$(int start_at = 0);
    bool not_parsed();
    bool no_substitutes_defined();


    /// Fields ///

    std::string template_;        // With ${vars}
    std::string template_parsed_; // No ${vars}
    std::string result_;          // ${vars} replaced with provided values

    Indices<std::vector<std::string>, std::vector<int>> vars_;
    std::map<std::string, std::string> definitions_;

    std::string status_{"OK"}; // Error message
};


inline Formula::Formula(std::string&& template_) : template_(std::forward<decltype(template_)>(template_)) { }

inline Formula::~Formula() = default;

inline const std::string& Formula::status() { return status_; }

inline const std::string& Formula::result()
{
    if (status_ != "OK")
        return status_;

    return result_;
}

inline Formula::SearchResult Formula::next_$(int start_at)
{
    for (int i = start_at + 1; i < template_parsed_.size(); ++i)
    {
        if (template_parsed_[i - 1] == '$' && template_parsed_[i] == '{') // If Var syntax detected
        {
            // Get variable name
            std::string var_name;
            for (int j = i + 1; j < template_parsed_.size() &&
                 template_parsed_[j] != '}'/* stop at } */; ++j)
            {
                if (template_parsed_[j] >= 'a' && template_parsed_[j] <= 'z' || // [a-z]
                    template_parsed_[j] >= 'A' && template_parsed_[j] <= 'Z' || // [A-Z]
                    template_parsed_[j] >= '0' && template_parsed_[j] <= '9' || // [0-9]
                    template_parsed_[j] == '_')                                 // [_]
                {
                    // Save the name
                    var_name += template_parsed_[j];
                }
                else
                {
                    // Otherwise: Report on invalid variable name
                    status_ =
                        "Invalid variable name at " + std::to_string(j) + ".\n"
                        "You are only allowed to use [a-z], [A-Z], [0-9] and '_'.";
                    return {EINVALID, -1};
                }
            }
            // Save var name and position into vars cache array
            vars_.names.push_back(var_name);
            vars_.indices.push_back(i - 1);

            auto sz = var_name.size() + 3;
            if (sz > INT32_MAX)
            {
                status_ = "Exceeded maximum length of signed integer. The program can no longer process indices.";
                return {EIDXOVF, -1};
            }
            return {ErrorCode(i - 1), static_cast<int>(sz)};
        }
    }
    return {ENONE, -1};
}

inline Formula&& Formula::parse()
{
    this->template_parsed_ = this->template_;
    for (int i = 0; i < template_parsed_.size();/* Update i manually */)
    {
        auto next$ = next_$(i);
        switch (next$.error_pos)
        {
            case ENONE:
            case EINVALID:                             // If errors
            case EIDXOVF: { return std::move(*this); } // Quit

            default:
                {
                    // Move i
                    i = next$.error_pos;

                    // Erase variable use
                    template_parsed_.erase(next$.error_pos, next$.len);
                    // Since we didn't dip into that part of the string
                    // quite yet - we can safely erase as much as we need
                    // without our pre saved indices becoming irrelevant
                }
        }
    }
    return std::move(*this);
}

inline Formula&& Formula::define(std::string&& var, std::string&& value)
{
    definitions_[var] = value;
    return std::move(*this);
}

inline Formula&& Formula::define(std::map<std::string, std::string>&& vars)
{
    definitions_ = std::move(vars);
    return std::move(*this);
}

inline bool Formula::not_parsed()
{
    if (template_parsed_.empty())
    {
        status_ = "You have to run .parse() first.";
        return true;
    }

    return false;
}

inline bool Formula::no_substitutes_defined()
{
    if (template_parsed_.empty())
    {
        status_ = "You have to .define() variables first.";
        return true;
    }

    return false;
}

inline Formula&& Formula::replace()
{
    if (not_parsed())
        return std::move(*this);

    if (no_substitutes_defined())
        return std::move(*this);

    if (vars_.names.size() != vars_.indices.size())
    {
        status_ = "An internal error occurred during the parsing process. This is a BUG!";
        return std::move(*this);
    }

    int last_pos = 0;
    for (int i = 0; i < vars_.names.size(); ++i)
    {
        if (vars_.indices[i] < 0)
        {
            status_ = "An index can not be negative (" + std::to_string(vars_.indices[i]) + "). This is a BUG!";
            return std::move(*this);
        }
        result_ += template_parsed_.substr(last_pos, vars_.indices[i] - last_pos);
        last_pos = vars_.indices[i];
        result_ += definitions_[vars_.names[i]];
    }

    result_ += template_parsed_.substr(last_pos);

    return std::move(*this);
}



#endif //SHEETS_FORMULA_GENERATOR_LIB_H
