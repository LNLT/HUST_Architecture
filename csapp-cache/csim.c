#include "cachelab.h"
#include "stdlib.h"
#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "getopt.h"
typedef struct
{
    int valid;
    int tag;
    int lru;//每次替换lru最大的，每次命中或者进来的lru=0，其余有效的lru+1；
    //如果采用替换lru最小的，需要将进来的lru设成最大的，其余有效的lru-1；
    //不能采用进来的lru=0，命中的lru+1，每次替换最大的。
} Line;
typedef struct
{
    Line *lines;
} Group;
typedef struct
{
    Group *group;
} Cache;

int s = 0, E = 0, b = 0, v = 0; //组数位数、行数、块大小位数,现式轨迹信息
char *t = NULL;
int miss_count = 0;
int hit_count = 0;
int eviction_count = 0;
Cache cache;

void printhelp()
{
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n-h         Print this help message.\n");
    printf("-v         Optional verbose flag.\n");
    printf("-s <num>   Number of set index bits.\n");
    printf("-E <num>   Number of lines per set.\n");
    printf("-b <num>   Number of block offset bits.\n");
    printf("-t <file>  Trace file.\n");
    printf("Examples:\n");
    printf("linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

void initCache() //初始化cache
{
    cache.group = (Group *)malloc(sizeof(Group) * (int)pow(2,s));
    for (int i = 0; i < (int)pow(2,s); i++)
    {
        cache.group[i].lines = (Line *)malloc(sizeof(Line) * E);
        for (int j = 0; j < E; j++)
        {
            cache.group[i].lines[j].valid = 0;
            cache.group[i].lines[j].tag = 0;
            cache.group[i].lines[j].lru = 0;
        }
    }
}

//addr:行标记+组索引（s位）+块偏移（b位）
void findData(long int addr)
{
    unsigned group_num = addr >> b & ((int)pow(2, s) - 1); //取低s位
    unsigned addr_tag = addr >> b >> s;                    //行标记
    int hit_flag = 0;
    for (int i = 0; i < E; i++) //E:相联度 即每一个组内有几块
    {
        if (cache.group[group_num].lines[i].tag == addr_tag && cache.group[group_num].lines[i].valid == 1) //命中
        {
            hit_count++;
            printf("hit ");
            cache.group[group_num].lines[i].lru=0; //lur清零
            hit_flag = 1;
        }
        else if (cache.group[group_num].lines[i].valid == 1){
            cache.group[group_num].lines[i].lru++;
        }
    }
    if (hit_flag == 0) //不命中
    {
        miss_count++;
        printf("miss ");
        int avail_flag = 0;
        for (int j = 0; j < E; j++) //寻找放入位置,有空位置
        {
            if (cache.group[group_num].lines[j].valid == 0) //无效说明为空闲位置
            {
                avail_flag = 1;
                cache.group[group_num].lines[j].valid = 1;
                cache.group[group_num].lines[j].lru = 0;
                cache.group[group_num].lines[j].tag = addr_tag;
                break;
            }
        }
        if (avail_flag == 0)
        { //无空位置
            int lru = cache.group[group_num].lines[0].lru;
            int max = 0;
            for (int j = 0; j < E; j++) //找lru最小时的位置
            {
                if (cache.group[group_num].lines[j].lru > lru)
                {
                    lru = cache.group[group_num].lines[j].lru;
                    max = j;
                }
            }
            cache.group[group_num].lines[max].valid = 1;
            cache.group[group_num].lines[max].lru = 0;
            cache.group[group_num].lines[max].tag = addr_tag;
            printf("eviction ");
            eviction_count++;
        }
    }
}

void readFile(char *filename)
{
    FILE *file = fopen(filename, "r");
    char opt[5];
    long int addr;
    int off;
    while (fscanf(file, "%s %lx,%d", opt, &addr, &off) != EOF)
    {
        if (strstr(opt, "S") != NULL)
        {
            printf("S %lx,%d ", addr, off);
            findData(addr);
            printf("\n");
        }
        else if (strstr(opt, "L") != NULL)
        {
            printf("L %lx,%d ", addr, off);
            findData(addr);
            printf("\n");
        }
        else if (strstr(opt, "M") != NULL)
        {
            printf("M %lx,%d ", addr, off);
            findData(addr);
            findData(addr);
            printf("\n");
        }
    }
}
int main(int argc, char **argv)
{
    int c;
    while ((c = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch (c)
        {
        case 'h':
            printf("help\n");
            printhelp();
            break;
        case 'v':
            v = 1;
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            t = optarg;
            break;
        default:
            printf("default\n");
            printhelp();
            break;
        }
    }
    if (s == 0 || E == 0 || b == 0 || t == NULL)
    {
        printf("%s: Missing required command line argument\n", argv[0]);
        printhelp();
        exit(1);
    }
    initCache();
    readFile(t);
	for(int i=0;i<(int)pow(2,s);i++)	
	{
        free(cache.group[i].lines);
	}
	free(cache.group);
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
