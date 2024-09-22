#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#define MAX_TRIGRAMS 1000
#define MAX_TRIGRAM_LEN 4  // 3 chars + null terminator

// Structure to hold trigrams and their counts
typedef struct {
    char trigram[MAX_TRIGRAM_LEN];
    int count;
} trigram_s_type;

// Function to extract trigrams from a string and store them in an array
static int extract_trigrams(const char *str, trigram_s_type trigrams[], int *count)
{
    int len = strlen(str);
    int i;
    *count = 0;

    if (len < 3) return 0;  // No trigrams possible if length < 3

    for (i = 0; i <= len - 3; i++) {
        trigram_s_type tg;
        strncpy(tg.trigram, &str[i], 3);
        tg.trigram[3] = '\0';  // Null-terminate the trigram

        // Check if trigram already exists
        int j;
        for (j = 0; j < *count; j++) {
            if (strcmp(trigrams[j].trigram, tg.trigram) == 0) {
                trigrams[j].count++;
                break;
            }
        }
        if (j == *count) {
            trigrams[*count] = tg;
            trigrams[*count].count = 1;
            (*count)++;
        }
    }
    return *count;
}

// Function to compute the dot product of two trigram arrays
static double dot_product(trigram_s_type trigrams1[], int count1, trigram_s_type trigrams2[], int count2)
{
    double dot = 0.0;
    int i, j;
    for (i = 0; i < count1; i++) {
        for (j = 0; j < count2; j++) {
            if (strcmp(trigrams1[i].trigram, trigrams2[j].trigram) == 0) {
                dot += trigrams1[i].count * trigrams2[j].count;
                break;
            }
        }
    }
    return dot;
}

// Function to compute the magnitude of a trigram array
static double magnitude(trigram_s_type trigrams[], int count)
{
    double mag = 0.0;
    int i;
    for (i = 0; i < count; i++) {
        mag += trigrams[i].count * trigrams[i].count;
    }
    return sqrt(mag);
}

// Function to compute cosine similarity between two strings based on their trigrams
static double str_cosine_similarity(const char *str1, const char *str2)
{
    char * w_str1 = strdup(str1);
    char * w_str2 = strdup(str2);

    for (int i= 0; i<strlen(str1); i++) {
        w_str1[i] = tolower(str1[i]);
    }
    for (int i= 0; i<strlen(str2); i++) {
        w_str2[i] = tolower(str2[i]);
    }

    trigram_s_type trigrams1[MAX_TRIGRAMS], trigrams2[MAX_TRIGRAMS];
    int count1, count2;
    
    // Extract trigrams
    extract_trigrams(w_str1, trigrams1, &count1);
    extract_trigrams(w_str2, trigrams2, &count2);

    // Compute cosine similarity
    double dot = dot_product(trigrams1, count1, trigrams2, count2);
    double mag1 = magnitude(trigrams1, count1);
    double mag2 = magnitude(trigrams2, count2);

    // Check for zero magnitudes
    if (mag1 == 0 || mag2 == 0) {
        int c = strcmp(w_str1, w_str2);
        c = (c == 0)?1:0;
        free(w_str1);
        free(w_str2);
        return (double)c;
    }
    free(w_str1);
    free(w_str2);

    return dot / (mag1 * mag2);
}

#ifdef TRIGRAPH_TESTING

int main() {
    char *str1 = "Quality self assessment report";
    char *str2 = "Quality assessment report";

    double similarity = str_cosine_similarity(str1, str2);
    printf("Cosine Similarity: %lf\n", similarity);

    return 0;
}

#endif // TRIGRAPH_TESTING
