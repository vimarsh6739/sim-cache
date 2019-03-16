#include <random>
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <set>
using namespace std;

/*
*	Defining a block in the cache memory
*	Has 3 parameters- The tag string, a valid bit and a dirty bit
*
*/
struct Node
{
	long tag;
	int valid;
	int dirty;

	Node()
	{
		tag = 0;
		valid = dirty = 0;
	}

	Node(long tag)
	{
		this->tag = tag;
		valid = 1;
		dirty = 0;
	}
};

/*
*
*	Defining the Virtual Cache Simulator class 
*	to carry out all operations	
*
*
*/
class VirtualCache
{
private:
	vector< vector<Node*> > cache;
	int setCnt;
	int wayCnt;
	int blockSize;
	int rePolicy;
	
	//Defining the counters for all the parameters
	//counter[0]	- # of cache accessses
	//counter[1]	- # of read accesses
	//counter[2]	- # of write accesses
	//counter[3]	- # of cache misses 
	//counter[4]	- # of compulsory misses
	//counter[5]	- # of conflict misses
	//counter[6]	- # of capacity misses(only for fuly associative cache)
	//counter[7]	- # of read misses 
	//counter[8]	- # of write misses 
	//counter[9]	- # of dirty blocks evicted
	vector<long long>counter;
	set< long > hMap;
public:

	VirtualCache()
	{
		//default constructor
	}

	VirtualCache(int sets, int ways, int bs, int rep)
	{
		//Parameterized Constructor to initialize values of cache
		cache.resize(sets);
		setCnt = sets;
		wayCnt = ways;
		blockSize = bs;
		rePolicy = rep;
		counter.resize(10,0);
	}

	void parseIncomingAddress(long address)
	{
		counter[0]++;
		//checking if the msb is 1 or 0 (address is 32 bits long)
		int msb = (address & 1<<31);

		//finding out the size of the word offset and the set index
		int wordOffset = log2(blockSize);
		int index = log2(setCnt);

		int setIndex = (address>>wordOffset) % (int)pow(2, index);
		long tag = 0;
		for(int i = 0;i<=30;++i)
			tag +=address & (1<<i);
		tag = tag>>(index + wordOffset);

		if(msb)
		{
			//perform write operation
			counter[2]++;
			writeToCache(tag, setIndex);
		}
		else
		{
			//perform read operation
			counter[1]++;
			readToCache(tag, setIndex);
		}
	}

	long long getStatistic(int opcode)
	{
		return counter[opcode];
	}
	/*
	*	Change the contents of the block at the required position
	*	
	*/
	void writeToCache(long tag, int setIndex)
	{
		if(isPresent(setIndex, tag))
		{
			//The tag is already present in the given setIndex
			//We change the contents of the block and set the dirty bit as 1
			for(int j=0;j<wayCnt;++j)
			{
				if(cache[setIndex][j]->valid == 1 && cache[setIndex][j]->tag == tag)
				{
					cache[setIndex][j]->dirty  = 1;
					break;
				}
			}
		}
		else
		{
			//This constitutes a write miss
			counter[3]++;//update no of cache misses
			counter[8]++;//update no of write misses
			loadIntoCache(tag, setIndex, 0);
		}
	}

	void readToCache(long tag, int setIndex)
	{
		if(isPresent(setIndex, tag))
		{
			//This constitutes a read hit
			//Do nothing in this case

		}
		else
		{
			//The tag is not present in the cache at set index
			//This is a read miss
			counter[3]++;//update no of cache misses
			counter[7]++;//update no of read misses
			loadIntoCache(tag, setIndex, 1);
		}
	}

	bool isPresent(int setIndex, long tag)
	{
		for(int j = 0;j<cache[setIndex].size();++j)
		{
			if(cache[setIndex][j]->valid == 1)
			{
				if(cache[setIndex][j]->tag == tag)
				{
					//Set this element as the first element of the index
					//and return true
					Node* ptr = new Node(tag);
					ptr->dirty = cache[setIndex][j]->dirty;
					//delete at the current position
					cache[setIndex].erase(cache[setIndex].begin() + j);
					//insert at the beginning	
					cache[setIndex].insert(cache[setIndex].begin(), ptr);
					return true;
				}
				
			}
			else 
				break;
		}
		return false;
	}

	void loadIntoCache(long tag, int setIndex, int fromRead)
	{
		Node* ptr = new Node(tag);
		//Setting the value of the dirty bit for the cache element
		if(fromRead==1)
			ptr->dirty = 0;
		else
			ptr->dirty = 1;

		//Counting the no of compulsory or conflict misses
		int wordOffset = log2(blockSize);
		int index = log2(setCnt);
		long address = tag<<index;
		address += setIndex;
		if(hMap.find(address)==hMap.end())
		{
			//this is a compulsory miss
			hMap.insert(address);
			counter[4]++;
		}
		else
		{
			//this is a capacity/conflict miss
			if(setCnt==1)
				counter[6]++;
			else
				counter[5]++;
		}

		if(cache[setIndex].size() == wayCnt)
		{
			int rand_int;

			switch(rePolicy)
			{
				case 0:
					//Random eviction
					
					rand_int = getRandom(0, wayCnt-1);		

					if(cache[setIndex][rand_int]->dirty == 1)
						counter[9]++;

					cache[setIndex].erase(cache[setIndex].begin() + rand_int);
					break;
				case 1:
					//LRU - evict the last block in the given set
					if(cache[setIndex][wayCnt-1]->dirty == 1)
						counter[9]++;

					cache[setIndex].pop_back();
					break;

				case 2:
					//pseudo LRU - evict any block except the 1st block in given set

					rand_int = getRandom(1, wayCnt-1);

					if(cache[setIndex][rand_int]->dirty == 1)
						counter[9]++;

					cache[setIndex].erase(cache[setIndex].begin() + rand_int);
					break;

			}
		}

		cache[setIndex].insert(cache[setIndex].begin(), ptr);
	}

	int getRandom(int low, int high)
	{
		//Generates and returns a random integer between low and high
		//Uses the Mersenne Twister PRNG
		std::mt19937 rng;
		rng.seed(std::random_device()());
		std::uniform_int_distribution<std::mt19937::result_type> dist(low, high);

		return dist(rng);
	}

};

int main()
{
	// For getting input from input.txt file 
    ifstream fin;
    fin.open("input.txt");

    // Printing the Output to output.txt file 
    ofstream fout;
    fout.open("output.txt");

    int cacheSize;
    int blockSize;
    int assoc;
    int rePolicy;
    long address;

    fin>>cacheSize>>blockSize>>assoc>>rePolicy;
	
	//Defining a cache object
	VirtualCache obj;
    switch(assoc)
    {
    	case 0:
    		//fully associative cache
    		obj = *(new VirtualCache(1, cacheSize/blockSize, blockSize, rePolicy));
    		break;

    	case 1:
    		//direct mapped cache
    		obj = *(new VirtualCache(cacheSize/blockSize, 1, blockSize, rePolicy));
    		break;
    	case 2:
    	case 4:
    	case 8:
    	case 16:
    		//set-associative cache with associativity assoc = # of ways
    		obj = *(new VirtualCache((cacheSize/blockSize)/assoc, assoc, blockSize, rePolicy));
    		break;
	}

	while(!fin.eof())
    {
    	//read address as a hexadecimal integer
    	fin>>hex>>address;
    	if(fin.eof())break;
    	obj.parseIncomingAddress(address);
    }

	fout<<cacheSize<<"\n";
	fout<<blockSize<<"\n";
	fout<<assoc<<"\n";
	fout<<rePolicy<<"\n";

	
	for(int i=0;i<10;++i)
		fout<<obj.getStatistic(i)<<"\n";
	
	fin.close();
	fout.close();

	return 0;
}