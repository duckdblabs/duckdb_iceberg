
#pragma once

#include "storage/ic_catalog_set.hpp"
#include "storage/ic_schema_entry.hpp"

namespace duckdb {
struct CreateSchemaInfo;

class IBSchemaSet : public IBCatalogSet {
public:
	explicit IBSchemaSet(Catalog &catalog);

public:
	optional_ptr<CatalogEntry> CreateSchema(ClientContext &context, CreateSchemaInfo &info);

protected:
	void LoadEntries(ClientContext &context) override;
};

} // namespace duckdb
