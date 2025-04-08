#include <cstring>
#include <cstdlib>
#include <iostream>

// static int abs_value(int nbr) {
//     return (nbr < 0) ? -nbr : nbr;
// }

// static int get_len(int nbr) {
//     int len = 0;
    
//     if (nbr <= 0)
//         ++len;
    
//     while (nbr != 0) {
//         ++len;
//         nbr /= 10;
//     }
    
//     return len;
// }

// char* ft_itoa(int nbr) {
//     int len = get_len(nbr);
    
//     char* result = static_cast<char*>(malloc(sizeof(char) * (len + 1)));
//     if (result == NULL)
//         return NULL;
    
//     result[len] = '\0';
    
//     if (nbr < 0)
//         result[0] = '-';
//     else if (nbr == 0)
//         result[0] = '0';
    
//     while (nbr != 0) {
//         --len;
//         result[len] = abs_value(nbr % 10) + '0';
//         nbr /= 10;
//     }
    
//     return result;
// }

std::string ft_itoa(int nbr) {
    // Calculate length needed for string
    int len = 0;
    int temp = nbr;
    
    // Handle 0 as special case
    if (nbr == 0)
        return "0";
    
    // Count digits and handle negative sign
    if (nbr < 0)
        len++; // For negative sign
    
    while (temp != 0) {
        len++;
        temp /= 10;
    }
    
    // Create string with correct length
    std::string result(len, ' ');
    
    // Handle negative sign
    int start = 0;
    if (nbr < 0) {
        result[0] = '-';
        start = 1;
        nbr = -nbr; // Make positive for digit conversion
    }
    
    // Convert digits from right to left
    for (int i = len - 1; i >= start; i--) {
        result[i] = (nbr % 10) + '0';
        nbr /= 10;
    }
    
    return result;
}