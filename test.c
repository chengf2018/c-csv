//²âÊÔÎÄ¼ş
#include <stdio.h>
#include <stdlib.h>

#include "csv.h"


int main(int argc, char** argv) {
	
	struct csv_data *data = Csv_Load("e:\\1.csv");

	if (!data) {
		printf("%s\n", Csv_GetErrorPtr());
		system("pause");
		return 0;
	}
	
	Csv_PrintData(data);
	Csv_PrintToFile(data, "e:\\2.csv");

	Csv_Delete(data);

	system("pause");
	return 0;
}

