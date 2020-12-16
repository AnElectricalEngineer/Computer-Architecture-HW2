#ifndef _MEMCLASSH_
#define _MEMCLASSH_

#include <vector>

using namespace std;
using std::vector;

//	Define what a tag consists of
typedef struct tag_line {
    int tag;
    int line;
    int way;
    bool found;
}tag_line;


//	Define an individual line in a Cache
class cacheLine {
public:
    cacheLine(int max_lru_value);
    long int get_tag();
    bool get_dirty();
    bool get_valid();
    int get_lru();
    void update_data(int tag);
    void set_lru(int change);
    void make_dirty();
    void make_invalid();

private:
    int max_lru_value_;
    bool valid_;
    bool dirty_;
    int lru_;
    long int tag_;
};

class wayTable {
public:
    wayTable(int lines_num, int lru_size);
    wayTable(const wayTable& cop);
    long int get_tag(int set);
    int get_lru(int set);
    int get_dirty(int set);
    void make_invalid(int set);
    void make_dirty(int set);
    void update_data(long int tag, int set);
    void set_lru(int lru, int set);
     bool get_valid(int set);

private:
    int lru_size_;
    int lines_num_;
    vector <cacheLine> table;
};

// Define a cache level
class cache {
public:
    cache(int num_ways, int size, int block_size);
    int access(long int tag, long int set);
    tag_line selectVictim(long int set);
    void invalidate_victim(long int tag, long int set);
    void update_data(long int tag, long int set, tag_line victim);
    void updateLru(long int set, int way);
    void make_dirty(long int tag, long int set);
    bool find_if_dirty(tag_line victim);
    int getAccessStat();
    double getMissRateStat();
    int find_way(long int tag , long int set);
    bool snoop(tag_line victim);
   
protected:
    int lines_num_;
    int block_size_;
    int max_lru_val_;
    int lru_size_;
    int num_ways_;
    vector  <wayTable> table;
    int miss_;
    int access_;
    int access_time_;
};

//	Define the entire cache
class theCache {
public:
    theCache(int block_size, int l1_size, int l2_size, int l1_ways, int l2_ways, bool writeAlloc);
    void readLine(long int line);
    void writeLine(long int line);
    int getL1Access();
    int getL2Access();
    int getMainMemAccess();
    double getL1MissRate();
    double getL2MissRate();
private:
    cache l1_;
    cache l2_;
    int main_mem_access_;
    int bits_num_offset_;//block_size_;
    int tag_size1_;
    int tag_size2_;
    int max_num_lines1_;
    int max_num_lines2_;
    bool write_alloc_;
};
#endif