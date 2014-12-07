
#ifndef _rm_h_
#define _rm_h_

#include <string>
#include <vector>
#include <map>

#include "../rbf/rbfm.h"
#include "../ix/ix.h"

using namespace std;


# define RM_EOF (-1)  // end of a scan operator

// RM_ScanIterator is an iteratr to go through tuples
// The way to use it is like the following:
//  RM_ScanIterator rmScanIterator;
//  rm.open(..., rmScanIterator);
//  while (rmScanIterator(rid, data) != RM_EOF) {
//    process the data;
//  }
//  rmScanIterator.close();

class RM_ScanIterator {
public:
  RM_ScanIterator() {};
  ~RM_ScanIterator() {};

  // "data" follows the same format as RelationManager::insertTuple()
  RC getNextTuple(RID &rid, void *data) {
	  if(rbfm_ScanIterator.getNextRecord(rid,data)==RM_EOF)
      {
          return -1;
      }
	  return 0;
  };
  RC close() { return rbfm_ScanIterator.close(); };
  RC setScanIterator(FileHandle &fileHandle,
	      const vector<Attribute> &recordDescriptor,
	      const string &conditionAttribute,
	      const CompOp compOp,                  // comparision type such as "<" and "="
	      const void *value,                    // used in the comparison
	      const vector<string> &attributeNames){
	  RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	  rbfm->scan(fileHandle,recordDescriptor,conditionAttribute,compOp,value,attributeNames,rbfm_ScanIterator);
	  return 0;
  }
private:

  RBFM_ScanIterator rbfm_ScanIterator ;
};

class RM_IndexScanIterator {
public:
  RM_IndexScanIterator() {};  	// Constructor
  ~RM_IndexScanIterator() {}; 	// Destructor

  // "key" follows the same format as in IndexManager::insertEntry()
  RC getNextEntry(RID &rid, void *key) {
	  return ix_ScanIterator.getNextEntry(rid, key);
  };  	// Get next matching entry

  RC close() {
	  return ix_ScanIterator.close();
  };             			// Terminate index scan

  RC initialize(Attribute attribute, const void *lowKey, const void *highKey, bool lowKeyInclusive, bool highKeyInclusive)
  {
	  return ix->scan(ixFileHandle, attribute, lowKey, highKey, lowKeyInclusive, highKeyInclusive, ix_ScanIterator);
  }

  IXFileHandle ixFileHandle;

private:
  IX_ScanIterator ix_ScanIterator;

  IndexManager* ix;

};


// Relation Manager
class RelationManager
{
public:
  static RelationManager* instance();

 // RC initialize();
    RC initialFiles();
    RC initialVectors();

  int memoryNeeded(const vector<Attribute> &attributes);

  int getTableId();

  RC createTable(const string &tableName, const vector<Attribute> &attrs);

  //RC createTable(const string &tableName, const vector<Attribute> &attrs, const string &type);

  RC deleteTable(const string &tableName);

  RC getAttributes(const string &tableName, vector<Attribute> &attrs);

  RC insertTuple(const string &tableName, const void *data, RID &rid);

  RC deleteTuples(const string &tableName);

  RC deleteTuple(const string &tableName, const RID &rid);

  // Assume the rid does not change after update
  RC updateTuple(const string &tableName, const void *data, const RID &rid);

  RC readTuple(const string &tableName, const RID &rid, void *data);

  RC readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data);

  RC reorganizePage(const string &tableName, const unsigned pageNumber);

  // scan returns an iterator to allow the caller to go through the results one by one.
  RC scan(const string &tableName,
      const string &conditionAttribute,
      const CompOp compOp,                  // comparision type such as "<" and "="
      const void *value,                    // used in the comparison
      const vector<string> &attributeNames, // a list of projected attributes
      RM_ScanIterator &rm_ScanIterator);


  RC createIndex(const string &tableName, const string &attributeName);

  RC destroyIndex(const string &tableName, const string &attributeName);

  // indexScan returns an iterator to allow the caller to go through qualified entries in index
  RC indexScan(const string &tableName,
      const string &attributeName,
      const void *lowKey,
      const void *highKey,
      bool lowKeyInclusive,
      bool highKeyInclusive,
      RM_IndexScanIterator &rm_IndexScanIterator);

// Extra credit
public:
  RC dropAttribute(const string &tableName, const string &attributeName);

  RC addAttribute(const string &tableName, const Attribute &attr);

  RC reorganizeTable(const string &tableName);



protected:
  RelationManager();
  ~RelationManager();

private:
  static RelationManager *_rm;

  vector<Attribute> Tables;
  vector<Attribute> column;
  RecordBasedFileManager* rbfm;
  PagedFileManager* pfm;
  IndexManager* ixManager;

  int tableId;

};

#endif
