/* Concatenate dirname and filename, and return an open
 * file pointer to file filename opened for writing in the
 * directory dirname (located in the current working directory)
 */
FILE *open_file_in_dir(char *filename, char *dirname);