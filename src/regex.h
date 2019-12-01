#ifndef REGEX_H
#define REGEX_H

/***** Exported Defines *****/
#define REGEX_SURPRESS_LOGGING 1 << 0


char *regex(char *pattern, char *string, unsigned int options);

#endif
