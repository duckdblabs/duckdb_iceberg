#include "catalog_api.hpp"
#include "catalog_utils.hpp"

#include "storage/ic_catalog.hpp"
#include "storage/ic_table_set.hpp"
#include "storage/ic_transaction.hpp"
#include "duckdb/parser/parsed_data/create_table_info.hpp"
#include "duckdb/parser/constraints/not_null_constraint.hpp"
#include "duckdb/parser/constraints/unique_constraint.hpp"
#include "duckdb/parser/expression/constant_expression.hpp"
#include "duckdb/planner/parsed_data/bound_create_table_info.hpp"
#include "duckdb/catalog/dependency_list.hpp"
#include "duckdb/parser/parsed_data/create_table_info.hpp"
#include "duckdb/parser/constraints/list.hpp"
#include "storage/ic_schema_entry.hpp"
#include "duckdb/parser/parser.hpp"

namespace duckdb {

IBTableSet::IBTableSet(IBSchemaEntry &schema) : IBInSchemaSet(schema) {
}

static ColumnDefinition CreateColumnDefinition(ClientContext &context, IBAPIColumnDefinition &coldef) {
	return {coldef.name, IBUtils::TypeToLogicalType(context, coldef.type_text)};
}

void IBTableSet::LoadEntries(ClientContext &context) {
	auto &transaction = IBTransaction::Get(context, catalog);

	auto &ic_catalog = catalog.Cast<IBCatalog>();

	// TODO: handle out-of-order columns using position property
	auto tables = IBAPI::GetTables(catalog.GetName(), catalog.GetDBPath(), schema.name, ic_catalog.credentials);

	for (auto &table : tables) {
		D_ASSERT(schema.name == table.schema_name);
		CreateTableInfo info;
		for (auto &col : table.columns) {
			info.columns.AddColumn(CreateColumnDefinition(context, col));
		}

		info.table = table.name;
		auto table_entry = make_uniq<IBTableEntry>(catalog, schema, info);
		table_entry->table_data = make_uniq<IBAPITable>(table);

		CreateEntry(std::move(table_entry));
	}
}

optional_ptr<CatalogEntry> IBTableSet::RefreshTable(ClientContext &context, const string &table_name) {
	auto table_info = GetTableInfo(context, schema, table_name);
	auto table_entry = make_uniq<IBTableEntry>(catalog, schema, *table_info);
	auto table_ptr = table_entry.get();
	CreateEntry(std::move(table_entry));
	return table_ptr;
}

unique_ptr<IBTableInfo> IBTableSet::GetTableInfo(ClientContext &context, IBSchemaEntry &schema,
                                                 const string &table_name) {
	throw NotImplementedException("IBTableSet::CreateTable");
}

optional_ptr<CatalogEntry> IBTableSet::CreateTable(ClientContext &context, BoundCreateTableInfo &info) {
	throw NotImplementedException("IBTableSet::CreateTable");
}

void IBTableSet::AlterTable(ClientContext &context, RenameTableInfo &info) {
	throw NotImplementedException("IBTableSet::AlterTable");
}

void IBTableSet::AlterTable(ClientContext &context, RenameColumnInfo &info) {
	throw NotImplementedException("IBTableSet::AlterTable");
}

void IBTableSet::AlterTable(ClientContext &context, AddColumnInfo &info) {
	throw NotImplementedException("IBTableSet::AlterTable");
}

void IBTableSet::AlterTable(ClientContext &context, RemoveColumnInfo &info) {
	throw NotImplementedException("IBTableSet::AlterTable");
}

void IBTableSet::AlterTable(ClientContext &context, AlterTableInfo &alter) {
	throw NotImplementedException("IBTableSet::AlterTable");
}

} // namespace duckdb
