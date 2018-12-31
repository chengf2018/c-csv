/* csv�ļ�����
 * author:chengf
 * date:2018-12-31
 */

#define TYPE_NIL     0
#define TYPE_INT     1
#define TYPE_DOUBLE  2
#define TYPE_STRING  3
#define TYPE_BOOL    4

#ifdef __cplusplus
extern "C"
{
#endif

struct csv_value {
	int type;

	int intvalue;
	double doublevalue;
	char* stringvalue;
};

struct csv_value_list {
	struct csv_value value;
	struct csv_value_list* next;
};

struct csv_data {
	struct csv_data* next;//��һ��
	struct csv_value_list* value_list;//value list
};

//��ȡ����λ���ı�
extern const char* Csv_GetErrorPtr();

//����Csv�ļ�ΪC�ṹ
extern struct csv_data* Csv_Load(const char* filename);

//�ͷ�C�ṹ
extern void Csv_Delete(struct csv_data* data);

//�����ı�ΪC�ṹ
extern struct csv_data* Csv_Parse(const char* strdata);

//���c�ṹ
extern void Csv_PrintData(struct csv_data* data);

//���c�ṹ���ļ�
extern int Csv_PrintToFile(struct csv_data* data, const char* filename);

#ifdef __cplusplus
}
#endif