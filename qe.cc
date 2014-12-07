
#include "qe.h"

Filter::Filter(Iterator* input, const Condition &condition) {
}

// ... the rest of your implementations go here



GHJoin::GHJoin(Iterator *leftIn,               // Iterator of input R
               Iterator *rightIn,               // Iterator of input S
               const Condition &condition,      // Join condition (CompOp is always EQ)
               const unsigned numPartitions     // # of partitions for each relation (decided by the optimizer)
)
{
    //initial class variable
    //partition R and S
    rbfm = RecordBasedFileManager::instance();
    this->leftIn=leftIn;
    this->rightIn=rightIn;
    this->condition=condition;
    this->numPartitions=numPartitions;
    string left="left";
    string right="right";
    //partition left & right
    Partitons(this->numPartitions,this->condition);

    
    
}
RC GHJoin::getNextTuple(void *data)
{
    //build hash table for smaller a relation
    //match tuples
    //join attributes left+right **different type of attributes will have different implement
    //count partition, page number, slot number
    FileHandle leftFileHandle;
    FileHandle rightFileHandle;
    void*dataPage=malloc(PAGE_SIZE);
    string leftFileName;
    string rightFileName;
    string numOfPartition;
    string numOfGHJoin;
    //read out the partition to be joined
    //get the name of the files，jPartition,suffixNum
    memcpy(&numOfPartition, &jPartition, sizeof(int));
    memcpy(&numOfGHJoin, &suffixNum, sizeof(int));
    leftFileName="left_join"+numOfPartition+"_"+numOfGHJoin;//e.g left_join1_0
    rightFileName="right_join"+numOfPartition+"_"+numOfGHJoin;
    rbfm->openFile(leftFileName, leftFileHandle);
    rbfm->openFile(rightFileName, rightFileHandle);
    //
    
    return QE_EOF;
}
void GHJoin:: getAttributes(vector<Attribute> &attrs) const
{
    //return all attributes used in this join
    vector<Attribute> leftAttrs;
    vector<Attribute> rightAttrs;
    string attriName="rel.attr";
    //get attributes from left relation and right relation
    leftIn->getAttributes(leftAttrs);
    rightIn->getAttributes(rightAttrs);
    for(int i = 0; i < leftAttrs.size(); ++i)
    {
        leftAttrs[i].name=attriName;
        attrs.push_back(leftAttrs[i]);
    }
    for(int i = 0; i < rightAttrs.size(); ++i)
    {
        rightAttrs[i].name=attriName;
        attrs.push_back(rightAttrs[i]);
    }
    
}
RC GHJoin::Partitons(unsigned numPartitions,Condition condition)
{
    //get joined attribute
    //hash
    //write to file
    int hashValue;
    vector<FileHandle>fileHandle;
    string fileName;
    int partitionNum=0;
    int totalSlotNum=0;
    int freeSpaceOffset=0;
    vector<Attribute> attrs;
    vector<void *>dataPage;
    void*data=malloc(PAGE_SIZE);
    void *attribute=malloc(PAGE_SIZE);
    
    
    
    /********partition left relation*************************/
    //initial space for temp page
    for(int i=0;i<numPartitions;i++)
    {
        dataPage[i]=malloc(PAGE_SIZE);
        memcpy((char*)dataPage[i]+PAGE_SIZE-2*sizeof(int), &totalSlotNum, sizeof(int));
        memcpy((char*)dataPage[i]+PAGE_SIZE-sizeof(int), &freeSpaceOffset, sizeof(int));
    }
    //open files
    for(int i=0;i<numPartitions;i++)
    {
        string numOfPartition;
        string numOfGHJoin;
        memcpy(&numOfPartition, &i, sizeof(int));
        memcpy(&numOfGHJoin, &suffixNum, sizeof(int));
        fileName="left_join"+numOfPartition+"_"+numOfGHJoin;//e.g left_join1_0
        rbfm->openFile(fileName, fileHandle[i]);
    }
    
    //scan tuples from table
    //   iterator->setIterator();
    leftIn->getAttributes(attrs);
    while(leftIn->getNextTuple(data)!=-1)
    {
        int i=0;
        //get attribute position
        for(;i<attrs.size();i++)
        {
            if(condition.lhsAttr==attrs[i].name)
                break;
        }
        if(i==attrs.size())
        {
            return -1;
        }
        
        //get the attribute value used to hash
        getAttribute(i, attrs, data, condition.lhsAttr, attribute);
        
        //hash the attribute get partition number
        hashValue=partitionHash(attribute,attrs[i],numPartitions);
        //insert attribute to temp page
        insertAttrToTempPage(fileHandle[hashValue],hashValue,dataPage[hashValue],attribute,attrs[i]);
        
    }
    //close file
    for(int i=0;i<numPartitions;i++)
    {
        
        rbfm->closeFile(fileHandle[i]);
    }
    //clear vector
    attrs.clear();
    
    /********partition right relation*************************/
    //initial space for temp page
    for(int i=0;i<numPartitions;i++)
    {
        memcpy((char*)dataPage[i]+PAGE_SIZE-2*sizeof(int), &totalSlotNum, sizeof(int));
        memcpy((char*)dataPage[i]+PAGE_SIZE-sizeof(int), &freeSpaceOffset, sizeof(int));
    
    }
    
    //open files
    for(int i=0;i<numPartitions;i++)
    {
        string numOfPartition;
        string numOfGHJoin;
        memcpy(&numOfPartition, &i, sizeof(int));
        memcpy(&numOfGHJoin, &suffixNum, sizeof(int));
        fileName="right_join"+numOfPartition+"_"+numOfGHJoin;//e.g right_join1_0
        rbfm->openFile(fileName, fileHandle[i]);
    }
    
    //scan tuples from table
    //   iterator->setIterator();
    rightIn->getAttributes(attrs);
    while(rightIn->getNextTuple(data)!=-1)
    {
        int i=0;
        //get attribute position
        for(;i<attrs.size();i++)
        {
            if(condition.rhsAttr==attrs[i].name)
                break;
        }
        if(i==attrs.size())
        {
            return -1;
        }
        
        //get the attribute value used to hash
        getAttribute(i, attrs, data, condition.rhsAttr, attribute);
        
        //hash the attribute get partition number
        hashValue=partitionHash(attribute,attrs[i],numPartitions);
        //insert attribute to temp page
        insertAttrToTempPage(fileHandle[hashValue],hashValue,dataPage[hashValue],attribute,attrs[i]);
        
    }
    
    //close file
    for(int i=0;i<numPartitions;i++)
    {
 
        rbfm->closeFile(fileHandle[i]);
    }
    //free page space
    for(int i=0;i<numPartitions;i++)
    {
        free(dataPage[i]);
    }

    free(data);
    free(attribute);
    //clear vector
    dataPage.clear();
    fileHandle.clear();
    attrs.clear();
    //add suffixNum
    suffixNum++;
    return 0;
}

int GHJoin::partitionHash(const void *data, const Attribute attrs, unsigned numPartitions )
{
    int hashValue=0;
    int attrValue=0;
    int varCharLength=0;
    string attr;
    switch (attrs.type) {
        case 0:
            memcpy(&attrValue, data, sizeof(int));
            hashValue=attrValue%numPartitions;
            break;
        case 1:
            memcpy(&attrValue, data, sizeof(int));
            hashValue=attrValue%numPartitions;
        case 2:
            memcpy(&varCharLength, data, sizeof(int));
            memcpy(&attr, (char*)data+sizeof(int), varCharLength);
            attrValue=stoi(attr);
            hashValue=attrValue%numPartitions;
        default:
            break;
    }
    return hashValue;
}
int GHJoin:: getAttribute(const unsigned int attriNum,const vector<Attribute> attrs,const void*data, const string attributeName, void *attribute)
{
  
    int attriOffset=0;
    int varCharLength=0;
    for(int i=0;i<(attriNum-1);i++)
    {
        switch (attrs[i].type) {
            case 0:
                attriOffset=attriOffset+sizeof(int);
                break;
            case 1:
                attriOffset=attriOffset+sizeof(int);
                break;
            case 2:
                // attriLength=recordDescriptor[attriCount].length+sizeof(int);
            {
                memcpy(&varCharLength, (char*)data, attriOffset);
                attriOffset=attriOffset+varCharLength+sizeof(int);
                break;
            }
            default:

                return -1;
                //   break;
        }
    }
    switch (attrs[attriNum].type) {
        case 0:
            memcpy(attribute, (char*)data+attriOffset, sizeof(int));
            break;
        case 1:
            memcpy(attribute, (char*)data+attriOffset, sizeof(int));
            break;
        case 2:
            memcpy(&varCharLength, (char*)data+attriOffset, sizeof(int));
            memcpy(attribute, (char*)data+attriOffset, varCharLength+sizeof(int));
            break;
        default:
            break;
    }
    return 0;
}
int GHJoin:: insertAttrToTempPage(FileHandle filehandle,int partitionNum,void *dataPage,const void* attribute,const Attribute attr)
{
    //int type : totalSlotNum
    //float type:totalSlotNum
    //varChar:<recordOffset,recordLength>,totalSlotNum,freeSpaceOffset
    int totalSlotNum=0;
    int recordOffset=0;
    int recordLength=0;
    int freeSpaceOffset=0;
    int vaildSpce=0;
    int varCharLength=0;
    switch (attr.type) {
        case 0:
            memcpy(&totalSlotNum, (char*)dataPage+PAGE_SIZE-2*sizeof(int), sizeof(int));
            if(totalSlotNum==MAX_ATTRI_NUM)
            {
                filehandle.writePage(partitionNum, dataPage);
                totalSlotNum=0;
            }
            memcpy((char*)dataPage+totalSlotNum*sizeof(int), attribute, sizeof(int));
            totalSlotNum=totalSlotNum+1;
            memcpy((char*)dataPage+PAGE_SIZE-2*sizeof(int), &totalSlotNum, sizeof(int));
            break;
        case 1:
            memcpy(&totalSlotNum, (char*)dataPage+PAGE_SIZE-2*sizeof(int), sizeof(int));
            if(totalSlotNum==MAX_ATTRI_NUM)
            {
                filehandle.writePage(partitionNum, dataPage);
                 totalSlotNum=0;
            }
            memcpy((char*)dataPage+totalSlotNum*sizeof(int), attribute, sizeof(int));
            totalSlotNum=totalSlotNum+1;
            memcpy((char*)dataPage+PAGE_SIZE-2*sizeof(int), &totalSlotNum, sizeof(int));
            break;
        case 2:
            memcpy(&varCharLength, attribute, sizeof(int));
            memcpy(&freeSpaceOffset, (char*)dataPage+PAGE_SIZE-sizeof(int), sizeof(int));
            memcpy(&totalSlotNum, (char*)dataPage+PAGE_SIZE-2*sizeof(int), sizeof(int));
            vaildSpce=PAGE_SIZE-freeSpaceOffset-2*totalSlotNum-2*sizeof(int);
            recordLength=varCharLength+sizeof(int);
            if(vaildSpce<recordLength)
            {
                filehandle.writePage(partitionNum, dataPage);
                totalSlotNum=0;
                freeSpaceOffset=0;
            }
            memcpy((char*)dataPage+freeSpaceOffset, attribute, recordLength);
            recordOffset=freeSpaceOffset;
            memcpy((char*)dataPage+PAGE_SIZE-(2*totalSlotNum+1), &recordOffset, sizeof(int));
            memcpy((char*)dataPage+PAGE_SIZE-(2*totalSlotNum+2), &recordLength, sizeof(int));
            freeSpaceOffset=freeSpaceOffset+recordLength;
            totalSlotNum=totalSlotNum+1;
            memcpy((char*)dataPage+PAGE_SIZE-sizeof(int), &freeSpaceOffset, sizeof(int));
            memcpy((char*)dataPage+PAGE_SIZE-2*sizeof(int), &totalSlotNum, sizeof(int));
            break;
        default:
            return -1;

    }
    return 0;
}