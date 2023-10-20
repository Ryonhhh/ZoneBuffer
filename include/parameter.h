#define ZNS_PATH "/dev/nvme1n2"
#define LOG_PATH "./log_file"

#define MAX_PAGE_NUM (50 * 1024)
#define META_PAGE_SIZE (MAX_PAGE_NUM * 4)

#define FRAME_SIZE 4096
#define PAGE_SIZE 4096

#define CAPACITY 0x1dc724000 //availible sector(512 byte) number
//blkzone capacity /dev/nvme1n2

typedef unsigned int ZONE_ID;
typedef long long off_st;
typedef long long PAGE_ID;