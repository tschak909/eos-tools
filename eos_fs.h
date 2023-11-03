/**
 * @brief   eos_fs.h
 * @verbose filesystem structures
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3
 */

#ifndef EOS_FS_H
#define EOS_FS_H

#include <stdint.h>

#define EOS_FILENAME_LEN 12
#define MAX_DIR_ENTRIES 233  /* Maximum # of directory entries */

#define ENTRY_ATTRIBUTE_BLOCKS_LEFT 0x01
#define ENTRY_ATTRIBUTE_EXEC_PROTECT 0x02
#define ENTRY_ATTRIBUTE_DELETED 0x04
#define ENTRY_ATTRIBUTE_SYSTEM_FILE 0x08
#define ENTRY_ATTRIBUTE_USER_FILE 0x10
#define ENTRY_ATTRIBUTE_READ_PROTECT 0x20
#define ENTRY_ATTRIBUTE_WRITE_PROTECT 0x40
#define ENTRY_ATTRIBUTE_LOCKED 0x80

/* EOS directory entry */
typedef struct __attribute__((__packed__)) _directoryEntry
{
  uint8_t filename[EOS_FILENAME_LEN];
  uint8_t attributes;
  uint32_t start_block;
  uint16_t allocated_blocks;
  uint16_t blocks_used;
  uint16_t last_block_bytes_used;
  uint8_t year;
  uint8_t month;
  uint8_t day;
} DirectoryEntry;

#endif /* EOS_FS_H */
