/*
 * Copyright (c) 2011 Pekka Enberg
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */

#include "lib/zip.h"

#include "arch/byteorder.h"

#include "lib/hash-map.h"
#include "lib/string.h"

#include "vm/utf8.h"

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <zlib.h>

/*
 *	On-disk data structures
 */

/*
 * Local file header
 */

#define ZIP_LFH_SIGNATURE	0x04034b50

struct zip_lfh {
	uint32_t		signature;
	uint16_t		version;
	uint16_t		flags;
	uint16_t		compression;
	uint16_t		mod_time;
	uint16_t		mod_date;
	uint32_t		crc32;
	uint32_t		comp_size;
	uint32_t		uncomp_size;
	uint16_t		filename_len;
	uint16_t		extra_field_len;
} __attribute__((packed));

/*
 *	Central directory file header
 */

#define ZIP_CDSFH_SIGNATURE	0x02014b50

struct zip_cdfh {
	uint32_t		signature;
	uint16_t		version;
	uint16_t		version_needed;
	uint16_t		flags;
	uint16_t		compression;
	uint16_t		mod_time;
	uint16_t		mod_date;
	uint32_t		crc32;
	uint32_t		comp_size;
	uint32_t		uncomp_size;
	uint16_t		filename_len;
	uint16_t		extra_field_len;
	uint16_t		file_comm_len;
	uint16_t		disk_start;
	uint16_t		internal_attr;
	uint32_t		external_attr;
	uint32_t		lh_offset;	/* local header offset */
} __attribute__((packed));

/*
 *	End of central directory record
 */
#define ZIP_EOCDR_SIGNATURE	0x06054b50

struct zip_eocdr {
	uint32_t		signature;
	uint16_t		disk_number;
	uint16_t		cd_disk_number;
	uint16_t		disk_entries;
	uint16_t		total_entries;
	uint32_t		cd_size;	/* central directory size */
	uint32_t		offset;
	uint16_t		comment_len;
} __attribute__((packed));

static inline const char *cdfh_filename(struct zip_cdfh *cdfh)
{
	return (void *) cdfh + sizeof *cdfh;
}

static struct zip *zip_new(void)
{
	return calloc(1, sizeof(struct zip));
}

static void zip_delete(struct zip *zip)
{
	struct zip_entry *entry;
	unsigned int idx;

	zip_for_each_entry(idx, entry, zip) {
		free(entry->filename);
	}

	free_hash_map(zip->entry_cache);
	free_hash_map(zip->class_cache);
	free(zip->entries);
	free(zip);
}

void zip_close(struct zip *zip)
{
	if (!zip)
		return;

	munmap(zip->mmap, zip->len);

	close(zip->fd);

	zip_delete(zip);
}

static size_t zip_lfh_size(struct zip_lfh *lfh)
{
	return sizeof *lfh + le16_to_cpu(lfh->filename_len) + le16_to_cpu(lfh->extra_field_len);
}

static size_t zip_cdfh_size(struct zip_cdfh *cdfh)
{
	return sizeof *cdfh
		+ le16_to_cpu(cdfh->filename_len)
		+ le16_to_cpu(cdfh->extra_field_len)
		+ le16_to_cpu(cdfh->file_comm_len);
}

static int zip_eocdr_traverse(struct zip *zip, struct zip_eocdr *eocdr)
{
	unsigned long nr_entries, nr = 0;
	unsigned int idx;
	void *p;

	zip->entry_cache = alloc_hash_map(&string_key);
	if (!zip->entry_cache)
		goto error;

	zip->class_cache = alloc_hash_map(&pointer_key);
	if (!zip->class_cache)
		goto error;

	nr_entries = le16_to_cpu(eocdr->total_entries);

	zip->entries = calloc(nr_entries, sizeof(struct zip_entry));

	p = zip->mmap + le32_to_cpu(eocdr->offset);

	for (idx = 0; idx < nr_entries; idx++) {
		struct zip_cdfh *cdfh = p;
		struct zip_entry *entry;
		uint16_t filename_len;
		const char *filename;
		char *s;

		if (le32_to_cpu(cdfh->signature) != ZIP_CDSFH_SIGNATURE)
			goto error;

		filename_len = le16_to_cpu(cdfh->filename_len);

		filename = cdfh_filename(cdfh);
		if (filename[filename_len - 1] == '/')
			goto next;

		s = strndup(filename, filename_len);
		if (!s)
			goto error;

		entry = &zip->entries[nr++];

		entry->filename		= s;
		entry->comp_size	= le32_to_cpu(cdfh->comp_size);
		entry->uncomp_size	= le32_to_cpu(cdfh->uncomp_size);
		entry->lh_offset	= le32_to_cpu(cdfh->lh_offset);
		entry->compression	= le16_to_cpu(cdfh->compression);

		hash_map_put(zip->entry_cache, s, entry);

		if (!strncmp(s + filename_len - strlen(".class"), ".class", strlen(".class"))) {
			struct string *class_name;

			class_name = string_intern_cstr(strndup(s, strlen(s) - strlen(".class")));

			hash_map_put(zip->class_cache, class_name, entry);
		}

next:
		p += zip_cdfh_size(cdfh);
	}

	zip->nr_entries = nr;

	return 0;

error:
	return -1;
}

static struct zip_eocdr *zip_eocdr_find(struct zip *zip)
{
	void *p;

	for (p = zip->mmap + zip->len - sizeof(uint32_t); p >= zip->mmap; p--) {
		uint32_t *sig = p;

		if (le32_to_cpu(*sig) == ZIP_EOCDR_SIGNATURE)
			return p;
	}

	return NULL;
}

static struct zip *zip_do_open(const char *pathname)
{
	struct zip *zip;
	struct stat st;

	zip = zip_new();
	if (!zip)
		return NULL;

	zip->fd = open(pathname, O_RDONLY);
	if (zip->fd < 0)
		goto error_free;

	if (fstat(zip->fd, &st) < 0)
		goto error_free;

	zip->len = st.st_size;

	zip->mmap = mmap(NULL, zip->len, PROT_READ, MAP_SHARED, zip->fd, 0);
	if (zip->mmap == MAP_FAILED)
		goto error_close;

	return zip;

error_close:
	close(zip->fd);

error_free:
	zip_delete(zip);

	return NULL;
}

struct zip *zip_open(const char *pathname)
{
	struct zip_eocdr *eocdr;
	uint32_t *lfh_sig;
	struct zip *zip;

	zip = zip_do_open(pathname);
	if (!zip)
		return NULL;

	lfh_sig = zip->mmap;

	if (le32_to_cpu(*lfh_sig) != ZIP_LFH_SIGNATURE)
		goto error;

	eocdr = zip_eocdr_find(zip);
	if (!eocdr)
		goto error;

	if (zip_eocdr_traverse(zip, eocdr) < 0)
		goto error;

	return zip;

error:
	zip_close(zip);

	return NULL;
}

void *zip_entry_data(struct zip *zip, struct zip_entry *entry)
{
	struct zip_lfh *lfh;
	void *output;
	void *input;

	lfh = zip->mmap + entry->lh_offset;

	input = zip->mmap + entry->lh_offset + zip_lfh_size(lfh);

	output = malloc(entry->uncomp_size);
	if (!output)
		return NULL;

	switch (entry->compression) {
	case Z_DEFLATED: {
		z_stream zs;
		int err;

		zs.next_in	= input;
		zs.avail_in	= entry->comp_size;
		zs.next_out	= output;
		zs.avail_out	= entry->uncomp_size;

		zs.zalloc	= Z_NULL;
		zs.zfree	= Z_NULL;
		zs.opaque	= Z_NULL;

		if (inflateInit2(&zs, -MAX_WBITS) != Z_OK)
			goto error;

		err = inflate(&zs, Z_SYNC_FLUSH);
		if ((err != Z_STREAM_END) && (err != Z_OK))
			goto error;

		if (inflateEnd(&zs) != Z_OK)
			goto error;

		break;
	}
	case 0: {
		memcpy(output, input, entry->comp_size);
		break;
	}
	default:
		goto error;
	}

	return output;
error:
	free(output);
	return NULL;
}

struct zip_entry *zip_entry_find(struct zip *zip, const char *pathname)
{
	void *entry;

	if (hash_map_get(zip->entry_cache, pathname, &entry))
		return NULL;

	return entry;
}

struct zip_entry *zip_entry_find_class(struct zip *zip, struct string *classname)
{
	void *entry;

	if (hash_map_get(zip->class_cache, classname, &entry))
		return NULL;

	return entry;
}
