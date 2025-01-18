
#pragma once

#include "duckdb/transaction/transaction_manager.hpp"
#include "storage/ic_catalog.hpp"
#include "storage/ic_transaction.hpp"

namespace duckdb {

class IBTransactionManager : public TransactionManager {
public:
	IBTransactionManager(AttachedDatabase &db_p, IBCatalog &ic_catalog);

	Transaction &StartTransaction(ClientContext &context) override;
	ErrorData CommitTransaction(ClientContext &context, Transaction &transaction) override;
	void RollbackTransaction(Transaction &transaction) override;

	void Checkpoint(ClientContext &context, bool force = false) override;

private:
	IBCatalog &ic_catalog;
	mutex transaction_lock;
	reference_map_t<Transaction, unique_ptr<IBTransaction>> transactions;
};

} // namespace duckdb
