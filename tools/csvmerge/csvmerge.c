#include <racdoor/buffer.h>
#include <racdoor/util.h>

#include <string.h>

int main(int argc, char** argv)
{
	const char* output_path = NULL;
	Buffer* input_csvs = checked_malloc(argc * sizeof(Buffer));
	const char** input_paths = checked_malloc(argc * sizeof(char*));
	u32 max_output_size = 0;
	u32 input_csv_count = 0;
	
	for (int i = 1; i < argc; i++)
		if (strcmp(argv[i], "-o") == 0)
			output_path = argv[++i];
		else
		{
			input_csvs[input_csv_count] = read_file(argv[i]);
			input_paths[input_csv_count] = argv[i];
			max_output_size += input_csvs[input_csv_count].size;
			input_csv_count++;
		}
	
	CHECK(input_csv_count > 0 || !output_path,
		"usage: %s <input csvs...> -o <output csv>\n",
		(argc > 0) ? argv[0] : "csvmerge");
	
	u32 header_size = 0;
	while (input_csvs[0].data[header_size] != '\0' && input_csvs[0].data[header_size] != '\n')
		header_size++;
	
	CHECK(input_csvs[0].data[header_size++] == '\n', "Unexpected end of input file '%s'.\n", input_paths[0]);
	
	Buffer output_csv = {
		.data = malloc(max_output_size),
		.size = 0
	};
	
	memcpy(output_csv.data, input_csvs[0].data, header_size);
	output_csv.size += header_size;
	
	for (u32 i = 0; i < input_csv_count; i++)
	{
		CHECK(input_csvs[i].size >= header_size
			&& memcmp(input_csvs[i].data, input_csvs[0].data, header_size) == 0,
			"CSV files '%s' and '%s' have mismatched headers.", input_paths[0], input_paths[i]);
		
		memcpy(output_csv.data + output_csv.size, input_csvs[i].data + header_size, input_csvs[i].size - header_size);
		output_csv.size += input_csvs[i].size - header_size;
	}
	
	write_file(output_path, output_csv);
	
	return 0;
}
