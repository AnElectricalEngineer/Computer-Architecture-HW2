/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include "memClass.h"

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

int main(int argc, char** argv) {

    if (argc < 19) {
        cerr << "Not enough arguments" << endl;
        return 0;
    }

    // Get input arguments

    // File
    // Assuming it is the first argument
    char* fileString = argv[1];
    ifstream file(fileString); //input file stream
    string line;
    if (!file || !file.good()) {
        // File doesn't exist or some other error
        cerr << "File not found" << endl;
        return 0;
    }

    unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
        L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;
    bool writeAllocation = false;

    for (int i = 2; i < 19; i += 2) {
        string s(argv[i]);
        if (s == "--mem-cyc") {
            MemCyc = atoi(argv[i + 1]);
        }
        else if (s == "--bsize") {
            BSize = atoi(argv[i + 1]);
            BSize = pow(2, BSize);
        }
        else if (s == "--l1-size") {
            L1Size = atoi(argv[i + 1]);
            L1Size = pow(2, L1Size);
        }
        else if (s == "--l2-size") {
            L2Size = atoi(argv[i + 1]);
            L2Size = pow(2, L2Size);
        }
        else if (s == "--l1-cyc") {
            L1Cyc = atoi(argv[i + 1]);
        }
        else if (s == "--l2-cyc") {
            L2Cyc = atoi(argv[i + 1]);
        }
        else if (s == "--l1-assoc") {
            L1Assoc = atoi(argv[i + 1]);
            L1Assoc = pow(2, L1Assoc);
        }
        else if (s == "--l2-assoc") {
            L2Assoc = atoi(argv[i + 1]);
            L2Assoc = pow(2, L2Assoc);
        }
        else if (s == "--wr-alloc") {
            WrAlloc = atoi(argv[i + 1]);
            if (WrAlloc == 1)writeAllocation = true;
            else writeAllocation = false;
        }
        else {
            cerr << "Error in arguments" << endl;
            return 0;
        }
    }
    theCache Mem = theCache(BSize, L1Size, L2Size, L1Assoc, L2Assoc, writeAllocation);
    int i = 0;
    while (getline(file, line)) {
        i++;
        stringstream ss(line);
        string address;
        char operation = 0; // read (R) or write (W)
        if (!(ss >> operation >> address)) {
            // Operation appears in an Invalid format
            cout << "Command Format error" << endl;
            return 0;
        }

        string cutAddress = address.substr(2); // Removing the "0x" part of the address

        unsigned long int num = 0;
        num = strtoul(cutAddress.c_str(), NULL, 16);

        if (operation == 'r')   Mem.readLine(num);
        if (operation == 'w')   Mem.writeLine(num);
    }

    int L1Access = Mem.getL1Access();
    int L2Access = Mem.getL2Access();
    int mainMemAccess = Mem.getMainMemAccess();

    double L1MissRate = Mem.getL1MissRate();
    double L2MissRate = Mem.getL2MissRate();
    double avgAccTime = 0;
    if (i != 0)
        avgAccTime = ((double)((L1Access * L1Cyc) + (L2Access * L2Cyc) + (mainMemAccess * MemCyc)) / (double)i);

    printf("L1miss=%.03f ", L1MissRate);
    printf("L2miss=%.03f ", L2MissRate);
    printf("AccTimeAvg=%.03f\n", avgAccTime);

    return 0;
}

//	C'tor
theCache::theCache(int block_size, int l1_size, int l2_size, int l1_ways, int l2_ways, bool writeAlloc) :l1_(l1_ways, l1_size, block_size), l2_(l2_ways, l2_size, block_size), main_mem_access_(0), write_alloc_(writeAlloc) {
    if (l1_ways == 0)
        l1_ways = 1;
    if (l2_ways == 0)
        l2_ways = 1;
    bits_num_offset_ = log2(block_size);
    int bits_num_lines1 = log2(l1_size / (l1_ways * block_size));
    max_num_lines1_ = pow(2, bits_num_lines1);
    int bits_num_lines2 = log2(l2_size / (l2_ways * block_size));
    max_num_lines2_ = pow(2, bits_num_lines2);
    tag_size1_ = 32 - bits_num_offset_ - bits_num_lines1;
    tag_size2_ = 32 - bits_num_offset_ - bits_num_lines2;
}

//	Function to perform a read op on the cache
void theCache::readLine(long int line) {
    long int tag1 = line >> (32 - tag_size1_);
    long int tag2 = line >> (32 - tag_size2_);
    long int set1 = line >> bits_num_offset_;
    set1 = set1 % max_num_lines1_;
    long int set2 = line >> bits_num_offset_;
    set2 = set2 % max_num_lines2_;

    int way1 = l1_.access(tag1, set1);	// positive-hit - num of way, negative -miss// access++, is miss then miss++
    if (way1 >= 0) { //hit  l1
        l1_.updateLru(set1, way1);
    }
    else {//miss l1

        tag_line l1_victim;
        tag_line l2_victim;
        long int helper;
        bool is_dirty;
        int way2 = l2_.access(tag2, set2); // positive-hit - num of way, negative -miss// access++, is miss then miss++
        if (way2 >= 0) {//hit l2
            l2_.updateLru(set2, way2);
            l1_victim = l1_.selectVictim(set1);	// select the max lru return tag and size
            if (l1_victim.found == true) {
                is_dirty = l1_.find_if_dirty(l1_victim);
                if (is_dirty) {
                    helper = (l1_victim.tag << (32 - tag_size1_)) + (l1_victim.line << bits_num_offset_);
                    l2_victim.line = (helper >> bits_num_offset_) % max_num_lines2_;
                    l2_victim.tag = helper >> (32 - tag_size2_);
                    l2_victim.way = l2_.find_way(l2_victim.tag, l2_victim.line);
                    l2_.make_dirty(l2_victim.tag, l2_victim.line);
                    l2_.updateLru(l2_victim.line, l2_victim.way);
                }
                l1_.invalidate_victim(l1_victim.tag, l1_victim.line);	// invalidate. if not exist, return
            }
            l1_.update_data(tag1, set1, l1_victim);
            l1_.updateLru(set1, l1_victim.way);
        }
        else { //miss l2
            main_mem_access_++;
            l2_victim = l2_.selectVictim(set2);
            if (l2_victim.found == true) {
                l2_.invalidate_victim(l2_victim.tag, l2_victim.line);
                helper = (l2_victim.tag << (32 - tag_size2_)) + (l2_victim.line << bits_num_offset_);
                l1_victim.tag = helper >> (32 - tag_size1_);
                l1_victim.line = (helper >> bits_num_offset_) % max_num_lines1_;
                l1_victim.way = l1_.find_way(l1_victim.tag, l1_victim.line);

                if (l1_.snoop(l1_victim)) {
                    l1_.invalidate_victim(l1_victim.tag, l1_victim.line);
                }
            }

        	//	Update statistics and LRU
            l2_.update_data(tag2, set2, l2_victim);
            l2_.updateLru(set2, l2_victim.way);

        	//	Choose a victim
            l1_victim = l1_.selectVictim(set1);
            if (l1_victim.found == true) {
                helper = (l1_victim.tag << (32 - tag_size1_)) + (l1_victim.line << bits_num_offset_);
                l2_victim.line = (helper >> bits_num_offset_) % max_num_lines2_;
                l2_victim.tag = helper >> (32 - tag_size2_);
                l2_victim.way = l2_.find_way(l2_victim.tag, l2_victim.line);
                is_dirty = l1_.find_if_dirty(l1_victim);
                if (is_dirty) {
                    l2_.make_dirty(l2_victim.tag, l2_victim.line);
                    l2_.updateLru(l2_victim.line, l2_victim.way);
                }
                l1_.invalidate_victim(l1_victim.tag, l1_victim.line);
            }
            l1_.update_data(tag1, set1, l1_victim);
            l1_.updateLru(set1, l1_victim.way);
        }
    }
}

//Function to write a new line to the cache
void theCache::writeLine(long int line) {
    long int tag1 = line >> (32 - tag_size1_);
    long int tag2 = line >> (32 - tag_size2_);
    long int set1 = line >> bits_num_offset_;
    set1 = set1 % max_num_lines1_;
    long int set2 = line >> bits_num_offset_;
    set2 = set2 % max_num_lines2_;
    int way1, way2;
    long int helper;
    tag_line l1_victim, l2_victim;
    way1 = l1_.access(tag1, set1);
    if (way1 >= 0) {//hit l1
        l1_.make_dirty(tag1, set1);
        l1_.updateLru(set1, way1);
    }
    else {	// L1 miss
        way2 = l2_.access(tag2, set2);
        if (way2 >= 0) {
            l2_.updateLru(set2, way2);
            if (write_alloc_) {
                l1_victim = l1_.selectVictim(set1);
                if (l1_victim.found == true) {
                    if (l1_.find_if_dirty(l1_victim)) {
                        l2_.make_dirty(tag2, set2);
                        l2_.updateLru(set2, way2);
                    }
                    l1_.invalidate_victim(l1_victim.tag, l1_victim.line);
                }
                l1_.update_data(tag1, set1, l1_victim);
                l1_.make_dirty(tag1, set1);
                l1_.updateLru(l1_victim.line, l1_victim.way);
            }
            else {
                l2_.make_dirty(tag2, set2);
            }
        }
        else {	//	L2 miss
            main_mem_access_++;
            if (write_alloc_) {
                l2_victim = l2_.selectVictim(set2);
                if (l2_victim.found == true) {
                    l2_.invalidate_victim(l2_victim.tag, l2_victim.line);
                    helper = (l2_victim.tag << (32 - tag_size2_)) + (l2_victim.line << bits_num_offset_);
                    l1_victim.line = (helper >> bits_num_offset_) % max_num_lines1_;
                    l1_victim.tag = helper >> (32 - tag_size1_);
                    l1_victim.way = l1_.find_way(l1_victim.tag, l1_victim.line);
                    if (l1_.snoop(l1_victim)) {
                        l1_.invalidate_victim(l1_victim.tag, l1_victim.line);
                    }
                }
                l2_.update_data(tag2, set2, l2_victim);
                l2_.updateLru(set2, l2_victim.way);

                l1_victim = l1_.selectVictim(set1);
                if (l1_victim.found == true) {
                    if (l1_.find_if_dirty(l1_victim)) {
                        helper = (l1_victim.tag << (32 - tag_size1_)) + (l1_victim.line << bits_num_offset_);
                        l2_victim.line = (helper >> bits_num_offset_) % max_num_lines2_;
                        l2_victim.tag = helper >> (32 - tag_size2_);
                        l2_victim.way = l2_.find_way(l2_victim.tag, l2_victim.line);
                        l2_.make_dirty(l2_victim.tag, l2_victim.line);
                        l2_.updateLru(l2_victim.line, l2_victim.way);
                    }
                    l1_.invalidate_victim(l1_victim.tag, l1_victim.line);
                }
                l1_.update_data(tag1, set1, l1_victim);
                l1_.make_dirty(tag1, set1);
                l1_.updateLru(l1_victim.line, l1_victim.way);
            }
        }
    }
}

//	Statistics methods
int theCache::getL1Access() {
    return l1_.getAccessStat();
}
int theCache::getL2Access() {
    return l2_.getAccessStat();
}
int theCache::getMainMemAccess() {
    return main_mem_access_;
}
double theCache::getL1MissRate() {
    return l1_.getMissRateStat();
}
double theCache::getL2MissRate() {
    return l2_.getMissRateStat();
}

//	Cache c'tor
cache::cache(int num_ways, int size, int block_size) :num_ways_(num_ways), block_size_(block_size), access_(0), miss_(0), access_time_(0) {
    if (num_ways_ == 0)
        num_ways_ = 1;
    lines_num_ = size / (block_size * num_ways_);
    lru_size_ = log2(num_ways_);
    wayTable basic = wayTable(lines_num_, lru_size_);
    for (int i = 0; i < num_ways_; i++) {
        table.push_back(basic);
    }
    max_lru_val_ = pow(2, lru_size_);
}

//	Function to check cache for a hit or miss
int cache::access(long int tag, long int set) {
    int i = 0;

	//Update statistics
    access_++;
    access_time_++;
    vector <wayTable> ::iterator it;
    for (it = table.begin(); it != table.end(); it++) {
        if (it->get_valid(set) == false) {
            i++;
            continue;
        }
        if (it->get_tag(set) == tag) return i;
        i++;
    }
    miss_++;
    return -1;
}

//	Function to choose a cache line to evict 
tag_line cache::selectVictim(long int set) {
    int max = 0;
    int max_way = -1;
    long int max_tag = 0;
    int i = 0;
    int found = false;
    vector <wayTable> ::iterator it;
    for (it = table.begin(); it != table.end(); it++) {
        if (it->get_valid(set) == false) {
            max_tag = it->get_tag(set);
            max_way = i;
            found = false;
            break;
        }
        else {
            if (it->get_lru(set) >= max) {
                max = it->get_lru(set);
                max_tag = it->get_tag(set);
                max_way = i;
                found = true;
            }
        }
        i++;
    }
    tag_line ret_val;
    ret_val.tag = max_tag;
    ret_val.line = set;
    ret_val.way = max_way;
    ret_val.found = found;
    return ret_val;

}

//	Function to evict a line from the cache
void cache::invalidate_victim(long int tag, long int set) {
    vector <wayTable> ::iterator it;
    for (it = table.begin(); it != table.end(); it++) {
        if (it->get_valid(set)) {
            if (it->get_tag(set) == tag) {
                it->make_invalid(set); 
                return;
            }
        }
    }
}

//	Function to update the statistics and data of a cache
void cache::update_data(long int tag, long int set, tag_line victim) {
    access_time_++;
    table[victim.way].update_data(tag, set);
}

//	Function to update the LRU of a cache line entry
void cache::updateLru(long int set, int way) {
    vector <wayTable> ::iterator it;
    int change = table[way].get_lru(set);
    table[way].set_lru(0, set);
    int i = 0;
    int lru;
    for (it = table.begin(); it != table.end(); it++) {
        lru = it->get_lru(set);
        if ((lru < change) && (i != way)) it->set_lru(lru + 1, set);
        i++;
    }
}

//	Function to mark a cache line as dirty
void cache::make_dirty(long int tag, long int set) {
    vector <wayTable> ::iterator it;
    for (it = table.begin(); it != table.end(); it++) {
        if (it->get_tag(set) == tag)it->make_dirty(set); 
    }
}

//	Function to check if a dirty version exists
bool cache::find_if_dirty(tag_line victim) {
    vector <wayTable> ::iterator it;
    for (it = table.begin(); it != table.end(); it++) {
        if (it->get_tag(victim.line) == victim.tag) {
            return it->get_dirty(victim.line);//true if dirty false if not
        }   
    }
    return false;
}

//	Function to choose a way
int cache::find_way(long int tag, long int set) {
    vector <wayTable>::iterator it;
    int i = 0;
    for (it = table.begin(); it != table.end(); it++) {
        if (it->get_tag(set) == tag)return i;
        i++;
    }
    return -1;
}

bool cache::snoop(tag_line victim) {
    if (victim.way < 0) return false;
    return true;

}

//	Statistics function
int cache::getAccessStat() {
    return access_;
}

//	Statistics function
double cache::getMissRateStat() {
    return access_ == 0 ? 0 : (double)miss_ / access_;
}

//	C'tor
wayTable::wayTable(int lines_num, int lru_size) :lru_size_(pow(2, lru_size)), lines_num_(lines_num) {
    cacheLine basic = cacheLine(lru_size_);
    for (int i = 0; i < lines_num; i++) {
        table.push_back(basic);
    }
}

//	Copy C'tor
wayTable::wayTable(const wayTable& cop) :lru_size_(cop.lru_size_), lines_num_(cop.lines_num_) {
    for (int i = 0; i < lines_num_; i++) {
        table.push_back(cop.table[i]);
    }
}

//	Getters
long int wayTable::get_tag(int set) {
    return table[set].get_tag();
}

int wayTable::get_lru(int set) {
    return table[set].get_lru();
}

int wayTable::get_dirty(int set) {
    return table[set].get_dirty();
}

bool wayTable::get_valid(int set) {
    return table[set].get_valid();
}

//	Setters
void wayTable::make_invalid(int set) {
    table[set].make_invalid();
}
void wayTable::make_dirty(int set) {
    table[set].make_dirty();
}
void wayTable::update_data(long int tag, int set) {
    table[set].update_data(tag);
}
void wayTable::set_lru(int lru, int set) {
    table[set].set_lru(lru);
}

//	Cache line c'tor
cacheLine::cacheLine(int max_lru_value) :max_lru_value_(max_lru_value), valid_(false), dirty_(false), lru_(max_lru_value), tag_(0) {};

//	Getters
long int cacheLine::get_tag() {
    return tag_;
}

bool cacheLine::get_dirty() {
    return dirty_;
}

int cacheLine::get_lru() {
    return lru_;
}

bool cacheLine::get_valid() {
    return valid_;
}

//	Setters
void cacheLine::update_data(int tag) {
    tag_ = tag;
    valid_ = true;
}
void cacheLine::set_lru(int change) {
    if (change < max_lru_value_)
        lru_ = change;
}
void cacheLine::make_dirty() {
    dirty_ = true;
}
void cacheLine::make_invalid() {
    valid_ = false;
}
