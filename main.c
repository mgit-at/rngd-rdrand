/*
 * A simple rngd to collect entropy using rdrand (Intel Bull Mountain) and feed
 * into the /dev/random entropy pool.
 *
 * Copyright (C) 2012 Ben Jencks <ben@bjencks.net>
 * Written while looking at rng-tools, copyright 2001/2004 Philipp Rumpf and
 * Henrique de Moraes Holschuh
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <error.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>
#include <linux/random.h>

#include "cpuid.h"

// If rdrand fails, retry this many times
static const int RETRY_COUNT = 10;

// >1022 calls to 64-bit rdrand is guaranteed to include a reseed.
// https://software.intel.com/en-us/articles/intel-digital-random-number-generator-drng-software-implementation-guide
//   "The DRBG autonomously decides when it needs to be reseeded to refresh the
//   random number pool in the buffer and is both unpredictable and transparent
//   to the RDRAND caller. An upper bound of 511 128-bit samples will be
//   generated per seed. That is, no more than 511*2=1022 sequential DRNG
//   random numbers will be generated from the same seed value."
#define RANDOM_COUNT 1024

// The seed is 256 bits, so that's the amount of actual entropy we're adding
static const int ENTROPY_INCREMENT = 256;

// Add entropy at least this often, regardless of need (milliseconds)
static const int MAX_SLEEP = 300000;

// When woken up, fill until the pool has at least this much entropy
static const int FILL_WATERMARK = 3072;

static inline void fill_random_buf(uint64_t *buf)
{
    register unsigned int goodcalls = 0;
    for (int i = 0; i < RANDOM_COUNT; i++)
    {
        register int counter = RETRY_COUNT;
        register uint64_t val;
        asm volatile ("1:\t"
                      "dec %1\n\t"
                      "je 2f\n\t"
                      "rdrand %0\n\t"
                      "jnc 1b\n\t"
                      "2:\t"
                      "adc $0,%2" : "=r" (val), "+g" (counter), "+g" (goodcalls) : : "cc");
        buf[i] = val;
    }
    // Keeping a counter and checking later takes a branch out of the inner loop
    if (goodcalls < RANDOM_COUNT) {
        error(EXIT_FAILURE, 0,
              "Error: rdrand failed %d times\n", RANDOM_COUNT - goodcalls);
    }
}

static void send_entropy(int fd)
{
    struct {
        int ent_count;
        int size;
        uint64_t buf[RANDOM_COUNT];
    } entropy;
    entropy.ent_count = ENTROPY_INCREMENT;
    entropy.size = RANDOM_COUNT * sizeof(uint64_t);
    fill_random_buf(entropy.buf);
    if (ioctl(fd, RNDADDENTROPY, &entropy) != 0) {
        perror("failed to add entropy");
    }
    memset(entropy.buf, 0, RANDOM_COUNT * sizeof(uint64_t));
    // Tell gcc that buf is used, so it doesn't optimize away the memset
    asm ("" : : "m" (entropy.buf[0]) : "memory" );
}

int main(int argc, char **argv)
{
    int random_fd, urandom_fd;
    int ent_count;
    struct pollfd pfd;

    if (!has_rdrand()) {
        error(EXIT_FAILURE, 0, "This CPU does not support RDRAND");
    }

    fprintf(stderr, "Starting\n");
    random_fd = open("/dev/random", O_RDWR);
    if (random_fd == -1) {
        perror("Failed to open /dev/random");
    }
    urandom_fd = open("/dev/urandom", O_RDWR);
    if (urandom_fd == -1) {
        perror("Failed to open /dev/urandom");
    }

    pfd.fd = random_fd;
    pfd.events = POLLOUT;

    while (1) {
        do {
            send_entropy(random_fd);
        } while (ioctl(random_fd, RNDGETENTCNT, &ent_count) == 0 &&
                 ent_count < FILL_WATERMARK);
        send_entropy(urandom_fd);
        if (poll(&pfd, 1, MAX_SLEEP) == -1) {
            perror("poll failed");
        }
    }

    return 0;
}
