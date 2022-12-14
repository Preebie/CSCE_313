#ifndef Histogram_h
#define Histogram_h

#include <queue>
#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
using namespace std;

class Histogram {
private:
	mutex muted;
	vector<int> hist;
	int nbins;
	double start, end;
public:
    Histogram(int, double, double);
	~Histogram();
	void update (double ); 		// updates the histogram
    vector<int> get_hist();		// prints the histogram
    int size ();
	vector<double> get_range ();
};

#endif 
