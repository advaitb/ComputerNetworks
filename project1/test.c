#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
    char *s;
    s = (char *)malloc(10*sizeof(char));
    
    s = "Hello\r\n\r\n";

    char* last = s[strlen(s)-1];
    printf("The string is: %s", last);
    // free(s);
    return 0;
}
