/**
 * @brief   eos-make-image
 * @verbose create an EOS image with blank directory
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "eos_fs.h"

/**
 * @brief # of arguments required
 */
#define ARGS_REQD 4

/**
 * @brief EOS directory entry array
 */
DirectoryEntry entries[MAX_DIR_ENTRIES];

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
    return DSK;
  else if (strstr(fn,".ddp") ||
	   strstr(fn,".DDP"))
    return DDP;
  else
    return UNKNOWN;
}

/**
 * @brief Create image file via sparse write
 * @param fn pointer to filenam e
 * @param total_blocks size in # of 1024 byte blocks
 * @return errno
 */
size_t create_image(char *fn, uint32_t total_blocks)
{
  size_t sz = (total_blocks * 1024) - 1;
  FILE *fp = fopen(fn,"wb");

  if (!fp)
    {
      perror("create_image open");
      return errno;
    }

  fseek(fp,sz,SEEK_SET);
    
  fputc(0x00,fp);

  if (feof(fp))
    {
      perror("create_image fputc");
      fclose(fp);
      return errno;
    }
  
  fclose(fp);
  
  return sz;
}

int create_directory()
{
  // todo come back here
}

/**
 * @brief Make image optionally containing files
 * @param argc # of arguments from main()
 * @param argv array of arguments from main()
 * @return exit code to main()
 * @verbose argv[1] = filename, argv[2] = label, argv[3] = total blocks, argv[4] = dir blocks, argv[5] = opt dir
 */
int eos_make_image(int argc, char *argv[])
{
  char *fn = argv[1];
  char *label = argv[2];
  uint32_t total_blocks = atol(argv[3]);
  uint8_t dir_blocks = atoi(argv[4]);
  char *dir = NULL;

  if (!set_image_mode(fn))
    {
      printf("<fname> must contain either .dsk or .ddp\n");
      return(1);
    }

  printf("%16s: %s\n","Filename",fn);
  printf("%16s: %s\n","Label",label);
  printf("%16s: %lu\n","# Total Blocks",total_blocks);
  printf("%16s: %u\n","# Dir Blocks",dir_blocks);
  
  if (argc > ARGS_REQD)
    dir = argv[5];

  if (dir)
    printf("%16s: %s\n","Directory",dir);

  if (!create_image(fn,total_blocks))
    {
      printf("Could not create image file. Aborting.\n");
      return errno;
    }
  
  return 0;
}

/**
 * @brief main program entry point
 * @return exit code
 */
int main(int argc, char *argv[])
{
  if (argc < ARGS_REQD)
    {
      printf("%s <fname.ddp|dsk> <label> <total_blocks> <dir_blocks> [dir]\n\n",argv[0]);
      printf("%20s %s","<fname.ddp|dsk>","Filename for image with DDP or DSK extender\n");
      printf("%20s %s","<label>","Volume label (12 chars max)\n");
      printf("%20s %s","<total_blocks>","Total # of blocks for volume (4,294,967,296 max)\n");
      printf("%20s %s","<dir_blocks>","# of directory blocks for volume (6 max)\n");
      printf("%20s %s","[dir]","Optional name of directory of files to copy in\n\n");
      exit(1);
    }
  else
    return eos_make_image(argc,argv);
}
