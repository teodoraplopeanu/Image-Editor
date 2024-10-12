#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
	int is_loaded;
	int width, height, max_value;
	int width_sel, height_sel;
	int x_st, y_st;
	unsigned char **data;
	char magic_number[3];
} PBM;

int clamp(int value, int min, int max)
{
	if (value < min)
		return min;
	if (value > max)
		return max;
	return value;
}

int check_numbers(char *buffer, int number)
{
	int cnt_spaces = 0;
	for (int i = 0; i < (int)(strlen(buffer)); i++)
		if (strchr(" -0123456789\n", buffer[i])) {
			if (buffer[i] == ' ')
				cnt_spaces++;
		} else {
			return 0;
		}
	if (cnt_spaces != number - 1)
		return 0;

	return 1;
}

void load(const char file[], PBM *image)
{
	// Free image data, if there has been another image previously loaded
	if (image->is_loaded == 1) {
		for (int i = 0; i < image->height; i++)
			free(image->data[i]);
		free(image->data);
	}

	FILE * f = fopen(file, "r");
	// Failed to open file
	if (!f) {
		printf("Failed to load %s\n", file);
		image->is_loaded = 0;
		return;
	}

	char magic_number[3];
	fscanf(f, "%2s", magic_number);

	if (magic_number[0] == 'P' &&
		(magic_number[1] <= '6' && magic_number[1] >= '1')) {
		FILE *fp;
		fclose(f);
		// Open again file
		if (magic_number[1] <= '3')
			fp = fopen(file, "rt");
		else
			fp = fopen(file, "rb");
		// Memory allocation error
		if (!fp) {
			printf("Failed to load %s\n", file);
			image->is_loaded = 0;
			return;
		}
		fscanf(fp, "%2s", image->magic_number);
		// Scanning the dimensions
		fscanf(fp, "%d%d%d", &image->width, &image->height, &image->max_value);

		// Setting the dimensions
		image->width_sel = image->width;
		image->height_sel = image->height;
		image->x_st = 0;
		image->y_st = 0;
		int channels = 1;	// PGM image
		if (magic_number[1] == '3' || magic_number[1] == '6')	// PPM image
			channels = 3;

		// Allocate memory for the image
		image->data = malloc(image->height * sizeof(unsigned char *));
		for (int i = 0; i < image->height; i++)
			image->data[i] = malloc(image->width * channels);
		// Memory allocation error
		if (!image->data) {
			fprintf(stderr, "Allocation failed\n");
			exit(-1);
		}

		// Scan image data
		if (magic_number[1] >= '1' && magic_number[1] <= '3') {
			for (int i = 0; i < image->height; i++)
				for (int j = 0; j < image->width; j++)
					for (int c = 0; c < channels; c++)
						fscanf(fp, "%hhu", &image->data[i][j * channels + c]);
		} else {
			fscanf(fp, "\n");
			for (int i = 0; i < image->height; i++)
				fread(image->data[i], 1, image->width * channels, fp);
		}

		// Close the file
		fclose(fp);

		// Update status
		image = image;
		image->is_loaded = 1;

		// Print status
		printf("Loaded %s\n", file);
	}
}

void select_region(char *buffer, PBM *image)
{
	// Print error
	if (image->is_loaded == 0) {
		printf("No image loaded\n");
		return;
	}

	int x1, y1, x2, y2;

	if (check_numbers(buffer, 4)) {
		if (sscanf(buffer, "%d %d %d %d", &x1, &y1, &x2, &y2) == 0)
			return;
	} else {
		printf("Invalid command\n");
		return;
	}

	// Arrange the coordinates in correct order
	if (y1 > y2 && x1 < x2) {
		int aux = y1;
		y1 = y2;
		y2 = aux;
	}

	if (y1 < y2 && x1 > x2) {
		int aux = x1;
		x1 = x2;
		x2 = aux;
	}

	if (y1 > y2 && x1 > x2) {
		int aux = y1;
		y1 = y2;
		y2 = aux;
		aux = x1;
		x1 = x2;
		x2 = aux;
	}

	// Print error
	if (x1 < 0 || x2 > image->width || y1 < 0 ||
		y2 > image->height || x1 == x2 || y1 == y2) {
		printf("Invalid set of coordinates\n");
		return;
	}

	// Update values
	image->width_sel = abs(x2 - x1);
	image->height_sel = abs(y2 - y1);
	image->x_st = x1;
	image->y_st = y1;

	// Print status
	printf("Selected %d %d %d %d\n", x1, y1, x2, y2);
}

void select_all(PBM *image)
{
	// Print error
	if (image->is_loaded == 0) {
		printf("No image loaded\n");
		return;
	}

	// Update coordiantes and dimensions
	image->height_sel = image->height;
	image->width_sel = image->width;
	image->x_st = 0;
	image->y_st = 0;

	// Print status
	printf("Selected ALL\n");
}

void histogram(char *buffer, PBM *image)
{
	// Print error
	if (!image->is_loaded) {
		printf("No image loaded\n");
		return;
	}

	int x, y;
	// Check number and type of arguments
	if (check_numbers(buffer, 3) == 1) {
		if (sscanf(buffer, "%d %d", &x, &y) == 0)
			return;
	} else {
		printf("Invalid command\n");
		return;
	}

	// Print error
	if (image->magic_number[1] == '3' || image->magic_number[1] == '6') {
		printf("Black and white image needed\n");
		return;
	}

	// Allocate memory for the histogram
	int *histogram, max = 0, *bins;
	histogram = malloc(256 * sizeof(int));
	bins = malloc(256 * sizeof(int));

	// Memory allocation error
	if (!histogram || !bins) {
		fprintf(stderr, "Allocation failed\n");
		exit(-1);
	}

	// Set values to 0
	for (int i = 0; i < 256; i++) {
		histogram[i] = 0;
		bins[i] = 0;
	}

	int length = 256 / y;

	// Calculate histogram
	for (int i = image->y_st; i < image->y_st + image->height_sel; i++)
		for (int j = image->x_st; j < image->x_st + image->width_sel; j++) {
			int pixel_value = image->data[i][j];
			histogram[pixel_value]++;
		}

	// Calculate the bins
	int ind = 0;
	while (ind++ < y) {
		for (int i = 0; i < length; i++)
			bins[ind] += histogram[ind * length + i];
	}

	// Calculate max value of the array
	for (int i = 0; i < y; i++)
		if (bins[i] > max)
			max = bins[i];

	// Calculate values of the histogram
	for (int i = 0; i < y; i++)
		bins[i] = floor((double)x * (bins[i] * 1. / max));

	// Print histogram
	for (int i = 0; i < y; i++) {
		printf("%d\t|\t", bins[i]);
		for (int j = 0; j < bins[i]; j++)
			printf("*");
		printf("\n");
	}

	// Free the memory allocated to the histogram
	free(histogram);
	free(bins);
}

void equalize(PBM *image)
{
	// Print error
	if (image->is_loaded == 0) {
		printf("No image loaded\n");
		return;
	}

	// Print error
	if (image->magic_number[1] == '3' || image->magic_number[1] == '6') {
		printf("Black and white image needed\n");
		return;
	}

	double area = image->width * image->height;

	int *h = malloc((image->max_value + 1) * sizeof(int));
	for (int i = 0; i <= image->max_value; i++)
		h[i] = 0;

	// Memory allocation error
	if (!h) {
		fprintf(stderr, "Allocation failed\n");
		exit(-1);
	}

	// Build frequency array
	for (int i = 0; i < image->height; i++)
		for (int j = 0; j < image->width; j++) {
			int val = image->data[i][j];
			h[val]++;
		}

	int h_min = h[0];
	for (int i = 1; i <= image->max_value; i++)
		if (h[i] < h_min)
			h_min = h[i];

	// Build array of partial sums
	for (int i = 1; i <= image->max_value; i++)
		h[i] += h[i - 1];

	// Calculate the equalized values
	for (int i = 0; i <= image->max_value; i++) {
		double val = (h[i] - h[0]) * 1. / (area - h[0]) * 255.;
		h[i] = clamp(round(val), 0, 255);
	}

	// Copy the results
	for (int i = 0; i < image->height; i++)
		for (int j = 0; j < image->width; j++)
			image->data[i][j] = h[image->data[i][j]];

	// Free the memory
	free(h);

	// Print status
	printf("Equalize done\n");
}

// Rotate the whole matrix
void rotate_all(int angle, PBM *image)
{
	int sign = angle / abs(angle);
	int count = abs(angle) / 90;
	// Channels
	int ch = 1;
	if (image->magic_number[1] == '3' || image->magic_number[1] == '6')
		ch = 3;
	// Temporary matrix
	int dim = fmax(image->height, image->width);
	unsigned char **temp = malloc(dim * ch * sizeof(unsigned char *));
	for (int i = 0; i < dim; i++)
		temp[i] = malloc(dim * ch * sizeof(unsigned char));

	// Memory allocation error
	if (!temp) {
		fprintf(stderr, "Memory allocation failed\n");
		exit(-1);
	}

	// Rotate the matrix
	while (count--) {
		int n = image->height;
		int m = image->width;

		for (int i = 0; i < image->height; i++)
			for (int j = 0; j < image->width; j++)
				for (int c = 0; c < ch; c++)
					if (sign > 0)
						temp[j][(n - i - 1) * ch + c] = image->data[i]
						[j * ch + c];
					else
						temp[m - j - 1][i * ch + c] = image->data[i]
						[j * ch + c];

		for (int i = 0; i < image->height; i++)
			free(image->data[i]);
		free(image->data);
		// Realloc image's memory, by the new dimension
		image->data = malloc(image->width * sizeof(unsigned char *));
		for (int i = 0; i < image->width; i++)
			image->data[i] = malloc(image->height * ch);

		// Copy the results
		for (int i = 0; i < image->width; i++)
			for (int j = 0; j < image->height; j++)
				for (int c = 0; c < ch; c++)
					image->data[i][j * ch + c] = temp[i][j * ch + c];

		// Update the dimensions
		int temp_dim = image->height;
		image->height = image->width;
		image->width = temp_dim;
		image->height_sel = image->height;
		image->width_sel = image->width;
	}

	// Free the memory
	for (int i = 0; i < dim; i++)
		free(temp[i]);
	free(temp);

	// Print status
	printf("Rotated %d\n", angle);
}

// Rotation to the left algorithm
void rotate_left(int i, int j, int n, int ch, PBM *image)
{
	for (int c = 0; c < ch; c++) {
		int temp = image->data[image->y_st + i][(image->x_st + j) * ch + c];

		image->data[image->y_st + i][(image->x_st + j) * ch + c] =
		image->data[image->y_st + j][(image->x_st + n - i - 1) * ch + c];

		image->data[image->y_st + j][(image->x_st + n - 1 - i) * ch + c] =
		image->data[image->y_st + n - 1 - i]
		[(image->x_st + n - 1 - j) * ch + c];

		image->data[image->y_st + n - 1 - i]
		[(image->x_st + n - 1 - j) * ch + c] =
		image->data[image->y_st + n - 1 - j][(image->x_st + i) * ch + c];

		image->data[image->y_st + n - 1 - j][(image->x_st + i) * ch + c] =
		temp;
	}
}

// Rotation to the right algorithm
void rotate_right(int i, int j, int n, int ch, PBM *image)
{
	for (int c = 0; c < ch; c++) {
		int temp = image->data[image->y_st + n - j - 1]
					[(image->x_st + i) * ch + c];

		image->data[image->y_st + n - j - 1][(image->x_st + i) * ch + c] =
		image->data[image->y_st + n - 1 - i]
		[(image->x_st + n - 1 - j) * ch + c];

		image->data[image->y_st + n - 1 - i]
		[(image->x_st + n - 1 - j) * ch + c] =
		image->data[image->y_st + j][(image->x_st + n - 1 - i) * ch + c];

		image->data[image->y_st + j][(image->x_st + n - 1 - i) * ch + c] =
		image->data[image->y_st + i][(image->x_st + j) * ch + c];

		image->data[image->y_st + i][(image->x_st + j) * ch + c] = temp;
	}
}

void rotate(int angle, PBM *image)
{
	// Print error
	if (!image->is_loaded) {
		printf("No image loaded\n");
		return;
	}

	// Print error
	if (abs(angle) % 90 != 0 || abs(angle) > 360) {
		printf("Unsupported rotation angle\n");
		return;
	}

	// Rotate the whole image
	if (image->x_st == 0 && image->y_st == 0 && image->height ==
		image->height_sel && image->width == image->width_sel) {
		rotate_all(angle, image);
		return;
	}

	// Print error
	if (image->height_sel != image->width_sel) {
		printf("The selection must be square\n");
		return;
	}

	// Direction of the rotation ('-' for left and '+' for right)
	int sign = angle / abs(angle);
	// How many times rotate 90 degrees
	int rotation_count = abs(angle) / 90;
	// Dimension
	int n = image->height_sel;
	// Channels
	int ch = 1;
	if (image->magic_number[1] == '3' || image->magic_number[1] == '6')
		ch = 3;

	// Rotate the image
	while (rotation_count--) {
		for (int i = 0; i < n / 2; i++) {
			//for (int j = i; j < n - i - 1; j++) {
			for (int j = i; j < n - i - 1; j++) {
				if (sign < 0)
					rotate_left(i, j, n, ch, image);
				else
					rotate_right(i, j, n, ch, image);
			}
		}
	}

	// Print status
	printf("Rotated %d\n", angle);
}

void crop(PBM *image)
{
	// Print error
	if (!image->is_loaded) {
		printf("No image loaded\n");
		return;
	}

	int channels = 1;	// PGM image
	if (image->magic_number[1] == '3' || image->magic_number[1] == '6')
		channels = 3;	// PPM image

	// Allocate memory for the new image data
	char **crop_data = malloc(image->height_sel * sizeof(char *));
	for (int i = 0; i < image->height_sel; i++)
		crop_data[i] = malloc(image->width_sel * channels);

	// Memory allocation error
	if (!crop_data) {
		fprintf(stderr, "Allocation failed\n");
		exit(-1);
	}

	// Copy the selected region to the new data array
	for (int i = 0; i < image->height_sel; i++)
		for (int j = 0; j < image->width_sel; j++)
			for (int c = 0; c < channels; c++)
				crop_data[i][j * channels + c] = image->data[image->y_st + i]
										[(image->x_st + j) * channels + c];

	// Free the old image data and update with the cropped data
	for (int i = 0; i < image->height; i++)
		free(image->data[i]);
	free(image->data);

	// Transfer new data
	image->data = (unsigned char **)crop_data;

	// Update the dimensions based on the selected region
	image->width = image->width_sel;
	image->height = image->height_sel;

	// Reset the starting coordinates for the cropped image
	image->x_st = 0;
	image->y_st = 0;

	// Print status
	printf("Image cropped\n");
}

// Check if element is on the edge of the matrix
int is_edge(int y, int x, PBM *image)
{
	int val1 = (image->y_st + y == 0 || y + image->y_st == image->height - 1);
	int val2 = (image->x_st + x == 0 || x + image->x_st == image->width - 1);
	return (val1 || val2);
}

void apply_parameter(double kernel[3][3], PBM *image)
{
	int channels = 3; // RGB image

	// Allocate memory for support matrix
	unsigned char **temp = malloc(image->height_sel * sizeof(unsigned char *));
	for (int i = 0; i < image->height_sel; i++)
		temp[i] = (unsigned char *)malloc(image->width_sel * channels);

	// Memory allocation error
	if (!temp) {
		fprintf(stderr, "Allocation failed\n");
		exit(-1);
	}

	// Apply kernel matrix to the image
	for (int y = 0; y < image->height_sel; y++)
		for (int x = 0; x < image->width_sel; x++) {
			//for (int c = 0; c < channels; c++) {
			int x_nou = image->x_st + x;
			int y_nou = image->y_st + y;
			if (is_edge(y, x, image)) {
				temp[y][x * 3 + 0] = image->data[y_nou][x_nou * 3 + 0];
				temp[y][x * 3 + 1] = image->data[y_nou][x_nou * 3 + 1];
				temp[y][x * 3 + 2] = image->data[y_nou][x_nou * 3 + 2];
			} else {
				double sum0 = 0, sum1 = 0, sum2 = 0;
				// Direction arrays for accessing neighbours
				for (int i = -1; i <= 1; i++)
					for (int j = -1; j <= 1; j++) {
						sum0 += kernel[i + 1][j + 1] * image->data[y_nou + i]
						[(x_nou + j) * 3 + 0];
						sum1 += kernel[i + 1][j + 1] * image->data[y_nou + i]
						[(x_nou + j) * 3 + 1];
						sum2 += kernel[i + 1][j + 1] * image->data[y_nou + i]
						[(x_nou + j) * 3 + 2];
					}
				sum0 = clamp((int)(round(sum0)), 0, 255);
				sum1 = clamp((int)(round(sum1)), 0, 255);
				sum2 = clamp((int)(round(sum2)), 0, 255);

				temp[y][x * channels + 0] = sum0;
				temp[y][x * channels + 1] = sum1;
				temp[y][x * channels + 2] = sum2;
			}
		}

	// Update the image data
	for (int i = 0; i < image->height_sel; i++)
		for (int j = 0; j < image->width_sel; j++)
			for (int c = 0; c <= 2; c++)
				image->data[image->y_st + i][(image->x_st + j) * 3 + c] =
				temp[i][j * 3 + c];

	// Free temp
	for (int i = 0; i < image->height_sel; i++)
		free(temp[i]);
	free(temp);
}

void apply(PBM *image)
{
	// Scan parameter to apply
	char *parameter = malloc(100 * sizeof(char));
	char c;
	parameter[0] = '\0';
	// Memory allocation error
	if (!parameter) {
		fprintf(stderr, "Allocation failed\n");
		exit(-1);
	}

	scanf("%c", &c);
	if (c == ' ')
		fgets(parameter, 100, stdin);

	// Print error
	if (!image->is_loaded) {
		printf("No image loaded\n");
		free(parameter);
		return;
	}

	if (c == '\n') {
		printf("Invalid command\n");
		free(parameter);
		return;
	}

	if (parameter[strlen(parameter) - 1] == '\n')
		parameter[strlen(parameter) - 1] = '\0';

	// Print error
	if (image->magic_number[1] != '3' && image->magic_number[1] != '6') {
		printf("Easy, Charlie Chaplin\n");
		free(parameter);
		return;
	}

	// Define kernel matrices for each parameter
	double kernel_edge[3][3] = {{-1, -1, -1}, {-1, 8, -1}, {-1, -1, -1}};

	double kernel_sharpen[3][3] = {{0, -1, 0}, {-1, 5, -1}, {0, -1, 0}};

	double kernel_blur[3][3] = {{1. / 9, 1. / 9, 1. / 9},
							 {1. / 9, 1. / 9, 1. / 9},
							 {1. / 9, 1. / 9, 1. / 9}};

	double kernel_gaussian_blur[3][3] = {{1. / 16, 2. / 16, 1. / 16},
										 {2. / 16, 4. / 16, 2. / 16},
										 {1. / 16, 2. / 16, 1. / 16}};

	// Apply certain parameter and print status
	if (strcmp(parameter, "EDGE") == 0) {
		apply_parameter(kernel_edge, image);
		printf("APPLY EDGE done\n");
	} else {
		if (strcmp(parameter, "SHARPEN") == 0) {
			apply_parameter(kernel_sharpen, image);
			printf("APPLY SHARPEN done\n");
		} else {
			if (strcmp(parameter, "BLUR") == 0) {
				apply_parameter(kernel_blur, image);
				printf("APPLY BLUR done\n");
			} else {
				if (strcmp(parameter, "GAUSSIAN_BLUR") == 0) {
					apply_parameter(kernel_gaussian_blur, image);
					printf("APPLY GAUSSIAN_BLUR done\n");
				} else {
					// Print error
					printf("APPLY parameter invalid\n");
					free(parameter);
					return;
				}
			}
		}
	}
	// Free memory
	free(parameter);
}

void save(const char file_name[], int ascii, PBM *image)
{
	// Print error
	if (!image->is_loaded) {
		printf("No image loaded\n");
		return;
	}

	int channels = 1;	// PGM image
	if (image->magic_number[1] == '3' || image->magic_number[1] == '6')
		channels = 3;	// PPM image

	if (ascii) {
		// Save in ASCII format
		FILE *f = fopen(file_name, "wt");

		// Faied to open file error
		if (!f) {
			printf("Failed to save %s\n", file_name);
			return;
		}

		char mag_nr = image->magic_number[1];

		if (mag_nr > '3')
			mag_nr -= 3;

		// Write into the file, in text mode
		fprintf(f, "P%c\n%d %d\n%d\n", mag_nr,
				image->width, image->height, image->max_value);

		for (int i = 0; i < image->height; i++) {
			for (int j = 0; j < image->width; j++)
				for (int c = 0; c < channels; c++)
					fprintf(f, "%hhu ", image->data[i][j * channels + c]);
			fprintf(f, "\n");
		}

		// Close the file
		fclose(f);
	} else {
		// Save in binary format
		FILE *f = fopen(file_name, "wb");

		// Faied to open file error
		if (!f) {
			printf("Failed to save %s\n", file_name);
			return;
		}

		char mag_nr = image->magic_number[1];

		if (mag_nr < '4')
			mag_nr += 3;

		// Write into the file, in binary mode
		fprintf(f, "P%c\n%d %d\n%d\n", mag_nr,
				image->width, image->height, image->max_value);

		for (int i = 0; i < image->height; i++)
			fwrite(image->data[i], 1, image->width * channels, f);

		// Close the file
		fclose(f);
	}

	// Print status
	printf("Saved %s\n", file_name);
}

void free_image_data(PBM *image)
{
	if (!image->is_loaded) {
		// Print error
		printf("No image loaded\n");
		return;
	}
	// Free image memory
	for (int i = 0; i < image->height; i++)
		free(image->data[i]);
	free(image->data);
	image->is_loaded = 0;
}

int main(void)
{
	char *command, file_name[101];
	int angle;
	PBM image = {0, 0, 0, 0, 0, 0, 0, 0, NULL, {0}};
	command = malloc(30 * sizeof(char));
	// Memory allocation error
	if (!command) {
		fprintf(stderr, "Allocation failed\n");
		exit(-1);
	}

	while (scanf("%s", command)) {
		if (strcmp(command, "LOAD") == 0) {
			scanf("%s", file_name);
			load(file_name, &image);
			continue;
		}

		if (strcmp(command, "SELECT") == 0) {
			char buffer[51];
			scanf(" ");
			fgets(buffer, sizeof(buffer), stdin);
			if (strstr(buffer, "ALL"))
				select_all(&image);
			else
				select_region(buffer, &image);
			continue;
		}

		if (strcmp(command, "HISTOGRAM") == 0) {
			char buffer[51];
			fgets(buffer, sizeof(buffer), stdin);
			histogram(buffer, &image);
			continue;
		}

		if (strcmp(command, "EQUALIZE") == 0) {
			equalize(&image);
			continue;
		}

		if (strcmp(command, "ROTATE") == 0) {
			scanf("%d", &angle);
			rotate(angle, &image);
			continue;
		}

		if (strcmp(command, "CROP") == 0) {
			crop(&image);
			continue;
		}

		if (strcmp(command, "APPLY") == 0) {
			apply(&image);
			continue;
		}

		if (strcmp(command, "SAVE") == 0) {
			scanf("%s", file_name);
			char buffer[7];
			fgets(buffer, sizeof(buffer), stdin);
			if (strstr(buffer, "ascii"))
				save(file_name, 1, &image);
			else
				save(file_name, 0, &image);
			continue;
		}

		if (strcmp(command, "EXIT") == 0) {
			free_image_data(&image);
			break;
		}

		char buffer[101];
		fgets(buffer, sizeof(buffer), stdin);
		printf("Invalid command\n");
	}
	free(command);

	return 0;
}
