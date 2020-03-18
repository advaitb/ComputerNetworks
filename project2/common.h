#define MAX_DATA_SIZE 1000 /* max data size is 1000 bytes */
#define MAX_PACK_SIZE 1024 /* max packet size */
#define TIMEOUT 30 /* let timeout be 30 seconds */
//#define INIT_TIME 100  /* dummy time to set */
#define WINDOW_SIZE 20  /* size of the window */
#define BUFFER_SIZE 100 /* size of the buffer */
#define ACKN_SIZE 6 
#define MSB_MASK 0xFFFF0000
#define LSB_MASK 0xFFFF

char csum(char *packet, int cnt) {
    unsigned long pack_sum = 0;
    while(cnt > 0) {
        pack_sum += *packet++;
        if (pack_sum & MSB_MASK) { /* standard checksum */
            pack_sum = pack_sum & LSB_MASK;
            pack_sum++; /* increment sum */
        }
        cnt-=1;
    }
    return (pack_sum & LSB_MASK); /* final bit-wise and */
}
