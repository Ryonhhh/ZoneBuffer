#define ZNS_PATH "/dev/nvme4n2"
#define LOG_PATH "./log_file"

#define FRAME_SIZE 4096
#define PAGE_SIZE 4096
#define CLUSTER_NUM 4
#define HC_LV 3
#define DEF_BUF_SIZE 32768
#define WORK_REG_SIZE 24576
#define MAX_ZONE_NUM 500
#define MEM_ALIGN_SIZE 4096 

#define WRITE_BATCH 100

#define CAPACITY 0x1dc724000 //availible sector(512 byte) number
//blkzone capacity /dev/nvme1n2

typedef unsigned int ZONE_ID;
typedef long int off_st;
typedef long int PAGE_ID;
typedef int FRAME_ID;
typedef int ACCTIME;