#include <cstring>
#include <cstdlib>
#include <iostream>

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