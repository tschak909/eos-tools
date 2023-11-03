/**
 * @brief list an EOS directory
 * @author Thomas Cherryhomes
 * @email thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "eos_fs.h"

#define BUF_SIZE 7168            // a buffer for 7 blocks
#define DISK_SECTOR_SIZE 512     // floppy disk sector size
#define BLOCK_SIZE 1024          // AdamNet block size
#define INTERLEAVE 5             // disks have 5:1 interleave

char tmp[16];                    // temporary output buffer
uint8_t buf[BUF_SIZE];           // directory buffer

/**
 * @brief image mode, used for block write functions
 */
enum _image_mode
  {
    UNKNOWN,
    DSK,
    DDP
  } imageMode;

/**
 * @brief set image mode based on filename
 * @param fn the filename to check
 * @return 0 = unknown, 1 = DSK, 2 = DDP
 */
enum _image_mode set_image_mode(char *fn)
{
  if (strstr(fn,".dsk") ||
      strstr(fn,".DSK"))
    {
      imageMode=DSK;
      return DSK;
    }
  else if (strstr(fn,".ddp") ||
	   strstr(fn,".DDP"))
    {
      imageMode=DDP;
      return DDP;
    }
  else
    {
      imageMode=UNKNOWN;
      return UNKNOWN;
    }
}

/**
 * @brief is current entry the BLOCKS LEFT entry? (last file in dir)
 */
bool entry_is_blocks_left(DirectoryEntry *d)
{
  return (d->attributes & ENTRY_ATTRIBUTE_BLOCKS_LEFT) == ENTRY_ATTRIBUTE_BLOCKS_LEFT;
}

/**
 * @brief is current entry protected from execution?
 */
bool entry_is_exec_protect(DirectoryEntry *d)
{
  return (d->attributes & ENTRY_ATTRIBUTE_EXEC_PROTECT) == ENTRY_ATTRIBUTE_EXEC_PROTECT;
}

/**
 * @brief is current entry a deleted file?
 */
bool entry_is_deleted(DirectoryEntry *d)
{
  return (d->attributes & ENTRY_ATTRIBUTE_DELETED) == ENTRY_ATTRIBUTE_DELETED;
}

/**
 * @brief is current entry a system file?
 */
bool entry_is_system_file(DirectoryEntry *d)
{
  return (d->attributes & ENTRY_ATTRIBUTE_SYSTEM_FILE) == ENTRY_ATTRIBUTE_SYSTEM_FILE;
}

/**
 * @brief is current entry a user file?
 */
bool entry_is_user_file(DirectoryEntry *d)
{
  return (d->attributes & ENTRY_ATTRIBUTE_USER_FILE) == ENTRY_ATTRIBUTE_USER_FILE;
}

/**
 * @brief is current entry read protected?
 */
bool entry_is_read_protected(DirectoryEntry *d)
{
  return (d->attributes & ENTRY_ATTRIBUTE_READ_PROTECT) == ENTRY_ATTRIBUTE_READ_PROTECT;
}

/**
 * @brief is current entry write protected?
 */
bool entry_is_write_protected(DirectoryEntry *d)
{
  return (d->attributes & ENTRY_ATTRIBUTE_WRITE_PROTECT) == ENTRY_ATTRIBUTE_WRITE_PROTECT;
}

/**
 * @brief is current entry locked?
 */
bool entry_is_locked(DirectoryEntry *d)
{
  return (d->attributes & ENTRY_ATTRIBUTE_LOCKED) == ENTRY_ATTRIBUTE_LOCKED;
}

/**
 * @brief read directory sectors from DSK (interleaved)
 * @param fp File ptr to currently open image
 * @param buf ptr to 7168 byte buffer
 * @return true on success, false on error
 */
bool read_directory_sectors_dsk(FILE *fp)
{
  size_t l = 0;

  for (uint8_t blockNum = 1; blockNum < 8; blockNum++)
    {
      int r = blockNum % 4;
      uint32_t o1, o2;

      // Calculate seek offsets for each of the two disk sectors, according to
      // the 5:1 interleave of the DSK image format.
      o1 = blockNum * BLOCK_SIZE;
      o2 = ((r == 0) || (r == 1)) ? (blockNum * BLOCK_SIZE) + (INTERLEAVE * 512)
	: (blockNum * BLOCK_SIZE) - (4096 - (INTERLEAVE * 512));

      // Read upper sector
      fseek(fp,o1,SEEK_SET);
      if (fread(&buf[l],sizeof(uint8_t),DISK_SECTOR_SIZE,fp) != DISK_SECTOR_SIZE)
	{
	  perror("read_directory_sectors_dsk upper");
	  printf("Block #%u\n",blockNum);
	  return 0;
	}

      l += DISK_SECTOR_SIZE;

      // Read lower sector
      fseek(fp,o2,SEEK_SET);
      if (fread(&buf[l],sizeof(uint8_t),DISK_SECTOR_SIZE,fp) != DISK_SECTOR_SIZE)
	{
	  perror("read_directory_sectors_dsk lower");
	  printf("Block #%u\n",blockNum);
	  return false;
	}

      l += DISK_SECTOR_SIZE;

      blockNum++;
    }
  
  return true;
}

/**
 * @brief read directory sectors from DDP (non-interleaved)
 * @param fp File ptr to currently open image
 * @return # of bytes read
 */
bool read_directory_sectors_ddp(FILE *fp)
{
  size_t l = 0;
  
  for (uint8_t blockNum=1;blockNum<8;blockNum++)
    {
      size_t o = blockNum * BLOCK_SIZE;

      fseek(fp,o,SEEK_SET);

      if (fread(&buf[l],sizeof(uint8_t),BLOCK_SIZE,fp) != BLOCK_SIZE)
	{
	  perror("read_directory_sectors_ddp");
	  printf("Block #%u\n",blockNum);
	  return false;
	}
     
      blockNum++;
      l += BLOCK_SIZE;
    }

  return true;
}

/**
 * @brief read directory sectors into 7168 byte buffer
 * @verbose calls _dsk or _ddp depending on image mode
 * @param fp File ptr of currently open image
 * @param buf ptr to 7168 byte buffer
 * @return # of bytes read
 */
int read_directory_sectors(FILE *fp)
{
  if (imageMode==DSK)
    return read_directory_sectors_dsk(fp);
  else if (imageMode==DDP)
    return read_directory_sectors_ddp(fp);
}

/**
 * @brief return filename without type
 * @return ptr to temporary buffer containing string
 */
void eos_ls_filename(DirectoryEntry *d)
{
  char *p = d->filename;
  
  while (*p != 0x03)
    {
      putchar(*p++);
    }
}

/**
 * @brief Display directory entry (terse)
 * @param d pointer to directory entry to display
 */
void eos_ls_verbose(DirectoryEntry *d)
{
  // Attributes
  if (entry_is_blocks_left(d))
    putchar('L');
  else
    putchar('-');

  if (entry_is_exec_protect(d))
    putchar('X');
  else
    putchar('-');

  if (entry_is_deleted(d))
    putchar('D');
  else
    putchar('-');

  if (entry_is_system_file(d))
    putchar('S');
  else
    putchar('-');

  if (entry_is_user_file(d))
    putchar('U');
  else
    putchar('-');

  if (entry_is_read_protected(d))
    putchar('R');
  else
    putchar('-');

  if (entry_is_write_protected(d))
    putchar('W');
  else
    putchar('-');

  if (entry_is_locked(d))
    putchar('K');
  else
    putchar('-');

  // Starting block
  printf(" %10lu ",d->start_block);

  // Used/allocated size
  printf(" %5u / %-5u ",((d->blocks_used - 1)*BLOCK_SIZE) + d->last_block_bytes_used,d->allocated_blocks * BLOCK_SIZE);

  // Date
  printf(" %02u-%02u-%02u ",d->year,d->month,d->day);

  // File Name
  eos_ls_filename(d);

  // EOL
  putchar('\n');
}

/**
 * @brief Display directory entry (verbose)
 * @param d pointer to directory entry to display
 */
void eos_ls_terse(DirectoryEntry *d)
{
  eos_ls_filename(d);
  putchar('\n');
}

/**
 * @brief Display directory
 * @param buf pointer to read directory
 * @param verbose 1 = verbose 0 = terse
 */
void eos_ls(bool verbose)
{
  DirectoryEntry *d = (DirectoryEntry *)buf;

  if (verbose)
    {
      printf("\nVOLUME: ");
      eos_ls_filename(d);
      printf("\n\n");
    }

  d++;
  
  while (!entry_is_blocks_left(d))
    {
      if (verbose)
	{
	  eos_ls_verbose(d);
	}
      else
	{
	  eos_ls_terse(d);
	}

      d++;
    }

  if (verbose)
    {
      printf("\n %10u BYTES FREE\n\n",d->allocated_blocks * BLOCK_SIZE);
    }
}

/**
 * @brief Main entrypoint
 * @param argc Argument count on command line
 * @param argv ptr to arguments
 */
int main(int argc, char *argv[])
{
  FILE *fp = NULL;
  bool verbose=false;
  int i=1;
  
  if (argc<2)
    {
      printf("%s [-l] <image.dsk|ddp>\n\n",argv[0]);
      return 1;
    }

  while (i<argc)
    {
      if (argv[i][0]=='-' && argv[i][1]=='l')
	{
	  verbose=1;
	  i++;
	}
      else if (!set_image_mode(argv[i]))
	{
	  printf("Image filename must end with .dsk or .ddp\n");
	  return 1;
	}
      else
	{
	  fp = fopen(argv[i],"rb");
	  i++;
	}
    }
  
  if (!fp)
    {
      perror("could not open image");
      return 1;
    }

  read_directory_sectors(fp);

  eos_ls(verbose);
  
  return 0;
}
