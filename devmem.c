/*
 * devmem.c: Simple program to read/write from/to any location in memory.
 *
 *  Copyright (C) 2000, Jan-Derk Bakker (jdb@lartmaker.nl)
 *
 *
 * This software has been developed for the LART computing board
 * (http://www.lart.tudelft.nl/). The development has been sponsored by
 * the Mobile MultiMedia Communications (http://www.mmc.tudelft.nl/)
 * and Ubiquitous Communications (http://www.ubicom.tudelft.nl/)
 * projects.
 *
 *   Copyright (C) 2015, Trego Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <inttypes.h>
#ifdef X128
#include <emmintrin.h>
#endif
#include <time.h>

#define printerr(fmt, ...)               \
    do                                   \
{                                        \
    fprintf(stderr, fmt, ##__VA_ARGS__); \
    fflush(stderr);                      \
} while (0)

#define VER_STR "devmem version 1.0.0"

int f_dbg = 0;

static void usage(const char *cmd)
{
    fprintf(stderr, VER_STR "\n");
    fprintf(stderr, "\nUsage:\t%s [-switches] address [ type [ data ] ]\n"
            "\taddress : memory address to act upon\n"
            "\ttype    : access operation type : [b]yte, [h]alfword, [w]ord, [q]word\n"
            "\tdata    : data to be written\n\n"
            "Switches:\n"
            "\t-r      : read back after write\n"
            "\t-a      : do not check alignment\n"
            "\t--version | -V : print version\n"
            "\n",
            cmd);
}

#ifdef X128
void Write16ByteAligned(const __m128i* data, void* pDestination)
{
    _mm_store_si128((__m128i *)pDestination, *data);

}

#endif

uint64_t get_time_tick()
{
    struct timespec temp;
    clock_gettime(CLOCK_REALTIME, &temp);
    return((temp.tv_sec * 1000000000) + temp.tv_nsec);
}

int main(int argc, char **argv)
{
    int fd;
    void *map_base, *virt_addr;
    uint64_t read_result = -1, writeval = -1;
    uint64_t target;
    int access_type = 'w';
    int access_size = 4;
    unsigned int pagesize = (unsigned)getpagesize(); /* or sysconf(_SC_PAGESIZE)  */
    unsigned int map_size = pagesize;
    unsigned offset;
    char *endp = NULL;
    int f_readback = 0;    // flag to read back after write
    int f_align_check = 1; // flag to require alignment
    int f_loop = 1;        // flag to do looping
    const char *progname = argv[0];
    int opt;

    opterr = 0;
    while ((opt = getopt(argc, argv, "+raAdVl:")) != -1)
    {
        switch (opt)
        {
            case 'r':
                f_readback = 1;
                break;
            case 'a':
                f_align_check = 0;
                break;
            case 'A':
                // Absolute address mode. Does nothing now, for future compat.;
                break;
            case 'd':
                f_dbg = 1;
                break;
            case 'l':
                f_loop = strtoull(optarg, &endp, 0);
                ;
                break;
            case 'V':
                printf(VER_STR "\n");
                exit(0);
            default:
                if ((!argv[optind]) || 0 == strcmp(argv[optind], "--help"))
                {
                    usage(progname);
                    exit(1);
                }

                if (0 == strncmp(argv[optind], "--vers", 6))
                {
                    printf(VER_STR "\n");
                    exit(0);
                }

                printerr("Unknown long option: %s\n", argv[optind]);
                exit(2);
        }
    }

    argc -= optind - 1;
    argv += optind - 1;

    if (argc < 2 || argc > 4)
    {
        usage(progname);
        exit(1);
    }

    if (argc > 2)
    {
        if (!isdigit(argv[1][0]))
        {
            // Allow access_type be 1st arg, then swap 1st and 2nd
            char *t = argv[2];
            argv[2] = argv[1];
            argv[1] = t;
        }

        access_type = tolower(argv[2][0]);
        if (argv[2][1])
            access_type = '?';
    }

    errno = 0;
    target = strtoull(argv[1], &endp, 0);
    if (errno != 0 || (endp && 0 != *endp))
    {
        printerr("Invalid memory address: %s\n", argv[1]);
        exit(2);
    }

    switch (access_type)
    {
        case 'b':
            access_size = 1;
            break;
        case 'h':
            access_size = 2;
            break;
        case 'w':
            access_size = 4;
            break;
        case 'q':
            access_size = 8;
            break;
#ifdef X128
        case 'x':
            access_size = 16;
            break;
#endif
        default:
            printerr("Illegal data type: %s\n", argv[2]);
            exit(2);
    }

    if ((target + access_size - 1) < target)
    {
        printerr("ERROR: rolling over end of memory\n");
        exit(2);
    }

    if ((sizeof(off_t) < sizeof(int64_t)) && (target > UINT32_MAX))
    {
        printerr("The address %s > 32 bits. Try to rebuild in 64-bit mode.\n", argv[1]);
        // Consider mmap2() instead of this check
        exit(2);
    }

    offset = (unsigned int)(target & (pagesize - 1));
    if (offset + access_size > pagesize)
    {
        // Access straddles page boundary:  add another page:
        map_size += pagesize;
    }

    if (f_dbg)
    {
        printerr("Address: %#" PRIX64 " op.size=%d\n", target, access_size);
    }
    if (f_align_check && offset & (access_size - 1))
    {
        printerr("ERROR: address not aligned on %d!\n", access_size);
        exit(2);
    }

    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1)
    {
        printerr("Error opening /dev/mem (%d) : %s\n", errno, strerror(errno));
        exit(1);
    }
    //printf("/dev/mem opened.\n");
    //fflush(stdout);

    map_base = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED,
            fd,
            target & ~((typeof(target))pagesize - 1));
    if (map_base == (void *)-1)
    {
        printerr("Error mapping (%d) : %s\n", errno, strerror(errno));
        exit(1);
    }
    //printf("Memory mapped at address %p.\n", map_base);
    //fflush(stdout);

    virt_addr = map_base + offset;

    if (argc > 3)
    {
        int i;
        errno = 0;
        writeval = strtoull(argv[3], &endp, 0);
        if (errno || (endp && 0 != *endp))
        {
            printerr("Invalid data value: %s\n", argv[3]);
            exit(2);
        }

        if (access_size < sizeof(writeval) && 0 != (writeval >> (access_size * 8)))
        {
            printerr("ERROR: Data value %s does not fit in %d byte(s)\n", argv[3], access_size);
            exit(2);
        }

        uint64_t start_tick = get_time_tick();
        for (i = 0; i < f_loop; i++)
        {
            switch (access_size)
            {
                case 1:
                    *((volatile uint8_t *)virt_addr) = writeval;
                    break;
                case 2:
                    *((volatile uint16_t *)virt_addr) = writeval;
                    break;
                case 4:
                    *((volatile uint32_t *)virt_addr) = writeval;
                    break;
                case 8:
                    *((volatile uint64_t *)virt_addr) = writeval;
                    break;
#ifdef X128
                case 16:
                    {
                        uint64_t dx[2] __attribute__ ((aligned (16))) = {writeval,~writeval};
                        Write16ByteAligned((__m128i *)&dx, virt_addr);
                        break;
                    }
#endif
            }
        }
        uint64_t stop_tick = get_time_tick();
        uint64_t delta_tics = stop_tick - start_tick;
        if (f_loop > 1)
        {
            printf("%i writes in %f ms.\n", f_loop, delta_tics / 1000000.0);
            printf("%f ns per write.\n", (double)delta_tics / f_loop);
        }
        if (f_dbg)
        {
            printerr("Address: %#" PRIX64 " Written: %#" PRIX64 "\n", target, writeval);
            fflush(stdout);
        }
    }

    if (argc <= 3 || f_readback)
    {
        int i;
        for (i = 0; i < f_loop; i++)
        {
            switch (access_size)
            {
                case 1:
                    read_result = *((volatile uint8_t *)virt_addr);
                    break;
                case 2:
                    read_result = *((volatile uint16_t *)virt_addr);
                    break;
                case 4:
                    read_result = *((volatile uint32_t *)virt_addr);
                    break;
                case 8:
                    read_result = *((volatile uint64_t *)virt_addr);
                    break;
            }
        }

        //printf("Value at address 0x%lld (%p): 0x%lu\n", (long long)target, virt_addr, read_result);
        //fflush(stdout);
        if (f_readback && argc > 3)
            printf("Written 0x%" PRIX64 "; readback 0x%" PRIX64 "\n", writeval, read_result);
        else
            printf("%08" PRIX64 "\n", read_result);
        fflush(stdout);
    }

    if (munmap(map_base, map_size) != 0)
    {
        printerr("ERROR munmap (%d) %s\n", errno, strerror(errno));
    }

    close(fd);

    return 0;
}

