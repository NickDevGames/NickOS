int split(char *str, char delimiter, char *parts[], int maxParts) {
    int count = 0;
    parts[count++] = str; // first token starts at beginning

    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == delimiter) {
            str[i] = '\0'; // end current token
            if (count < maxParts)
                parts[count++] = &str[i + 1]; // next token starts after delimiter
        }
    }

    return count; // number of parts
}