#ifndef JATO__LIB_ZIP_H
#define JATO__LIB_ZIP_H

#include <stdint.h>
#include <stddef.h>

struct hash_map;
struct string;

/*
 *	In-memory data structures
 */

struct zip_entry {
	char			*filename;
	uint32_t		comp_size;
	uint32_t		uncomp_size;
	uint32_t		lh_offset;
	uint16_t		compression;
};

struct zip {
	int			fd;
	size_t			len;
	void			*mmap;
	unsigned long		nr_entries;
	struct zip_entry	*entries;
	struct hash_map		*entry_cache;
	struct hash_map		*class_cache;
};

#define zip_for_each_entry(idx, entry, zip)		\
	for (idx = 0, entry = &zip->entries[0];		\
		idx < zip->nr_entries;			\
		entry = &zip->entries[++idx])

struct zip *zip_open(const char *pathname);
void zip_close(struct zip *zip);
struct zip_entry *zip_entry_find(struct zip *zip, const char *filename);
struct zip_entry *zip_entry_find_class(struct zip *zip, struct string *classname);
void *zip_entry_data(struct zip *zip, struct zip_entry *entry);

#endif /* JATO__LIB_ZIP_H */
