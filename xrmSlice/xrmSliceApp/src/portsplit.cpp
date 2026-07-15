// test splitting port name
// we need to store samplePrams, eslo, eoff per uut.
// XrmSliceCommon doesn't know the UUT name, but it does have proxy
/*..

XRM0PM00
XRM0PM01

XRM1PM00
XRM1PM01

XRM0HT00
..

Goal: select the number after XRM, maybe just keep as a string and store in a std::map

Needs to handle

XRM10HT etc in the extreme case..

strcspn()	Returns the length of a string up to the first occurrence of one of the specified characters
strspn()	Returns the length of a string up to the first character which is not one of the specified characters

result:
(base) pgm@hoy6:~/PROJECTS/HTSCOPE/xrmSlice/xrmSliceApp/src$ ./portsplit XRM0PM00 XRM0PM01 XRM0HT00 XRM1PM01 XRM10PM01
port:XRM0PM00 id:XRM0
port:XRM0PM01 id:XRM0
port:XRM0HT00 id:XRM0
port:XRM1PM01 id:XRM1
port:XRM10PM01 id:XRM10

*/

#include <string.h>
#include <stdio.h>

#define CS "0123456789"

void get_uut_id(char* port) {
	char id[8];
	int i0 = strcspn(port, CS);
	int i1 = strspn(port+i0, CS);

	strncpy(id, port, i0+i1);
	printf("port:%s id:%s\n", port, id);
}
int main(int argc, char* argv[]){
	for (int ii = 1; ii < argc; ++ii){
		get_uut_id(argv[ii]);
	}
	return 0;
}

