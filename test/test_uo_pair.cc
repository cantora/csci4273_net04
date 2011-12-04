#include <cstdio>

#include <cstdlib>

#include <map>
#include "uo_pair.h"

using namespace std;
using namespace net04;

int main() {	
	uo_pair<int> p1(1,2), p2(2,1), p3(4,5);
	map<uo_pair<int>, int> m;
	map<uo_pair<int>, int>::const_iterator it;
	int i;

	printf("test uo_pair...\n");

	printf("p1 == p2: %d\n", p1 == p2); 
	printf("p1 == p3: %d\n", p1 == p3);
	printf("p1 < p3: %d\n", p1 < p3);

	//for(i = 0; i < 10; i++) {
	//	m.insert(rand() % 100, rand() % 50);
	//}

	/*m.insert(make_pair(p1, 3));
	m.insert(make_pair(p1, 4));
	m.insert(make_pair(p2, 2));
	m.insert(make_pair(p3, 5));
	*/
	m[p1] = 3;
	m[p1] = 4;
	m[p2] = 2;
	m[p3] = 5;

	printf("map: \n");
	for(it = m.begin(); it != m.end(); it++) {
		printf("%d<->%d: %d\n", it->first.first, it->first.second, it->second);
	}

	return 0;
}