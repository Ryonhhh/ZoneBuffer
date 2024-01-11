#define ZNS_PATH "/dev/nvme1n2"
#define LOG_PATH "./log_file"

#define FRAME_SIZE 4096
#define PAGE_SIZE 4096
#define CLUSTER_NUM 4
#define HC_LV 3
#define DEF_BUF_SIZE 2048
#define WORK_REG_SIZE 1536
#define MAX_ZONE_NUM 200

#define CAPACITY 0x1dc724000 //availible sector(512 byte) number
//blkzone capacity /dev/nvme1n2

typedef unsigned int ZONE_ID;
typedef long int off_st;
typedef long int PAGE_ID;
typedef int FRAME_ID;