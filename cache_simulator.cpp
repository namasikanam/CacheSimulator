#include <bits/stdc++.h>
#include "bitvector.h"
using namespace std;

void print64(uint64_t x)
{
    for (int i = 64; i--;)
        putchar('0' ^ x >> i & 1);
}

constexpr int FILE_NUM = 7;
struct CacheFile
{
    string filename;
    FILE *tracefile, *logfile;
    CacheFile(string filename) : filename(filename)
    {
        tracefile = fopen(("trace/" + filename + ".trace").c_str(), "r");
        logfile = fopen(("trace/" + filename + ".log").c_str(), "w");
    }
    ~CacheFile()
    {
        fclose(tracefile);
        fclose(logfile);
    }
} cacheFiles[FILE_NUM] = {CacheFile("astar"), CacheFile("bzip2"), CacheFile("gcc"), CacheFile("mcf"), CacheFile("perlbench"), CacheFile("swim"), CacheFile("twolf")};
// } cacheFiles[FILE_NUM] = {CacheFile("astar"), CacheFile("bzip2"), CacheFile("gcc"), CacheFile("mcf"), CacheFile("perlbench"), CacheFile("swim"), CacheFile("twolf")};
enum OperationType
{
    load,
    store
};
struct Operation
{
    OperationType op_t;
    uint64_t addr;

    Operation(OperationType op_t, uint64_t addr) : op_t(op_t), addr(addr) {}
};
vector<Operation> trace[FILE_NUM];

// ***** meta datas
constexpr int WORD_LENGTH = 64;
constexpr int CACHE_SIZE = 128 * 1024;
constexpr int CACHE_LENGTH = 17;
// **** strategies
void (*cache_replacement)(int, uint64_t);
bool (*cache_write)(uint64_t);
void (*cache_meta_update)(int, int);
// **** cache layouts
vector<vector<BitVector>> tags;
uint64_t tag_mask, index_mask;
int grp_num, way_num;
int tag_length, index_length, block_length, way_length;
// **** replacement strategies
// *** lru
vector<BitVector> stacks;
// *** random
random_device rd;
mt19937 eng(rd());
uniform_int_distribution<> distr;
// *** bt
vector<BitVector> bts;

void cache_assign(int grp_id, int way_id, uint64_t tag)
{
    // printf("cache_assign(grp_id = %d, way_id = %d, tag = %" PRIx64 "\n", grp_id, way_id, tag);

    tags[grp_id][way_id] = tag | 1ull << tag_length;

    // printf("In the assignment\n");

    cache_meta_update(grp_id, way_id);
}

#define HIT true
#define MISS false
bool cache_read(uint64_t addr)
{
    int grp_id = (addr & index_mask) >> block_length;
    uint64_t tag = (addr & tag_mask) >> (CACHE_LENGTH - way_length);
    uint64_t tag_with_valid = tag | 1ull << tag_length;
    int empty_way_id = -1;

    // printf("==== cache_read ====\n");
    // printf("addr = ");
    // print64(addr);
    // puts("");
    // printf("grp_id = ");
    // print64(grp_id);
    // puts("");
    // printf("tag = ");
    // print64(tag);
    // puts("");

    for (int i = 0; i < way_num; ++i)
    {
        if (tags[grp_id][i] == tag_with_valid)
        {
            cache_meta_update(grp_id, i);
            return HIT;
        }
        if (empty_way_id == -1 && !tags[grp_id][i][tag_length])
            empty_way_id = i;
    }

    if (empty_way_id == -1)
        cache_replacement(grp_id, tag);
    else
        cache_assign(grp_id, empty_way_id, tag);
    return MISS;
}

void update_lru(int grp_id, int way_id)
{
    for (int i = 0; i < way_num; ++i)
        if (stacks[grp_id].get(i * way_length, (i + 1) * way_length - 1) == way_id)
        {
            for (int j = i; j + 1 < way_num; ++j)
                stacks[grp_id].set(j * way_length, (j + 1) * way_length - 1, stacks[grp_id].get((j + 1) * way_length, (j + 2) * way_length - 1));
            stacks[grp_id].set((way_num - 1) * way_length, way_num * way_length - 1, way_id);

            // printf("After update, stack:\n");
            // stacks[grp_id].print();
            // puts("");
            // for (int j = 0; j < way_num; ++j)
            //     printf("%" PRId64 " ", stacks[grp_id].get(j * way_length, (j + 1) * way_length - 1));
            // puts("");

            return;
        }
}
void update_bt(int grp_id, int way_id)
{
    for (int i = way_length, x = 1; i--;)
        if (way_id >> i & 1)
        {
            bts[grp_id].set(x - 1, 1);
            x = x << 1 | 1;
        }
        else
        {
            bts[grp_id].set(x - 1, 0);
            x = x << 1;
        }
    // printf("After update, bt:\n");
    // for (int j = 0; j < way_num - 1; ++j)
    //     cout << ' ' << bts[grp_id][j];
    // puts("");
}

void replacement_lru(int grp_id, uint64_t tag)
{
    int way_id = way_num == 1 ? 0 : stacks[grp_id].get(0, way_length - 1);
    cache_assign(grp_id, way_id, tag);
}
void replacement_rand(int grp_id, uint64_t tag)
{
    cache_assign(grp_id, distr(eng), tag);
}
void replacement_bt(int grp_id, uint64_t tag)
{
    int way_id = 0;
    for (int i = way_length, x = 1; i--;)
        if (bts[grp_id][x - 1])
        {
            way_id = way_id << 1;
            x = x << 1;
        }
        else
        {
            way_id = way_id << 1 | 1;
            x = x << 1 | 1;
        }
    cache_assign(grp_id, way_id, tag);
}

bool (*write_allocation)(uint64_t addr) = cache_read;
bool write_no_allocation(uint64_t addr)
{
    int grp_id = (addr & index_mask) >> block_length;
    uint64_t tag = (addr & tag_mask) >> (CACHE_LENGTH - way_length);
    uint64_t tag_with_valid = tag | 1ull << tag_length;
    for (int i = 0; i < way_num; ++i)
        if (tags[grp_id][i] == tag_with_valid)
        {
            cache_meta_update(grp_id, i);
            return HIT;
        }
    return MISS;
}

void cache_init(uint64_t _way_length = 3, uint64_t _block_length = 3,
                void (*_cache_replacement)(int, uint64_t) = replacement_lru,
                bool (*_cache_write)(uint64_t) = write_allocation)
{
    // layouts
    way_length = _way_length;
    block_length = _block_length;
    index_length = CACHE_LENGTH - way_length - block_length;
    tag_length = WORD_LENGTH - index_length - block_length;

    index_mask = (1 << CACHE_LENGTH - way_length) - 1 & ~((1 << block_length) - 1);
    tag_mask = ~((1 << CACHE_LENGTH - way_length) - 1ull);

    // printf("index_mask = ");
    // print64(index_mask);
    // puts("");
    // printf("tag_mask = ");
    // print64(tag_mask);
    // puts("");

    grp_num = CACHE_SIZE >> way_length >> block_length;
    way_num = 1 << way_length;

    // printf("grp_num = %d\n", grp_num);
    // printf("way_num = %d\n", way_num);

    // the length-th bit is the valid bit
    tags = vector<vector<BitVector>>(grp_num, vector<BitVector>(way_num, BitVector(tag_length + 1)));

    cache_replacement = _cache_replacement;
    if (_cache_replacement == replacement_lru)
    {
        cache_meta_update = update_lru;

        BitVector init_stack = BitVector(way_length * way_num);
        for (int i = 0; i < way_num; ++i)
            init_stack.set(i * way_length, (i + 1) * way_length - 1, i);

        // printf("After init, stack:\n");
        // init_stack.print();
        // puts("");
        // for (int j = 0; j < way_num; ++j)
        //     printf("%" PRId64 " ", init_stack.get(j * way_length, (j + 1) * way_length - 1));
        // puts("");

        stacks = vector<BitVector>(grp_num, init_stack);
    }
    else if (_cache_replacement == replacement_rand)
    {
        cache_meta_update = [](int, int) -> void {};
        distr = uniform_int_distribution<>(0, way_num - 1);
    }
    else if (_cache_replacement == replacement_bt)
    {
        cache_meta_update = update_bt;
        bts = vector<BitVector>(grp_num, BitVector(way_num - 1));
    }
    cache_write = _cache_write;
}

double cache_run(uint64_t _way_length = 3, uint64_t _block_length = 3,
                 void (*_cache_replacement)(int, uint64_t) = replacement_lru,
                 bool (*_cache_write)(uint64_t) = write_allocation)
{
    for (int i = 0; i < FILE_NUM; ++i)
    // for (int i = 0; i < 1; ++i)
    {
        cache_init(_way_length, _block_length, _cache_replacement, _cache_write);
        int hit_count = 0;
        for (auto op : trace[i])
            hit_count += op.op_t == load ? cache_read(op.addr) : cache_write(op.addr);
        cout << cacheFiles[i].filename << ": " << trace[i].size() - hit_count << " / " << trace[i].size() << " = ";
        int miss_rate = round(10000 * (1 - (double)hit_count / trace[i].size()));
        printf("%d.%02d%%\n", miss_rate / 100, miss_rate % 100);
    }
}

int main()
{
    // freopen("experiment.result", "w", stdout);
    // Input
    for (int i = 0; i < FILE_NUM; ++i)
    {
        char line_s[50], addr_s[20];
        char c;
        uint64_t addr;
        while (fgets(line_s, 50, cacheFiles[i].tracefile) != nullptr)
            if (strcmp(line_s, "") != 0)
            {
                sscanf(line_s, "%c%s", &c, addr_s);
                sscanf(addr_s, "%*c%*c%" SCNx64, &addr);

                trace[i].push_back(Operation(c == 'r' || c == 'l' ? load : store, addr));
            }
            else
                break;
    }

    // Log
    for (int i = 0; i < FILE_NUM; ++i)
    {
        cache_init();

        for (auto op : trace[i])
        {
            if (op.op_t == load ? cache_read(op.addr) : cache_write(op.addr))
                fprintf(cacheFiles[i].logfile, "Hit\n");
            else
                fprintf(cacheFiles[i].logfile, "Miss\n");
        }
    }

    // Experiemnt of different cache layouts
    printf("====== Experiment of different cache layouts ======\n");
    for (int _block_length : {3, 5, 6})
        for (int _way_length : {0, 2, 3, CACHE_LENGTH - _block_length})
        {
            printf(">>> way_length = %d, block_length = %d\n", _way_length, _block_length);
            cache_run(_way_length, _block_length);
        }

    // Experiment of different replacement strategies
    printf("====== Experiment of different replacement strategies ======\n");
    printf(">>> replacement_lru\n");
    cache_run(3, 3, replacement_lru);
    printf(">>> replacement_rand\n");
    cache_run(3, 3, replacement_rand);
    printf(">>> replacement_bt\n");
    cache_run(3, 3, replacement_bt);

    // Experiment of different write strategies
    printf("====== Experiment of different replacement strategies ======\n");
    printf(">>> write allocation\n");
    cache_run(3, 3, replacement_lru, write_allocation);
    printf(">>> write no allocation\n");
    cache_run(3, 3, replacement_lru, write_no_allocation);
}