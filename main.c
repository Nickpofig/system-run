// Just to remove Windows annoying stuff    -_-
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


void panic(const char *message, ...)
{
	va_list args;
	va_start(args, message);
	fprintf(stderr, "[Error]");
	vfprintf(stderr, message, args);
	fflush(stderr);
	va_end(args);
	exit(EXIT_FAILURE);
}

int string_is_empty(char *input)
{
	int result = 1;

	for (; (*input) != '\0'; input++) 
	{
		if (!strchr(" \n\t\r", (*input))) 
		{
			result = 0;
			break;
		}
	}

	return result;
}

void ignore_until_newline(const char *input, int length, int *index)
{
	do
	{
		if (input[*index] == '\n') return;
		(*index)++;
	}
	while(*index < length);
}

void ignore_comment(const char *input, int length, int *index)
{
	do
	{
		if (input[*index] != '#') return;
		ignore_until_newline(input, length, index);
	}
	while(*index < length);
}

char* read_label(char *input, int length, int *index)
{
	char *result = calloc(length - (*index), sizeof(char));

	int result_index = 0;
	int is_previous_newline = 0;

	// Skips first '@' label character.
	(*index)++;

	for (; *index < length; (*index)++)
	{
		if (input[*index] == '\n') is_previous_newline = 1;
		else if (input[*index] == '@' && is_previous_newline) continue;
		else if (is_previous_newline) break;
		else 
		{
			result[result_index] = input[*index];
			result_index++;
			is_previous_newline  = 0;
		}
	}

	// Makes sure we wont lose last read character.
	(*index)--; 

	// Trims result buffer.
	result = realloc(result, result_index + 1);
	result[result_index] = '\0';

	return result;
}

int execute_next_command(char *input, int input_length, int *input_index)
{
	char *label = NULL;
	char *output = calloc(input_length - (*input_index), sizeof(char));
	if (!output) panic("Could not allocate output!\n");


	int output_index = 0;

	int is_previous_newline    = 1;
	int is_previous_whitespace = 0;
	int is_current_whitespace;

	for (; *input_index < input_length; (*input_index)++)
	{
		is_current_whitespace = 0;

		if      (input[*input_index] == '\n') is_current_whitespace = 1;
		else if (input[*input_index] == '\t') is_current_whitespace = 1;
		else if (input[*input_index] == '\r') is_current_whitespace = 1;
		else if (input[*input_index] == ' ' ) is_current_whitespace = 1;
		else if (input[*input_index] == '#' && is_previous_newline) 
		{
			ignore_comment(input, input_length, input_index);
			continue;
		}
		else if (input[*input_index] == '@' && is_previous_newline)
		{
			label = read_label(input, input_length, input_index);
			continue;
		}
		// Checks for a command separation
		else if (input[*input_index] == '=' && is_previous_newline)
		{
			ignore_until_newline(input, input_length, input_index);
			break;
		}
		else 
		{
			output[output_index] = input[*input_index];
			output_index++;
		}
		
		if (is_current_whitespace && !is_previous_whitespace) 
		{
			output[output_index] = ' ';
			output_index++;
		}

		is_previous_whitespace = is_current_whitespace;
		is_previous_newline       = input[*input_index] == '\n';
	}


	// Trims the output (a.k.a the builded command)
	output = realloc(output, output_index + 1);
	output[output_index] = '\0';
	if (!output) panic("Failed to reallocate buffer of the next executable command!\n");

	// Prints the builded command
	if (label) printf("<+> %s\n", label);
	int output_is_empty = string_is_empty(output);
	if (!output_is_empty) printf("%s\n", output);
	fflush(stdout);

	// Executes the builded command
	int status_code = output_is_empty ? EXIT_SUCCESS : system(output);
	
	if (label) free(label);
	free(output);

	return status_code;
}


int main(int argc, char **argv)
{
	char *filename     = NULL;
	int is_sub_process = 0;

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--help") == 0) 
		{
			fprintf(stdout, 
				" +------- FILE FORMAT -------\n"
				" | When line starts with:\n"
				" |	'#'   -> the whole current line is skipped (comment line).\n"
				" |	'@'   -> the whole current line is printed to the standard output (label/description line).\n"
				" |	'='   -> symbol to separate calls. Represents the end of the current call and the start of the next call.\n"
				" +------- PROPERTIES --------\n"
				" |	<filepath>   -> path to the file containing \"calls\" in the accepatable \"format\"\n"
				" |	--help       -> prints this text information and exits.\n"
				" +---------------------------\n"
			);
			exit(EXIT_SUCCESS);
		}
		else 
		{
			filename = argv[i];
		}
	}

	if (!filename) 
	{
		panic("No source file has been provided!\n");
	}
	
	// Opens a file of the given path
	FILE *file  = fopen(filename, "r");
	if (!file) panic("Failed to read file: %s\n!", filename);

	// Calculates buffer size to store the file content
	fseek (file, 0, SEEK_END);
	long read_buffer_length = ftell (file);
	fseek (file, 0, SEEK_SET);
	
	// Allocates buffer to store the file content
	char *read_buffer = calloc (read_buffer_length, sizeof(char));
	if (!read_buffer) panic("Failed to allocate buffer to read file: %s\n!", filename);

	// Writes the file content into the buffer and closes file
	fread (read_buffer, 1, read_buffer_length, file);
	fclose (file);

	int read_buffer_index = 0;
	int status_code        = EXIT_SUCCESS;

	for (int number = 1; read_buffer_index < read_buffer_length; number++)
	{
		status_code = execute_next_command(read_buffer, read_buffer_length, &read_buffer_index);
		if (status_code) break;
	}
	
	free(read_buffer);

	return status_code;
}