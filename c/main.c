#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "erreur_builder.h"         // Generated builder for Erreur
#include "erreur_reader.h"          // Generated reader for Erreur
#include "erreur_verifier.h"        // Generated verifier for Erreur
#include "flatbuffers_common_builder.h"
#include "flatbuffers_common_reader.h"

/*#undef ns
#define ns(x) FLATBUFFERS_WRAP_NAMESPACE(erreurs, x) */

/* Helper function to read an entire file into memory.
   Returns a pointer to a buffer allocated with malloc() (or NULL on error).
   The caller must free() the buffer. */
unsigned char *read_file(const char *file_path, size_t *out_size) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", file_path);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    *out_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    unsigned char *buffer = (unsigned char *)malloc(*out_size);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return NULL;
    }
    if (fread(buffer, *out_size, 1, file) != 1) {
        fprintf(stderr, "Failed to read file: %s\n", file_path);
        free(buffer);
        fclose(file);
        return NULL;
    }
    fclose(file);
    return buffer;
}

/* Helper function for string access.
   flatbuffers_string_t in FlatCC is typically a char pointer. */
static inline const char *get_string_chars(flatbuffers_string_t s) {
    return s ? (const char *)s : "";
}

/* Function to deserialize an error from a FlatBuffer.
   It verifies the buffer and then extracts the error code and context. */
void deserialize_error(const void *buffer, size_t size, int16_t *out_code, const char **out_context) {
    if (!erreurs_Erreur_verify_as_root(buffer, size)) {
        fprintf(stderr, "Error buffer verification failed\n");
        *out_code = 0;
        *out_context = "";
        return;
    }
    erreurs_Erreur_table_t error = erreurs_Erreur_as_root(buffer);
    *out_code = erreurs_Erreur_code(error);
    if (erreurs_Erreur_contexte_is_present(error)) {
        flatbuffers_string_t contexte = erreurs_Erreur_contexte(error);
        *out_context = get_string_chars(contexte);
    } else {
        *out_context = "";
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <filename> <code> <context>\n", argv[0]);
        return 1;
    }
    const char *filename = argv[1];
    int16_t code = (int16_t)atoi(argv[2]);
    const char *context = argv[3];

    // Initialize the FlatCC builder.
    flatcc_builder_t builder, *B;
    B = &builder;
    flatcc_builder_init(B);

    // Create the FlatBuffer string for the context.
    flatbuffers_string_ref_t context_ref = flatcc_builder_create_string_str(B, context);

    //Create erreur table
    erreurs_Erreur_ref_t erreur = erreurs_Erreur_create(B, code, context_ref);

    // Get the finished buffer and its size.
    uint8_t *buffer;
    size_t size;
    buffer = flatcc_builder_finalize_buffer(B, &size);

    // Write the serialized data to a file.
    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Failed to create file: %s\n", filename);
        flatcc_builder_clear(B);
        return 1;
    }
    fwrite(buffer, size, 1, file);
    fclose(file);

    // Read the file back.
    size_t read_size;
    unsigned char *read_buffer = read_file(filename, &read_size);
    if (!read_buffer) {
        flatcc_builder_clear(B);
        return 1;
    }

    // Deserialize the error.
    int16_t out_code;
    const char *out_context;
    //deserialize_error(read_buffer, read_size, &out_code, &out_context);

    // Print results.
    printf("Prototype terminé.\ncode = %d\ncontexte = %s\nlongueur de la sérialisation = %zu\n",
           out_code, out_context, size);

    // Clean up.
    free(buffer);
    free(read_buffer);
    flatcc_builder_clear(B);
    return 0;
}