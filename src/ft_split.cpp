#include "../include/Helper.hpp"

char *ft_substr(const char *s, unsigned int start, size_t len)
{
    if (!s)
        return NULL;

    size_t slen = std::strlen(s);
    if (start >= slen || len == 0)
        return strdup("");

    if (slen - start < len)
        len = slen - start;

    char *str = (char *)malloc(sizeof(char) * (len + 1));
    if (!str)
        return NULL;

    std::strncpy(str, s + start, len);
    str[len] = '\0';
    return str;
}

static size_t ft_count(const char *s, char c)
{
    size_t i = 0;

    if (!s || *s == '\0')
        return 0;

    while (*s)
    {
        while (*s == c && *s)
            ++s;
        if (*s)
            ++i;
        while (*s != c && *s)
            ++s;
    }
    return i;
}

static size_t ft_place(const char *s, char c, size_t n)
{
    while (s[n] != '\0' && s[n] != c)
        ++n;
    return n;
}

char **ft_split(const char *s, char c)
{
    if (!s)
        return NULL;

    size_t count = ft_count(s, c);
    char **lst = (char **)malloc((count + 1) * sizeof(char *));
    if (!lst)
        return NULL;

    size_t i = 0;
    size_t n = 0;
    while (i < count)
    {
        while (s[n] == c)
            ++n;

        size_t end = ft_place(s, c, n);
        lst[i] = ft_substr(s, n, end - n);
        if (!lst[i])
        {
            while (i > 0)
                free(lst[--i]);
            free(lst);
            return NULL;
        }

        ++i;
        n = end + 1;
    }
    lst[i] = NULL;
    return lst;
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = 0;
    
    while ((end = str.find(delimiter, start)) != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + 1;
    }
    
    // Don't forget the last token after the final delimiter (or the whole string if no delimiter found)
    tokens.push_back(str.substr(start));
    
    return tokens;
}

std::map<std::string, std::string> mapSplit(const std::vector<std::string>& lines) {
    std::map<std::string, std::string> result;
    
    // Skip the first line (which is the request line)
    for (size_t i = 1; i < lines.size(); i++) {
        // Skip empty lines
        if (lines[i].empty()) {
            continue;
        }
        
        // Find the separator (colon)
        size_t colonPos = lines[i].find(':');
        if (colonPos != std::string::npos) {
            // Split into key-value
            std::string key = lines[i].substr(0, colonPos);
            std::string value = lines[i].substr(colonPos + 1);
            
            // Trim whitespace
            size_t valueStart = value.find_first_not_of(" \t");
            if (valueStart != std::string::npos) {
                value = value.substr(valueStart);
            }
            
            // Remove carriage returns
            size_t crPos;
            while ((crPos = value.find('\r')) != std::string::npos) {
                value.erase(crPos, 1);
            }
            
            result[key] = value;
        }
    }
    
    return result;
}