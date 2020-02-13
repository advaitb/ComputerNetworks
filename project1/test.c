#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
    char *s;

    s = (char *)malloc(10*sizeof(char));
    char *token = "\r\n\r\n";
    s = "Hello\r\n\r\n";

    for (size_t i = 0; i < strlen(s); i++)
     if (s[i] == '\r' && s[i+1] == '\n' && s[i+2] == '\r' && s[i+3] == '\n')
        printf("braki");
    return 0;
}

