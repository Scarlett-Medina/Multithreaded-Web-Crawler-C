#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h> // libcurl: HTTP client library for downloading web pages

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

/**
 * write_data
 *
 * libcurl write callback: writes 'size * nmemb' bytes from ptr into stream.
 *
 * @param ptr    Pointer to the incoming data buffer.
 * @param size   Size of each data element (in bytes).
 * @param nmemb  Number of elements in the data buffer.
 * @param stream FILE* to which data should be written.
 * @return       Number of bytes successfully written.
 */
size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream) {
    // fwrite writes nmemb elements of size 'size' from ptr into our FILE*
    return fwrite(ptr, size, nmemb, (FILE*)stream);
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
        buffer[strcspn(buffer, "\r\n")] = '\0'; // Remove newline

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

/**
 * fetch_html
 *
 * Downloads the HTML content of the given URL and saves it to "page<index>.html".
 *
 * Steps:
 *  1. Initialize a CURL handle.
 *  2. Construct an output filename using the thread’s index.
 *  3. Open the file for writing.
 *  4. Configure libcurl to fetch the URL and write via write_data().
 *  5. Perform the HTTP GET request.
 *  6. Report any errors or confirm success.
 *  7. Clean up resources.
 *
 * @param url    The web address to download.
 * @param index  Numeric identifier used to name the output file.
 */
void fetch_html(const char* url, int index) {
    // 1) Start libcurl
    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "[ERROR] curl init failed (out of memory?)\n");
        return;
    }

    // 2) Prepare a filename for this page
    char filename[64];
    snprintf(filename, sizeof(filename), "page%d.html", index);

    // 3) Open the file for writing the HTML
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "[ERROR] cannot open %s for writing\n", filename);
        curl_easy_cleanup(curl);
        return;
    }

    // 4) Configure curl:
    curl_easy_setopt(curl, CURLOPT_URL, url);               // which web page
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); // writer callback
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);           // file to write into

    // 5) Actually perform the HTTP GET
    CURLcode res = curl_easy_perform(curl);

    // 6) Check result
    if (res != CURLE_OK) {
        fprintf(stderr,
                "[ERROR] download failed for %s: %s\n",
                url,
                curl_easy_strerror(res)); // human-readable error
    } else {
        printf("[INFO] Saved HTML to %s\n", filename);
    }

    // 7) Clean up
    fclose(fp);
    curl_easy_cleanup(curl);
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

    // Fetch the page and save it to page<index>.html
    fetch_html(data->url, data->index);

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
int main(void) 
{
    // Prepare libcurl for use across all threads
    curl_global_init(CURL_GLOBAL_DEFAULT);

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
    
    // Clean up any global resources allocated by libcurl
    curl_global_cleanup();

    return 0;
}
