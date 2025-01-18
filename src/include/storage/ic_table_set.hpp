
#pragma once

#include "storage/ic_catalog_set.hpp"
#include "storage/ic_table_entry.hpp"

namespace duckdb {
struct CreateTableInfo;
class IBResult;
class IBSchemaEntry;

class IBTableSet : public IBInSchemaSet {
public:
	explicit IBTableSet(IBSchemaEntry &schema);

public:
	optional_ptr<CatalogEntry> CreateTable(ClientContext &context, BoundCreateTableInfo &info);

	static unique_ptr<IBTableInfo> GetTableInfo(ClientContext &context, IBSchemaEntry &schema,
	                                            const string &table_name);
	optional_ptr<CatalogEntry> RefreshTable(ClientContext &context, const string &table_name);

	void AlterTable(ClientContext &context, AlterTableInfo &info);

protected:
	void LoadEntries(ClientContext &context) override;

	void AlterTable(ClientContext &context, RenameTableInfo &info);
	void AlterTable(ClientContext &context, RenameColumnInfo &info);
	void AlterTable(ClientContext &context, AddColumnInfo &info);
	void AlterTable(ClientContext &context, RemoveColumnInfo &info);

	static void AddColumn(ClientContext &context, IBResult &result, IBTableInfo &table_info, idx_t column_offset = 0);
};

} // namespace duckdb
