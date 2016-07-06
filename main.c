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
#include <unistd.h>

#include "cpuid.h"
#include "rand.h"

// Add entropy at least this often, regardless of need (milliseconds)
static const int MAX_SLEEP = 300000;

// When woken up, fill until the pool has at least this much entropy
static const int FILL_WATERMARK = 3072;

static void send_entropy(struct entropy (*get_entropy)(void), int fd)
{
    struct entropy entropy = get_entropy();
    if (ioctl(fd, RNDADDENTROPY, &entropy) != 0) {
        perror("failed to add entropy");
    }
    memset(entropy.buf, 0, BUFFER_SIZE * sizeof(uint64_t));
    // Tell gcc that buf is used, so it doesn't optimize away the memset
    asm ("" : : "m" (entropy.buf[0]) : "memory" );
}

static void print_entropy(struct entropy (*get_entropy)(void))
{
    struct entropy entropy = get_entropy();

    for(int i = 0;;) {
      int w = write(1, &(entropy.buf[i]), entropy.size-i);
      if(w<0) {
        perror("failed to write entropy");
      }
      i+=w;
      if(i >= entropy.size)
        break;
    }
    memset(entropy.buf, 0, BUFFER_SIZE * sizeof(uint64_t));
    // Tell gcc that buf is used, so it doesn't optimize away the memset
    asm ("" : : "m" (entropy.buf[0]) : "memory" );
}

int main(int argc, char **argv)
{
    int random_fd, urandom_fd;
    int ent_count;
    struct pollfd pfd;

    struct entropy (*get_entropy)(void) = NULL;

    if (has_rdseed()) {
        fprintf(stderr, "CPU has RDSEED support\n");
        get_entropy = get_entropy_rdseed;
    } else if (has_rdrand()) {
        fprintf(stderr, "CPU has RDRAND support\n");
        get_entropy = get_entropy_rdrand;
    } else {
        error(EXIT_FAILURE, 0, "This CPU supports neither RDSEED nor RDRAND");
    }


    if(argc > 1) {
      while(1) {
        print_entropy(get_entropy);
      }
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
            send_entropy(get_entropy, random_fd);
        } while (ioctl(random_fd, RNDGETENTCNT, &ent_count) == 0 &&
                 ent_count < FILL_WATERMARK);
        send_entropy(get_entropy, urandom_fd);
        if (poll(&pfd, 1, MAX_SLEEP) == -1) {
            perror("poll failed");
        }
    }

    return 0;
}
