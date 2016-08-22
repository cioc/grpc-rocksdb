#include <stdio.h>
#include <iostream>
#include <assert.h>
#include "rocksdb/db.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"
#include "rocksdb/utilities/optimistic_transaction_db.h"

int main(int argc, const char* argv[]) {
    rocksdb::DB* db;
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, "/tmp/testdb", &db);
    assert(status.ok());
    delete db;
    
    rocksdb::DB* exists_db;
    options.create_if_missing = false;
    rocksdb::Status status2 = rocksdb::DB::Open(options, "/tmp/testdb", &exists_db); 
    assert(status2.ok());
    delete exists_db;

    rocksdb::DB* noexists_db;
    rocksdb::Status status3 = rocksdb::DB::Open(options, "/tmp/testdb2", &noexists_db); 
    assert(!status3.ok());
    delete noexists_db;

    rocksdb::DB* base_db;
    rocksdb::OptimisticTransactionDB* txn_db;
    options.create_if_missing = true;
    rocksdb::Status status4 = rocksdb::OptimisticTransactionDB::Open(options, "/tmp/testdb", &txn_db);
    assert(status4.ok());
    base_db = txn_db->GetBaseDB();
    
    rocksdb::WriteOptions write_options;
    write_options.sync = true;
    rocksdb::ReadOptions read_options;
    rocksdb::Transaction* txn = txn_db->BeginTransaction(write_options); 
    txn->Put("key", "value2345");
    rocksdb::Status write_status = txn->Commit();
    assert(write_status.ok());
    
    std::string value;
    rocksdb::Status read_status = base_db->Get(rocksdb::ReadOptions(), "key", &value);
    assert(read_status.ok());
    std::cout << "Value: " << value << "\n";

    rocksdb::Transaction* txn_occ = txn_db->BeginTransaction(write_options);
    std::string value2; 
    rocksdb::Status gfu_status = txn_occ->GetForUpdate(read_options, "key", &value2);
    assert(gfu_status.ok());
    base_db->Put(write_options, "key", "A value"); 
    rocksdb::Status occ_status = txn_occ->Commit(); 
    assert(!occ_status.ok());
    
    rocksdb::Status delete_status = base_db->Delete(write_options, "key");
    assert(delete_status.ok());
    
    base_db->Put(write_options, "key1", "value1");
    base_db->Put(write_options, "key2", "value2");
    base_db->Put(write_options, "key3", "value3");
    base_db->Put(write_options, "key4", "value4");

    rocksdb::Iterator* it = base_db->NewIterator(read_options);
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::cout << it->key().ToString() << ": " << it->value().ToString() << "\n";
    }
    assert(it->status().ok());
    
    std::cout << "\n\n";

    for (it->Seek("key2"); it->Valid() && it->key().ToString() < "key4"; it->Next()) {
        std::cout << it->key().ToString() << ": " << it->value().ToString() << "\n";
    }

    delete txn;
    delete txn_db;
    
    return 0;
}
