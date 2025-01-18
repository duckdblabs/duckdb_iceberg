#include "storage/ic_catalog_set.hpp"
#include "storage/ic_transaction.hpp"
#include "duckdb/parser/parsed_data/drop_info.hpp"
#include "storage/ic_schema_entry.hpp"

namespace duckdb {

IBCatalogSet::IBCatalogSet(Catalog &catalog) : catalog(catalog), is_loaded(false) {
}

optional_ptr<CatalogEntry> IBCatalogSet::GetEntry(ClientContext &context, const string &name) {
	if (!is_loaded) {
		is_loaded = true;
		LoadEntries(context);
	}
	lock_guard<mutex> l(entry_lock);
	auto entry = entries.find(name);
	if (entry == entries.end()) {
		return nullptr;
	}
	return entry->second.get();
}

void IBCatalogSet::DropEntry(ClientContext &context, DropInfo &info) {
	throw NotImplementedException("IBCatalogSet::DropEntry");
}

void IBCatalogSet::EraseEntryInternal(const string &name) {
	lock_guard<mutex> l(entry_lock);
	entries.erase(name);
}

void IBCatalogSet::Scan(ClientContext &context, const std::function<void(CatalogEntry &)> &callback) {
	if (!is_loaded) {
		is_loaded = true;
		LoadEntries(context);
	}
	lock_guard<mutex> l(entry_lock);
	for (auto &entry : entries) {
		callback(*entry.second);
	}
}

optional_ptr<CatalogEntry> IBCatalogSet::CreateEntry(unique_ptr<CatalogEntry> entry) {
	lock_guard<mutex> l(entry_lock);
	auto result = entry.get();
	if (result->name.empty()) {
		throw InternalException("IBCatalogSet::CreateEntry called with empty name");
	}
	entries.insert(make_pair(result->name, std::move(entry)));
	return result;
}

void IBCatalogSet::ClearEntries() {
	entries.clear();
	is_loaded = false;
}

IBInSchemaSet::IBInSchemaSet(IBSchemaEntry &schema) : IBCatalogSet(schema.ParentCatalog()), schema(schema) {
}

optional_ptr<CatalogEntry> IBInSchemaSet::CreateEntry(unique_ptr<CatalogEntry> entry) {
	if (!entry->internal) {
		entry->internal = schema.internal;
	}
	return IBCatalogSet::CreateEntry(std::move(entry));
}

} // namespace duckdb
