#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#define SPI0_BASE 0x20204000
#define SPI0_LEN 24

static volatile uint32_t * spi0Reg = MAP_FAILED;

static uint32_t * initMapMem(int fd, uint32_t addr, uint32_t len)
{
  return (uint32_t *) mmap(0, len,
   PROT_READ|PROT_WRITE|PROT_EXEC,
   MAP_SHARED|MAP_LOCKED,
   fd, addr);
}

int main(int argc, char * argv[])
{
 int fdMem;
 int i;

 if ((fdMem = open("/dev/mem", O_RDWR | O_SYNC) ) < 0)
 {
   fprintf(stderr,
    "\n********************************************************\n" \
    "Sorry, you don't have permission to run this program.\n" \
    "Try running as root, e.g. precede the command with sudo.\n\n");
   exit(-1);
 }

 spi0Reg = initMapMem(fdMem, SPI0_BASE, SPI0_LEN);

 close(fdMem);

 if (spi0Reg == MAP_FAILED)
 {
   fprintf(stderr, "Couldn't map spi0\n");
   exit(-1);
 }

 for (i=0; i<6; i++)
 {
   printf("%08X = %08X\n", spi0Reg+i, *(spi0Reg+i));
 }

 munmap((void *)spi0Reg, SPI0_LEN);

}