# 046267 Computer Architecture - Winter 20/21 - HW #2

cacheSim:	cacheSim.o
	g++ -o  cacheSim cacheSim.o 

cacheSim.o:	cacheSim.cpp memClass.h
	g++ -c -std=c++11 cacheSim.cpp

	
.PHONY: clean
clean:
	rm -f *.o
	rm -f cacheSim