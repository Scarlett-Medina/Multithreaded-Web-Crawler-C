#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_URLS 100         // Maximum number of URLs to process
#define MAX_URL_LENGTH 2048  // Max length for each URL string

// Global array to store URLs read from the input file
char* urls[MAX_URLS];
int url_count = 0;           // Total number of URLs read

/*
 * Custom string duplication function (safe alternative to strdup)
 * Allocates memory for a copy of the input string and returns it
 */
char* my_strdup(const char* src) 
{
    char* dst = malloc(strlen(src) + 1); 
    if (dst) {
        strcpy(dst, src);
    }
    return dst;
}

/*
 * Checks if the URL starts with "http://" or "https://".
 * Very basic validation—doesn't verify domain structure.
 */
int is_valid_url(const char* url) 
{
    return strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0;
}


/*
 * Struct to pass multiple arguments to threads.
 * Each thread gets a URL and a numeric index (used to name output files like page1.html)
 */
typedef struct 
{
    char* url;
    int index;  
} ThreadArg;

/*
 * Function: read_urls
 * --------------------
 * Reads URLs line-by-line from a file called "urls.txt"
 * and stores them into the global `urls` array.
 */
void read_urls(const char* filename) 
{
    FILE* file = fopen(filename, "r");
    
    // Error Handling: If urls.txt doesn't exist
    if (!file) {
        perror("[ERROR] Could not open urls.txt");
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_URL_LENGTH];
    while (fgets(buffer, sizeof(buffer), file) && url_count < MAX_URLS) {
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline

        if (is_valid_url(buffer)) {
            urls[url_count] = my_strdup(buffer);
            url_count++;
        } else {
            // Logging: Warning when URL is invalid
            printf("[WARNING] Skipped invalid URL: %s\n", buffer);
        }
    }

    fclose(file);

    // Error Handling: No valid URLs in file
    if (url_count == 0) {
        fprintf(stderr, "[ERROR] No valid URLs found in urls.txt\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Thread function that processes a single URL.
 * Logs the URL being handled, and calls fetch_html() to process it.
 */
void* thread_worker(void* arg) 
{
    ThreadArg* data = (ThreadArg*)arg;

    // Logging: Show which thread is starting and what URL it's working on
    printf("[INFO] Thread %d: Handling URL: %s\n", data->index, data->url);

    // Placeholder for teammate's HTML fetching function(Just a example)
    // Example: fetch_html(data->url, data->index);

    // Logging: Thread is done
    printf("[INFO] Thread %d: Finished processing.\n", data->index);

    free(data->url);
    free(data);
    pthread_exit(NULL);
}

/*
 * Main function:
 *  - Reads input URLs
 *  - Creates one thread per URL
 *  - Waits for all threads to complete
 */
int main() 
{
    // Logging: Indicate program started
    printf("[INFO] Web Crawler started.\n");

    read_urls("urls.txt");

    pthread_t threads[MAX_URLS];

    for (int i = 0; i < url_count; i++) 
    {
        ThreadArg* arg = malloc(sizeof(ThreadArg));
        
        // Error Handling: Memory allocation failure
        if (!arg) {
            perror("[ERROR] Failed to allocate memory for thread argument");
            exit(EXIT_FAILURE);
        }

        arg->url = my_strdup(urls[i]);  // Pass a safe copy of the URL
        arg->index = i + 1;             // Index used to name output files

        int rc = pthread_create(&threads[i], NULL, thread_worker, arg);

        // Error Handling: Failed to create thread
        if (rc) 
        {
            fprintf(stderr, "[ERROR] Failed to create thread %d\n", i + 1);
            free(arg->url);
            free(arg);
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < url_count; i++) 
    {
        pthread_join(threads[i], NULL);
    }

    // Free global URL memory
    for (int i = 0; i < url_count; i++) 
    {
        free(urls[i]);
    }

    // Logging: Crawler finished
    printf("[INFO] Web Crawler finished. All threads completed.\n");

    return 0;
}
