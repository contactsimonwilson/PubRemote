#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*
 * Truncate a string to the specified number of characters
 * @param str The string to be truncated
 * @param max_len The number of characters to retain
 */
char *truncate_string(char *str, int max_len, bool ellipsis) {
  // Check if the string is NULL or max_len is non-positive
  if (str == NULL || max_len <= 0) {
    return str;
  }

  // Calculate the length of the string
  int len = strlen(str);

  // If the string is longer than max_len, truncate it
  if (len > max_len) {
    if (ellipsis && max_len >= 3) {
      // Append ellipsis if specified
      str[max_len - 3] = '\0';
      strcat(str, "...");
    }
    else {
      // No ellipsis or not enough space for ellipsis
      str[max_len] = '\0';
    }
  }

  return str;
};