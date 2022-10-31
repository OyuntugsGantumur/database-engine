#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE 1

typedef struct {
    char* input;
    size_t inputLength;
} InputBuffer;

InputBuffer* create_buffer() {
    InputBuffer* buffer = malloc(sizeof(InputBuffer));
    buffer->input = NULL;
    buffer->inputLength = 0;

    return buffer;
}

// void read_query(InputBuffer* buffer) {
//     size_t input_size = getline(&(buffer->input), &(buffer->inputLength), stdin);

//     if(input_size == 0) {
//         printf("Error in reading query!");
//         EXIT(1);
//     }
// }

void free_buffer(InputBuffer* buffer) {
    free(buffer->input);
    free(buffer);
}

int main(int argc, char* argv[]) {
    InputBuffer* buffer = create_buffer();

    while (TRUE) {
        printf("input >");

        //Read the input from the command line
        size_t input_size = getline(&(buffer->input), &(buffer->inputLength), stdin);
        if(input_size == 0) {
            printf("Error in reading query!");
            exit(1);
        }

        if(strcmp(buffer->input, ".exit")) {
            free_buffer(buffer);
            exit(0);
        } else {
            printf("SUCCESS: %s", buffer->input);
        }

        // free_buffer(buffer);
    }
}