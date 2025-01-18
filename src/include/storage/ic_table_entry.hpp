
#pragma once

#include "catalog_api.hpp"
#include "duckdb/catalog/catalog_entry/table_catalog_entry.hpp"
#include "duckdb/parser/parsed_data/create_table_info.hpp"

namespace duckdb {

struct IBTableInfo {
	IBTableInfo() {
		create_info = make_uniq<CreateTableInfo>();
	}
	IBTableInfo(const string &schema, const string &table) {
		create_info = make_uniq<CreateTableInfo>(string(), schema, table);
	}
	IBTableInfo(const SchemaCatalogEntry &schema, const string &table) {
		create_info = make_uniq<CreateTableInfo>((SchemaCatalogEntry &)schema, table);
	}

	const string &GetTableName() const {
		return create_info->table;
	}

	unique_ptr<CreateTableInfo> create_info;
};

class IBTableEntry : public TableCatalogEntry {
public:
	IBTableEntry(Catalog &catalog, SchemaCatalogEntry &schema, CreateTableInfo &info);
	IBTableEntry(Catalog &catalog, SchemaCatalogEntry &schema, IBTableInfo &info);

	unique_ptr<IBAPITable> table_data;

public:
	unique_ptr<BaseStatistics> GetStatistics(ClientContext &context, column_t column_id) override;

	TableFunction GetScanFunction(ClientContext &context, unique_ptr<FunctionData> &bind_data) override;

	TableStorageInfo GetStorageInfo(ClientContext &context) override;

	void BindUpdateConstraints(Binder &binder, LogicalGet &get, LogicalProjection &proj, LogicalUpdate &update,
	                           ClientContext &context) override;
};

} // namespace duckdb
