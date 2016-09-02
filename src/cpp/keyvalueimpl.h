#include "keyvalue.grpc.pb.h"
#include "rocksdb/db.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using keyvalue::KeyValue;
using rocksdb::TransactionDB;

class KeyValueImpl final : public KeyValue::Service {
public:
    explicit KeyValueImpl();
    Status CreateTable(ServerContext* context, const keyvalue::CreateTableReq* req, keyvalue::CreateTableRes* res) override;
    Status DeleteTable(ServerContext* context, const keyvalue::DeleteTableReq* req, keyvalue::DeleteTableRes* res) override;
    Status Get(ServerContext* context, const keyvalue::GetReq* req, keyvalue::GetRes* res) override;
    Status Put(ServerContext* context, const keyvalue::PutReq* req, keyvalue::PutRes* res) override;
    Status Delete(ServerContext* context, const keyvalue::DeleteReq* req, keyvalue::DeleteRes* res) override;
    Status Range(ServerContext* context, const keyvalue::RangeReq* req, ServerWriter<keyvalue::RangeRes>* writer) override;

private:
    static const std::string TABLE_ALREADY_EXISTS; 
    static const std::string TABLE_DOES_NOT_EXIST; 
    static const std::string DISK_ERROR;
    static const std::string INTERNAL_ERROR;
    static const std::string KEY_NOT_FOUND;
    static const std::string CONDITION_NOT_MET;
    static const std::string NO_ERROR;
    
    std::shared_timed_mutex mtx;
    TransactionDB* tablesTable;
    std::unordered_map<std::string, TransactionDB*> tableLookup;
    std::string dbDir; 
    std::string shuffleSource;
    rocksdb::WriteOptions writeOptions;
    rocksdb::ReadOptions readOptions;
    rocksdb::TransactionOptions txnOptions;
    
    grpc::StatusCode ToStatusCode(keyvalue::ErrorCode err); 
    std::string ToErrorMsg(keyvalue::ErrorCode err);
};
