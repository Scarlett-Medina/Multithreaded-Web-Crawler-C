# Multithreaded Web Crawler in C

Project Description:


## Roles:

Scarlett – Thread Management & Input Handling

- Reads input URLs from 'urls.txt'
- Validates URLs (must start with 'http://' or 'https://')
- Creates one thread per valid URL using 'pthread'
- Passes each thread the assigned URL and index
- Logs which thread handles which URL

.
.
.

## Project Files

| File          | Description                                  |
|---------------|----------------------------------------------|
| 'crawler.c'   | Main program: input handling and threading   |
| 'Makefile'    | Build instructions using `make`              |
| 'urls.txt'    | List of URLs to be processed (one per line)  |

## Install Required Packages

(I am using Visual Studio Code with the WSL extension to work on the project inside the Ubuntu environment) -Scar

- sudo apt update
- sudo apt install build-essential make
- sudo apt install dos2unix

## How to Run

Step 1: Navigate to the project folder

cd ~/web-crawler

Step 2: Compile the program

make

Step 3: Execute

./crawler
