#include "rm.h"

#include <stdio.h>
#include <string.h>
static bool init;
vector <Attribute> catalogsRecordDescriptor;

RelationManager* RelationManager::_rm = 0;

RelationManager* RelationManager::instance()
{
    if(!_rm)
        _rm = new RelationManager();

    return _rm;
}

RelationManager::RelationManager()
{
	tableId=0;
	rbfm = RecordBasedFileManager::instance();
	ixManager = IndexManager::instance();
	pfm = PagedFileManager::instance();
	initialVectors();
}

RelationManager::~RelationManager()
{
}
RC RelationManager::initialVectors()
{
    Attribute attr;

    attr.name = "tableId";
    attr.type = TypeInt;
    attr.length = 4;
    Tables.push_back(attr);
    column.push_back(attr);

    attr.name = "tableName";
    attr.type = TypeVarChar;
    attr.length = 256;
    Tables.push_back(attr);

    attr.name = "fileName";
    attr.type = TypeVarChar;
    attr.length = 256;
    Tables.push_back(attr);


    attr.name = "columnName";
    attr.type = TypeVarChar;
    attr.length = 256;
    column.push_back(attr);

    attr.name = "columnType";
    attr.type = TypeInt;
    attr.length = 4;
    column.push_back(attr);

    attr.name = "columnLength";
    attr.type = TypeInt;
    attr.length = 4;
    column.push_back(attr);

    attr.name = "position";
    attr.type = TypeInt;
    attr.length = 4;
    column.push_back(attr);
    return 0;
}
RC RelationManager::initialFiles()
{
    //initialVectors();

    FileHandle fileHandle;
    FileHandle _fileHandle;
    RID rid;
    rbfm ->createFile("Tables");
    rbfm ->openFile("Tables", fileHandle);
    rbfm ->createFile("column");
    rbfm ->openFile("column", _fileHandle);


	//*******prepareTableData********

    void* data=malloc(PAGE_SIZE);

    //tableId

    tableId = getTableId();
    *(int*)data = tableId;
    //tabelName
    int sizeOfTables=6;
    *((int*)data+1)=sizeOfTables;  //size of "Tables"
    memcpy((char*)data+2*sizeof(int),"Tables",sizeOfTables);
    //fileName,the same to tableName
    *(int*)((char*)data+2*sizeof(int)+sizeOfTables)=sizeOfTables;
    memcpy((char*)data+3*sizeof(int)+sizeOfTables,"Tables",sizeOfTables);

	int insertTablesResult=rbfm->insertRecord(fileHandle,Tables, data, rid);
    free(data);
  
    
    /*****Insert Tables table information into column table*****/
    

    data=malloc(PAGE_SIZE);

    for(unsigned int i=0;i<3;++i){
        *(int*)data = tableId;   //table id
        *((int*)data+1)=Tables[i].name.size();   //columns name size
        memcpy((char*)data+2*sizeof(int),Tables[i].name.c_str(),Tables[i].name.size());   //column name
        *(int*)((char*)data+2*sizeof(int)+Tables[i].name.size())=Tables[i].type;      //column type
        *(int*)((char*)data+3*sizeof(int)+Tables[i].name.size())=Tables[i].length;    //column length
        *(int*)((char*)data+4*sizeof(int)+Tables[i].name.size())=i+1;                  //position, start from 1
        rbfm->insertRecord(_fileHandle,Tables, data, rid);                      //insert into column
    }
    free(data);
    /********Insert end*********************************/


/*******Insert column information into Tables***********************/
    data=malloc(PAGE_SIZE);
    
    //tableId
    
    tableId = getTableId();
    *(int*)data = tableId;
    //tabelName
    int sizeOfColumn=6;
    *((int*)data+1)=sizeOfColumn;  //size of "Tables"
    memcpy((char*)data+2*sizeof(int),"column",sizeOfColumn);
    //fileName,the same to tableName
    *(int*)((char*)data+2*sizeof(int)+sizeOfColumn)=sizeOfColumn;
    memcpy((char*)data+3*sizeof(int)+sizeOfColumn,"column",sizeOfColumn);
    
    insertTablesResult=rbfm->insertRecord(fileHandle,Tables, data, rid);
    free(data);
  /***************Insert end****************************************/
    //*******prepareColumsData********
    data=malloc(PAGE_SIZE);
  //  rbfm->openFile("column",_fileHandle);
    for(unsigned int i=0;i<5;++i){
    	*(int*)data = tableId;   //table id
    	*((int*)data+1)=column[i].name.size();   //columns name size
    	memcpy((char*)data+2*sizeof(int),column[i].name.c_str(),column[i].name.size());   //column name
    	*(int*)((char*)data+2*sizeof(int)+column[i].name.size())=column[i].type;      //column type
    	*(int*)((char*)data+3*sizeof(int)+column[i].name.size())=column[i].length;    //column length
    	*(int*)((char*)data+4*sizeof(int)+column[i].name.size())=i+1;                  //position, start from 1

        
    	rbfm->insertRecord(_fileHandle,column, data, rid);
        //insert into column
    }
    free(data);
  //  rbfm->closeFile(_fileHandle);

    rbfm->closeFile(fileHandle);
    rbfm->closeFile(_fileHandle);
    init = true;
    return 0;
}

int RelationManager::getTableId()
{
	int tableId = 0;
	FileHandle fileHandle;
    void* pageData = malloc(PAGE_SIZE);
    int totalSlotNum;
	rbfm->openFile("Tables", fileHandle);
	unsigned int totalPageNum = fileHandle.getNumberOfPages();

	if(totalPageNum == 0){
		tableId = 1;
		return tableId;
	}
	//unsigned int pageNum = totalPageNum-1;
    for(int i=0;i<totalPageNum;i++)
    {
        fileHandle.readPage(i, pageData);
        memcpy(&totalSlotNum, (char*)pageData+PAGE_SIZE-2*sizeof(int), sizeof(int));
        tableId=tableId+totalSlotNum;
        
    }

    tableId=tableId+1;
	return tableId;
}

RC RelationManager::createTable(const string &tableName, const vector<Attribute> &attrs)
{
	//check initialization
	FILE* pFile = fopen ("Tables", "r+b");
	FILE* _pFile = fopen("column", "r+b");
	if ((pFile == NULL) || (_pFile = NULL)){
        initialFiles();
	    init=true;
	}
	// check if table name is valid
	if ((tableName.compare("Tables") == 0) || (tableName.compare("column") == 0))
	{
		cout << "Table name has been used by the system, please change table name!" << std::endl;
		return -1;
	}

	//check if table name already exists
	if(fopen(tableName.c_str(), "r+b") != NULL)
	{
		cout<<"This table has already been created"<<endl;
		return 0;
	}

	FileHandle fileHandle;
	RID rid;

	//*******prepareTableData********

   // void* data = malloc(memoryNeeded(Tables));
    void* data=malloc(PAGE_SIZE);

    //tableId
    //*(int*)data = this->tableId+1;
    tableId = getTableId();
    *(int*)data = tableId;
    //tabelName
    *((int*)data+1)=tableName.size();
    int test=tableName.size();
    memcpy((char*)data+2*sizeof(int),tableName.c_str(),tableName.size());
    //fileName,the same to tableName
    *(int*)((char*)data+2*sizeof(int)+tableName.size())=tableName.size();
    memcpy((char*)data+3*sizeof(int)+tableName.size(),tableName.c_str(),tableName.size());
    //insert into Tables
	rbfm->openFile("Tables", fileHandle);
	int insertTablesResult=rbfm->insertRecord(fileHandle,Tables, data, rid);
    free(data);
    rbfm->closeFile(fileHandle);


    //*******prepareColumsData********
	//data = malloc(memoryNeeded(column));
    data=malloc(PAGE_SIZE);
	rbfm->openFile("column",fileHandle);
	for(unsigned int i=0;i<attrs.size();++i){
		*(int*)data = tableId;   //table id
		*((int*)data+1)=attrs[i].name.size();   //columns name size
		memcpy((char*)data+2*sizeof(int),attrs[i].name.c_str(),attrs[i].name.size());   //column name
		*(int*)((char*)data+2*sizeof(int)+attrs[i].name.size())=attrs[i].type;      //column type
		*(int*)((char*)data+3*sizeof(int)+attrs[i].name.size())=attrs[i].length;    //column length
		*(int*)((char*)data+4*sizeof(int)+attrs[i].name.size())=i+1;                  //position, start from 1
		rbfm->insertRecord(fileHandle,column, data, rid);                      //insert into column
	}

    rbfm->closeFile(fileHandle);

    rbfm->createFile(tableName);
    free(data);
	return 0;
}

int RelationManager::memoryNeeded(const vector<Attribute> &attributes)
{
    int size = 0;

    for (int i = 0; i < (int)attributes.size(); i++) {
        if(attributes[i].type == TypeVarChar) {
            size += sizeof(int);
            //size+=attributes[i].length;
        }
        size += attributes[i].length;
    }

    return size;
}

RC RelationManager::deleteTable(const string &tableName)
{
   /* if(!init)
    	initialFiles();*/
   // FileHandle
    int result=0;
     FileHandle fileHandle;
    FileHandle _fileHandle;
    result=rbfm->openFile("Tables", fileHandle);
    if(result==-1)
        return -1;
    result=rbfm->openFile("column", _fileHandle);
    if (result==-1) {
        return -1;
    }
    rbfm->destroyFile(tableName);
    RBFM_ScanIterator rbfm_scanIterator;
    //FileHandle fileHandle;

    rbfm->openFile("Tables",fileHandle);       //delete tables Table
    void* value=malloc(sizeof(int)+tableName.size());
    *(int*)value=tableName.size();
    memcpy((char*)value+sizeof(int),tableName.c_str(),tableName.size());

    vector<string>cols;
    cols.push_back("tableId");

    rbfm->scan(fileHandle,Tables,string("tableName"),EQ_OP,value,cols,rbfm_scanIterator);

    void*data=malloc(PAGE_SIZE);
    RID rid;
    rid.pageNum=0;
    rid.slotNum=1;
    while(rbfm_scanIterator.getNextRecord(rid,data)!= RBFM_EOF){
    	rbfm->deleteRecord(fileHandle,Tables,rid);
    }
    rbfm->closeFile(fileHandle);
    free(value);


    rbfm->openFile("column",_fileHandle);
    value=malloc(sizeof(int));
    *(int*)value=*(int*)data;
    rbfm->scan(_fileHandle,column,string("tableId"),EQ_OP,value,cols,rbfm_scanIterator);
    rid.pageNum=0;
    rid.slotNum=1;
    while(rbfm_scanIterator.getNextRecord(rid,data)!=-1){
    	rbfm->deleteRecord(_fileHandle,column,rid);
    }
    free(value);
    free(data);

	return 0;
}

RC RelationManager::getAttributes(const string &tableName, vector<Attribute> &attrs)
{

    RBFM_ScanIterator rbfm_ScanIterator;
    FileHandle tableFileHandle;
    FileHandle columnFileHandle;

    vector<string> attributeNames;
    attributeNames.push_back("tableId");

    void *tableRecord=malloc(PAGE_SIZE);
    void *columnRecord=malloc(PAGE_SIZE);
    void *tableNameRecord=malloc(PAGE_SIZE);

    int tableId=0;
    int attriCount=0;
    char nameTemp;

    string name;
    AttrType type;
    AttrLength length;
    Attribute attrsTemp;

    RID rid;
    rid.pageNum=0;
    rid.slotNum=1;
    //initialVectors();
    int tableNameSize=tableName.size();
    *(int*)tableNameRecord=tableNameSize;
    memcpy((char*)tableNameRecord+sizeof(int), tableName.c_str(), tableNameSize);//CHANGE!!!!!!!!!!!!!!!!!!!!
    rbfm->openFile("Tables",tableFileHandle);
    rbfm->openFile("column",columnFileHandle);

    rbfm->scan(tableFileHandle, Tables, "tableName", EQ_OP, tableNameRecord, attributeNames, rbfm_ScanIterator);

    while(rbfm_ScanIterator.getNextRecord(rid,tableRecord)!=RBFM_EOF)
    {
        memcpy(&tableId, (char*)tableRecord, sizeof(int));
        attributeNames.clear();
        attributeNames.push_back("columnName");
        attributeNames.push_back("columnType");
        attributeNames.push_back("columnLength");
        rbfm->scan(columnFileHandle, column, "tableId", EQ_OP, &tableId, attributeNames, rbfm_ScanIterator);
        while (rbfm_ScanIterator.getNextRecord(rid,columnRecord)!=RBFM_EOF)
        {
            name="";
            int varCharLength=0;
            memcpy(&varCharLength,columnRecord,sizeof(int));
            for(int i=0;i<varCharLength;i++)
            {
                memcpy(&nameTemp, (char*)columnRecord+sizeof(int)+i, 1);
                name+=nameTemp;
            }
            memcpy(&type, (char*)columnRecord+sizeof(int)+varCharLength, sizeof(int));
            memcpy(&length, (char*)columnRecord+2*sizeof(int)+varCharLength, sizeof(int));
            attrsTemp.name.assign(name);
            attrsTemp.type=type;
            attrsTemp.length=length;
            attrs.push_back(attrsTemp);
            attriCount++;
            
        }
        attributeNames.clear();
        break;
    }

    free(tableRecord);
    free(columnRecord);
    free(tableNameRecord);
    rbfm->closeFile(tableFileHandle);
    rbfm->closeFile(columnFileHandle);
    return 0;
}

RC RelationManager::insertTuple(const string &tableName, const void *data, RID &rid)
{

    int result=0;
    FileHandle fileHandle;
    vector<Attribute> recordDescriptor;
    result=rbfm->openFile(tableName, fileHandle);
    if(result==-1)
        return -1;
    getAttributes(tableName, recordDescriptor);
    rbfm->insertRecord(fileHandle, recordDescriptor, data, rid);
    rbfm->closeFile(fileHandle);
    return 0;
}

RC RelationManager::deleteTuples(const string &tableName)
{
    FileHandle fileHandle;
    rbfm->openFile(tableName, fileHandle);
    rbfm->deleteRecords(fileHandle);
    rbfm->closeFile(fileHandle);
    return 0;
}

RC RelationManager::deleteTuple(const string &tableName, const RID &rid)
{
    FileHandle fileHandle;
    vector<Attribute> recordDescriptor;
    rbfm->openFile(tableName, fileHandle);
    getAttributes(tableName, recordDescriptor);
    rbfm->deleteRecord(fileHandle, recordDescriptor, rid);
    rbfm->closeFile(fileHandle);
    return 0;
}

RC RelationManager::updateTuple(const string &tableName, const void *data, const RID &rid)
{
    FileHandle fileHandle;
    vector<Attribute> recordDescriptor;
    rbfm->openFile(tableName, fileHandle);
    getAttributes(tableName, recordDescriptor);
    rbfm->updateRecord(fileHandle, recordDescriptor, data, rid);
    rbfm->closeFile(fileHandle);
    return 0;
}

RC RelationManager::readTuple(const string &tableName, const RID &rid, void *data)
{
    int result=0;
    FileHandle fileHandle;
    vector<Attribute> recordDescriptor;
    result=rbfm->openFile(tableName, fileHandle);
    if(result==-1)
        return -1;
    getAttributes(tableName, recordDescriptor);
    result=rbfm->readRecord(fileHandle, recordDescriptor, rid, data);
    if(result==-1)
        return -1;
    rbfm->closeFile(fileHandle);
    return 0;
}

RC RelationManager::readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data)
{
    int result=0;
    FileHandle fileHandle;
    vector<Attribute> recordDescriptor;
    result=rbfm->openFile(tableName, fileHandle);
    if(result==-1)
        return -1;
    getAttributes(tableName, recordDescriptor);
    result=rbfm->readAttribute(fileHandle, recordDescriptor, rid, attributeName, data);
    if(result==-1)
        return -1;
    rbfm->closeFile(fileHandle);
    return 0;
}

RC RelationManager::reorganizePage(const string &tableName, const unsigned pageNumber)
{
    int result=0;
    FileHandle fileHandle;
    vector<Attribute> recordDescriptor;
    result=rbfm->openFile(tableName, fileHandle);
    if(result==-1)
        return -1;
    getAttributes(tableName, recordDescriptor);
    result=rbfm->reorganizePage(fileHandle, recordDescriptor, pageNumber);
    if(result==-1)
        return-1;
    rbfm->closeFile(fileHandle);
    return 0;
}


RC RelationManager::scan(const string &tableName,
      const string &conditionAttribute,
      const CompOp compOp,
      const void *value,
      const vector<string> &attributeNames,
      RM_ScanIterator &rm_ScanIterator)
{
    int result=0;
    FileHandle fileHandle;
    vector<Attribute>attrs;
    result=rbfm->openFile(tableName,fileHandle);
    if(result==-1)
        return -1;
    getAttributes(tableName,attrs);
   // result=rbfm->openFile(tableName,fileHandle);
    rm_ScanIterator.setScanIterator(fileHandle,attrs,conditionAttribute,compOp,value,attributeNames);
  //  rbfm->closeFile(fileHandle);
	return 0;
}

RC RelationManager::createIndex(const string &tableName, const string &attributeName)
{
    return -1;
}

RC RelationManager::destroyIndex(const string &tableName, const string &attributeName)
{
    return -1;
}

// indexScan returns an iterator to allow the caller to go through qualified entries in index
RC RelationManager::indexScan(const string &tableName,
      const string &attributeName,
      const void *lowKey,
      const void *highKey,
      bool lowKeyInclusive,
      bool highKeyInclusive,
      RM_IndexScanIterator &rm_IndexScanIterator)
{
	string indexFileName = tableName + "_" + attributeName + "_idx";
	// STEP1 : check if this idx file exists;
	FILE* pFile = fopen(indexFileName.c_str(), "r+b");
	if (pFile == NULL)
		return -1;

	// STEP2 : open file with fileHandle
	RC returnValue = ixManager->openFile(indexFileName, rm_IndexScanIterator.ixFileHandle);
	if (returnValue != 0)
		return returnValue;
	// STEP3 : get Attribute;
	Attribute keyAttribute;
	vector<Attribute> attributes;
	returnValue = getAttributes(tableName, attributes);
	if (returnValue != 0)
		return returnValue;

	unsigned i = 0;
	for (; i < attributes.size(); i++)
	{
		if (attributes[i].name.compare(attributeName) == 0)
		{
			keyAttribute = attributes[i];
			break;
		}
	}
	// attribute not found
	if (i == attributes.size())
	{
		ixManager->closeFile(rm_IndexScanIterator.ixFileHandle);
		return -1;
	}
	returnValue = rm_IndexScanIterator.initialize(keyAttribute, lowKey, highKey, lowKeyInclusive, highKeyInclusive);
	return returnValue;
}



// Extra credit
RC RelationManager::dropAttribute(const string &tableName, const string &attributeName)
{
	RBFM_ScanIterator rbfm_ScanIterator;
	vector<string> attributeNames;

    FileHandle tableFileHandle;
	attributeNames.push_back("tableId");
	int tableNameSize = tableName.size();
	void* tableNameRecord = malloc(PAGE_SIZE);
	*(int*)tableNameRecord = tableNameSize;
	memcpy((char*)tableNameRecord+sizeof(int), tableName.c_str(), tableNameSize);
    rbfm -> openFile("Tables", tableFileHandle);
    rbfm -> scan(tableFileHandle, Tables, "tableName", EQ_OP, tableNameRecord, attributeNames, rbfm_ScanIterator);

    FileHandle columnFileHandle;
    rbfm -> openFile("column", columnFileHandle);

    RID rid;
    void* tableRecord = malloc(PAGE_SIZE);
    void* columnRecord = malloc(PAGE_SIZE);
    int tableId = 0;

    string name;
    char nameTemp;

    while(rbfm_ScanIterator.getNextRecord(rid, tableRecord) != RBFM_EOF)
    {
    	attributeNames.clear();
    	attributeNames.push_back("columnName");
    	attributeNames.push_back("columnType");
    	attributeNames.push_back("columnLength");

    	memcpy(&tableId, tableRecord, sizeof(int));
    	rbfm->scan(columnFileHandle, column, "tableId", EQ_OP, &tableId, attributeNames, rbfm_ScanIterator);

    	while(rbfm_ScanIterator.getNextRecord(rid, columnRecord) != RBFM_EOF)
    	{
    		name="";
    		int varCharLength=0;
    		memcpy(&varCharLength,columnRecord,sizeof(int));
    		for(int i=0;i<varCharLength;i++)
    		{
    			memcpy(&nameTemp, (char*)columnRecord+sizeof(int)+i, 1);
    		    name+=nameTemp;
    		}

    		if(name == attributeName)
    		{
    			void* data = malloc(PAGE_SIZE);
    			void* newData = malloc(PAGE_SIZE);
    			rbfm->readRecord(columnFileHandle, column, rid, data);

    			int columnNameSize = 0;
    			memcpy(&columnNameSize, (char*)data, sizeof(int));
    			memcpy((char*)newData, (char*)data, sizeof(int)+columnNameSize+sizeof(int));  //column name & type
    			int tempLength = -1;
    			memcpy((char*)newData+sizeof(int)+columnNameSize+sizeof(int), &tempLength, sizeof(int));  //column length = -1
    			memcpy((char*)newData+sizeof(int)+columnNameSize+2*sizeof(int), (char*)data+sizeof(int)+columnNameSize+2*sizeof(int), sizeof(int)); //position

    			rbfm->updateRecord(columnFileHandle, column, newData, rid);  //update column
    			free(data);
    			free(newData);
    		}
    	}
    }

    free(tableNameRecord);
    free(tableRecord);
    free(columnRecord);

	return 0;
}

// Extra credit
RC RelationManager::addAttribute(const string &tableName, const Attribute &attr)
{
	RBFM_ScanIterator rbfm_ScanIterator;
	vector<string> attributeNames;

	FileHandle tableFileHandle;
	attributeNames.push_back("tableId");
	int tableNameSize = tableName.size();
	void* tableNameRecord = malloc(PAGE_SIZE);
	*(int*)tableNameRecord = tableNameSize;
	memcpy((char*)tableNameRecord+sizeof(int), tableName.c_str(), tableNameSize);
	rbfm -> openFile("Tables", tableFileHandle);
	rbfm -> scan(tableFileHandle, Tables, "tableName", EQ_OP, tableNameRecord, attributeNames, rbfm_ScanIterator);

	FileHandle columnFileHandle;
	rbfm -> openFile("column", columnFileHandle);

	RID rid;
	void* tableRecord = malloc(PAGE_SIZE);
	void* columnRecord = malloc(PAGE_SIZE);
	int tempPosition = 0;

	void* tempTableId = malloc(sizeof(int));
	while(rbfm_ScanIterator.getNextRecord(rid, tableRecord) != RBFM_EOF)
	{
		rbfm->readAttribute(tableFileHandle, Tables, rid, "tableId", tempTableId);

		rbfm->scan(columnFileHandle, column, "tableId", EQ_OP, (int*)tempTableId, attributeNames, rbfm_ScanIterator);
		while(rbfm_ScanIterator.getNextRecord(rid, columnRecord) != RBFM_EOF)
		{
			tempPosition += 1;
		}
		//*******prepareColumsData********
		void* data=malloc(PAGE_SIZE);
		*(int*)data = tableId;   //table id
		*((int*)data+1)=attr.name.size();   //columns name size
		memcpy((char*)data+2*sizeof(int),attr.name.c_str(),attr.name.size());   //column name
		*(int*)((char*)data+2*sizeof(int)+attr.name.size())=attr.type;      //column type
		*(int*)((char*)data+3*sizeof(int)+attr.name.size())=attr.length;    //column length
		*(int*)((char*)data+4*sizeof(int)+attr.name.size())=tempPosition+1; //position, start from 1
		rbfm->insertRecord(columnFileHandle,column, data, rid);             //insert into column
		free(data);
	}

	free(tableNameRecord);
	free(tableRecord);
	free(columnRecord);
	free(tempTableId);

	return 0;
}

// Extra credit
RC RelationManager::reorganizeTable(const string &tableName)
{
	int result=0;
	FileHandle fileHandle;
	vector<Attribute>recordDescriptor;
	result=rbfm->openFile(tableName,fileHandle);
	if(result==-1)
	    return -1;
	getAttributes(tableName,recordDescriptor);
	result=rbfm->reorganizeFile(fileHandle, recordDescriptor);
	if(result==-1)
	    return -1;
	return 0;
}
