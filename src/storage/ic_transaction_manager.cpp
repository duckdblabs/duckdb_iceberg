#include "storage/ic_transaction_manager.hpp"
#include "duckdb/main/attached_database.hpp"

namespace duckdb {

IBTransactionManager::IBTransactionManager(AttachedDatabase &db_p, IBCatalog &ic_catalog)
    : TransactionManager(db_p), ic_catalog(ic_catalog) {
}

Transaction &IBTransactionManager::StartTransaction(ClientContext &context) {
	auto transaction = make_uniq<IBTransaction>(ic_catalog, *this, context);
	transaction->Start();
	auto &result = *transaction;
	lock_guard<mutex> l(transaction_lock);
	transactions[result] = std::move(transaction);
	return result;
}

ErrorData IBTransactionManager::CommitTransaction(ClientContext &context, Transaction &transaction) {
	auto &ic_transaction = transaction.Cast<IBTransaction>();
	ic_transaction.Commit();
	lock_guard<mutex> l(transaction_lock);
	transactions.erase(transaction);
	return ErrorData();
}

void IBTransactionManager::RollbackTransaction(Transaction &transaction) {
	auto &ic_transaction = transaction.Cast<IBTransaction>();
	ic_transaction.Rollback();
	lock_guard<mutex> l(transaction_lock);
	transactions.erase(transaction);
}

void IBTransactionManager::Checkpoint(ClientContext &context, bool force) {
	auto &transaction = IBTransaction::Get(context, db.GetCatalog());
	//	auto &db = transaction.GetConnection();
	//	db.Execute("CHECKPOINT");
}

} // namespace duckdb
