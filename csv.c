#include "csv.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_ERR_STRING 512
static char err[MAX_ERR_STRING] = {0};
static const char* ep = 0;

void* mymalloc(size_t size) { return malloc(size); }
void myfree(void* mem) { free(mem); }

static void DeleteValueList(struct csv_value_list *list);
static struct csv_data* NewData();
static void DeleteData(struct csv_data* data);
static struct csv_value_list* NewValueList();
static char* GetFileString(const char* filename);
static const char* Skip(const char* str);
static const char* Skip2(const char* str);
static size_t Trim(char* str);
static size_t Trim2(char *str);
static size_t Trim3(char* src, char* dest);
static int GetCharCount(const char* str, char c);
static const char* FindValueEnd(const char* str);
static int GetNumberType(const char* str);
static const char* ParseValue(struct csv_value *value, const char* strdata);
static const char* ParseLine(struct csv_value_list* value_list, const char* strdata);
static const char* Parse(struct csv_data *data, const char* strdata);
static void PrintValue(struct csv_value *value, FILE* fp);


static void DeleteValueList(struct csv_value_list *list) {
	struct csv_value_list *p, *temp;
	
	if (!list) return;
	p = list;
	while (p) {
		temp = p;
		p = p->next;

		if (temp->value.stringvalue) {
			myfree(temp->value.stringvalue);
		}
		myfree(temp);
	}
}

static struct csv_data* NewData() {
	struct csv_data* data = (struct csv_data*)mymalloc(sizeof(struct csv_data));
	if (!data) return 0;

	memset(data, 0, sizeof(struct csv_data));
	return data;
}

static void DeleteData(struct csv_data* data) {
	struct csv_data *p = data, *temp;
	if (!data) return;

	while (p) {
		if (p->value_list) DeleteValueList(p->value_list);
		p->value_list = 0;
		temp = p;
		p = p->next;
		
		myfree(temp);
	}
}

static struct csv_value_list* NewValueList() {
	struct csv_value_list *list = (struct csv_value_list*)mymalloc(sizeof(struct csv_value_list));
	if (!list) return 0;
	memset(list, 0, sizeof(struct csv_value_list));
	return list;
}

static char* GetFileString(const char* filename) {
	FILE* fp;
	size_t flen, rlen = 0;
	char* strdata;
	int errCode;
	//����ļ�
	fp = fopen(filename, "r");
	if (!fp) return 0;

	//��ȡ�ļ���С
	fseek(fp, 0, SEEK_END);
	flen = ftell(fp);
	if (flen <= 0) {fclose(fp); return 0;}
	strdata = (char*)mymalloc(flen + 1);
	if (!strdata) { fclose(fp); return 0; }
	fseek(fp, 0, SEEK_SET);
	rlen = fread(strdata, 1, flen, fp);
	
	errCode = ferror(fp);
	if (errCode != 0) {
		sprintf(err, "read file error! error code: %d", errCode);
		fclose(fp);
		myfree(strdata);
		return 0;	
	}
	
	strdata[rlen] = '\0';

	fclose(fp);
	return strdata;
}

//�������пո�
static const char* Skip(const char* str) {
	while(str && *str && *str == ' ') str++;
	return str;
}

//�������п����ַ�
static const char* Skip2(const char* str) {
	while(str && *str && (unsigned char)*str <= 32) str++;
	return str;
}

//ȥ���ַ�������Ŀո�,�����޸ĺ���ַ�������
static size_t Trim(char* str) {
	int pos = strlen(str) - 1;
	if (!str || *str == '\0') return 0;
	
	while(pos >= 0 && *(str + pos) == ' ') {
		pos--;
	}
	str[pos+1] = 0;

	return pos+1;
}

//�ϳ�����������,�����޸ĺ���ַ�������
static size_t Trim2(char *str) {
	int pos = strlen(str) - 1;
	if (!str) return 0;

	while (pos >= 1) {
		if (str[pos] == '\"' && str[pos-1] == '\"') {
			strcpy(str + pos - 1, str + pos);
			pos -= 2;
		}
		else
			pos--;
	}
	return strlen(str);
}

/*��src�е�����double������ǰ��������ţ������dest
 *�����޸ĺ���ַ������ȣ���Ҫע�����dest���ַ������ȱ����㹻
*/
static size_t Trim3(char* src, char* dest) {
	size_t count = 2;
	if (!src || !dest) return 0;
	*dest = '\"';
	dest++;
	while (*src) {
		if (*src == '\"') {
			*dest++ = '\"';
			*dest = '\"';
			count ++;
		} else
			*dest = *src;
		dest++;
		src++;
		count++;
	}
	*dest++ = '\"';
	*dest = '\0';
	return count;
}

//��ȡ�ı����е��ƶ��ַ�����
static int GetCharCount(const char* str, char c) {
	int count = 0;
	if (!str) return 0;
	while (*str) {
		if (*str == c) count ++;
		str++;
	}
	return count;
}

//Ѱ��һ��ֵ�Ľ���λ��
static const char* FindValueEnd(const char* str) {
	if (*str == '\"') {//���ſ�ʼ���ַ���
		str++;
		while(*str) {
			//Ѱ����һ������
			if (*str == '\"' && *(str+1) == '\"')
				str += 2;
			else if (*str == '\"') {
				str = Skip(str + 1);
				if (*str == ',' || *str == '\n' || *str == '\0' ) return str;
				return 0;
			} else
				str++;
		}
		return 0;
	} else {
		while(*str) {
			if (*str == ',' || *str == '\n')
				return str;
			else if (*str == '\"' && *(str+1) == '\"') 
				str += 2;
			else
				str++;
		}
	}
	return str;
}

//�ж��Ƿ����תΪ����,�����������ͣ�0Ϊ������
static int GetNumberType(const char* str) {
	int type = TYPE_INT, point = 0;
	if (!str) return 0;
	if (*str == '-') { str++; }

	if (!(*str >= '0' && *str <= '9')) return 0;
	str++;
	while (*str) {
		if (*str >= '0' && *str <= '9') {
			str++;
		} else if (*str == 'e' || *str == 'E') {
			type = TYPE_DOUBLE;
			break;
		} else if (*str == '.' && point == 0) {
			if (!(*(str+1) >= '0' && *(str+1) <= '9'))//С�����û������
				return 0;
			point++;
			type = TYPE_DOUBLE;
			str++;
			if (*str == 0) return 0;
		} else return 0;
	}

	if (*str && (*str == 'e' || *str == 'E')) {
		str++;
		if (*str != '+' && *str != '-') return 0;
		str++;
		while (*str && *str >= '0' && *str <= '9') str++;
		if (*str) return 0;
	}
	return type;
}

//����ֵ���Զ��ж�ֵ����
static const char* ParseValue(struct csv_value *value, const char* strdata) {
	const char* p = strdata;
	const char* stringvalue;
	const char* valueend;
	char* tempstring;
	int len;
	int trimlen;

	valueend = FindValueEnd(p);
	if (!valueend) return 0;
	

	len = valueend - p;
	tempstring = (char*)mymalloc(len+1);
	if (!tempstring) return 0;

	if (*p == '\"') {//�����ŵ�string
		
		strncpy(tempstring, p + 1, len-1);
		tempstring[len-1] = '\0';
		
		//ȥ��ǰ�����ź����ź�Ŀո�,�����ı����������������Ž�����һ��
		trimlen = Trim(tempstring);
		
		if (tempstring[trimlen-1] != '\"') {
			myfree(tempstring);
			return 0;
		}
		tempstring[--trimlen] = '\0';

		trimlen = Trim2(tempstring);
		

		value->type = TYPE_STRING;
		value->stringvalue = (char*)mymalloc(trimlen + 1);
		if (!value->stringvalue) {
			myfree(tempstring);
			return 0;
		}
		memset(value->stringvalue, 0, trimlen + 1);
		strcpy(value->stringvalue, tempstring);
		value->stringvalue[trimlen] = '\0';
	} else {
		strncpy(tempstring, p, len);
		tempstring[len] = '\0';

		if (!stricmp(tempstring, "true") || !stricmp(tempstring, "false")) {
			//bool
			value->type = TYPE_BOOL;
			value->intvalue = !stricmp(tempstring, "true") ? 1 : 0;
		} else {
			int type = GetNumberType(tempstring);
			if (type == TYPE_INT) {
				//int
				value->type = TYPE_INT;
				value->intvalue = atoi(tempstring);
			} else if (type == TYPE_DOUBLE) {
				//double
				value->type = TYPE_DOUBLE;
				value->doublevalue = atof(tempstring);
			} else {
				//string
				trimlen = Trim2(tempstring);

				value->type = TYPE_STRING;
				value->stringvalue = (char*)mymalloc(trimlen + 1);
				if (!value->stringvalue) {
					myfree(tempstring);
					return 0;
				}
				memset(value->stringvalue, 0, trimlen + 1);
				strcpy(value->stringvalue, tempstring);
				value->stringvalue[trimlen] = '\0';
			}
		}
	}
	p += len;

	myfree(tempstring);
	return p;
}

//����һ��
static const char* ParseLine(struct csv_value_list* value_list, const char* strdata) {
	const char* p = strdata;

	struct csv_value_list *nextvalue = value_list;
	struct csv_value_list *newvalue;
	
	ep = Skip(p);
	p = ParseValue(&nextvalue->value, ep);
	if (!p) return 0;
	
	while(*p == ',') {
		p++;
		
		newvalue = NewValueList();
		if (!newvalue) return 0;
		nextvalue->next = newvalue;
		
		ep = Skip(p);
		
		p = ParseValue(&(newvalue->value), ep);
		
		if (!p) return 0;
		
		nextvalue = nextvalue->next;
	}

	p = Skip(p);
	ep = p;
	if (*p == '\0') {
		return p;
	} else if (*p != '\n') {
		return 0;
	}

	return p + 1;
}

//��strdata����Ϊcsv_data�����ؽ���ֹͣ��λ��,����0��ʾ����ʧ��
static const char* Parse(struct csv_data *data, const char* strdata) {
	
	const char* p = strdata;
	const char* temp;
	struct csv_value_list* new_value_list;
	struct csv_data* datanext = data;
	struct csv_data* newdata;
	int lineindex = 1;

	//ѭ������������
	while (*p) {
		new_value_list = NewValueList();
		if (!new_value_list) return 0;
		datanext->value_list = new_value_list;

		ep = temp = p;
		p = ParseLine(new_value_list, p);
		if (!p) {
			sprintf(err, "line:%d,column:%d,error test:", lineindex, ep - temp);
			strncat(err, ep, 100);
			return 0;
		}

		p = Skip2(p);
		if (*p) {
			newdata = NewData();
			if (!newdata) return 0;
			datanext->next = newdata;
			datanext = newdata;
			lineindex++;
		}
	}
	
	return p;
}

//��ӡvalue��fp������
static void PrintValue(struct csv_value *value, FILE *fp) {
	if (!value) return;
	if (!fp) fp = stdout;
	switch(value->type) {
		case TYPE_NIL:
			fprintf(fp, "nil");
			break;
		case TYPE_INT:
			fprintf(fp, "%d", value->intvalue);
			break;
		case TYPE_DOUBLE:
			fprintf(fp, "%E", value->doublevalue);
			break;
		case TYPE_STRING:
			fprintf(fp, "%s", value->stringvalue);
			break;
		case TYPE_BOOL:
			fprintf(fp, value->intvalue ? "true" : "false");
			break;
	}
}


const char* Csv_GetErrorPtr() { return err; }

struct csv_data* Csv_Load(const char* filename) {
	struct csv_data* data;
	char* strdata;
	if (!filename) return 0;
	
	memset(err, 0, sizeof(err));

	strdata = GetFileString(filename);
	if (!strdata) return 0;

	data = Csv_Parse(strdata);
	if (!data) { myfree(strdata); return 0; }

	myfree(strdata);
	return data;
}

struct csv_data* Csv_Parse(const char* strdata) {
	struct csv_data* data;
	const char* end;

	if (!strdata) return 0;
	data = NewData();
	if (!data) return 0;

	ep = 0;
	memset(err, 0, sizeof(err));
	end = Parse(data, strdata);
	if (!end) { 
		DeleteData(data);
		return 0;
	}

	end = Skip2(end);
	//�����������,˵����ȡ���������
	if (*end) {
		DeleteData(data);
		return 0;
	}

	return data;
}

void Csv_Delete(struct csv_data* data) {
	DeleteData(data);
}

void Csv_PrintData(struct csv_data* data) {
	struct csv_data *p = data;
	struct csv_value_list *list;
	int lineindex = 1;
	int col = 0;
	if (!data) return;
	while (p) {
		printf("line:%d:", lineindex);
		col = 0;
		if (p->value_list) {
			list = p->value_list;
			while (list) {
				
				PrintValue(&(list->value), stdout);
				list = list->next;
				col++;
				if (list) printf(",");
			}
		} else {
			printf("--NIL LINE--");
		}
		printf("|col:%d", col);
		putchar('\n');
		lineindex++;
		p = p->next;
	}
}

int Csv_PrintToFile(struct csv_data* data, const char* filename) {
	FILE *fp;
	struct csv_data *p = data;
	struct csv_value_list *list;
	int othercount, quotescount, stringcount;
	char *temp;
	if (!data || !filename) return 1;

	fp = fopen(filename, "w");
	if (!fp) return 2;
	while(p) {
		if (p->value_list) {
			list = p->value_list;
			while (list) {
				if (list->value.type != TYPE_STRING) {
					PrintValue(&(list->value), fp);
				} else {
					//���string�к��ж������Ż��з�����Ҫ�����Ž�������������ǰ���������
					quotescount = GetCharCount(list->value.stringvalue, '\"');
					othercount = GetCharCount(list->value.stringvalue, ',') + GetCharCount(list->value.stringvalue, '\n');
					stringcount = strlen(list->value.stringvalue) + quotescount*2 + 3;
					if (quotescount + othercount > 0) {
						temp = (char*)mymalloc(stringcount);
						if (!temp) return 3;
				
						stringcount = Trim3(list->value.stringvalue, temp);
						fprintf(fp, temp);
						myfree(temp);

					} else {
						PrintValue(&(list->value), fp);
					}
					
				}
				list = list->next;
				if (list) fprintf(fp, ",");
			}
		}
		p = p->next;
		if (p) fprintf(fp, "\n");
	}

	fclose(fp);
	return 0;
}