#include "storage/ic_schema_set.hpp"
#include "storage/ic_catalog.hpp"
#include "catalog_api.hpp"
#include "storage/ic_transaction.hpp"
#include "duckdb/parser/parsed_data/create_schema_info.hpp"
#include "duckdb/catalog/catalog.hpp"

namespace duckdb {

IBSchemaSet::IBSchemaSet(Catalog &catalog) : IBCatalogSet(catalog) {
}

static bool IsInternalTable(const string &catalog, const string &schema) {
	if (schema == "information_schema") {
		return true;
	}
	return false;
}
void IBSchemaSet::LoadEntries(ClientContext &context) {

	auto &ic_catalog = catalog.Cast<IBCatalog>();
	auto tables = IBAPI::GetSchemas(catalog.GetName(), ic_catalog.internal_name, ic_catalog.credentials);

	for (const auto &schema : tables) {
		CreateSchemaInfo info;
		info.schema = schema.schema_name;
		info.internal = IsInternalTable(schema.catalog_name, schema.schema_name);
		auto schema_entry = make_uniq<IBSchemaEntry>(catalog, info);
		schema_entry->schema_data = make_uniq<IBAPISchema>(schema);
		CreateEntry(std::move(schema_entry));
	}
}

optional_ptr<CatalogEntry> IBSchemaSet::CreateSchema(ClientContext &context, CreateSchemaInfo &info) {
	throw NotImplementedException("Schema creation");
}

} // namespace duckdb
