
#include "qe.h"

/*******************************************************************
 *********************Helper Function Declaration*******************
 *******************************************************************/
// compare two attribute
bool compareField(const void *attribute, const void *condition, AttrType type, CompOp compOp) {
	if (condition == NULL)
		return true;
	bool result = true;
	switch (type)
	{
	case TypeInt: {
		int attr = *(int *)attribute;
		int cond = *(int *)condition;
		switch(compOp) {
		case EQ_OP: result = attr == cond; break;
		case LT_OP: result = attr < cond; break;
		case GT_OP: result = attr > cond; break;
		case LE_OP: result = attr <= cond; break;
		case GE_OP: result = attr >= cond; break;
		case NE_OP: result = attr != cond; break;
		case NO_OP: break;
		}
		break;
	}
	case TypeReal: {
		float attr = *(float *)attribute;
		float cond = *(float *)condition;
		int temp = 0;
		if (attr - cond > 0.00001)
			temp = 1;
		else if (attr - cond < -0.00001)
			temp = -1;
		switch(compOp) {
		case EQ_OP: result = temp == 0; break;
		case LT_OP: result = temp < 0; break;
		case GT_OP: result = temp > 0; break;
		case LE_OP: result = temp <= 0; break;
		case GE_OP: result = temp >= 0; break;
		case NE_OP: result = temp != 0; break;
		case NO_OP: break;
		}
		break;
	}
	case TypeVarChar: {
		int attriLeng = *(int *)attribute;
		string attr((char *)attribute + sizeof(int), attriLeng);
		int condiLeng = *(int *)condition;
		string cond((char *)condition + sizeof(int), condiLeng);
		switch(compOp) {
		case EQ_OP: result = strcmp(attr.c_str(), cond.c_str()) == 0; break;
		case LT_OP: result = strcmp(attr.c_str(), cond.c_str()) < 0; break;
		case GT_OP: result = strcmp(attr.c_str(), cond.c_str()) > 0; break;
		case LE_OP: result = strcmp(attr.c_str(), cond.c_str()) <= 0; break;
		case GE_OP: result = strcmp(attr.c_str(), cond.c_str()) >= 0;break;
		case NE_OP: result = strcmp(attr.c_str(), cond.c_str()) != 0; break;
		case NO_OP: break;
		}
		break;
	}
	}
	return result;
}

// read attribute from a tuple given its descriptor and attribute position and type,
void readField(const void *input, void *data, vector<Attribute> attrs, Attribute conditionAttri, AttrType type) {
	int offset = 0;
	int attrLength = 0;
	for (unsigned i = 0; i < attrs.size(); i++) {
		if(attrs[i].name.c_str() == conditionAttri.name.c_str()) break;
		else{
			if (attrs[i].type == TypeInt)
				offset += sizeof(int);
			else if (attrs[i].type == TypeReal)
				offset += sizeof(float);
			else {
				int stringLength = *(int *)((char *)input + offset);
				offset += sizeof(int) + stringLength;
			}
		}
	}
	if (type == TypeInt) {
		attrLength = sizeof(int);
	}
	else if (type == TypeReal) {
		attrLength = sizeof(float);
	}
	else {
		attrLength = *(int *)((char *)input + offset) + sizeof(int);
	}
	memcpy(data, (char *)input + offset, attrLength);
}


/*******************************************************************
 **************************     Filter     *************************
 *******************************************************************/

Filter::Filter(Iterator* input, const Condition &condition):itr(input) {

	op = condition.op;
	this->condition = condition.rhsValue.data;
	conditionType = condition.rhsValue.type;
	// get the attributes from input Iterator
	attrs.clear();
	this->itr->getAttributes(attrs);
	for (unsigned i = 0; i < attrs.size(); i++) {
		if (attrs[i].name.compare(condition.lhsAttr) == 0) {  //find the attribute needed
			conditionAttri.name = attrs[i].name;
			conditionAttri.length = attrs[i].length;
			conditionAttri.type = attrs[i].type;
			value = malloc(attrs[i].length + sizeof(int));
			break;
		}
	}
}

// ... the rest of your implementations go here
Filter::~Filter(){
	free(value);
}

RC Filter::getNextTuple(void* data){
	int returnValue = -1;
	//void* returnTuple = malloc(PAGE_SIZE);

	do {
		returnValue = itr->getNextTuple(data);
		if (returnValue != 0)
			return returnValue;
		readField(data, this->value, attrs, conditionAttri, conditionType);
	}
	while (!compareField(this->value, condition, conditionType, op));
	return returnValue;
}

void Filter::getAttributes(vector<Attribute> &attrs) const{
	attrs.clear();
	attrs = this->attrs;
}

/*******************************************************************
 *************************     Project     *************************
 *******************************************************************/
Project::Project(Iterator *input, const vector<string> &attrNames):itr(input){
	attrs.clear();
	oriAttrs.clear();
	itr->getAttributes(oriAttrs);
	unsigned i = 0, j = 0;
	for (; i < oriAttrs.size(); i++) {
		if (oriAttrs[i].name.compare(attrNames[j]) == 0) {
			attrs.push_back(oriAttrs[i]);
			j++;
		}
	}
	tuple = malloc(PAGE_SIZE);
}
Project::~Project(){
	free(tuple);
}
RC Project::getNextTuple(void *data){
	int returnValue = -1;
	returnValue = itr->getNextTuple(tuple);
	if (returnValue != 0) {
		return returnValue;
	}
	//project unwanted attri out of the tuple
	unsigned i=0, j=0;
	int offsetI = 0, offsetJ = 0;
	while (i < oriAttrs.size() && j < attrs.size()) {
		// get the length of field input[i]
		int length = 0;
		if (oriAttrs[i].type == TypeInt)
			length = sizeof(int);
		else if (oriAttrs[i].type == TypeReal)
			length = sizeof(float);
		else {
			length = sizeof(int) + *(int *)((char *)tuple + offsetI);
		}
		// if same attribute as data[j] copy attribute and increase offset of data
		if (oriAttrs[i].name.compare(attrs[j].name) == 0) {
			memcpy((char *)data + offsetJ, (char *)tuple + offsetI, length);
			offsetJ += length;
			j++;
		}
		// go to next field
		i++;
		offsetI += length;
	}

	return 0;
}
void Project::getAttributes(vector<Attribute> &attrs) const{
	attrs.clear();
	attrs = this->attrs;
}

/*******************************************************************
 ***********************     Aggregate     *************************
 *******************************************************************/

Aggregate::Aggregate(Iterator *input, Attribute aggAttr, AggregateOp op):itr(input){
	this->op=op;
	aggrAttribute.name = aggAttr.name;
	aggrAttribute.length = aggAttr.length;
	aggrAttribute.type = aggAttr.type;

	isNextTuple = false;

	input->getAttributes(tblAttributes);
}

Aggregate::~Aggregate(){

}

void Aggregate::getAttributes(vector<Attribute> &attrs) const{
	attrs.clear();
	Attribute attr;
	string name (aggrAttribute.name);
	string close (")");
	if (op == MIN){
		string min ("MIN(");
		attr.name = min + name + close;
	}
	else if (op == MAX) {
		string max ("MAX(");
		attr.name = max + name + close;
	}
	else if (op == COUNT) {
		string count ("COUNT(");
		attr.name = count + name + close;
	}
	else if (op == AVG) {
		string avg ("AVG(");
		attr.name = avg + name + close;
	}
	else if (op == SUM) {
		string sum ("SUM(");
		attr.name = sum + name + close;
	}
	attrs.push_back(attr);
}

RC Aggregate::getNextTuple(void *data){

	if (isNextTuple == false){
		return QE_EOF;
	}
	if (op == MIN){
		return getMin(data);
	}
	else if (op == MAX) {
		return getMax(data);
	}
	else if (op == COUNT) {
		return getCount(data);
	}
	else if (op == AVG) {
		return getAvg(data);
	}
	else if (op == SUM) {
		return getSum(data);
	}
	return QE_EOF;
}

RC Aggregate::getMin(void* data){
	void * tempData = malloc(PAGE_SIZE);
	void * minimum = malloc(sizeof(int));
	if (aggrAttribute.type == TypeInt){
		int max = INT_MAX;
		memcpy(minimum, &max, sizeof(int));
	}
	else{
		float max = FLT_MAX;
		memcpy(minimum, &max, sizeof(int));
	}
	while (itr->getNextTuple(tempData) != QE_EOF){
		//short currentField = 0;
		int varCharLength;
		char * dataPtr = (char*) tempData;
		//go to the beginning of the field we want to read for current tuple
		unsigned i=0;
		do{
			if (tblAttributes[i].type == TypeVarChar) {

				memcpy(&varCharLength, dataPtr, sizeof(int));
				dataPtr += sizeof(int) + varCharLength;
			}
			else{
				dataPtr += sizeof(int);
			}
		}
		while(tblAttributes[i].name.c_str() != aggrAttribute.name.c_str());

		//read the field
		if (aggrAttribute.type == TypeInt) {
			if(*(int*)dataPtr<*(int*)minimum)
				memcpy(minimum, dataPtr, sizeof(int));
		}
		else {
			if(*(float*)dataPtr<*(float*)minimum)
				memcpy(minimum, dataPtr, sizeof(int));
		}
	}
	memcpy(data, minimum, sizeof(int));
	free(minimum);
	free(tempData);
	isNextTuple = false;
	return 0;
}

RC Aggregate::getMax(void* data){
	void * tempData = malloc(PAGE_SIZE);
	void * maximum = malloc(sizeof(int));
	if (aggrAttribute.type == TypeInt){
		int max = INT_MIN;
		memcpy(maximum, &max, sizeof(int));
	}
	else{
		float max = FLT_MIN;
		memcpy(maximum, &max, sizeof(int));
	}
	while (itr->getNextTuple(tempData) != QE_EOF){
		//short currentField = 0;
		int varCharLength;
		char * dataPtr = (char*) tempData;
		//go to the beginning of the field we want to read for current tuple
		unsigned i=0;
		do{
			if (tblAttributes[i].type == TypeVarChar) {

				memcpy(&varCharLength, dataPtr, sizeof(int));
				dataPtr += sizeof(int) + varCharLength;
			}
			else{
				dataPtr += sizeof(int);
			}
		}
		while(tblAttributes[i].name.c_str() != aggrAttribute.name.c_str());

		//read the field
		if (aggrAttribute.type == TypeInt) {
			if(*(int*)dataPtr>*(int*)maximum)
				memcpy(maximum, dataPtr, sizeof(int));
		}
		else {
			if(*(float*)dataPtr>*(float*)maximum)
				memcpy(maximum, dataPtr, sizeof(int));
		}
	}
	memcpy(data, maximum, sizeof(int));
	free(maximum);
	free(tempData);
	isNextTuple = false;
	return 0;
}

RC Aggregate::getSum(void *data){
	void * tempData = malloc(PAGE_SIZE);
	void * currentSum = malloc(sizeof(int));
	if (aggrAttribute.type == TypeInt){
		int tempSum = 0;
		memcpy(currentSum, &tempSum, sizeof(int));
	}
	else{
		float tempSum = 0;
		memcpy(currentSum, &tempSum, sizeof(int));
	}
	while (itr->getNextTuple(tempData) != QE_EOF){
		int varCharLength;
		char * dataPtr = (char*) tempData;
		//go to the beginning of the field we want to read for current tuple
		unsigned i=0;
		do{
			if (tblAttributes[i].type == TypeVarChar) {
					memcpy(&varCharLength, dataPtr, sizeof(int));
				dataPtr += sizeof(int) + varCharLength;
			}
			else{
				dataPtr += sizeof(int);
			}
		}
		while(tblAttributes[i].name.c_str() != aggrAttribute.name.c_str());
			//read the field
		if (aggrAttribute.type == TypeInt) {
			*(int*)currentSum = *(int*)currentSum+*(int*)dataPtr;
		}
		else {
			*(float*)currentSum = *(float*)currentSum+*(float*)dataPtr;
		}
	}
	memcpy(data, currentSum, sizeof(int));
	free(currentSum);
	free(tempData);
	isNextTuple = false;
	return 0;
}

RC Aggregate::getAvg(void *data){
	void * tempData = malloc(PAGE_SIZE);
	void * currentSum = malloc(sizeof(int));
	int count = 0;
	if (aggrAttribute.type == TypeInt){
		int tempSum = 0;
		memcpy(currentSum, &tempSum, sizeof(int));
	}
	else{
		float tempSum = 0;
		memcpy(currentSum, &tempSum, sizeof(int));
	}
	while (itr->getNextTuple(tempData) != QE_EOF){
		count++;
		int varCharLength;
		char * dataPtr = (char*) tempData;
		//go to the beginning of the field we want to read for current tuple
		unsigned i=0;
		do{
			if (tblAttributes[i].type == TypeVarChar) {
					memcpy(&varCharLength, dataPtr, sizeof(int));
				dataPtr += sizeof(int) + varCharLength;
			}
			else{
				dataPtr += sizeof(int);
			}
		}
		while(tblAttributes[i].name.c_str() != aggrAttribute.name.c_str());
			//read the field
		if (aggrAttribute.type == TypeInt) {
			*(int*)currentSum = *(int*)currentSum+*(int*)dataPtr;
		}
		else {
			*(float*)currentSum = *(float*)currentSum+*(float*)dataPtr;
		}
	}

	if (aggrAttribute.type == TypeInt) {
		*(int*)currentSum = *(int*)currentSum/count;
	}
	else {
		*(float*)currentSum = *(float*)currentSum/count;
	}

	memcpy(data, currentSum, sizeof(int));
	free(currentSum);
	free(tempData);
	isNextTuple = false;
	return 0;
}

RC Aggregate::getCount(void *data){
	void * tempData = malloc(PAGE_SIZE);
	int count = 0;
	while (itr->getNextTuple(tempData) != QE_EOF){
		count++;
	}

	memcpy(data, &count, sizeof(int));
	free(currentSum);
	free(tempData);
	isNextTuple = false;
	return 0;
}
