
#pragma once

#include "duckdb/transaction/transaction.hpp"

namespace duckdb {
class IBCatalog;
class IBSchemaEntry;
class IBTableEntry;

enum class IBTransactionState { TRANSACTION_NOT_YET_STARTED, TRANSACTION_STARTED, TRANSACTION_FINISHED };

class IBTransaction : public Transaction {
public:
	IBTransaction(IBCatalog &ic_catalog, TransactionManager &manager, ClientContext &context);
	~IBTransaction() override;

	void Start();
	void Commit();
	void Rollback();

	static IBTransaction &Get(ClientContext &context, Catalog &catalog);
	AccessMode GetAccessMode() const {
		return access_mode;
	}

private:
	IBTransactionState transaction_state;
	AccessMode access_mode;
};

} // namespace duckdb
