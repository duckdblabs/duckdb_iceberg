#include "storage/ic_catalog.hpp"
#include "storage/ic_schema_entry.hpp"
#include "storage/ic_transaction.hpp"
#include "duckdb/storage/database_size.hpp"
#include "duckdb/parser/parsed_data/drop_info.hpp"
#include "duckdb/parser/parsed_data/create_schema_info.hpp"
#include "duckdb/main/attached_database.hpp"

namespace duckdb {

IBCatalog::IBCatalog(AttachedDatabase &db_p, const string &internal_name, AccessMode access_mode,
                     IBCredentials credentials)
    : Catalog(db_p), internal_name(internal_name), access_mode(access_mode), credentials(std::move(credentials)),
      schemas(*this) {
}

IBCatalog::~IBCatalog() = default;

void IBCatalog::Initialize(bool load_builtin) {
}

optional_ptr<CatalogEntry> IBCatalog::CreateSchema(CatalogTransaction transaction, CreateSchemaInfo &info) {
	if (info.on_conflict == OnCreateConflict::REPLACE_ON_CONFLICT) {
		DropInfo try_drop;
		try_drop.type = CatalogType::SCHEMA_ENTRY;
		try_drop.name = info.schema;
		try_drop.if_not_found = OnEntryNotFound::RETURN_NULL;
		try_drop.cascade = false;
		schemas.DropEntry(transaction.GetContext(), try_drop);
	}
	return schemas.CreateSchema(transaction.GetContext(), info);
}

void IBCatalog::DropSchema(ClientContext &context, DropInfo &info) {
	return schemas.DropEntry(context, info);
}

void IBCatalog::ScanSchemas(ClientContext &context, std::function<void(SchemaCatalogEntry &)> callback) {
	schemas.Scan(context, [&](CatalogEntry &schema) { callback(schema.Cast<IBSchemaEntry>()); });
}

optional_ptr<SchemaCatalogEntry> IBCatalog::GetSchema(CatalogTransaction transaction, const string &schema_name,
                                                      OnEntryNotFound if_not_found, QueryErrorContext error_context) {
	if (schema_name == DEFAULT_SCHEMA) {
		if (default_schema.empty()) {
			throw InvalidInputException("Attempting to fetch the default schema - but no database was "
			                            "provided in the connection string");
		}
		return GetSchema(transaction, default_schema, if_not_found, error_context);
	}
	auto entry = schemas.GetEntry(transaction.GetContext(), schema_name);
	if (!entry && if_not_found != OnEntryNotFound::RETURN_NULL) {
		throw BinderException("Schema with name \"%s\" not found", schema_name);
	}
	return reinterpret_cast<SchemaCatalogEntry *>(entry.get());
}

bool IBCatalog::InMemory() {
	return false;
}

string IBCatalog::GetDBPath() {
	return internal_name;
}



DatabaseSize IBCatalog::GetDatabaseSize(ClientContext &context) {
	if (default_schema.empty()) {
		throw InvalidInputException("Attempting to fetch the database size - but no database was provided "
		                            "in the connection string");
	}
	DatabaseSize size;
	return size;
}

void IBCatalog::ClearCache() {
	schemas.ClearEntries();
}

unique_ptr<PhysicalOperator> IBCatalog::PlanInsert(ClientContext &context, LogicalInsert &op,
                                                   unique_ptr<PhysicalOperator> plan) {
	throw NotImplementedException("IBCatalog PlanInsert");
}
unique_ptr<PhysicalOperator> IBCatalog::PlanCreateTableAs(ClientContext &context, LogicalCreateTable &op,
                                                          unique_ptr<PhysicalOperator> plan) {
	throw NotImplementedException("IBCatalog PlanCreateTableAs");
}
unique_ptr<PhysicalOperator> IBCatalog::PlanDelete(ClientContext &context, LogicalDelete &op,
                                                   unique_ptr<PhysicalOperator> plan) {
	throw NotImplementedException("IBCatalog PlanDelete");
}
unique_ptr<PhysicalOperator> IBCatalog::PlanUpdate(ClientContext &context, LogicalUpdate &op,
                                                   unique_ptr<PhysicalOperator> plan) {
	throw NotImplementedException("IBCatalog PlanUpdate");
}
unique_ptr<LogicalOperator> IBCatalog::BindCreateIndex(Binder &binder, CreateStatement &stmt, TableCatalogEntry &table,
                                                       unique_ptr<LogicalOperator> plan) {
	throw NotImplementedException("IBCatalog BindCreateIndex");
}

} // namespace duckdb
